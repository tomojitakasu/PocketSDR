//
//  Pocket SDR C Library - GNSS SDR PVT Functions
//
//  Author:
//  T.TAKASU
//
//  History:
//  2024-04-28  1.0  new
//
#include "pocket_sdr.h"

// constants and macros --------------------------------------------------------
#define LAG_EPOCH  0.15         // PVT epoch lag (s)
#define FILE_NAV   "pocket.nav" // navigation data file

#define ROUND(x)   (int)floor((x) + 0.5)

// system index ----------------------------------------------------------------
static int sys_idx(int sat)
{
    switch (satsys(sat, NULL)) {
        case SYS_GPS: return 0;
        case SYS_GLO: return 1;
        case SYS_GAL: return 2;
        case SYS_QZS: return 3;
        case SYS_CMP: return 4;
        case SYS_IRN: return 5;
        case SYS_SBS: return 6;
    }
    return -1;
}

// signal ID to signal code -----------------------------------------------------
static uint8_t sig2code(const char *sig)
{
    static const char *sigs[] = {
        "L1CA" , "L1S"  , "L1CB" , "L1CP" , "L1CD" , "L2CM" , "L2CL" ,
        "L5I"  , "L5Q"  , "L5SI" , "L5SQ" , "L5SIV", "L5SQV", "L6D"  ,
        "L6E"  , "G1CA" , "G2CA" , "G1OCD", "G1OCP", "G2OCP", "G3OCD",
        "G3OCP", "E1B"  , "E1C"  , "E5AI" , "E5AQ" , "E5BI" , "E5BQ" ,
        "E6B"  , "E6C"  , "B1I"  , "B1CD" , "B1CP" , "B2I"  , "B2AD" ,
        "B2AP" , "B2BI" , "B3I"  , "I1SD" , "I1SP" , "I5S"  , "ISS"  , NULL
    };
    static const uint8_t codes[] = {
        CODE_L1C, CODE_L1Z, CODE_L1E, CODE_L1L, CODE_L1S, CODE_L2S, CODE_L2L,
        CODE_L5I, CODE_L5Q, CODE_L5D, CODE_L5P, CODE_L5D, CODE_L5P, CODE_L6S,
        CODE_L6E, CODE_L1C, CODE_L2C, CODE_L4A, CODE_L4B, CODE_L6B, CODE_L3I,
        CODE_L3Q, CODE_L1B, CODE_L1C, CODE_L5I, CODE_L5Q, CODE_L7I, CODE_L7Q,
        CODE_L6B, CODE_L6C, CODE_L2I, CODE_L1D, CODE_L1P, CODE_L7I, CODE_L5D,
        CODE_L5P, CODE_L7D, CODE_L6I, CODE_L1D, CODE_L1P, CODE_L5A, CODE_L9A
    };
    for (int i = 0; sigs[i]; i++) {
        if (!strcmp(sig, sigs[i])) return codes[i];
    }
    return 0;
}

// output log $POS -------------------------------------------------------------
static void out_log_POS(double time, const sol_t *sol)
{
    double ep[6], pos[3];
    time2epoch(sol->time, ep);
    ecef2pos(sol->rr, pos);
    sdr_log(3, "$POS,%.3f,%.0f,%.0f,%.0f,%.0f,%.0f,%.9f,%.9f,%.9f,%.3f,%d",
        time, ep[0], ep[1], ep[2], ep[3], ep[4], ep[5], pos[0] * R2D,
        pos[1] * R2D, pos[2], sol->ns);
}

// output NMEA RMC, GGA, GSA and GSV -------------------------------------------
static void out_nmea(const sol_t *sol, const ssat_t *ssat, stream_t *str)
{
    uint8_t buff[4096];
    int n = 0;
    if (!str) return;
    n += outnmea_rmc(buff + n, sol);
    n += outnmea_gga(buff + n, sol);
    n += outnmea_gsa(buff + n, sol, ssat);
    n += outnmea_gsv(buff + n, sol, ssat);
    strwrite(str, buff, n);
}

