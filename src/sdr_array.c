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

// beam-forming constants ------------------------------------------------------
#define ARRAY_W_SCALE 256       // weight Q-factor (Q8)
#define ARRAY_FREQ    1.57542e9 // L1 frequency (Hz)
#define ARRAY_FREQ_TOL 1e6      // RF CH center freq tolerance for L1 match (Hz)
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

// EKF predict + update from a single epoch -----------------------------------
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
    
    // EKF measurement update
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

//------------------------------------------------------------------------------
//  Allocate and initialize antenna array. Beam-forming requires rcv != NULL
//  and narch > 0 (uses rcv->nrfch, rcv->fs, rcv->rfch[], rcv->mtx).
//  Calibration alone works with rcv == NULL and narch == 0 (e.g., pocket_calib).
//
//  args:
//      rcv      (I)   back-pointer to SDR receiver (NULL = standalone, no
//                     beam-forming)
//      freq     (I)   frequency index (0:L1, 1:L2, ...)
//      narch    (I)   number of array channels (0..SDR_MAX_ARCH; 0 = no
//                     beam-forming)
//      ant_ena  (I)   antenna enable flags (SDR_MAX_RFCH ints; ant_ena[0]=1
//                     required for reference CH)
//      ant_pos  (I)   antenna positions in body frame (m), or NULL for zeros
//
//  returns: allocated sdr_array_t (NULL: error). When rcv is provided and
//      narch > 0, all array CH beams are initialized to default zenith
//      (equal-weight L1 sum, scale=1/rcv->nrfch).
//
sdr_array_t *sdr_array_new(sdr_rcv_t *rcv, int freq, int narch,
    const int *ant_ena, double ant_pos[][3])
{
    if (!ant_ena || !ant_ena[0]) return NULL; // CH1 must be enabled
    if (narch < 0 || narch > SDR_MAX_ARCH) return NULL;
    
    sdr_array_t *array = (sdr_array_t *)sdr_malloc(sizeof(sdr_array_t));
    array->rcv = rcv;
    array->freq = freq;
    array->narch = narch;
    for (int i = 0; i < SDR_MAX_RFCH; i++) {
        array->ant_ena[i] = ant_ena[i];
        if (ant_pos) matcpy(array->ant_pos[i], ant_pos[i], 3, 1);
    }
    // initialize all array CH beams to default zenith (equal-weight L1 sum)
    if (rcv && narch > 0) {
        for (int m = 0; m < narch; m++) {
            array->arch[m].scale = 1.0 / rcv->nrfch;
            sdr_array_set_beam(array, m, 0.0, PI / 2, array->arch[m].scale);
        }
    }
    return array;
}

//------------------------------------------------------------------------------
//  Free antenna array.
//
//  args:
//      array    (I)   antenna array
//
//  returns:
//      none
//
void sdr_array_free(sdr_array_t *array)
{
    if (!array) return;
    for (int m = 0; m < SDR_MAX_ARCH; m++) sdr_free(array->arch[m].w);
    sdr_free(array);
}

// install precomputed weights atomically; frees previous w. Caller-locked.-----
static int install_beam(sdr_array_t *array, int m, double az, double el,
    int16_t *w)
{
    if (!array || m < 0 || m >= SDR_MAX_ARCH) {
        sdr_free(w);
        return 0;
    }
    int16_t *old = array->arch[m].w;
    array->arch[m].az = az;
    array->arch[m].el = el;
    array->arch[m].w = w;
    sdr_free(old);
    return 1;
}

