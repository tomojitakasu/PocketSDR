//
//  Pocket SDR C Library - GNSS SDR PVT Functions
//
//  Author:
//  T.TAKASU
//
//  References:
//  [1] RINEX: The Receiver Independent Exchange Format version 3.05,
//      December 1, 2020
//
//  History:
//  2024-04-28  1.0  new
//  2024-12-30  1.1  add and update log contents
//                   add nav data consistency tests
//
#include "pocket_sdr.h"

// constants and macros --------------------------------------------------------
#define SDR_EPOCH      1.0      // epoch time interval (s)
#define LAG_EPOCH      0.05     // max PVT epoch lag (s)
#define EL_MASK        15.0     // elavation mask (deg)
#define STD_ERR        0.015    // std-dev of carrier phase noise (m)
#define FILE_NAV       ".pocket_navdata.csv" // navigation data file

#define ROUND(x)   (int)floor((x) + 0.5)
#define SQRT(x)    ((x) > 0.0 ? sqrt(x) : 0.0)
#define EQ(x, y)   (fabs((double)((x) - (y))) < 1e-12)

// global variable -------------------------------------------------------------
double sdr_epoch     = SDR_EPOCH;
double sdr_lag_epoch = LAG_EPOCH;
double sdr_el_mask   = EL_MASK;
static const int systems[] = {
    SYS_GPS, SYS_GLO, SYS_GAL, SYS_QZS, SYS_CMP, SYS_IRN, SYS_SBS, 0
};

// satellite ID to system ------------------------------------------------------
static int sat2sys(const char *sat)
{
    static const char *str = "GREJCIS";
    const char *p = strchr(str, sat[0]);
    return p ? systems[(int)(p - str)] : 0;
}

// system to system index ------------------------------------------------------
static int sys2idx(int sys)
{
    for (int i = 0; systems[i]; i++) {
        if (systems[i] == sys) return i;
    }
    return -1;
}