// count number of signals -----------------------------------------------------
static int num_sigs(int idx, const obs_t *obs)
{
    int nsig = 0, mask[MAXCODE] = {0};
    
    for (int i = 0; i < obs->n; i++) {
        obsd_t *data = obs->data + i;
        if (sys_idx(data->sat) != idx) continue;
        for (int j = 0; j < NFREQ + NEXOBS; j++) {
            if (!data->code[j] || mask[data->code[j]-1]) continue;
            mask[data->code[j]-1] = 1;
            nsig++;
        }
    }
    return nsig;
}

// output RTCM3 observation data -----------------------------------------------
static void out_rtcm3_obs(rtcm_t *rtcm, const obs_t *obs, stream_t *str)
{
    // RTCM3 MSM message types
    static const int msgs[] = {1077, 1087, 1097, 1117, 1127, 1137, 1107, 0};
    int nsig[7] = {0}, idx_tail = 0;
    
    if (!str || obs->n <= 0) return;
    
    rtcm->time = obs->data[0].time;
    for (int i = 0; msgs[i]; i++) {
        if ((nsig[i] = num_sigs(i, obs))) idx_tail = i;
    }
    for (int i = 0; i < msgs[i]; i++) {
        rtcm->obs.n = 0;
        for (int j = 0; j < obs->n; j++) {
            obsd_t *data = obs->data + j;
            if (sys_idx(data->sat) != i) continue;
            
            // separate messages if nsat} x nsig > 64
            if ((rtcm->obs.n + 1) * nsig[i] > 64) {
                if (gen_rtcm3(rtcm, msgs[i], 0, 1)) {
                    strwrite(str, rtcm->buff, rtcm->nbyte);
                }
                rtcm->obs.n = 0;
            }
            rtcm->obs.data[rtcm->obs.n++] = *data;
        }
        if (rtcm->obs.n > 0 && gen_rtcm3(rtcm, msgs[i], 0, i < idx_tail)) {
            strwrite(str, rtcm->buff, rtcm->nbyte);
        }
    }
}

// output RTCM3 navigation data ------------------------------------------------
static void out_rtcm3_nav(rtcm_t *rtcm, int sat, int type, const nav_t *nav,
    stream_t *str)
{
    // RTCM3 navigation message types
    static const int msgs[] = {1019, 1020, 1046, 1044, 1042, 1041, 0, 0};
    int prn, sys = satsys(sat, &prn), idx = sys_idx(sat);
    
    if (!str || idx < 0 || !msgs[idx]) return;
    if (sys == SYS_GLO) {
        rtcm->nav.geph[prn-1] = nav->geph[prn-1];
    }
    else {
        rtcm->nav.eph[MAXSAT*type+sat-1] = nav->eph[MAXSAT*type+sat-1];
    }
    rtcm->ephsat = sat;
    if (gen_rtcm3(rtcm, msgs[idx] - type, 0, 0)) {
        strwrite(str, rtcm->buff, rtcm->nbyte);
    }
}

