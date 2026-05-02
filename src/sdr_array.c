//
//  Pocket SDR C Library - Antenna Array Functions
//
//  Author:
//  T.TAKASU
//
//  History:
//  2026-04-17  1.0  new
//  2026-04-25  1.1  switch from batch LS to per-epoch EKF
//
#include "pocket_sdr.h"

#define DPI         (2.0 * PI)
#define YAW_STEP    15.0        // yaw search step (deg)
#define MAX_ITER    15          // max LSQ iterations for init
#define CONV_THRES_A 1e-6       // angle update convergence (rad)
#define CONV_THRES_B 1e-5       // bias update convergence (m)
#define MIN_EL      15.0        // elevation mask (deg)
#define MAX_RMS     0.03        // max RMS of residuals (m)
#define NX          (3+SDR_MAX_RFCH)
#define MAX_NV      ((SDR_MAX_RFCH-1)*MAXOBS)
#define VAR_INIT_RPY  (1.0)     // initial covariance for rpy (rad^2)
#define VAR_INIT_BIAS (1.0)     // initial covariance for bias (m^2)
#define VAR_PROC_RPY  (1e-4)    // process noise variance per epoch (rad^2)
#define VAR_PROC_BIAS (1e-8)    // process noise variance per epoch (m^2)
#define VAR_MEAS      (1e-4)    // measurement variance (m^2)
#define ARRAY_W_SCALE 256       // array weight Q-factor (Q8)
#define ARRAY_FREQ    1.57542e9 // array L1 frequency (Hz)
#define ARRAY_FREQ_TOL 1e6      // RF CH center freq tolerance (Hz)
#define CLIP(x, mn, mx) ((x) < (mn) ? (mn) : ((x) > (mx) ? (mx) : (x)))

// rotation matrix and partials by Euler angles (Z-Y-X) ------------------------
static void euler_rot(const double *rpy, double *R, double *Dr, double *Dp,
    double *Dy)
{
    double cr = cos(rpy[0]), sr = sin(rpy[0]), cp = cos(rpy[1]);
    double sp = sin(rpy[1]), cy = cos(rpy[2]), sy = sin(rpy[2]);
    
    // Rz(yaw) * Ry(pitch) * Rx(roll) (X:forward,Y:left,Z:up)
    R [0] =  cy*cp; R [3] =  cy*sp*sr - sy*cr; R [6] =  cy*sp*cr + sy*sr;
    R [1] =  sy*cp; R [4] =  sy*sp*sr + cy*cr; R [7] =  sy*sp*cr - cy*sr;
    R [2] = -sp;    R [5] =  cp*sr;            R [8] =  cp*cr;
    
    // Dr = dR/droll, Dp = dR/dpitch, Dy = dR/dyaw
    Dr[0] =  0.0;   Dr[3] =  cy*sp*cr + sy*sr; Dr[6] = -cy*sp*sr + sy*cr;
    Dr[1] =  0.0;   Dr[4] =  sy*sp*cr - cy*sr; Dr[7] = -sy*sp*sr - cy*cr;
    Dr[2] =  0.0;   Dr[5] =  cp*cr;            Dr[8] = -cp*sr;
    Dp[0] = -cy*sp; Dp[3] =  cy*cp*sr;         Dp[6] =  cy*cp*cr;
    Dp[1] = -sy*sp; Dp[4] =  sy*cp*sr;         Dp[7] =  sy*cp*cr;
    Dp[2] = -cp;    Dp[5] = -sp*sr;            Dp[8] = -sp*cr;
    Dy[0] = -sy*cp; Dy[3] = -sy*sp*sr - cy*cr; Dy[6] = -sy*sp*cr + cy*sr;
    Dy[1] =  cy*cp; Dy[4] =  cy*sp*sr - sy*cr; Dy[7] =  cy*sp*cr + sy*sr;
    Dy[2] =  0.0;   Dy[5] =  0.0;              Dy[8] =  0.0;
}

// LOS unit vector (receiver -> satellite) in ENU frame ------------------------
static int los_vec(gtime_t time, int sat, const nav_t *nav, const double *pos,
    const double *rr, double *e)
{
    double rs[6], dts[2], var, e_ecef[3], azel[2];
    int svh;
    
    if (!satpos(time, time, sat, EPHOPT_BRDC, nav, rs, dts, &var, &svh)) {
        return 0;
    }
    if (svh || geodist(rs, rr, e_ecef) <= 0.0) return 0;
    if (satazel(pos, e_ecef, azel) < MIN_EL * D2R) return 0;
    ecef2enu(pos, e_ecef, e);
    return 1;
}