// satellite to system index ---------------------------------------------------
static int sys_idx(int sat)
{
    return sys2idx(satsys(sat, NULL));
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
//  Output log $CH (receiver channel information).
//
//  format:
//      $CH,time,ch,rfch,sat,sig,prn,lock,cn0,coff,dop,adr,ssync,bsync,fsync,
//          rev,week,tow,towv,nnav,nerr,nlol,nfec
//          time  receiver time (s)
//          ch    receiver channel number
//          rfch  RF channel number
//          sat   satellite ID
//          sig   signal ID
//          prn   PRN number
//          lock  lock time (s)
//          cn0   C/N0 (dB-Hz)
//          coff  code offset (ms)
//          dop   Doppler frequency (Hz)
//          adr   accumlated Doppler range (cyc)
//          ssync secondary code sync flag (0:async, 1:sync)
//          bsync symbol/bit sync flag (0:async, 1:sync)
//          fsync frame sync flag (0:async, 1:sync)
//          rev   code polarity (0:normal, 1:reversed)
//          towv  tow valid flag (0:invalid, 1:valid)
//          tow   TOW (s)
//          week  week number (week)
//          nnav  navigation subframe/message count
//          nerr  error subframe/message count
//          nlol  loss-of-lock count
//          nfec  number of error corrected (bits)
//
static void out_log_ch(sdr_ch_t *ch)
{
    sdr_log(3, "$CH,%.3f,%d,%d,%s,%s,%d,%.3f,%.1f,%.9f,%.3f,%.3f,%d,%d,%d,%d,"
        "%d,%.3f,%d,%d,%d,%d,%d", ch->time, ch->no, ch->rf_ch + 1, ch->sat,
        ch->sig, ch->prn, ch->lock * ch->T, ch->cn0, ch->coff * 1e3, ch->fd,
        ch->adr, ch->trk->sec_sync != 0, ch->nav->ssync != 0,
        ch->nav->fsync != 0, ch->nav->rev, ch->tow_v, ch->tow * 1e-3, ch->week,
        ch->nav->count[0], ch->nav->count[1], ch->lost, ch->nav->nerr);
}

//------------------------------------------------------------------------------
//  Output log $OBS (observation data).
//
//  format:
//      $OBS,time,year,month,day,hour,min,sec,sat,code,cn0,pr,cp,dop,lli,fcn
//          time  receiver time (s)
//          year,month,day  obs data day (GPST)
//          hour,min,sec  obs data time (GPST)
//          sat   satellite ID
//          code  RINEX obs code
//          cn0   C/N0 (dB-Hz)
//          pr    pseudorange (m)
//          cp    carrier phase (cyc)
//          dop   Doppler frequency (Hz)
//          lli   loss of lock indicator
//          fcn   frequency channel number for GLONASS FDMA signals
//
static void out_log_obs(double time, const obs_t *obs, const nav_t *nav)
{
    for (int i = 0; i < obs->n; i++) {
        const obsd_t *data = obs->data + i;
        double ep[6];
        char sat[16];
        time2epoch(data->time, ep);
        satno2id(data->sat, sat);
        if (sat[0] == '1') sat[0] = 'S';
        int fcn = 0, prn;
        if (satsys(data->sat, &prn) == SYS_GLO) {
            fcn = nav->geph[prn-1].frq;
        }
        for (int j = 0; j < NFREQ + NEXOBS; j++) {
            if (!data->code[j]) continue;
            sdr_log(3, "$OBS,%.3f,%.0f,%.0f,%.0f,%.0f,%.0f,%.3f,%s,%s,%.1f,"
                "%.3f,%.3f,%.3f,%d,%d", time, ep[0], ep[1], ep[2], ep[3], ep[4],
                ep[5], sat, code2obs(data->code[j]), data->SNR[j] * SNR_UNIT,
                data->P[j], data->L[j], data->D[j], data->LLI[j], fcn);
        }
    }
}

//------------------------------------------------------------------------------
//  Output log $POS (position solution).
//
//  format:
//      $POS,time,year,month,day,hour,min,sec,lat,lon,hgt,Q,ns,stdn,stde,stdu
//          time  receiver time (s)
//          year,month,day  solution day (GPST)
//          hour,min,sec  solution time (GPST)
//          lat   solution latitude (deg, +:north, -:south)
//          lon   solution longitude (deg, +:east, -:west)
//          hgt   solution ellipsoidal height (m)
//          Q     quality flag (=5: single)
//          ns    number of valid satellites
//          stdn  solution standard deviation north (m)
//          stde  solution standard deviation east (m)
//          stdu  solution standard deviation up (m)
//
static void out_log_pos(double time, const sol_t *sol, int nsat)
{
    double ep[6], pos[3], P[9], Q[9];
    time2epoch(sol->time, ep);
    ecef2pos(sol->rr, pos);
    P[0] = sol->qr[0];
    P[4] = sol->qr[1];
    P[8] = sol->qr[2];
    P[1] = P[3] = sol->qr[3];
    P[5] = P[7] = sol->qr[4];
    P[2] = P[6] = sol->qr[5];
    covenu(pos, P, Q);
    sdr_log(3, "$POS,%.3f,%.0f,%.0f,%.0f,%.0f,%.0f,%.9f,%.9f,%.9f,%.3f,%d,%d,"
        "%.3f,%.3f,%.3f", time, ep[0], ep[1], ep[2], ep[3], ep[4], ep[5],
        pos[0] * R2D, pos[1] * R2D, pos[2], 5, sol->ns, SQRT(Q[4]), SQRT(Q[0]),
        SQRT(Q[8]));
}

//------------------------------------------------------------------------------
//  Output log $SAT (satellite information).
//
//  format:
//      $SAT,time,sat,pvt,obs,cn0,az,el,res
//          time  receiver time (s)
//          sat   satellite ID
//          pvt   PVT status (0: not used, 1: used)
//          obs   L1 obs data status (0: not available, 1: available)
//          cn0   L1 signal C/N0 (dB-Hz)
//          az    azimuth angle (deg)
//          el    elavation angle (deg)
//          res   L1 pseudorange residual (m)
//
static void out_log_sat(double time, int sat, const ssat_t *ssat)
{
    char str[16];
    satno2id(sat, str);
    if (str[0] == '1') str[0] = 'S';
    sdr_log(3, "$SAT,%.3f,%s,%d,%d,%.1f,%.1f,%.1f,%.3f", time, str, ssat->vs,
        ssat->snr[0] > 0, ssat->snr[0] * SNR_UNIT, ssat->azel[0] * R2D,
        ssat->azel[1] * R2D, ssat->resp[0]);
}

//------------------------------------------------------------------------------
//  Output log $EPH (decoded ephemeris).
//
//  format:
//      $EPH,time,sat,sig,IODE,IODC,SVA,SVH,Toe,Toc,Ttr,A,e,i0,OMEGA0,omega,M0,
//        delta-n,OMEGAdot,Idot,Crc,Crs,Cuc,Cus,Cic,Cis,Toes,Fit,Af0,Af1,Af2,
//        TGD,code,flag (GPS,Galileo,QZSS,BeiDou,NavIC)
//      $EPH,time,sat,sig,tb,fcn,SVH,SVA,age,Toe,Tof,pos-x,pos-y,pos-z,vel-x,
//        vel-y,vel-z,acc-x,acc-y,acc-z,tau-n,gamma-n,delta-tau-n (GLONASS)
//      
//          time  receiver time (s)
//          sat   satellite ID
//          sig   signal ID
//          ...   ephemeris parameters
//
static void out_log_eph(double time, const char *sat, const char *sig,
    const void *p)
{
    char buff[2048];
    
    if (sat[0] != 'R') {
        const eph_t *eph = (const eph_t *)p;
        sprintf(buff, "%d,%d,%d,%d,%d,%d,%d,%.14E,%.14E,%.14E,%.14E,%.14E,"
            "%.14E,%.14E,%.14E,%.14E,%.14E,%.14E,%.14E,%.14E,%.14E,%.14E,%.14E,"
            "%.14E,%.14E,%.14E,%.14E,%.14E,%d,%d", eph->iode, eph->iodc,
            eph->sva, eph->svh, (int)eph->toe.time, (int)eph->toc.time,
            (int)eph->ttr.time, eph->A, eph->e, eph->i0, eph->OMG0, eph->omg,
            eph->M0, eph->deln, eph->OMGd, eph->idot, eph->crc, eph->crs,
            eph->cuc, eph->cus, eph->cic, eph->cis, eph->toes, eph->fit,
            eph->f0, eph->f1, eph->f2, eph->tgd[0], eph->code, eph->flag);
    }
    else {
        const geph_t *geph = (const geph_t *)p;
        sprintf(buff, "%d,%d,%d,%d,%d,%d,%d,%.14E,%.14E,%.14E,%.14E,%.14E,"
            "%.14E,%.14E,%.14E,%.14E,%.14E,%.14E,%.14E", geph->iode,
            geph->frq, geph->svh, geph->sva, geph->age, (int)geph->toe.time,
            (int)geph->tof.time, geph->pos[0], geph->pos[1], geph->pos[2],
            geph->vel[0], geph->vel[1], geph->vel[2], geph->acc[0], geph->acc[1],
            geph->acc[2], geph->taun, geph->gamn,geph->dtaun);
    }
    sdr_log(3, "$EPH,%.3f,%s,%s,%s", time, sat, sig, buff);
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
    sdr_str_write(str, buff, n);
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
                    sdr_str_write(str, rtcm->buff, rtcm->nbyte);
                }
                rtcm->obs.n = 0;
            }
            rtcm->obs.data[rtcm->obs.n++] = *data;
        }
        if (rtcm->obs.n > 0 && gen_rtcm3(rtcm, msgs[i], 0, i < idx_tail)) {
            sdr_str_write(str, rtcm->buff, rtcm->nbyte);
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
        sdr_str_write(str, rtcm->buff, rtcm->nbyte);
    }
}

