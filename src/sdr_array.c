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
#define YAW_STEP    30.0        // yaw search step (deg)
#define MAX_ITER    15          // max NLSQ iterations for init
#define CONV_THRES_A 1e-6       // angle update convergence (rad)
#define CONV_THRES_B 1e-5       // bias update convergence (m)
#define MIN_EL      15.0        // elevation mask (deg)
#define ROUND(x)    floor((x) + 0.5)

#define VAR_INIT_RPY  (1.0)     // initial covariance for rpy after init (rad^2)
#define VAR_INIT_BIAS (1.0)     // initial covariance for bias after init (m^2)
#define VAR_PROC_RPY  (1e-4)    // process noise variance per epoch (rad^2)
#define VAR_PROC_BIAS (1e-8)    // process noise variance per epoch (m^2)
#define VAR_MEAS      (1e-4)    // measurement variance (m^2) (~1cm phase noise)

// rotation matrix and partials by Euler angles (Z-X-Y: yaw,pitch,roll) --------
// Body frame: X=right, Y=forward, Z=up. R maps body to local (ENU).
//   roll (rpy[0]) : rotation about body Y (forward)
//   pitch(rpy[1]) : rotation about body X (right)
//   yaw  (rpy[2]) : rotation about body Z (up)
//   R = Rz(yaw) * Rx(pitch) * Ry(roll)
//
// Yaw sign convention: CCW positive looking from +Z (right-hand rule, math
// convention). At yaw=0 body Y axis aligns with ENU +Y (north). Increasing
// yaw rotates body Y from north toward west.
// Note: this is OPPOSITE in sign to compass heading (CW positive from north).
// Equivalence: compass_heading_of_body_Y = -yaw  (modulo 360 deg).
// Beam direction in sdr_rcv_set_array_beam() takes az in compass convention
// (from N CW positive) and converts to body frame using R^T internally.
static void euler_rot(const double *rpy, double *R, double *Dr, double *Dp,
    double *Dy)
{
    double cr = cos(rpy[0]), sr = sin(rpy[0]), cp = cos(rpy[1]);
    double sp = sin(rpy[1]), cy = cos(rpy[2]), sy = sin(rpy[2]);
    
    R [0] =  cy*cr - sy*sp*sr; R [3] = -sy*cp; R [6] =  cy*sr + sy*sp*cr;
    R [1] =  sy*cr + cy*sp*sr; R [4] =  cy*cp; R [7] =  sy*sr - cy*sp*cr;
    R [2] = -cp*sr;            R [5] =  sp;    R [8] =  cp*cr;
    Dr[0] = -cy*sr - sy*sp*cr; Dr[3] =  0.0;   Dr[6] =  cy*cr - sy*sp*sr;
    Dr[1] = -sy*sr + cy*sp*cr; Dr[4] =  0.0;   Dr[7] =  sy*cr + cy*sp*sr;
    Dr[2] = -cp*cr;            Dr[5] =  0.0;   Dr[8] = -cp*sr;
    Dp[0] = -sy*cp*sr;         Dp[3] =  sy*sp; Dp[6] =  sy*cp*cr;
    Dp[1] =  cy*cp*sr;         Dp[4] = -cy*sp; Dp[7] = -cy*cp*cr;
    Dp[2] =  sp*sr;            Dp[5] =  cp;    Dp[8] = -sp*cr;
    Dy[0] = -sy*cr - cy*sp*sr; Dy[3] = -cy*cp; Dy[6] = -sy*sr + cy*sp*cr;
    Dy[1] =  cy*cr - sy*sp*sr; Dy[4] = -sy*cp; Dy[7] =  cy*sr + sy*sp*cr;
    Dy[2] =  0.0;              Dy[5] =  0.0;   Dy[8] =  0.0;
}

// LOS unit vector from receiver to satellite in local (ENU) frame -------------
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

// SD model term e.(R*b) and partials wrt roll/pitch/yaw -----------------------
static double sd_proj(const double *e, const double *b, const double *R,
    const double *Dr, const double *Dp, const double *Dy, double *hrpy)
{
    double v[3], vr[3], vp[3], vy[3];
    
    matmul("NN", 3, 1, 3, 1.0, R,  b, 0.0, v );
    matmul("NN", 3, 1, 3, 1.0, Dr, b, 0.0, vr);
    matmul("NN", 3, 1, 3, 1.0, Dp, b, 0.0, vp);
    matmul("NN", 3, 1, 3, 1.0, Dy, b, 0.0, vy);
    hrpy[0] = dot(e, vr, 3);
    hrpy[1] = dot(e, vp, 3);
    hrpy[2] = dot(e, vy, 3);
    return dot(e, v, 3);
}

