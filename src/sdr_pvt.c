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
#define LAG_EPOCH  0.25         // max PVT epoch lag (s)
#define FILE_NAV   ".pocket_navdata.csv" // navigation data file

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

//------------------------------------------------------------------------------
//  Output log $OBS.
//
//  format:
//      $OBS,time,week,tow,sat,sig,cn0,pr,cp,dop,lli
//          time  receiver time (s)
//          week,tow observation data epoch GPS week and TOW (s)
//          sat   satellite ID
//          sig   signal ID
//          cn0   C/N0 (dB-Hz)
//          pr    pseudorange (m)
//          cp    carrier phase (cyc)
//          dop   Doppler frequency (Hz)
//          lli   loss of lock indicator
//
static void out_log_obs(double time, const obs_t *obs)
{
    for (int i = 0; i < obs->n; i++) {
        const obsd_t *data = obs->data + i;
        char sat[16];
        int week;
        double tow = time2gpst(data->time, &week);
        satno2id(data->sat, sat);
        for (int j = 0; j < NFREQ + NEXOBS; j++) {
            if (!data->code[j]) continue;
            sdr_log(3, "$OBS,%.3f,%d,%.3f,%s,%s,%.1f,%.3f,%.3f,%.3f,%d", time,
                week, tow, sat, code2obs(data->code[j]),
                data->SNR[j] * SNR_UNIT, data->P[j], data->L[j], data->D[j],
                data->LLI[j]);
        }
    }
}

//------------------------------------------------------------------------------
//  Output log $POS.
//
//  format:
//      $POS,time,year,month,day,hour,min,sec,lat,lon,hgt,ns,nsat
//          time  receiver time (s)
//          year,month,day  solution day (GPST)
//          hour,min,sec  solution time (GPST)
//          lat   solution latitude (deg, +:north, -:south)
//          lon   solution longitude (deg, +:east, -:west)
//          hgt   solution ellipsoidal height (m)
//          ns    number of valid satellites
//          nsat  number of satellites
//
static void out_log_pos(double time, const sol_t *sol, int nsat)
{
    double ep[6], pos[3];
    time2epoch(sol->time, ep);
    ecef2pos(sol->rr, pos);
    sdr_log(3, "$POS,%.3f,%.0f,%.0f,%.0f,%.0f,%.0f,%.9f,%.9f,%.9f,%.3f,%d,%d",
        time, ep[0], ep[1], ep[2], ep[3], ep[4], ep[5], pos[0] * R2D,
        pos[1] * R2D, pos[2], sol->ns, nsat);
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
            
            // separate messages if nsat x nsig > 64
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
    int msg = (sys == SYS_GAL && type == 1) ? 1045 : msgs[idx];
    if (gen_rtcm3(rtcm, msg, 0, 0)) {
        strwrite(str, rtcm->buff, rtcm->nbyte);
    }
}

//------------------------------------------------------------------------------
//  Generate a new SDR PVT.
//
//  args:
//      rcv      (I)  SDR receiver
//
//  returns:
//      SDR PVT (NULL: error)
//
sdr_pvt_t *sdr_pvt_new(sdr_rcv_t *rcv)
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
    pvt->rcv = rcv;
    pthread_mutex_init(&pvt->mtx, NULL);
    readnav(FILE_NAV, pvt->nav); // load navigation data
    return pvt;
}

//------------------------------------------------------------------------------
//  Free a SDR PVT.
//
//  args:
//      pvt      (I)  SDR PVT generated by sdr_pvt_new()
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
    sdr_free(pvt->ssat);
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
    else if (ch->tow_v == 2) { // resolve 100 ms ambiguity (0.05 <= tau < 0.15)
        tau = tow - ch->tow * 1e-3 + ch->coff + ch->nav->coff;
        tau -= floor(tau / 0.1) * 0.1;
        if (tau < 0.05) tau += 0.1;
    }
#if 1 // for debug
    trace(2, "%s %-5s %3d %4d %10.3f %10.3f %12.9f %12.9f\n", ch->sat, ch->sig,
        ch->prn, ch->week, tow, ch->tow * 1e-3, ch->coff, tau);
#endif
    return CLIGHT * tau;
}