// SD model term -e'*(R*b) and partials wrt roll/pitch/yaw ---------------------
static double sd_proj(const double *e, const double *b, const double *R,
    const double *Dr, const double *Dp, const double *Dy, double *H)
{
    double v[3], vr[3], vp[3], vy[3];
    
    matmul("NN", 3, 1, 3, 1.0, R,  b, 0.0, v );
    matmul("NN", 3, 1, 3, 1.0, Dr, b, 0.0, vr);
    matmul("NN", 3, 1, 3, 1.0, Dp, b, 0.0, vp);
    matmul("NN", 3, 1, 3, 1.0, Dy, b, 0.0, vy);
    H[0] = -dot(e, vr, 3);
    H[1] = -dot(e, vp, 3);
    H[2] = -dot(e, vy, 3);
    return -dot(e, v, 3);
}

// build measurements and design matrix ----------------------------------------
static int build_meas(const sdr_array_t *array, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr, const double *x, double *v, double *H)
{
    double pos[3], R[9], Dr[9], Dp[9], Dy[9];
    int nv = 0, f = array->freq;
    
    ecef2pos(rr, pos);
    euler_rot(x, R, Dr, Dp, Dy);
    
    for (int i = 0; i < nobs; i++) {
        const obsd_t *p = obs + i;
        double freq, lam, e[3];
        
        // search CH1
        if (p->rcv != 1 || p->L[f] == 0.0 || !array->ant_ena[0]) continue;
        if (!los_vec(p->time, p->sat, nav, pos, rr, e)) continue;
        if ((freq = sat2freq(p->sat, p->code[f], nav)) == 0.0) continue;
        lam = CLIGHT / freq;
        
        // residuals of SD phase
        for (int j = 0; j < nobs && nv < MAX_NV; j++) {
            const obsd_t *q = obs + j;
            double b_body[3], proj;
            int k = q->rcv - 1;
            
            if (q->sat != p->sat || k <= 0 || !array->ant_ena[k]) continue;
            if (q->L[f] == 0.0) continue;
            
            for (int l = 0; l < 3; l++) {
                b_body[l] = array->ant_pos[k][l] - array->ant_pos[0][l];
            }
            proj = sd_proj(e, b_body, R, Dr, Dp, Dy, H + nv * NX);
            v[nv] = lam * (q->L[f] - p->L[f]) - (proj - x[3+k]);
            v[nv] -= lam * floor(v[nv] / lam + 0.5); // -lam / 2 <= v < lam / 2
            H[3+k+NX*(nv++)] = -1.0;
        }
    }
    return nv;
}

// LSQ for attitude/bias ------------------------------------------------------
static int lsq_init(const sdr_array_t *array, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr, double *x, double *rms)
{
    for (int iter = 0; iter < MAX_ITER; iter++) {
        double v[MAX_NV+SDR_MAX_RFCH] = {0}, H[NX*(MAX_NV+SDR_MAX_RFCH)] = {0};
        double dx[NX], Q[NX*NX];
        
        int nv = build_meas(array, obs, nobs, nav, rr, x, v, H);
        if (nv == 0) return 0;

        for (int i = 0; i < SDR_MAX_RFCH; i++) { // avoid rank-deficient
            if (i == 0 || !array->ant_ena[i]) H[3+i+NX*(nv++)] = 1e-12;
        }
        if (lsq(H, v, NX, nv, dx, Q)) break;
        
        for (int i = 0; i < NX; i++) x[i] += dx[i];
        
        if (norm(dx, 3) < CONV_THRES_A && norm(dx + 3, NX - 3) < CONV_THRES_B) {
            x[2] -= DPI * floor(x[2] / DPI + 0.5); // -PI <= yaw < PI
            *rms = sqrt(dot(v, v, nv) / nv);
            return 1;
        }
    }
    return 0;
}

// initialize state by yaw grid search + LSQ -----------------------------------
static int kf_init(sdr_array_t *array, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr)
{
    array->rms = 1e3;
    
    for (double y = -PI; y < PI; y += YAW_STEP * D2R) {
        double x[NX] = {0.0, 0.0, y}, rms = 0.0;
        
        if (!lsq_init(array, obs, nobs, nav, rr, x, &rms)) continue;
        
        if (rms < array->rms) {
            matcpy(array->x, x, NX, 1);
            array->rms = rms;
        }
    }
    if (array->rms > MAX_RMS) return 0;
    
    memset(array->P, 0, sizeof(double) * NX * NX);
    for (int i = 0; i < NX; i++) {
        array->P[i+i*NX] = (i < 3) ? VAR_INIT_RPY : VAR_INIT_BIAS;
    }
    return 1;
}