// set observation data index --------------------------------------------------
static void set_obs_idx(sdr_rcv_t *rcv)
{
    int codes[7][NFREQ+NEXOBS] = {{0}};
    
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_t *ch = rcv->th[i]->ch;
        int sys = sat2sys(ch->sat), code = sig2code(ch->sig);
        int j = sys2idx(sys), k = code2idx(sys, code);
        if (j < 0 || k < 0 || codes[j][k] == code) continue;
        if (codes[j][k] == 0) {
            codes[j][k] = code;
            continue;
        }
        for (k = NFREQ; k < NFREQ + NEXOBS; k++) {
            if (codes[j][k] == code) break;
            if (codes[j][k] == 0) {
                codes[j][k] = code;
                break;
            }
        }
    }
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_t *ch = rcv->th[i]->ch;
        int sys = sat2sys(ch->sat), code = sig2code(ch->sig);
        int j = sys2idx(sys);
        if (j < 0) continue;
        for (int k = 0; k < NFREQ + NEXOBS; k++) {
            if (codes[j][k] != code) continue;
            ch->obs_idx = k;
            break;
        }
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
    pvt->nav->seph = (seph_t *)sdr_malloc(sizeof(seph_t) * NSATSBS * 2);
    pvt->nav->ns = pvt->nav->nsmax = NSATSBS * 2;
    pvt->sol = (sol_t *)sdr_malloc(sizeof(sol_t));
    pvt->ssat = (ssat_t *)sdr_malloc(sizeof(ssat_t) * MAXSAT);
    pvt->rtcm = (rtcm_t *)sdr_malloc(sizeof(rtcm_t));
    init_rtcm(pvt->rtcm);
    set_obs_idx(rcv);
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
    sdr_free(pvt->nav->seph);
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
    double tow = floor(ch->tow * 1e-3 / sdr_epoch) * sdr_epoch + sdr_epoch;
    pvt->time = gpst2time(ch->week, tow);
    pvt->ix = ix + ROUND((tow - ch->tow * 1e-3 - 0.07) / SDR_CYC);
    pvt->ix = (pvt->ix / 20) * 20; // round by 20 ms
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