// update observation data -----------------------------------------------------
static void update_obs(gtime_t time, obs_t *obs, sdr_ch_t *ch)
{
    uint8_t code = sig2code(ch->sig);
    int i, j, sat;
    
    if (strstr(ch->sat, "R-") || strstr(ch->sat, "R+")) return;
    if (!(sat = satid2no(ch->sat))) return;
    
    for (i = 0; i < obs->n; i++) {
        if (sat == obs->data[i].sat) break;
    }
    if (i >= obs->n) {
        memset(obs->data + i, 0, sizeof(obsd_t));
        obs->data[i].time = time;
        obs->data[i].sat = sat;
        obs->data[i].rcv = 1;
        obs->n++;
    }
    double P = gen_prng(time, ch);
    if (P > 0.0 && (j = data_idx(sat, obs->data + i, code)) >= 0) {
        obs->data[i].code[j] = code;
        obs->data[i].P[j] = P;
        obs->data[i].L[j] = -ch->adr + (ch->nav->rev ? 0.5 : 0.0);
        obs->data[i].D[j] = ch->fd;
        obs->data[i].SNR[j] = (uint16_t)(ch->cn0 / SNR_UNIT + 0.5);
        if (ch->lock * ch->T <= 2.0 || fabs(ch->trk->err_phas) > 0.2) {
            obs->data[i].LLI[j] |= 1; // PLL unlock
        }
        if (ch->nav->fsync <= 0 && ch->trk->sec_sync <= 0) {
            obs->data[i].LLI[j] |= 2; // half-cyc-amb unresolved
        }
    }
}

//------------------------------------------------------------------------------
//  Update observation data.
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
    
    if (pvt->ix <= 0) { // initialize epoch time and cycle
        init_epoch(pvt, ix, ch);
    }
    if (ix == pvt->ix) { // update observation data
        if (ch->state == SDR_STATE_LOCK && ch->tow >= 0 && ch->tow_v > 0 &&
            (ch->nav->fsync > 0 || ch->trk->sec_sync > 0)) {
            update_obs(pvt->time, pvt->obs, ch);
        }
        pvt->nch++;
    }
    pthread_mutex_unlock(&pvt->mtx);
}

//------------------------------------------------------------------------------
//  Update navigation data.
//
//  args:
//      pvt      (IO) SDR PVT
//      ch       (IO) SDR receiver channel
//
//  returns:
//      none
//
void sdr_pvt_udnav(sdr_pvt_t *pvt, sdr_ch_t *ch)
{
    uint8_t *data = ch->nav->data;
    int prn, sat = satid2no(ch->sat), sys = satsys(sat, &prn);
    
    if (sys == SYS_NONE || sys == SYS_SBS) return;
    
    pthread_mutex_lock(&pvt->mtx);
    
    if (!strcmp(ch->sig, "L1CA") || !strcmp(ch->sig, "L1CB")) { // GPS/QZS LNAV
        if (ch->nav->type == 3 &&
            decode_frame(data, pvt->nav->eph + sat - 1, NULL, NULL, NULL)) {
            pvt->nav->eph[sat-1].sat = sat;
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
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
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
        }
    }
    else if (!strcmp(ch->sig, "E1B") || !strcmp(ch->sig, "E5BI")) { // GAL I/NAV
        if (ch->nav->type == 4 &&
            decode_gal_inav(data, pvt->nav->eph + sat - 1, NULL, NULL)) {
            pvt->nav->eph[sat-1].sat = sat;
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
        }
    }
    else if (!strcmp(ch->sig, "E5AI")) { // GAL F/NAV
        if (ch->nav->type == 4 &&
            decode_gal_fnav(data, pvt->nav->eph + MAXSAT + sat - 1, NULL,
                NULL)) {
            pvt->nav->eph[MAXSAT+sat-1].sat = sat;
            out_rtcm3_nav(pvt->rtcm, sat, 1, pvt->nav, pvt->rcv->strs[1]);
        }
    }
    else if (!strcmp(ch->sig, "B1I") || !strcmp(ch->sig, "B2I") ||
             !strcmp(ch->sig, "B3I")) {
        if (ch->prn >= 6 && ch->prn <= 58) { // BDS D1 NAV
            if (ch->nav->type == 5 &&
                decode_bds_d1(data, pvt->nav->eph + sat - 1, NULL, NULL)) {
                pvt->nav->eph[sat-1].sat = sat;
                out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
            }
        }
        else { // BDS D2 NAV
            if (ch->nav->type == 10 &&
                decode_bds_d2(data, pvt->nav->eph + sat - 1, NULL)) {
                pvt->nav->eph[sat-1].sat = sat;
                out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
            }
        }
    }
    else if (!strcmp(ch->sig, "I5S") || !strcmp(ch->sig, "ISS")) { // NavIC NAV
        if (ch->nav->type == 2 &&
            decode_irn_nav(data, pvt->nav->eph + sat - 1, NULL, NULL)) {
            pvt->nav->eph[sat-1].sat = sat;
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
        }
    }
    pthread_mutex_unlock(&pvt->mtx);
}