// KF predict and update -------------------------------------------------------
static int kf_update(sdr_array_t *array, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr)
{
    double v[MAX_NV], H[NX*MAX_NV] = {0};
    
    // predict: P += Q (random walk; identity transition)
    for (int i = 0; i < NX; i++) {
        array->P[i+i*NX] += (i < 3) ? VAR_PROC_RPY : VAR_PROC_BIAS;
    }
    // build measurements
    int nv = build_meas(array, obs, nobs, nav, rr, array->x, v, H);
    if (nv < 1) return 0;
    
    // KF measurement update
    double *R = (double *)sdr_malloc(sizeof(double) * nv * nv);
    for (int i = 0; i < nv; i++) R[i+i*nv] = VAR_MEAS;
    int info = filter(array->x, array->P, H, v, R, NX, nv);
    sdr_free(R);
    if (info) return 0;
    
    array->x[2] -= DPI * floor(array->x[2] / DPI + 0.5); // -PI <= yaw < PI
    
    // post-fit residual RMS
    nv = build_meas(array, obs, nobs, nav, rr, array->x, v, H);
    array->rms = nv > 0 ? sqrt(dot(v, v, nv) / nv) : 0.0;
    return 1;
}

// allocate and initialize antenna array ---------------------------------------
sdr_array_t *sdr_array_new(sdr_rcv_t *rcv, int freq)
{
    sdr_array_t *array = (sdr_array_t *)sdr_malloc(sizeof(sdr_array_t));
    array->rcv = rcv;
    array->freq = freq;
    array->nrfch = rcv->nrfch;
    for (int i = 0; i < SDR_MAX_RFCH; i++) {
        array->ant_ena[i] = (i < array->nrfch);
    }
    for (int m = 0; m < rcv->narch; m++) {
        rcv->arch[m].scale = 1.0 / array->nrfch;
        sdr_arch_set_beam(rcv->arch + m, array, 0.0, PI / 2, rcv->arch[m].scale);
    }
    return array;
}

// set antenna element positions and enable flags ------------------------------
int sdr_array_ant_pos(sdr_array_t *array, const double *ant_pos,
    const int *ant_ena)
{
    if (!ant_ena[0]) return 0; // CH1 must be enabled
    
    matcpy(&array->ant_pos[0][0], ant_pos, array->nrfch * 3, 1);
    for (int i = 0; i < array->nrfch; i++) {
        array->ant_ena[i] = ant_ena[i] ? 1 : 0;
    }
    return 1;
}

// free antenna array -----------------------------------------------------------
void sdr_array_free(sdr_array_t *array)
{
    sdr_free(array);
}

// calibrate antenna array ------------------------------------------------------
int sdr_array_calib(sdr_array_t *array, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr)
{
    if (nobs <= 0) return 0;

    if (array->P[0] == 0.0) {
        return kf_init(array, obs, nobs, nav, rr);
    }
    return kf_update(array, obs, nobs, nav, rr);
}

// reset calibration state ------------------------------------------------------
static void reset_calib(sdr_array_t *array)
{
    memset(array->x, 0, sizeof(array->x));
    memset(array->P, 0, sizeof(array->P));
    array->nep = 0;
    array->rms = 0.0;
}

// drive array calibration run flag --------------------------------------------
int sdr_array_run(sdr_array_t *array, int run)
{
    if (array->nrfch < 2) return 0;

    if (run == 1 || run == 2) reset_calib(array);
    if (run == 0 || run == 1) array->calib_run = run;
    if (run == 2) array->calib_run = 0;
    return 1;
}

// inject calibration state ----------------------------------------------------
int sdr_array_inject(sdr_array_t *array, const double *rpy, const double *bias)
{
    if (array->nrfch < 2) return 0;

    reset_calib(array);
    matcpy(array->x, rpy, 3, 1);
    matcpy(array->x + 3, bias, array->nrfch, 1);
    for (int i = 0; i < NX; i++) {
        array->P[i+i*NX] = (i < 3) ? 1e-2 : 1e-4;
    }
    return 1;
}