//------------------------------------------------------------------------------
//  Generate a new SDR PVT.
//
//  args:
//      strs     (I) NMEA and RTCM3 streams
//
//  returns:
//      SDR PVT (NULL: error)
//
sdr_pvt_t *sdr_pvt_new(stream_t **strs)
{
    sdr_pvt_t *pvt = (sdr_pvt_t *)sdr_malloc(sizeof(sdr_pvt_t));
    pvt->obs = (obs_t *)sdr_malloc(sizeof(obs_t));
    pvt->obs->data = (obsd_t *)sdr_malloc(sizeof(obsd_t) * MAXSAT);
    pvt->obs->nmax = MAXSAT;
    pvt->nav = (nav_t *)sdr_malloc(sizeof(nav_t));
    pvt->nav->eph = (eph_t *)sdr_malloc(sizeof(eph_t) * MAXSAT * 4);
    pvt->nav->n = pvt->nav->nmax = MAXSAT * 4;
    pvt->nav->geph = (geph_t *)sdr_malloc(sizeof(geph_t) * MAXPRNGLO);
    pvt->nav->ng = pvt->nav->ngmax = MAXPRNGLO;
    pvt->sol = (sol_t *)sdr_malloc(sizeof(sol_t));
    pvt->ssat = (ssat_t *)sdr_malloc(sizeof(ssat_t) * MAXSAT);
    pvt->rtcm = (rtcm_t *)sdr_malloc(sizeof(rtcm_t));
    init_rtcm(pvt->rtcm);
    pvt->strs[0] = strs[0];
    pvt->strs[1] = strs[1];
    pthread_mutex_init(&pvt->mtx, NULL);
    readnav(FILE_NAV, pvt->nav); // load navigation data
    return pvt;
}

//------------------------------------------------------------------------------
//  Free a SDR PVT.
//
//  args:
//      pvt      (I) SDR PVT generated by sdr_pvt_new()
//
//  returns:
//      none
//
void sdr_pvt_free(sdr_pvt_t *pvt)
{
    if (!pvt) return;
    savenav(FILE_NAV, pvt->nav); // save navigation data
    sdr_free(pvt->obs->data);
    sdr_free(pvt->obs);
    sdr_free(pvt->nav->eph);
    sdr_free(pvt->nav->geph);
    sdr_free(pvt->nav);
    sdr_free(pvt->sol);
    free_rtcm(pvt->rtcm);
    sdr_free(pvt->rtcm);
    sdr_free(pvt);
}

// initialize epoch time and cycle ---------------------------------------------
static void init_epoch(sdr_pvt_t *pvt, int64_t ix, sdr_ch_t *ch)
{
    if (!ch->week) return;
    double tow = floor(ch->tow * 1e-3 / SDR_EPOCH) * SDR_EPOCH + SDR_EPOCH;
    pvt->time = gpst2time(ch->week, tow);
    pvt->ix = ix + ROUND((tow - ch->tow * 1e-3 - 0.07) / SDR_CYC);
    pvt->ix = (pvt->ix / 20) * 20; // round by 20 ms
}

// get observation data index --------------------------------------------------
static int data_idx(int sat, obsd_t *data, uint8_t code)
{
    int i = code2idx(satsys(sat, NULL), code);
    if (!data->code[i]) return i;
    for (i = NFREQ; i < NFREQ + NEXOBS; i++) {
        if (!data->code[i]) return i;
    }
    return -1;
}

// generate pseudorange --------------------------------------------------------
static double gen_prng(gtime_t time, const sdr_ch_t *ch)
{
    int week;
    double tau = 0.0, tow = time2gpst(time, &week);
    
    if (ch->week > 0) {
        tau = (week - ch->week) * 86400.0 * 7 + tow - ch->tow * 1e-3 + ch->coff;
    }
    else { // resolve 100 ms ambiguity w/o week number (0.06 <= tau < 0.16)
        tau = tow - ch->tow * 1e-3 + ch->coff + ch->nav->coff;
        tau -= floor(tau / 0.1) * 0.1;
        if (tau < 0.06) tau += 0.1;
    }
#if 1 // for debug
    trace(2, "%s %5s %3d %4d %10.3f %12.9f %12.9f\n", ch->sat, ch->sig, ch->prn,
        ch->week, ch->tow * 1e-3, ch->coff, tau);
#endif
    return CLIGHT * tau;
}