// build SD measurements and design matrix for single epoch -------------------
//   H stored column-major (nx x nv): H[state + nv*nx]
static int build_meas(const obsd_t *obs, int nobs, const nav_t *nav,
    const double *rr, const double *ant_pos, int nant, int freq,
    const double *rpy, const double *bias, double *H, double *v)
{
    double pos[3], R[9], Dr[9], Dp[9], Dy[9];
    int nv = 0, nx = 3 + nant - 1, nv_max = MAXOBS * (nant - 1);
    
    ecef2pos(rr, pos);
    euler_rot(rpy, R, Dr, Dp, Dy);
    
    for (int i = 0; i < nobs; i++) {
        const obsd_t *p = obs + i;
        double f, lam, e[3];
        
        // search CH1
        if (p->rcv != 1 || p->L[freq] == 0.0) continue;
        if (!los_vec(p->time, p->sat, nav, pos, rr, e)) continue;
        if ((f = sat2freq(p->sat, p->code[freq], nav)) == 0.0) continue;
        lam = CLIGHT / f;
        
        // generate SD
        for (int j = 0; j < nobs && nv < nv_max; j++) {
            const obsd_t *q = obs + j;
            double b_body[3], hrpy[3], proj, dr;
            int a = q->rcv;
            
            if (q->sat != p->sat || a <= 1 || a > nant) continue;
            if (q->L[freq] == 0.0) continue;
            
            for (int l = 0; l < 3; l++) {
                b_body[l] = ant_pos[(a-1)*3+l] - ant_pos[l];
            }
            proj = sd_proj(e, b_body, R, Dr, Dp, Dy, hrpy);
            dr = lam * (q->L[freq] - p->L[freq]) - (bias[a-1] - proj);
            dr -= lam * ROUND(dr / lam);
            for (int l = 0; l < nx; l++) H[l + nv*nx] = 0.0;
            for (int l = 0; l < 3; l++) H[l + nv*nx] = -hrpy[l];
            H[3 + (a-2) + nv*nx] = 1.0;
            v[nv++] = dr;
        }
    }
    return nv;
}

// NLSQ for attitude/bias from a single epoch (used by initialization) --------
static int nlsq_init(const obsd_t *obs, int nobs, const nav_t *nav,
    const double *rr, const double *ant_pos, int nant, int freq, double *rpy,
    double *bias, double *rms, double *H, double *v)
{
    int nx = 3 + nant - 1;
    double dx[3+SDR_MAX_RFCH], Q[(3+SDR_MAX_RFCH)*(3+SDR_MAX_RFCH)];
    
    for (int iter = 0; iter < MAX_ITER; iter++) {
        int nv = build_meas(obs, nobs, nav, rr, ant_pos, nant, freq, rpy, bias,
            H, v);
        if (nv < nx || lsq(H, v, nx, nv, dx, Q)) break;
        
        for (int i = 0; i < 3; i++) rpy[i] += dx[i];
        for (int i = 1; i < nant; i++) bias[i] += dx[2+i];
        *rms = sqrt(dot(v, v, nv) / nv);
        
        if (norm(dx, 3) < CONV_THRES_A && norm(dx + 3, nx - 3) < CONV_THRES_B) {
            rpy[2] -= DPI * floor((rpy[2] + PI) / DPI);
            return 1;
        }
    }
    return 0;
}

// initialize state by yaw grid search + NLSQ ---------------------------------
static int kf_init(const obsd_t *obs, int nobs, const nav_t *nav,
    const double *rr, const double *ant_pos, int nant, int freq, double *x,
    double *P, double *rms, double *H, double *v)
{
    int nx = 3 + nant - 1, ok = 0;
    double rpy_b[3] = {0}, bias_b[SDR_MAX_RFCH] = {0}, rms_b = 1e9;
    
    for (double y = -PI; y < PI; y += YAW_STEP * D2R) {
        double rpy_s[] = {0.0, 0.0, y}, bias_s[SDR_MAX_RFCH] = {0};
        double rms_s = 0.0;
        
        if (!nlsq_init(obs, nobs, nav, rr, ant_pos, nant, freq, rpy_s, bias_s,
            &rms_s, H, v)) {
            continue;
        }
        if (rms_s < rms_b) {
            matcpy(rpy_b, rpy_s, 3, 1);
            matcpy(bias_b, bias_s, nant, 1);
            rms_b = rms_s;
            ok = 1;
        }
    }
    if (!ok) return 0;
    
    for (int i = 0; i < 3; i++) x[i] = rpy_b[i];
    for (int i = 1; i < nant; i++) x[2+i] = bias_b[i];
    memset(P, 0, sizeof(double) * nx * nx);
    for (int i = 0; i < 3; i++) P[i + i*nx] = VAR_INIT_RPY;
    for (int i = 3; i < nx; i++) P[i + i*nx] = VAR_INIT_BIAS;
    *rms = rms_b;
    return 1;
}