// get array calibration state --------------------------------------------------
int sdr_array_stat(sdr_array_t *array, double *rpy, double *bias, double *rms,
    int *nep)
{
    matcpy(rpy, array->x, 3, 1);
    bias[0] = 0.0;
    matcpy(bias + 1, array->x + 4, array->nrfch - 1, 1);
    *rms = array->rms;
    *nep = array->nep;
    return array->calib_run;
}

// run one receiver array calibration epoch -------------------------------------
void sdr_array_step(sdr_array_t *array, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr)
{
    if (!array->calib_run || nobs <= 0) return;
    if (norm(rr, 3) < 1e-6) return;

    if (sdr_array_calib(array, obs, nobs, nav, rr)) array->nep++;
}

// save calibration state to file ----------------------------------------------
int sdr_array_save_calib(sdr_array_t *array, const char *file)
{
    double rpy[3], bias[SDR_MAX_RFCH], rms;
    int nep;

    sdr_array_stat(array, rpy, bias, &rms, &nep);
    if (nep <= 0 && norm(rpy, 3) < 1e-9) return 0;

    FILE *fp = fopen(file, "w");
    if (!fp) return 0;
    fprintf(fp, "# Pocket SDR Array Calibration\n");
    fprintf(fp, "# epochs=%d, rms=%.4fm\n", nep, rms);
    fprintf(fp, "# Roll Pitch Yaw (rad)\n");
    fprintf(fp, "%.9f %.9f %.9f\n", rpy[0], rpy[1], rpy[2]);
    fprintf(fp, "# Bias CH1..CHn (m)\n");
    for (int i = 0; i < array->nrfch; i++) {
        fprintf(fp, "%s%.9f", i ? " " : "", bias[i]);
    }
    fprintf(fp, "\n");
    fclose(fp);
    return 1;
}

// load calibration state from file --------------------------------------------
int sdr_array_load_calib(sdr_array_t *array, const char *file)
{
    FILE *fp = fopen(file, "r");
    if (!fp) return 0;

    double rpy[3] = {0}, bias[SDR_MAX_RFCH] = {0};
    char buff[1024];
    int line = 0;
    while (line < 2 && fgets(buff, sizeof(buff), fp)) {
        char *p = strchr(buff, '#');
        if (p) *p = '\0';
        for (p = buff; *p && (*p == ' ' || *p == '\t'); p++);
        if (!*p || *p == '\n' || *p == '\r') continue;
        if (line == 0) {
            if (sscanf(buff, "%lf %lf %lf", rpy, rpy + 1, rpy + 2) < 3) {
                fclose(fp);
                return 0;
            }
        } else {
            int n = 0;
            for (char *s = buff, *e; n < array->nrfch; s = e) {
                bias[n] = strtod(s, &e);
                if (e == s) break;
                n++;
            }
            if (n < 1) { fclose(fp); return 0; }
        }
        line++;
    }
    fclose(fp);
    if (line < 2) return 0;
    return sdr_array_inject(array, rpy, bias);
}

// install beam weights. caller locked. ----------------------------------------
static int install_beam(sdr_arch_t *arch, double az, double el, double scale,
    const int16_t *w, int nrfch)
{
    arch->az = az;
    arch->el = el;
    arch->scale = scale;
    memset(arch->w, 0, sizeof(arch->w));
    memcpy(arch->w, w, sizeof(int16_t) * nrfch * 2);
    return 1;
}

// free array CH beam state -----------------------------------------------------
void sdr_arch_free(sdr_arch_t *arch)
{
    arch->az = arch->el = arch->scale = 0.0;
    memset(arch->w, 0, sizeof(arch->w));
}