// update observation data -----------------------------------------------------
static void update_obs(gtime_t time, obs_t *obs, sdr_ch_t *ch)
{
    uint8_t code = sig2code(ch->sig);
    int i, j, sat = satid2no(ch->sat);
    
    for (i = 0; i < obs->n; i++) {
        if (sat == obs->data[i].sat) break;
    }
    if (i >= obs->n) {
        obs->data[i].time = time;
        obs->data[i].sat = sat;
        obs->data[i].rcv = 1;
        memset(obs->data[i].code, 0, NFREQ + NEXOBS);
        obs->n++;
    }
    double P = gen_prng(time, ch);
    if (P > 0.0 && (j = data_idx(sat, obs->data + i, code)) >= 0) {
        obs->data[i].P[j] = P;
        obs->data[i].L[j] = -ch->adr;
        obs->data[i].D[j] = ch->fd;
        obs->data[i].SNR[j] = (uint16_t)(ch->cn0 / SNR_UNIT + 0.5);
        obs->data[i].LLI[j] = ch->lock * ch->T <= 2.0 ? 1 : 0;
        obs->data[i].code[j] = code;
    }
}

// update navigation data ------------------------------------------------------
static void update_nav(sdr_pvt_t *pvt, sdr_ch_t *ch)
{
    uint8_t *data = ch->nav->data;
    int prn, sat = satid2no(ch->sat), sys = satsys(sat, &prn);
    
    if (sys == SYS_NONE || sys == SYS_SBS) return;
    
    if (!strcmp(ch->sig, "L1CA") || !strcmp(ch->sig, "L1CB")) { // GPS/QZS LNAV
        if (ch->nav->type == 3 &&
            decode_frame(data, pvt->nav->eph + sat - 1, NULL, NULL, NULL)) {
            pvt->nav->eph[sat-1].sat = sat;
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->strs[1]);
        }
        if (ch->nav->type == 4) {
            decode_frame(data, NULL, NULL, pvt->nav->ion_gps, NULL);
        }
    }
    else if (!strcmp(ch->sig, "G1CA") || !strcmp(ch->sig, "G2CA")) { // GLO NAV
        pvt->nav->geph[prn-1].tof = pvt->time;
        if (ch->nav->type == 3 &&
            decode_glostr(data, pvt->nav->geph + prn - 1, NULL)) {
            pvt->nav->geph[prn-1].sat = sat;
            pvt->nav->geph[prn-1].frq = ch->prn; // FCN
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->strs[1]);
        }
    }
    else if (!strcmp(ch->sig, "E1B") || !strcmp(ch->sig, "E5BI")) { // GAL I/NAV
        if (ch->nav->type == 4 &&
            decode_gal_inav(data, pvt->nav->eph + sat - 1, NULL, NULL)) {
            pvt->nav->eph[sat-1].sat = sat;
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->strs[1]);
        }
    }
    else if (!strcmp(ch->sig, "E5AI")) { // GAL F/NAV
        if (ch->nav->type == 4 &&
            decode_gal_fnav(data, pvt->nav->eph + MAXSAT + sat - 1, NULL,
                NULL)) {
            pvt->nav->eph[MAXSAT+sat-1].sat = sat;
            out_rtcm3_nav(pvt->rtcm, sat, 1, pvt->nav, pvt->strs[1]);
        }
    }
    else if (!strcmp(ch->sig, "B1I") || !strcmp(ch->sig, "B2I") ||
             !strcmp(ch->sig, "B3I")) {
        if (ch->prn >= 6 && ch->prn <= 58) { // BDS D1 NAV
            if (ch->nav->type == 5 &&
                decode_bds_d1(data, pvt->nav->eph + sat - 1, NULL, NULL)) {
                pvt->nav->eph[sat-1].sat = sat;
                out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->strs[1]);
            }
        }
        else { // BDS D2 NAV
            if (ch->nav->type == 10 &&
                decode_bds_d2(data, pvt->nav->eph + sat - 1, NULL)) {
                pvt->nav->eph[sat-1].sat = sat;
                out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->strs[1]);
            }
        }
    }
    else if (!strcmp(ch->sig, "I5S") || !strcmp(ch->sig, "ISS")) { // NavIC NAV
        if (ch->nav->type == 2 &&
            decode_irn_nav(data, pvt->nav->eph + sat - 1, NULL, NULL)) {
            pvt->nav->eph[sat-1].sat = sat;
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->strs[1]);
        }
    }
}