// generate carrier-phase ------------------------------------------------------
static double gen_cphas(const sdr_ch_t *ch, double P)
{
    double L = -ch->adr;
    
    L += (ch->nav->rev ? 0.5 : 0.0) + (ch->trk->sec_pol == 1 ? 0.5 : 0.0);
    
    // phase alignment ([1] Table A23)
    if (!strcmp(ch->sig, "L1CD") || !strcmp(ch->sig, "L1CP")) {
        L += 0.25; // + 1/4 cyc
    }
    else if (!strcmp(ch->sig, "L5Q") || !strcmp(ch->sig, "G3OCP") ||
        !strcmp(ch->sig, "E5AQ") || !strcmp(ch->sig, "E5BQ") ||
        !strcmp(ch->sig, "L5SQ") || !strcmp(ch->sig, "L5SQV") ||
        !strcmp(ch->sig, "B1CP") || !strcmp(ch->sig, "B2AP")) {
        L -= 0.25; // - 1/4 cyc
    }
    else if (!strcmp(ch->sig, "E1C") || !strcmp(ch->sig, "E6C")) {
        L += 0.5; // + 1/2 cyc
    }
    else if (!strcmp(ch->sig, "L2CM")) {
        L += (ch->sat[0] == 'J') ? 0.0 : -0.25; // 0 cyc (QZSS), -1/4 cyc (GPS)
    }
    return L;
}

// update observation data -----------------------------------------------------
static void update_obs(gtime_t time, obs_t *obs, sdr_ch_t *ch)
{
    uint8_t code = sig2code(ch->sig);
    double P = gen_prng(time, ch);
    int i, j = ch->obs_idx, sat;
    
    if (strstr(ch->sat, "R-") || strstr(ch->sat, "R+")) return;
    if (P <= 0.0 || j < 0 || !(sat = satid2no(ch->sat))) return;
    
    for (i = 0; i < obs->n; i++) {
        if (sat == obs->data[i].sat) break;
    }
    if (i >= obs->n) {
        if (i >= MAXSAT) return;
        memset(obs->data + i, 0, sizeof(obsd_t));
        obs->data[i].time = time;
        obs->data[i].sat = sat;
        obs->data[i].rcv = 1;
        obs->n++;
    }
    obs->data[i].code[j] = code;
    obs->data[i].P[j] = P;
    obs->data[i].L[j] = gen_cphas(ch, P);
    obs->data[i].D[j] = ch->fd;
    obs->data[i].SNR[j] = (uint16_t)(ch->cn0 / SNR_UNIT + 0.5);
    if (ch->lock * ch->T <= 2.0 || fabs(ch->trk->err_phas) > 0.2) {
        obs->data[i].LLI[j] |= 1; // PLL unlock
    }
    if (ch->nav->fsync <= 0 && ch->trk->sec_sync <= 0) {
        obs->data[i].LLI[j] |= 2; // half-cyc-amb unknown
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
        
        // output log $CH
        if (ch->state == SDR_STATE_LOCK && ch->lock > 0) {
            out_log_ch(ch);
        }
    }
    pthread_mutex_unlock(&pvt->mtx);
}