// EKF predict + update from a single epoch -----------------------------------
static int kf_update(const obsd_t *obs, int nobs, const nav_t *nav,
    const double *rr, const double *ant_pos, int nant, int freq, double *x,
    double *P, double *rms, double *H, double *v)
{
    int nx = 3 + nant - 1;
    double rpy[3], bias[SDR_MAX_RFCH] = {0};
    
    // predict: P += Q (random walk; identity transition)
    for (int i = 0; i < 3; i++) P[i + i*nx] += VAR_PROC_RPY;
    for (int i = 3; i < nx; i++) P[i + i*nx] += VAR_PROC_BIAS;
    
    // unpack state
    for (int i = 0; i < 3; i++) rpy[i] = x[i];
    for (int i = 1; i < nant; i++) bias[i] = x[2+i];
    
    // build linearized measurements at current x
    int nv = build_meas(obs, nobs, nav, rr, ant_pos, nant, freq, rpy, bias,
        H, v);
    if (nv < 1) return 0;
    
    // R = sigma^2 * I
    double *R = mat(nv, nv);
    memset(R, 0, sizeof(double) * nv * nv);
    for (int i = 0; i < nv; i++) R[i + i*nv] = VAR_MEAS;
    
    // EKF measurement update via RTKLIB filter()
    //   note: filter() skips state i where x[i]==0; init guarantees nonzero.
    int info = filter(x, P, H, v, R, nx, nv);
    free(R);
    if (info) return 0;
    
    // wrap yaw to [-pi, pi)
    x[2] -= DPI * floor((x[2] + PI) / DPI);
    
    // post-fit residual RMS
    for (int i = 0; i < 3; i++) rpy[i] = x[i];
    for (int i = 1; i < nant; i++) bias[i] = x[2+i];
    nv = build_meas(obs, nobs, nav, rr, ant_pos, nant, freq, rpy, bias, H, v);
    *rms = nv > 0 ? sqrt(dot(v, v, nv) / nv) : 0.0;
    return 1;
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
//      obs      (I)   single-epoch obs data (length nobs); .rcv = element no
//      nobs     (I)   number of obs data in the epoch
//      nav      (I)   navigation data
//      ant_pos  (I)   array element positions in body frame (m)
//                     (X=right, Y=forward, Z=up)
//      nant     (I)   number of array elements (<= SDR_MAX_RFCH)
//      freq     (I)   frequency index (0:L1, 1:L2, ...)
//      rr       (I)   receiver ECEF position {x,y,z} (m)
//      x        (IO)  state vector (3+nant-1 x 1):
//                     {roll, pitch, yaw, bias_2..bias_nant} (rad, m).
//                     Pass zeros on the first call to trigger initialization.
//      P        (IO)  state covariance matrix (nx x nx, column-major).
//                     Pass zeros on the first call to trigger initialization.
//      rms      (O)   residual RMS for this epoch (m)
//
//  returns: status (1:ok, 0:error)
//
int sdr_array_calib(const obsd_t *obs, int nobs, const nav_t *nav,
    const double *ant_pos, int nant, int freq, const double *rr, double *x,
    double *P, double *rms)
{
    if (nant < 2 || nant > SDR_MAX_RFCH || freq < 0 || freq >= NFREQ ||
        nobs <= 0) {
        return 0;
    }
    int nx = 3 + nant - 1, nv_max = MAXOBS * (nant - 1);
    double *H = mat(nx, nv_max), *v = mat(nv_max, 1);
    int ret;
for (int i = 0; i < 8; i++) {
printf("ant_pos %d: %8.4f %8.4f %8.4f\n", i + 1, ant_pos[i*3], ant_pos[1+i*3], ant_pos[2+i*3]);
}
double pos[3];
ecef2pos(rr, pos);
printf("rcv_pos   : %8.6f %8.6f %8.4f\n", pos[0] * R2D, pos[1] * R2D, pos[2]);
    
    *rms = 0.0;
    
    if (P[0] == 0.0) {
        ret = kf_init(obs, nobs, nav, rr, ant_pos, nant, freq, x, P, rms, H, v);
    } else {
        ret = kf_update(obs, nobs, nav, rr, ant_pos, nant, freq, x, P, rms, H,
            v);
    }
    free(H); free(v);
    return ret;
}
