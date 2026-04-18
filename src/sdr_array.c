//
//  Pocket SDR C Library - Antenna Array Functions
//
//  Author:
//  T.TAKASU
//
//  History:
//  2026-04-17  1.0  new
//
#include "pocket_sdr.h"

#define DPI         (2.0 * PI)
#define YAW_STEP    30.0        // yaw search step (deg)
#define MAX_ITER    20          // max NLSQ iterations per yaw initial
#define CONV_THRES_A 1e-6       // angle update convergence (rad)
#define CONV_THRES_B 1e-5       // bias update convergence (m)
#define MIN_EL      15.0        // elevation mask (deg)
#define MIN_MEAS    8           // min number of SD measurements
#define MAX_RMS     0.03        // max RMS of residuals (m)
#define ROUND(x)    floor((x) + 0.5)

// rotation matrix and partials by Euler angles (Z-Y-X: yaw,pitch,roll) --------
static void euler_rot(const double *rpy, double *R, double *Dr, double *Dp,
    double *Dy)
{
    double cr = cos(rpy[0]), sr = sin(rpy[0]), cp = cos(rpy[1]);
    double sp = sin(rpy[1]), cy = cos(rpy[2]), sy = sin(rpy[2]);
    
    R [0] =  cp*cy;  R [3] =  sr*sp*cy - cr*sy; R [6] =  cr*sp*cy + sr*sy;
    R [1] =  cp*sy;  R [4] =  sr*sp*sy + cr*cy; R [7] =  cr*sp*sy - sr*cy;
    R [2] = -sp;     R [5] =  sr*cp;            R [8] =  cr*cp;
    Dr[0] =  0.0;    Dr[3] =  cr*sp*cy + sr*sy; Dr[6] = -sr*sp*cy + cr*sy;
    Dr[1] =  0.0;    Dr[4] =  cr*sp*sy - sr*cy; Dr[7] = -sr*sp*sy - cr*cy;
    Dr[2] =  0.0;    Dr[5] =  cr*cp;            Dr[8] = -sr*cp;
    Dp[0] = -sp*cy;  Dp[3] =  sr*cp*cy;         Dp[6] =  cr*cp*cy;
    Dp[1] = -sp*sy;  Dp[4] =  sr*cp*sy;         Dp[7] =  cr*cp*sy;
    Dp[2] = -cp;     Dp[5] = -sr*sp;            Dp[8] = -cr*sp;
    Dy[0] = -cp*sy;  Dy[3] = -sr*sp*sy - cr*cy; Dy[6] = -cr*sp*sy + sr*cy;
    Dy[1] =  cp*cy;  Dy[4] =  sr*sp*cy - cr*sy; Dy[7] =  cr*sp*cy + sr*sy;
    Dy[2] =  0.0;    Dy[5] =  0.0;              Dy[8] =  0.0;
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

// build normal equations ------------------------------------------------------
static int build_neq(obsd_t * const *obs, const int *nobs, int nep,
    const nav_t *nav, const double *rr, const double *ant_pos, int nant,
    int freq, double *rpy, const double *bias, double *H, double *v)
{
    double pos[3], R[9], Dr[9], Dp[9], Dy[9];
    int nv = 0, nx = 3 + nant - 1, nv_max = MAXOBS * (nant - 1) * nep;

    ecef2pos(rr, pos);
    euler_rot(rpy, R, Dr, Dp, Dy);
    
    for (int k = 0; k < nep; k++) {
        for (int i = 0; i < nobs[k]; i++) {
            const obsd_t *p = obs[k] + i;
            double f, lam, e[3];
            
            // search CH1
            if (p->rcv != 1 || p->L[freq] == 0.0) continue;
            if (!los_vec(p->time, p->sat, nav, pos, rr, e)) continue;
            if ((f = sat2freq(p->sat, p->code[freq], nav)) == 0.0) continue;
            lam = CLIGHT / f;
            
            // generate SD
            for (int j = 0; j < nobs[k] && nv < nv_max; j++) {
                const obsd_t *q = obs[k] + j;
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
                memset(H + nx * nv, 0, sizeof(double) * nx);
                for (int l = 0; l < 3; l++) H[nx*nv+l] = -hrpy[l];
                H[nx*nv+3+(a-2)] = 1.0;
                v[nv++] = dr;
            }
        }
    }
    return nv;
}

// estimate H/W delays and attitude --------------------------------------------
static int est_bias_att(obsd_t * const *obs, const int *nobs, int nep,
    const nav_t *nav, const double *rr, const double *ant_pos, int nant,
    int freq, double *rpy, double *bias, double *rms, double *H, double *v)
{
    int nx = 3 + nant - 1;
    double dx[3+SDR_MAX_RFCH], Q[(3+SDR_MAX_RFCH)*(3+SDR_MAX_RFCH)];
    int iter, nv = 0, ret = 0;

    for (iter = 0; iter < MAX_ITER; iter++) {
        
        // build normal equations
        nv = build_neq(obs, nobs, nep, nav, rr, ant_pos, nant, freq, rpy, bias,
            H, v);
        
        // least square estimation
        if (nv < MIN_MEAS || lsq(H, v, nx, nv, dx, Q)) break;

        for (int i = 0; i < 3; i++) rpy[i] += dx[i];
        for (int i = 1; i < nant; i++) bias[i] += dx[2+i];
        *rms = sqrt(dot(v, v, nv) / nv);

        // test convergence
        if (norm(dx, 3) < CONV_THRES_A && norm(dx + 3, nx - 3) < CONV_THRES_B) {
            rpy[2] -= DPI * floor((rpy[2] + PI) / DPI);
            ret = 1;
            break;
        }
    }
#if 0 // for debug
    printf("iter=%2d,nv=%d,rms=%.5f\n", iter, nv, *rms);
#endif
    return ret;
}

//------------------------------------------------------------------------------
//  Calibrate antenna array hardware delays and attitude.
//
//  args:
//      obs      (I)  array of per-epoch obs data. obs[k] points to k-th
//                    epoch's obs data of length nobs[k]. .rcv indicates
//                    array element no (1,2,...)
//      nobs,nep (I)  number of obs data per epoch and epochs
//      nav      (I)  navigation data
//      ant_pos  (I)  array element positions in body frame (m)
//      nant     (I)  number of array elements (<= SDR_MAX_RFCH)
//      freq     (I)  frequency index (0:L1,1:L2,...)
//      rr       (I)  receiver ECEF position {x,y,z} (m)
//      bias     (O)  hardware delays wrt CH1 (m) (nant x 1) (bias[0] = 0)
//      rpy      (O)  attitude {roll,pitch,yaw} (rad)
//      rms      (O)  RMS of residuals (m)
//
//  returns: status (1:OK, 0:error)
//
int array_calib(obsd_t * const *obs, const int *nobs, int nep, const nav_t *nav,
    const double *ant_pos, int nant, int freq, const double *rr, double *bias,
    double *rpy, double *rms)
{
    if (nant < 2 || nant > SDR_MAX_RFCH || freq < 0 || freq >= NFREQ ||
        nep <= 0) {
        return 0;
    }
    int nx = 3 + nant - 1, nv_max = MAXOBS * (nant - 1) * nep;
    double *H = mat(nx, nv_max), *v = mat(nv_max, 1);
    
    *rms = 1e9;

    for (double y = -PI; y < PI; y += YAW_STEP * D2R) {
        double rpy_s[] = {0.0, 0.0, y}, bias_s[SDR_MAX_RFCH] = {0};
        double rms_s = 0.0;
        
        // estimate H/W delays and attitude
        if (!est_bias_att(obs, nobs, nep, nav, rr, ant_pos, nant, freq,
            rpy_s, bias_s, &rms_s, H, v)) {
            continue;
        }
        if (rms_s < *rms) {
            matcpy(rpy, rpy_s, 3, 1);
            matcpy(bias, bias_s, nant, 1);
            *rms = rms_s;
        }
    }
    free(H); free(v);
    return *rms <= MAX_RMS;
}