// test nav data consistency for GLONASS ---------------------------------------
static int test_nav_glo(const sdr_ch_t *ch)
{
    int t[3];
    for (int i = 0; i < 3; i++) {
        t[i] = ch->nav->lock_sf[i+1] - ch->nav->lock_sf[i];
    }
    return t[0] == 2000 && t[1] == 2000 && t[2] == 2000;
}

// test match of ephemeris parameters ------------------------------------------
static int match_eph(const eph_t *e1, const eph_t *e2)
{
    return EQ(e1->iode, e2->iode) && EQ(e1->iodc, e2->iodc) &&
        EQ(e1->A, e2->A) && EQ(e1->e, e2->e) && EQ(e1->i0, e2->i0) &&
        EQ(e1->OMG0, e2->OMG0) && EQ(e1->omg, e2->omg) && EQ(e1->M0, e2->M0) &&
        EQ(e1->deln, e2->deln) && EQ(e1->OMGd, e2->OMGd) &&
        EQ(e1->idot, e2->idot) && EQ(e1->crc, e2->crc) &&
        EQ(e1->crs, e2->crs) && EQ(e1->cuc, e2->cuc) && EQ(e1->cus, e2->cus) &&
        EQ(e1->cic, e2->cic) && EQ(e1->cis, e2->cis) && EQ(e1->f0, e2->f0) &&
        EQ(e1->f1, e2->f1) && EQ(e1->f2, e2->f2) &&
        EQ(e1->tgd[0], e2->tgd[0]) && EQ(e1->toes, e2->toes);
}

