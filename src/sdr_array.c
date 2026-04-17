//
//  Pocket SDR C Library - Antenna Array Live Calibration.
//
//  Given the array geometry, per-element carrier-phase observations over
//  some period, and the navigation data, this estimates the per-element
//  hardware delay (referenced to CH1) and the array attitude (roll, pitch,
//  yaw) by non-linear least squares on SD carrier-phase only.
//
//  The integer ambiguity is resolved by rounding to the nearest cycle, which
//  is valid as long as element spacing and hardware delays are both within
//  +/- lambda/2. The yaw initial is searched at 15-deg intervals; roll and
//  pitch initials are fixed to zero.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2026-04-17  1.0  new
//  2026-04-17  1.1  SD carrier-phase only (pseudorange dropped)
//
#include "pocket_sdr.h"

// constants and macros --------------------------------------------------------
#define YAW_STEP    15.0        // yaw search step (deg)
#define MAX_ITER    20          // max NLSQ iterations per yaw initial
#define CONV_THRES  1e-5        // state update convergence (m or rad)
#define MIN_EL      15.0        // elevation mask (deg)
#define MIN_MEAS    8           // min number of SD measurements

// rotation matrix and partials by Euler angles (Z-Y-X: yaw,pitch,roll) --------
static void euler_rot(double roll, double pitch, double yaw, double *R,
    double *Dr, double *Dp, double *Dy)
{
    double cr = cos(roll), sr = sin(roll);
    double cp = cos(pitch), sp = sin(pitch);
    double cy = cos(yaw), sy = sin(yaw);

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

// find observation slot matching calibration frequency ------------------------
//   Uses sat2freq() so GLONASS FCN-dependent frequencies are handled.
static int freq_slot(const obsd_t *ob, const nav_t *nav, double freq)
{
    for (int j = 0; j < NFREQ + NEXOBS; j++) {
        if (!ob->code[j]) continue;
        double f = sat2freq(ob->sat, ob->code[j], nav);
        if (f > 0.0 && fabs(f - freq) < 1.0) return j;
    }
    return -1;
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
    const double *Dr, const double *Dp, const double *Dy,
    double *hr, double *hp, double *hy)
{
    double v[3], vr[3], vp[3], vy[3];

    matmul("NN", 3, 1, 3, 1.0, R,  b, 0.0, v );
    matmul("NN", 3, 1, 3, 1.0, Dr, b, 0.0, vr);
    matmul("NN", 3, 1, 3, 1.0, Dp, b, 0.0, vp);
    matmul("NN", 3, 1, 3, 1.0, Dy, b, 0.0, vy);
    *hr = dot(e, vr, 3);
    *hp = dot(e, vp, 3);
    *hy = dot(e, vy, 3);
    return dot(e, v, 3);
}

// build normal equations for one iteration ------------------------------------
//   x = [droll, dpitch, dyaw, dbias_2, ..., dbias_narr]
//   bias[i] is hardware delay of element i+1 in meters (i=0 is reference, =0)
//   Uses SD carrier-phase with ambiguity resolved by rounding.
static int build_eq(obsd_t * const *obs, const int *nobs, int nep,
    const nav_t *nav, const double *rr, const double *ant_pos, int narr,
    double freq, double roll, double pitch, double yaw, const double *bias,
    double *H, double *v, double *res_sqr, int max_meas)
{
    double R[9], Dr[9], Dp[9], Dy[9], pos[3], lam = CLIGHT / freq;
    int m = 0, nx = 3 + narr - 1;

    euler_rot(roll, pitch, yaw, R, Dr, Dp, Dy);
    ecef2pos(rr, pos);
    *res_sqr = 0.0;

    for (int k = 0; k < nep; k++) {
        const obsd_t *ob = obs[k];
        int n = nobs[k];

        for (int i = 0; i < n; i++) { // look for ref (CH1) obs
            if (ob[i].rcv != 1) continue;
            int sat = ob[i].sat, j0 = freq_slot(ob + i, nav, freq);
            if (j0 < 0 || ob[i].L[j0] == 0.0) continue;

            double e[3];
            if (!los_vec(ob[i].time, sat, nav, pos, rr, e)) continue;

            for (int ii = 0; ii < n; ii++) { // SD with other elements
                int a = ob[ii].rcv;
                if (ob[ii].sat != sat || a <= 1 || a > narr) continue;
                int ja = freq_slot(ob + ii, nav, freq);
                if (ja < 0 || ob[ii].L[ja] == 0.0 || m >= max_meas) continue;

                double b_body[3], hr, hp, hy;
                for (int d = 0; d < 3; d++) {
                    b_body[d] = ant_pos[(a-1)*3+d] - ant_pos[d];
                }
                double proj = sd_proj(e, b_body, R, Dr, Dp, Dy, &hr, &hp, &hy);
                double model = -proj + bias[a-1];
                double sd_cp = lam * (ob[ii].L[ja] - ob[i].L[j0]);
                double r0 = sd_cp - model;
                double r_cp = r0 - lam * round(r0 / lam);

                for (int d = 0; d < nx; d++) H[nx*m+d] = 0.0;
                H[nx*m  ] = -hr;
                H[nx*m+1] = -hp;
                H[nx*m+2] = -hy;
                H[nx*m+3+(a-2)] = 1.0;
                v[m] = r_cp;
                *res_sqr += r_cp * r_cp;
                m++;
            }
        }
    }
    return m;
}

// run non-linear LSQ iterations -----------------------------------------------
//   Returns number of measurements on convergence, 0 on failure.
//   On success, updates roll, pitch, yaw, bias[] and sets *rms_out (m).
//   On failure, *roll/pitch/yaw/bias[] are left untouched.
static int nlsq(obsd_t * const *obs, const int *nobs, int nep,
    const nav_t *nav, const double *rr, const double *ant_pos, int narr,
    double freq, double *roll, double *pitch, double *yaw, double *bias,
    double *rms_out)
{
    int nx = 3 + narr - 1;
    int max_meas = MAXSAT * narr * nep;
    double *H = mat(nx, max_meas), *v = mat(max_meas, 1);
    double dx[3+SDR_MAX_RFCH], Q[(3+SDR_MAX_RFCH)*(3+SDR_MAX_RFCH)];
    double r = *roll, p = *pitch, y = *yaw, b[SDR_MAX_RFCH];
    int m = 0, ret = 0;
    double res_sqr = 0.0;

    for (int i = 0; i < narr; i++) b[i] = bias[i];

    for (int iter = 0; iter < MAX_ITER; iter++) {
        m = build_eq(obs, nobs, nep, nav, rr, ant_pos, narr, freq,
            r, p, y, b, H, v, &res_sqr, max_meas);
        if (m < MIN_MEAS || m < nx) break;
        if (lsq(H, v, nx, m, dx, Q)) break;

        r += dx[0];
        p += dx[1];
        y += dx[2];
        for (int i = 0; i < narr - 1; i++) b[i+1] += dx[3+i];

        sdr_log(4, "$LOG,NLSQ_ITER,%d,m=%d,rms=%.4f,|dx|=%.3e,"
            "rpy=%.3f,%.3f,%.3f", iter, m, sqrt(res_sqr / m), norm(dx, nx),
            r * R2D, p * R2D, y * R2D);

        if (norm(dx, nx) < CONV_THRES) {
            // recompute residuals at the final (post-dx) state
            m = build_eq(obs, nobs, nep, nav, rr, ant_pos, narr, freq,
                r, p, y, b, H, v, &res_sqr, max_meas);
            *roll = r; *pitch = p; *yaw = y;
            for (int i = 0; i < narr; i++) bias[i] = b[i];
            ret = m;
            break;
        }
    }
    free(H); free(v);
    *rms_out = (ret > 0) ? sqrt(res_sqr / m) : 1e99;
    return ret;
}

//------------------------------------------------------------------------------
//  Calibrate antenna array hardware delays and attitude.
//
//  Per-element SD carrier-phase residuals against the array geometry model
//  are minimized by iterative non-linear least squares. The yaw initial is
//  searched over [-180, 180) deg at YAW_STEP-deg intervals. The integer
//  ambiguity is resolved by rounding each iteration.
//
//  args:
//      obs      (I)  array of per-epoch observation data. obs[k] points to the
//                    k-th epoch's obsd_t array of length nobs[k]. Each obsd_t's
//                    .rcv field identifies the array element (1:reference CH1,
//                    2..narr: other elements).
//      nobs     (I)  number of observations per epoch [nep]
//      nep      (I)  number of epochs
//      nav      (I)  navigation data (broadcast ephemerides used)
//      ant_pos  (I)  array element positions in body frame (m), length 3*narr
//                    (ant_pos[3*i+0..2] = element (i+1) position {x,y,z})
//      narr     (I)  number of array elements (<= SDR_MAX_RFCH)
//      freq     (I)  carrier frequency to calibrate (Hz, e.g. FREQ1)
//      rr       (I)  approximate receiver position in ECEF (m) [3]
//      bias     (O)  hardware delay of each element w.r.t. CH1 (s) [narr].
//                    bias[0] is always 0 (reference).
//      rpy      (O)  array attitude roll, pitch, yaw (rad) [3]
//      cp_rms   (O)  RMS of SD carrier-phase residuals at the solution (m).
//                    May be NULL.
//
//  returns:
//      Number of SD measurements used (>0 on success), 0 on failure
//      (insufficient data or non-convergence).
//
int array_calib(obsd_t * const *obs, const int *nobs, int nep,
    const nav_t *nav, const double *ant_pos, int narr, double freq,
    const double *rr, double *bias, double *rpy, double *cp_rms)
{
    if (!obs || !nobs || !nav || !ant_pos || !rr || !bias || !rpy) return 0;
    if (narr < 2 || narr > SDR_MAX_RFCH || nep <= 0 || freq <= 0.0) return 0;

    double best_rms = 1e99, best_rpy[3] = {0};
    double best_bias[SDR_MAX_RFCH] = {0};
    int best_m = 0;

    for (double y0 = -180.0; y0 < 180.0; y0 += YAW_STEP) {
        double r = 0.0, p = 0.0, y = y0 * D2R, rms = 0.0;
        double b[SDR_MAX_RFCH] = {0};
        int m = nlsq(obs, nobs, nep, nav, rr, ant_pos, narr, freq,
            &r, &p, &y, b, &rms);
        sdr_log(4, "$LOG,YAW_GRID,y0=%.1f,m=%d,rms=%.4f,rpy=%.2f,%.2f,%.2f",
            y0, m, rms, r * R2D, p * R2D, y * R2D);
        if (m > 0 && rms < best_rms) {
            best_rms = rms;
            best_rpy[0] = r; best_rpy[1] = p; best_rpy[2] = y;
            for (int i = 0; i < narr; i++) best_bias[i] = b[i];
            best_m = m;
        }
    }
    if (best_m == 0) return 0;

    // normalize yaw to [-pi, pi)
    best_rpy[2] -= 2.0 * PI * floor((best_rpy[2] + PI) / (2.0 * PI));

    rpy[0] = best_rpy[0];
    rpy[1] = best_rpy[1];
    rpy[2] = best_rpy[2];
    bias[0] = 0.0;
    for (int i = 1; i < narr; i++) bias[i] = best_bias[i] / CLIGHT;
    if (cp_rms) *cp_rms = best_rms;

    sdr_log(3, "$LOG,ARRAY_CALIB,NEP=%d,NARR=%d,M=%d,RMS=%.4f,"
        "RPY=%.2f,%.2f,%.2f", nep, narr, best_m, best_rms,
        rpy[0] * R2D, rpy[1] * R2D, rpy[2] * R2D);

    return best_m;
}