// set array CH beam ------------------------------------------------------------
int sdr_arch_set_beam(sdr_arch_t *arch, const sdr_array_t *array, double az,
    double el, double scale)
{
    const sdr_rcv_t *rcv = array->rcv;
    int nrfch = rcv->nrfch;

    if (scale <= 0.0) {
        arch->az = az;
        arch->el = el;
        arch->scale = 0.0;
        memset(arch->w, 0, sizeof(arch->w));
        return 1;
    }
    double rpy[3], bias[SDR_MAX_RFCH], ant_pos[SDR_MAX_RFCH][3];
    matcpy(rpy, array->x, 3, 1);
    bias[0] = 0.0;
    matcpy(bias + 1, array->x + 4, nrfch - 1, 1);
    matcpy(&ant_pos[0][0], &array->ant_pos[0][0], nrfch * 3, 1);

    double R[9], Dr[9], Dp[9], Dy[9], e_enu[3], e_body[3];
    euler_rot(rpy, R, Dr, Dp, Dy);
    e_enu[0] = sin(az) * cos(el);
    e_enu[1] = cos(az) * cos(el);
    e_enu[2] = sin(el);
    matmul("TN", 3, 1, 3, 1.0, R, e_enu, 0.0, e_body);

    int bits_min = 4;
    for (int a = 0; a < nrfch; a++) {
        int rtoc = (rcv->rfch[a].IQ == 1);
        double f_eff = rcv->rfch[a].fo + (rtoc ? rcv->fs * 0.25 : 0.0);
        if (fabs(f_eff - ARRAY_FREQ) > ARRAY_FREQ_TOL) continue;
        if (!array->ant_ena[a]) continue;
        if (rcv->rfch[a].bits > 0 && rcv->rfch[a].bits < bits_min) {
            bits_min = rcv->rfch[a].bits;
        }
    }
    double bit_gain = (bits_min < 4) ? 7.0 / (double)((1 << bits_min) - 1) : 1.0;

    double lam = CLIGHT / ARRAY_FREQ;
    int16_t w[SDR_MAX_RFCH * 2] = {0};
    for (int a = 0; a < nrfch; a++) {
        int rtoc = (rcv->rfch[a].IQ == 1);
        double f_eff = rcv->rfch[a].fo + (rtoc ? rcv->fs * 0.25 : 0.0);
        if (fabs(f_eff - ARRAY_FREQ) > ARRAY_FREQ_TOL || !array->ant_ena[a]) {
            w[a*2] = 0; w[a*2+1] = 0;
            continue;
        }
        double b_body[3];
        for (int k = 0; k < 3; k++) b_body[k] = ant_pos[a][k] - ant_pos[0][k];
        double proj = e_body[0]*b_body[0] + e_body[1]*b_body[1] +
            e_body[2]*b_body[2];
        double phi = -DPI * (proj + bias[a]) / lam;
        double wr = scale * bit_gain * cos(phi) * ARRAY_W_SCALE;
        double wi = scale * bit_gain * sin(phi) * ARRAY_W_SCALE;
        wr = CLIP(wr, -32768.0, 32767.0);
        wi = CLIP(wi, -32768.0, 32767.0);
        w[a*2  ] = (int16_t)floor(wr + 0.5);
        w[a*2+1] = (int16_t)floor(wi + 0.5);
    }
    return install_beam(arch, az, el, scale, w, nrfch);
}

// get array CH beam ------------------------------------------------------------
int sdr_arch_get_beam(const sdr_arch_t *arch, double *az, double *el)
{
    *az = arch->az;
    *el = arch->el;
    return 1;
}

// combine RF CHs into array CH -------------------------------------------------
void sdr_arch_combine(const sdr_arch_t *arch, const sdr_array_t *array, int base)
{
    if (arch->scale <= 0.0) return;
    sdr_rcv_t *rcv = array->rcv;
    int nrfch = rcv->nrfch, N = rcv->N;
    int m = (int)(arch - rcv->arch);
    if (m < 0 || m >= rcv->narch || nrfch < 2) return;

    sdr_cpx8_t *out = rcv->buff[nrfch+m]->data + base;
    const sdr_cpx8_t *in[SDR_MAX_RFCH];
    for (int a = 0; a < nrfch; a++) in[a] = rcv->buff[a]->data + base;

    for (int i = 0; i < N; i++) {
        int32_t sum_re = 0, sum_im = 0;
        for (int j = 0; j < nrfch; j++) {
            int32_t I = SDR_CPX8_I(in[j][i]);
            int32_t Q = SDR_CPX8_Q(in[j][i]);
            int32_t wr = arch->w[j*2], wi = arch->w[j*2+1];
            sum_re += I * wr - Q * wi;
            sum_im += I * wi + Q * wr;
        }
        sum_re += (sum_re >= 0) ? ARRAY_W_SCALE / 2 : -ARRAY_W_SCALE / 2;
        sum_im += (sum_im >= 0) ? ARRAY_W_SCALE / 2 : -ARRAY_W_SCALE / 2;
        sum_re /= ARRAY_W_SCALE;
        sum_im /= ARRAY_W_SCALE;
        sum_re = CLIP(sum_re, -8, 7);
        sum_im = CLIP(sum_im, -8, 7);
        out[i] = SDR_CPX8(sum_re, sum_im);
    }
}