// correct solution time -------------------------------------------------------
static void corr_sol_time(sol_t *sol)
{
    if (fabs(sol->dtr[0]) >= 1e-9) return;
    
    // use GLOT, GALT, BDT or IRT as solution time in case of GPS absence
    for (int i = 1; i < 5; i++) {
        if (fabs(sol->dtr[i]) < 1e-9) continue;
        sol->dtr[0] = sol->dtr[i];
        sol->time = timeadd(sol->time, -sol->dtr[0]);
        return;
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
#if 0 // RAIM-FDE on
    opt.posopt[4] = 1;
#endif
    double time = pvt->ix * SDR_CYC;
    char msg[128] = "";
    
    // point positioning with L1 pseudorange
    if (pntpos(pvt->obs->data, pvt->obs->n, pvt->nav, &opt, pvt->sol, NULL,
             pvt->ssat, msg)) {
        
        // correct solution time
        corr_sol_time(pvt->sol);
        
        // output log $POS and NMEA RMC, GGA, GSA and GSV
        out_log_pos(time, pvt->sol, pvt->obs->n);
        out_nmea(pvt->sol, pvt->ssat, pvt->rcv->strs[0]);
    }
    else {
        pvt->sol->ns = 0;
        sdr_log(3, "$LOG,%.3f,PNTPOS ERROR,%s", time, msg);
    }
    pvt->nsat = pvt->obs->n;
    
#if 1 // for debug
    double pos[3];
    ecef2pos(pvt->sol->rr, pos);
    trace(2, "%s %12.8f %13.8f %8.2f %d %2d/%2d DTR=%.1f %.1f %.1f (%s)\n",
        time_str(pvt->sol->time, 9), pos[0] * R2D, pos[1] * R2D, pos[2],
        pvt->sol->stat, pvt->sol->ns, pvt->nsat, pvt->sol->dtr[1] * 1e9,
        pvt->sol->dtr[2] * 1e9, pvt->sol->dtr[3] * 1e9, msg);
    for (int i = 0; i < MAXSAT; i++) {
        ssat_t *ssat = pvt->ssat + i;
        if (ssat->azel[1] <= 0.0) continue;
        char sat[16];
        satno2id(i+1, sat);
        trace(2, "%s %d %4.1f %5.1f %4.1f %12.3f\n", sat, ssat->vs,
            ssat->snr[0] * SNR_UNIT, ssat->azel[0] * R2D, ssat->azel[1] * R2D,
            ssat->resp[0]);
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
    
    if (pvt->ix > 0 && (pvt->nch >= pvt->rcv->nch ||
        ix >= pvt->ix + (int)(LAG_EPOCH / SDR_CYC))) {
        
        // resolve msec ambiguity in pseudorange
        res_obs_amb(pvt->obs, SYS_GPS | SYS_QZS, CODE_L5Q, 20e-3); // L5Q
        res_obs_amb(pvt->obs, SYS_QZS, CODE_L5P, 20e-3); // L5SQ, L5SQV
        res_obs_amb(pvt->obs, SYS_GLO, CODE_L3Q, 10e-3); // G3OCP
        res_obs_amb(pvt->obs, SYS_SBS, CODE_L5Q, 2e-3);  // L5Q SBAS
        
        // output log $OBS and RTCM3 observation data
        out_log_obs(pvt->ix * SDR_CYC, pvt->obs);
        out_rtcm3_obs(pvt->rtcm, pvt->obs, pvt->rcv->strs[1]);
        
        // update PVT solution
        update_sol(pvt);
        
        // set next epoch time and cycle
        pvt->time = timeadd(pvt->time, SDR_EPOCH);
        pvt->ix += (int)(SDR_EPOCH / SDR_CYC);
        pvt->nch = pvt->obs->n = 0; 
        
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
    sprintf(buff, "%23s %11.7f %12.7f %8.2f %2d/%2d %s", tstr, pos[0] * R2D,
        pos[1] * R2D, pos[2], pvt->sol->ns, pvt->nsat, stat ? "FIX" : "---");
}