// update PVT solution ---------------------------------------------------------
static void update_sol(sdr_pvt_t *pvt)
{
    prcopt_t opt = prcopt_default;
    opt.navsys |= SYS_GLO | SYS_GAL | SYS_QZS | SYS_CMP | SYS_IRN;
    opt.err[1] = opt.err[2] = 0.03;
    opt.ionoopt = IONOOPT_BRDC;
    opt.tropopt = TROPOPT_SAAS;
    char msg[128] = "";
    
    // point positioning with L1 pseudorange
    if (!pntpos(pvt->obs->data, pvt->obs->n, pvt->nav, &opt, pvt->sol, NULL,
             pvt->ssat, msg)) {
        sdr_log(3, "$LOG,%.3f,PNTPOS ERROR,%s", pvt->ix * SDR_CYC, msg);
    }
    else {
        // output log $POS and NMEA RMC, GGA, GSA and GSV
        out_log_POS(pvt->ix * SDR_CYC, pvt->sol);
        out_nmea(pvt->sol, pvt->ssat, pvt->strs[0]);
    }
#if 1 // for debug
    double pos[3];
    ecef2pos(pvt->sol->rr, pos);
    trace(2, "TIME=%s POS=%12.8f %13.8f %8.1f NS=%2d Q=%d DTR=%6.1f %6.1f %6.1f %6.1f MSG=%s\n",
        time_str(pvt->sol->time, 0), pos[0] * R2D, pos[1] * R2D, pos[2], pvt->sol->ns,
        pvt->sol->stat, pvt->sol->dtr[0] * 1e9, pvt->sol->dtr[1] * 1e9,
        pvt->sol->dtr[2] * 1e9, pvt->sol->dtr[3] * 1e9, msg);
    
    for (int i = 0; i < MAXSAT; i++) {
        if (pvt->ssat[i].resp[0] == 0.0) continue;
        char sat[16];
        satno2id(i+1, sat);
        trace(2, "%2s VS=%d AZ=%5.1f EL=%4.1f RES=%12.3f\n", sat, pvt->ssat[i].vs,
            pvt->ssat[i].azel[0] * R2D, pvt->ssat[i].azel[1] * R2D,
            pvt->ssat[i].resp[0]);
    }
#endif
}

// resolve msec ambiguity in pseudorange ---------------------------------------
static void res_obs_amb(obs_t *obs, int sys, uint8_t code, double sec)
{
    for (int i = 0; i < obs->n; i++) {
        obsd_t *data = obs->data + i;
        if (!(satsys(data->sat, NULL) & sys)) continue;
        
        for (int j = 0; j < NFREQ + NEXOBS; j++) {
            if (data->code[j] != code) continue;
            int k;
            for (k = 0; k < NFREQ + NEXOBS; k++) {
                if (!data->code[k] || data->code[k] == code ||
                    data->code[k] == CODE_L5Q || data->code[k] == CODE_L5P) {
                    continue;
                }
                double tau1 = data->P[j] / CLIGHT, tau2 = data->P[k] / CLIGHT;
                double tau3 = floor(tau2 / sec) * sec + fmod(tau1, sec);
                if      (tau3 < tau2 - sec / 2.0) tau3 += sec;
                else if (tau3 > tau2 + sec / 2.0) tau3 -= sec;
                data->P[j] = CLIGHT * tau3;
                break;
            }
            if (k >= NFREQ + NEXOBS) {
                data->P[j] = 0.0; // set invalid if unresolved
            }
        }
    }
}

