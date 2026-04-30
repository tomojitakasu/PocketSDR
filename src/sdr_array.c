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
//  Allocate and initialize antenna array.
//
//  Allocates a new sdr_array_t with calibration state zero-initialized and
//  rfch_ena set to all enabled. Antenna positions in body frame are copied
//  from ant_pos if non-NULL; otherwise initialized to zero (caller can fill
//  later via array->ant_pos[][]).
//
//  args:
//      freq     (I)   frequency index (0:L1, 1:L2, ...)
//      ant_ena  (I)   antenna enable flags (SDR_MAX_RFCH x 1)
//      ant_pos  (I)   antenna positions in body frame (m) (SDR_MAX_RFCH x 3)
//
//  returns:
//      antenna array (NULL: error)
//
sdr_array_t *sdr_array_new(int freq, const int *ant_ena, double ant_pos[][3])
{
    if (!ant_ena[0]) return NULL; // CH1 must be enabled

    sdr_array_t *array = (sdr_array_t *)sdr_malloc(sizeof(sdr_array_t));

    array->freq = freq;
    for (int i = 0; i < SDR_MAX_RFCH; i++) {
        array->ant_ena[i] = ant_ena[i];
        matcpy(array->ant_pos[i], ant_pos[i], 3, 1);
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
    sdr_free(array);
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
//                     elements is derived as max enabled index in rfch_ena+1.
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