//------------------------------------------------------------------------------
//  Set beam direction for an array CH. Computes beam-forming weights from the
//  current array state (rpy/bias from EKF state, ant_pos from geometry) and
//  the RF CH info accessed via array->rcv. Atomically installs under
//  array->rcv->mtx.
//
//  args:
//      array    (IO)  antenna array
//      m        (I)   array CH index (0..narch-1)
//      az       (I)   beam azimuth in local ENU (rad, from N CW positive)
//      el       (I)   beam elevation (rad, above horizon)
//      scale    (I)   per-CH weight magnitude. <=0 to disable (frees w).
//
//  returns: 1:OK, 0:error
//
int sdr_array_set_beam(sdr_array_t *array, int m, double az, double el,
    double scale)
{
    if (!array || !array->rcv || m < 0 || m >= SDR_MAX_ARCH) return 0;
    sdr_rcv_t *rcv = array->rcv;
    int nrfch = rcv->nrfch;
    sdr_mutex_t *mtx = &rcv->mtx;
    
    // disable: free weight buffer
    if (scale <= 0.0) {
        sdr_mutex_lock(mtx);
        int ok = install_beam(array, m, az, el, NULL);
        sdr_mutex_unlock(mtx);
        return ok;
    }
    // snapshot rpy/bias/ant_pos under lock
    double rpy[3], bias[SDR_MAX_RFCH], ant_pos[SDR_MAX_RFCH][3];
    sdr_mutex_lock(mtx);
    matcpy(rpy, array->x, 3, 1);
    bias[0] = 0.0; // reference CH (always 0)
    matcpy(bias + 1, array->x + 4, nrfch - 1, 1);
    matcpy(&ant_pos[0][0], &array->ant_pos[0][0], nrfch * 3, 1);
    sdr_mutex_unlock(mtx);
    
    // body->ENU rotation, beam direction in body frame: e_body = R^T * e_enu
    double R[9], Dr[9], Dp[9], Dy[9], e_enu[3], e_body[3];
    euler_rot(rpy, R, Dr, Dp, Dy); // partials unused
    // compass az (CW from N) -> ENU: E = sin(az)*cos(el), N = cos(az)*cos(el)
    e_enu[0] = sin(az) * cos(el);
    e_enu[1] = cos(az) * cos(el);
    e_enu[2] = sin(el);
    matmul("TN", 3, 1, 3, 1.0, R, e_enu, 0.0, e_body);
    
    // bit-range expansion gain: smallest bits among participating L1 RF CHs
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
    
    // compute and quantize weights
    double lam = CLIGHT / ARRAY_FREQ;
    int16_t *w = (int16_t *)sdr_malloc(sizeof(int16_t) * nrfch * 2);
    for (int a = 0; a < nrfch; a++) {
        int rtoc = (rcv->rfch[a].IQ == 1);
        double f_eff = rcv->rfch[a].fo + (rtoc ? rcv->fs * 0.25 : 0.0);
        if (fabs(f_eff - ARRAY_FREQ) > ARRAY_FREQ_TOL || !array->ant_ena[a]) {
            w[a*2] = 0; w[a*2+1] = 0; // not on L1 or CH disabled
            continue;
        }
        double b_body[3];
        for (int k = 0; k < 3; k++) b_body[k] = ant_pos[a][k] - ant_pos[0][k];
        double proj = e_body[0]*b_body[0] + e_body[1]*b_body[1] +
            e_body[2]*b_body[2];
        // coherent weight w_a = exp(-j*2π/λ*proj_a) cancels the arrival phase
        // 2π/λ*proj_a; bias compensates per-CH H/W delay (calibrated value).
        double phi = -2.0 * PI * (proj - bias[a]) / lam;
        double wr = scale * bit_gain * cos(phi) * ARRAY_W_SCALE;
        double wi = scale * bit_gain * sin(phi) * ARRAY_W_SCALE;
        if (wr >  32767.0) wr =  32767.0;
        else if (wr < -32768.0) wr = -32768.0;
        if (wi >  32767.0) wi =  32767.0;
        else if (wi < -32768.0) wi = -32768.0;
        w[a*2  ] = (int16_t)floor(wr + 0.5);
        w[a*2+1] = (int16_t)floor(wi + 0.5);
    }
    // atomic install (frees old w)
    sdr_mutex_lock(mtx);
    int ok = install_beam(array, m, az, el, w);
    sdr_mutex_unlock(mtx);
    return ok;
}

//------------------------------------------------------------------------------
//  Get current beam direction for an array CH.
//
int sdr_array_get_beam(const sdr_array_t *array, int m, double *az, double *el)
{
    if (!array || m < 0 || m >= SDR_MAX_ARCH) return 0;
    if (az) *az = array->arch[m].az;
    if (el) *el = array->arch[m].el;
    return 1;
}