//------------------------------------------------------------------------------
//  Update observation and navigation data.
//
//  args:
//      pvt      (IO) SDR PVT
//      ix       (I)  received IF data cycle (cyc)
//      ch       (IO) SDR receiver channel
//
//  returns:
//      none
//
void sdr_pvt_udobs(sdr_pvt_t *pvt, int64_t ix, sdr_ch_t *ch)
{
    pthread_mutex_lock(&pvt->mtx);
    
    if (ch->nav->stat) { // update navigation data
        update_nav(pvt, ch);
        ch->nav->stat = 0;
    }
    if (pvt->ix <= 0) { // initialize epoch time and cycle
        init_epoch(pvt, ix, ch);
    }
    else if (ix == pvt->ix) { // update observation data
        if (ch->state == SDR_STATE_LOCK && ch->tow >= 0 &&
            (ch->nav->fsync > 0 || ch->trk->sec_sync > 0)) {
            update_obs(pvt->time, pvt->obs, ch);
        }
    }
    pthread_mutex_unlock(&pvt->mtx);
}

//------------------------------------------------------------------------------
//  Update PVT solution.
//
//  args:
//      pvt      (IO) SDR PVT
//      ix       (I)  received IF data cycle (cyc)
//
//  returns:
//      none
//
void sdr_pvt_udsol(sdr_pvt_t *pvt, int64_t ix)
{
    pthread_mutex_lock(&pvt->mtx);
    
    if (pvt->ix > 0 && ix >= pvt->ix + (int)(LAG_EPOCH / SDR_CYC)) {
        
        // resolve msec ambiguity in pseudorange
        res_obs_amb(pvt->obs, SYS_GPS | SYS_QZS, CODE_L5Q, 20e-3); // L5Q
        res_obs_amb(pvt->obs, SYS_QZS, CODE_L5P, 20e-3); // L5SQ, L5SQV
        res_obs_amb(pvt->obs, SYS_GLO, CODE_L3Q, 10e-3); // G3OCP
        res_obs_amb(pvt->obs, SYS_SBS, CODE_L5Q, 2e-3);  // L5Q SBAS
        
        // update PVT solution
        update_sol(pvt);
        
        // output RTCM3 observation data
        out_rtcm3_obs(pvt->rtcm, pvt->obs, pvt->strs[1]);
        pvt->obs->n = 0; 
        
        // advance epoch time and cycle
        pvt->time = timeadd(pvt->time, SDR_EPOCH);
        pvt->ix += (int)(SDR_EPOCH / SDR_CYC);
        
        // adjust epoch cycle within 20 ms
        if (pvt->sol->stat) {
            double dtr = ROUND(pvt->sol->dtr[0] / 0.02) * 0.02;
            pvt->ix += (int)(dtr / SDR_CYC);
        }
    }
    pthread_mutex_unlock(&pvt->mtx);
}

//------------------------------------------------------------------------------
//  Get PVT solution string.
//
//  args:
//      pvt      (I)  SDR PVT
//      buff     (IO) PVT solution string buffer
//
//  returns:
//      none
//
void sdr_pvt_solstr(sdr_pvt_t *pvt, char *buff)
{
    char tstr[32];
    double pos[3] = {0};
    int stat = 0;
    
    pthread_mutex_lock(&pvt->mtx);
    
    if (norm(pvt->sol->rr, 3) > 1e-6) {
        time2str(pvt->sol->time, tstr, 3);
        ecef2pos(pvt->sol->rr, pos);
        stat = pvt->sol->stat;
    }
    else {
        time2str(pvt->time, tstr, 3);
    }
    pthread_mutex_unlock(&pvt->mtx);
    
    tstr[4] = tstr[7] = '-';
    sprintf(buff, "%23s %11.7f %12.7f %8.2f %3d  %s", tstr, pos[0] * R2D,
        pos[1] * R2D, pos[2], pvt->sol->ns, stat ? "FIX" : "---");
}