// test nav data consistency for BeiDou D1/D2 ----------------------------------
static int test_match_eph(eph_t *eph1, const eph_t *eph2)
{
    if (match_eph(eph1 + MAXSAT, eph2)) { // match previous ephemeris
        *(eph1 + MAXSAT) = *eph2;
        *eph1 = *eph2;
        return 1;   
    }
    else { // not match previous ephemeris
        *(eph1 + MAXSAT) = *eph2;
        return 0;   
    }
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
    
    if (sys == SYS_NONE) return;
    
    pthread_mutex_lock(&pvt->mtx);
    
    if (!strcmp(ch->sig, "L1CA") && sys == SYS_SBS) { // SBAS
        if (ch->nav->type == 9) { // geo navigation message
            int week, tow = (int)time2gpst(pvt->time, &week);
            sbsmsg_t msg = {week, tow, (uint8_t)ch->prn, 1};
            memcpy(msg.msg, data, 29);
            if (sbsupdatecorr(&msg, pvt->nav) == 9) {
                pvt->count[2]++;
            }
        }
    }
    else if (!strcmp(ch->sig, "L1CA") || !strcmp(ch->sig, "L1CB")) { // GPS/QZS LNAV
        if (ch->nav->type == 3 &&
            decode_frame(data, pvt->nav->eph + sat - 1, NULL, NULL, NULL)) {
            pvt->nav->eph[sat-1].sat = sat;
            out_log_eph(ch->time, ch->sat, ch->sig, pvt->nav->eph + sat - 1);
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
            pvt->count[2]++;
        }
        if (sys == SYS_GPS && ch->nav->type == 4) {
            decode_frame(data, NULL, NULL, pvt->nav->ion_gps, NULL);
        }
    }
    else if (!strcmp(ch->sig, "G1CA") || !strcmp(ch->sig, "G2CA")) { // GLO NAV
        pvt->nav->geph[prn-1].tof = pvt->time;
        if (ch->nav->type == 4 && test_nav_glo(ch) &&
            decode_glostr(data, pvt->nav->geph + prn - 1, NULL)) {
            pvt->nav->geph[prn-1].sat = sat;
            pvt->nav->geph[prn-1].frq = ch->prn; // FCN
            out_log_eph(ch->time, ch->sat, ch->sig, pvt->nav->geph + prn - 1);
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
            pvt->count[2]++;
        }
    }
    else if (!strcmp(ch->sig, "E1B") || !strcmp(ch->sig, "E5BI")) { // GAL I/NAV
        if (ch->nav->type == 4 &&
            decode_gal_inav(data, pvt->nav->eph + sat - 1, NULL, NULL)) {
            pvt->nav->eph[sat-1].sat = sat;
            out_log_eph(ch->time, ch->sat, ch->sig, pvt->nav->eph + sat - 1);
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
            pvt->count[2]++;
        }
    }
    else if (!strcmp(ch->sig, "E5AI")) { // GAL F/NAV
        if (ch->nav->type == 4 &&
            decode_gal_fnav(data, pvt->nav->eph + MAXSAT + sat - 1, NULL,
                NULL)) {
            pvt->nav->eph[MAXSAT+sat-1].sat = sat;
            out_log_eph(ch->time, ch->sat, ch->sig, pvt->nav->eph + MAXSAT +
                sat - 1);
            out_rtcm3_nav(pvt->rtcm, sat, 1, pvt->nav, pvt->rcv->strs[1]);
            pvt->count[2]++;
        }
    }
    else if (!strcmp(ch->sig, "B1I") || !strcmp(ch->sig, "B2I") ||
             !strcmp(ch->sig, "B3I")) {
        eph_t eph = {0};
        if (ch->prn >= 6 && ch->prn <= 58) { // BDS D1 NAV
            if (ch->nav->type == 3 && decode_bds_d1(data, &eph, NULL, NULL)) {
                if (test_match_eph(pvt->nav->eph + sat - 1, &eph)) {
                    pvt->nav->eph[sat-1].sat = sat;
                    out_log_eph(ch->time, ch->sat, ch->sig, pvt->nav->eph + sat - 1);
                    out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
                    pvt->count[2]++;
                }
                else {
                    out_log_eph(ch->time, ch->sat, ch->sig, &eph);
                    sdr_log(3, "$LOG,%.3f,%s,%s,EPHEMERIS UNMATCH", ch->time,
                        ch->sat, ch->sig);
                }
            }
        }
        else { // BDS D2 NAV
            if (ch->nav->type == 10 && decode_bds_d2(data, &eph, NULL)) {
                if (test_match_eph(pvt->nav->eph + sat - 1, &eph)) {
                    pvt->nav->eph[sat-1].sat = sat;
                    out_log_eph(ch->time, ch->sat, ch->sig, pvt->nav->eph + sat - 1);
                    out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
                    pvt->count[2]++;
                }
                else {
                    out_log_eph(ch->time, ch->sat, ch->sig, &eph);
                    sdr_log(3, "$LOG,%.3f,%s,%s,EPHEMERIS UNMATCH", ch->time,
                        ch->sat, ch->sig);
                }
            }
        }
    }
    else if (!strcmp(ch->sig, "I5S") || !strcmp(ch->sig, "ISS")) { // NavIC NAV
        if (ch->nav->type == 2 &&
            decode_irn_nav(data, pvt->nav->eph + sat - 1, NULL, NULL)) {
            pvt->nav->eph[sat-1].sat = sat;
            out_log_eph(ch->time, ch->sat, ch->sig, pvt->nav->eph + sat - 1);
            out_rtcm3_nav(pvt->rtcm, sat, 0, pvt->nav, pvt->rcv->strs[1]);
            pvt->count[2]++;
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

// update satellite az/el angles -----------------------------------------------
static void update_azel(const nav_t *nav, const sol_t *sol, ssat_t *ssat)
{
    for (int i = 0; i < MAXSAT; i++) {
        double rs[6], dts[2], var, pos[3], e[3];
        int svh;
        
        if (satpos(sol->time, sol->time, i + 1, EPHOPT_BRDC, nav, rs, dts,
            &var, &svh) && geodist(rs, sol->rr, e) > 0.0) {
            ecef2pos(sol->rr, pos);
            satazel(pos, e, ssat[i].azel);
        }
    }
}

// update PVT solution ---------------------------------------------------------
static void update_sol(sdr_pvt_t *pvt)
{
    prcopt_t opt = prcopt_default;
    opt.navsys |= SYS_GLO | SYS_GAL | SYS_QZS | SYS_CMP | SYS_IRN;
    opt.err[1] = opt.err[2] = STD_ERR;
    opt.ionoopt = IONOOPT_BRDC;
    opt.tropopt = TROPOPT_SAAS;
    opt.elmin = sdr_el_mask * D2R;
#if 1 // RAIM-FDE on
    opt.posopt[4] = 1;
#endif
    double time = pvt->ix * SDR_CYC;
    char msg[128] = "";
    
    // point positioning with L1 pseudorange
    if (pntpos(pvt->obs->data, pvt->obs->n, pvt->nav, &opt, pvt->sol, NULL,
             pvt->ssat, msg)) {
        
        // update satellite az/el angles
        update_azel(pvt->nav, pvt->sol, pvt->ssat);
        
        // correct solution time
        corr_sol_time(pvt->sol);
        
        // output log $POS and NMEA RMC, GGA, GSA and GSV
        out_log_pos(time, pvt->sol, pvt->obs->n);
        out_nmea(pvt->sol, pvt->ssat, pvt->rcv->strs[0]);
        pvt->count[0]++;
        
        // output log $SAT
        for (int i = 0; i < MAXSAT; i++) {
            if (pvt->ssat[i].snr[0] == 0) continue;
            out_log_sat(time, i + 1, pvt->ssat + i);
        }
    }
    else {
        pvt->sol->ns = 0;
        sdr_log(3, "$LOG,%.3f,PNTPOS ERROR,%s", time, msg);
    }
    pvt->nsat = pvt->obs->n;
    
#if 0 // for debug
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
        ix >= pvt->ix + (int)(sdr_lag_epoch / SDR_CYC))) {
        
        // resolve msec ambiguity in pseudorange
        res_obs_amb(pvt->obs, SYS_GPS | SYS_QZS, CODE_L5Q, 20e-3); // L5Q
        res_obs_amb(pvt->obs, SYS_QZS, CODE_L5P, 20e-3); // L5SQ, L5SQV
        res_obs_amb(pvt->obs, SYS_GLO, CODE_L3Q, 10e-3); // G3OCP
        res_obs_amb(pvt->obs, SYS_SBS, CODE_L5Q, 2e-3);  // L5Q SBAS
        
        // sort obs data
        sortobs(pvt->obs);
        
        // output log $OBS and RTCM3 observation data
        out_log_obs(pvt->ix * SDR_CYC, pvt->obs, pvt->nav);
        out_rtcm3_obs(pvt->rtcm, pvt->obs, pvt->rcv->strs[1]);
        if (pvt->obs->n > 0) pvt->count[1]++;
        
        // update PVT solution
        update_sol(pvt);
        
        // solution latency (s)
        pvt->latency = (ix - pvt->ix) * SDR_CYC;
        
        // set next epoch time and cycle
        pvt->time = timeadd(pvt->time, sdr_epoch);
        pvt->ix += (int)(sdr_epoch / SDR_CYC);
        pvt->nch = pvt->obs->n = 0; 
        
        // adjust epoch cycle within 20 ms
        if (pvt->sol->stat) {
            double dtr = ROUND(pvt->sol->dtr[0] / 0.02) * 0.02;
            if (fabs(dtr) > 0.01) {
                pvt->ix += (int)(dtr / SDR_CYC);
                sdr_log(3, "$LOG,%.3f,PVT EPOCH ADJUSTED (DT=%.3fs)",
                    pvt->ix * SDR_CYC, dtr);
            }
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
    char tstr[32], nstr[16];
    double pos[3] = {0};
    int stat = 0;
    
    pthread_mutex_lock(&pvt->mtx);
    
    if (norm(pvt->sol->rr, 3) > 1e-6) {
        time2str(pvt->sol->time, tstr, 1);
        ecef2pos(pvt->sol->rr, pos);
        stat = pvt->sol->stat;
    }
    else {
        time2str(pvt->time, tstr, 1);
    }
    pthread_mutex_unlock(&pvt->mtx);
    
    tstr[4] = tstr[7] = '-';
    sprintf(nstr, "%d/%d", pvt->sol->ns, pvt->nsat);
    sprintf(buff, "%21s %12.8f %13.8f %9.3f %-5s %s", tstr, pos[0] * R2D,
        pos[1] * R2D, pos[2], nstr, stat ? "FIX" : "---");
}