//------------------------------------------------------------------------------
//  Combine RF CH IF data into array CH IF data using current beam weights.
//  Reads rcv->buff, rcv->N via array->rcv. Snapshots weights atomically
//  (under rcv->mtx) then performs combination without lock.
//
//  args:
//      array    (I)   antenna array (must have rcv != NULL)
//      base     (I)   starting offset in each buffer
//
void sdr_array_combine(sdr_array_t *array, int base)
{
    if (!array || !array->rcv) return;
    sdr_rcv_t *rcv = array->rcv;
    int nrfch = rcv->nrfch, narch = array->narch, N = rcv->N;
    if (narch <= 0 || nrfch < 2) return;
    sdr_buff_t **buff = rcv->buff;
    int16_t w_snap[SDR_MAX_ARCH][SDR_MAX_RFCH * 2];
    int active[SDR_MAX_ARCH] = {0}, any = 0;
    
    sdr_mutex_lock(&rcv->mtx);
    for (int m = 0; m < narch; m++) {
        if (array->arch[m].w) {
            memcpy(w_snap[m], array->arch[m].w,
                sizeof(int16_t) * nrfch * 2);
            active[m] = 1;
            any = 1;
        }
    }
    sdr_mutex_unlock(&rcv->mtx);
    if (!any) return;
    
    for (int m = 0; m < narch; m++) {
        if (!active[m]) continue;
        const int16_t *w = w_snap[m];
        sdr_cpx8_t *out = buff[nrfch + m]->data + base;
        const sdr_cpx8_t *in[SDR_MAX_RFCH];
        for (int a = 0; a < nrfch; a++) in[a] = buff[a]->data + base;
        
        for (int i = 0; i < N; i++) {
            int32_t sum_re = 0, sum_im = 0;
            for (int a = 0; a < nrfch; a++) {
                int8_t I = SDR_CPX8_I(in[a][i]);
                int8_t Q = SDR_CPX8_Q(in[a][i]);
                int32_t wr = w[a*2], wi = w[a*2+1];
                sum_re += (int32_t)I * wr - (int32_t)Q * wi;
                sum_im += (int32_t)I * wi + (int32_t)Q * wr;
            }
            sum_re += (sum_re >= 0 ? ARRAY_W_SCALE/2 : -(ARRAY_W_SCALE/2));
            sum_im += (sum_im >= 0 ? ARRAY_W_SCALE/2 : -(ARRAY_W_SCALE/2));
            sum_re /= ARRAY_W_SCALE;
            sum_im /= ARRAY_W_SCALE;
            sum_re = CLIP(sum_re * 2, -8, 7);
            sum_im = CLIP(sum_im * 2, -8, 7);
            out[i] = SDR_CPX8(sum_re, sum_im);
        }
    }
}

//------------------------------------------------------------------------------
//  Calibrate antenna array hardware delays and attitude (per-epoch EKF).
//
//  Maintains an EKF state across calls. The first call (P all zero) initializes
//  the attitude by a yaw grid search with NLSQ on this epoch's data and selects
//  the yaw with the smallest residual RMS. Subsequent calls run a standard
//  EKF predict (random walk) + update on the new epoch's measurements.
//
//  args:
//      array    (IO)  antenna array state. ant_pos is input (element positions
//                     in body frame, m; X=right, Y=forward, Z=up). x, P, rms
//                     are updated. Pass with x and P zero-initialized on the
//                     first call to trigger initialization. Number of array
//                     elements is derived as max enabled index in ant_ena[]+1.
//      obs      (I)   single-epoch obs data (length nobs); .rcv = element no
//      nobs     (I)   number of obs data in the epoch
//      nav      (I)   navigation data
//      rr       (I)   receiver ECEF position {x,y,z} (m)
//
//  returns: status (1:ok, 0:error)
//
int sdr_array_calib(sdr_array_t *array, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr)
{
    if (!array || nobs <= 0) return 0;
    
    if (array->P[0] == 0.0) {
        return kf_init(array, obs, nobs, nav, rr);
    } else {
        return kf_update(array, obs, nobs, nav, rr);
    }
}
