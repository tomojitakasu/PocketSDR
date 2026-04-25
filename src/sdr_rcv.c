//
//  Pocket SDR C Library - GNSS SDR Receiver Functions
//
//  Author:
//  T.TAKASU
//
//  History:
//  2024-02-08  1.0  separated from pocket_trk.c
//  2024-03-25  1.1  modify API sdr_rcv_new(), sdr_rcv_start()
//  2024-04-04  1.2  modify API sdr_rcv_new(), sdr_rcv_start()
//  2024-04-28  1.3  support Pocket SDR FE 4CH and PVT
//  2024-05-28  1.4  modify API sdr_rcv_new(), sdr_rcv_start()
//  2024-06-21  1.5  add API sdr_rcv_open_dev(), sdr_rcv_open_file()
//                   sdr_rcv_close()
//  2024-06-28  1.6  modify API sdr_rcv_open_file()
//  2024-08-26  1.7  support tag-file output for raw IF data log
//  2024-12-30  1.8  support Pocket SDR FE 8CH
//                   modify API sdr_rcv_open_dev(), sdr_rcv_open_file()
//  2025-01-17  1.9  delete RINEX output by -OUTRNX option
//  2025-11-16  1.10 support alternative RF frontends
//
#include "pocket_sdr.h"

// constants and macros ---------------------------------------------------------
#define MAX_BUFF   8000         // max number of IF data buffer (* SDR_CYC)
#define LOG_CYC    1000         // receiver channel log cycle (* SDR_CYC)
#define TH_CYC     50           // receiver channel thread cycle (ms)
#define SCALE_CYC  1000         // scale update cycle (* SDR_CYC)
#define TO_REACQ   60.0         // re-acquisition timeout (s)
#define MIN_LOCK   2.0          // min lock time for re-acquisition (s)
#define NUM_COL    106          // number of channel status columns
#define MAX_ACQ    4.0          // max code length for direct acquisition (ms)
#define MAX_BUFF_USE 90         // max buffer usage rate (%)
#define MAX_BAR    12           // C/N0 bar width
#define SAMPLES_STATS 100       // samples for stats
#define AGC_LEVEL  2.3          // target std-dev for auto gain control
#define TRACE_FILE "./pocket_sdr.trace" // debug trace file
#define TRACE_LEVEL 3           // default debug trace level

#define SQR(x)     ((x) * (x))
#define MIN(x, y)  ((x) < (y) ? (x) : (y))
#define CLIP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

// global variables ------------------------------------------------------------
double sdr_max_acq = MAX_ACQ;

// append string ---------------------------------------------------------------
static int ap_str(char *buff, int size, const char *fmt, ...)
{
    if (size <= 0) return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buff, (size_t)size, fmt, ap);
    va_end(ap);
    if (n >= 0) return n >= size ? size - 1 : n;
    buff[0] = '\0';
    return 0;
}

// get IF data buffer pointer --------------------------------------------------
static int64_t get_buff_ix(sdr_rcv_t *rcv)
{
    sdr_mutex_lock(&rcv->mtx);
    int64_t ix = rcv->ix;
    sdr_mutex_unlock(&rcv->mtx);
    return ix;
}

// set IF data buffer pointer --------------------------------------------------
static void set_buff_ix(sdr_rcv_t *rcv, int64_t ix)
{
    sdr_mutex_lock(&rcv->mtx);
    rcv->ix = ix;
    sdr_mutex_unlock(&rcv->mtx);
}

// number of RF channels -------------------------------------------------------
static int num_rfch(int fmt)
{
    switch (fmt) {
        case SDR_FMT_RAW8  : return 2;
        case SDR_FMT_RAW16 : return 4;
        case SDR_FMT_RAW16I:
        case SDR_FMT_RAW32 : return 8;
    }
    return 1;
}

// bytes in a sample -----------------------------------------------------------
static int sample_byte(int fmt)
{
    switch (fmt) {
        case SDR_FMT_INT8X2:
        case SDR_FMT_RAW16 :
        case SDR_FMT_RAW16I:
        case SDR_FMT_CS8   : return 2;
        case SDR_FMT_CS16:
        case SDR_FMT_RAW32 : return 4;
    }
    return 1;
}

// C/N0 bar --------------------------------------------------------------------
static void cn0_bar(float cn0, char *buff, int max_bar)
{
    int n = CLIP((int)(max_bar / 20.0 * (cn0 - 30.0)) + 1, 0, max_bar);
    for (int i = 0; i < n; i++) {
        buff[i] = '|';
    }
    buff[n] = '\0';
}

// SDR receiver channel sync status --------------------------------------------
static void sync_stat(sdr_ch_t *ch, char *buff, int size)
{
    snprintf(buff, size, "%s%s%s%s", (ch->trk->sec_sync > 0) ? "S" : "-",
        (ch->nav->ssync > 0) ? "B" : "-", (ch->nav->fsync > 0) ? "F" : "-",
        (ch->nav->rev) ? "R" : "-");
}

// get number of tracking channels ---------------------------------------------
static int get_nch_trk(sdr_rcv_t *rcv, char *sys)
{
    static const char *syss = "GREJCIS";
    int nch = 0, mask = 0;
    for (int i = 0; i < rcv->nch; i++) {
        if (rcv->th[i]->ch->state != SDR_STATE_LOCK) continue;
        nch++;
        mask |= (1 << (int)(strchr(syss, rcv->th[i]->ch->sat[0]) - syss));
    }
    for (int i = 0, j = 0; i < 7; i++) {
        if (mask & (1 << i)) sys[j++] = syss[i];
    }
    return nch;
}

// print SDR receiver status header --------------------------------------------
static int print_head(sdr_rcv_t *rcv, char *buff, int size)
{
    char solstr[128] = "", sys[16] = "";
    int n = 0, nch_bb = 0, nch_trk = 0, ch_srch = 0;
    double buff_use = 0.0;
    
    if (rcv) {
        nch_bb = rcv->nch;
        nch_trk = get_nch_trk(rcv, sys);
        ch_srch = rcv->ich + 1;
        buff_use = rcv->buff_use;
        sdr_pvt_solstr(rcv->pvt, solstr, sizeof(solstr));
    }
    n += ap_str(buff + n, size - n, " %-*s BUFF:%3.0f%% SRCH:%4d LOCK:%4d/%4d\n",
        NUM_COL - 36, solstr, buff_use, ch_srch, nch_trk, nch_bb);
    n += ap_str(buff + n, size - n, "%4s %2s %4s %5s %3s %8s %4s %-12s %11s "
        "%7s %11s %4s %5s %4s %4s %3s\n", "CH", "RF", "SAT", "SIG", "PRN",
        "LOCK(s)", "C/N0", "(dB-Hz)", "COFF(ms)", "DOP(Hz)", "ADR(cyc)", "SYNC",
        "#NAV", "#ERR", "#LOL", "FEC");
    return n;
}

// print SDR receiver channel status -------------------------------------------
static int print_ch_stat(sdr_ch_t *ch, int opt, char *buff, int size)
{
    char bar[MAX_BAR+1], stat[16];
    int n = 0;
    cn0_bar((float)ch->cn0, bar, MAX_BAR);
    sync_stat(ch, stat, sizeof(stat));
    n += ap_str(buff + n, size - n, "%4d %2d %4s %5s %3d %8.2f %4.1f %-13s"
        "%11.7f %7.1f %11.1f %s %5d %4d %4d %3d", ch->no, ch->rf_ch + 1,
        *ch->sat ? ch->sat : "???", ch->sig, ch->prn, ch->lock * ch->T, ch->cn0,
        bar, ch->coff * 1e3, ch->fd, ch->adr, stat, ch->nav->count[0],
        ch->nav->count[1], ch->lost, ch->nav->nerr);
    if (opt) {
        n += ap_str(buff + n, size - n, " %8.3f %8.3f %1d %5d %1d %1d %4d "
            "%6.0f %1d", ch->trk->err_phas, ch->trk->err_code * CLIGHT,
            ch->sig_srch, ch->nav->seq, ch->nav->type, ch->nav->stat, ch->week,
            ch->tow * 1e-3, ch->tow_v);
    }
    n += ap_str(buff + n, size - n, "\n");
    return n;
}

// satellite selection ---------------------------------------------------------
static int sat_select(const char *sat, const char *sys)
{
    if (!strcmp(sys, "ALL")) return 1; 
    if (sat[0] == 'G' && !strcmp(sys, "GPS"    )) return 1;
    if (sat[0] == 'R' && !strcmp(sys, "GLONASS")) return 1;
    if (sat[0] == 'E' && !strcmp(sys, "Galileo")) return 1;
    if (sat[0] == 'J' && !strcmp(sys, "QZSS"   )) return 1;
    if (sat[0] == 'C' && !strcmp(sys, "BeiDou" )) return 1;
    if (sat[0] == 'I' && !strcmp(sys, "NavIC"  )) return 1;
    if ((sat[0] == '1' || sat[0] == 'S') && !strcmp(sys, "SBAS")) return 1;
    return 0;
}

//------------------------------------------------------------------------------
//  Get SDR receiver channel status as string.
//
//  args:
//      rcv       (I)  SDR receiver
//      sys       (I)  system (0: all)
//      chno      (I)  channel number (0: locked, -1: all including IDLE)
//      minlock   (I)  min lock time (s)
//      rfch      (I)  RF CH (0:all)
//      opt       (I)  option flag (0:short, 1:long)
//      buff      (IO) string buffer
//      size      (I)  size of string buffer
//
//  returns:
//      size of output string
//
int sdr_rcv_ch_stat(sdr_rcv_t *rcv, const char *sys, int chno, double min_lock,
    int rfch, int opt, char *buff, int size)
{
    int n = 0;
    
    n += print_head(rcv, buff + n, size - n);
    for (int i = 0; rcv && i < rcv->nch; i++) {
        sdr_ch_t *ch = rcv->th[i]->ch;
        if ((chno >= 1 && ch->no != chno) || !sat_select(ch->sat, sys)) continue;
        if (chno == -1 || (ch->state == SDR_STATE_LOCK &&
            ch->lock * ch->T >= min_lock && (!rfch || rfch == ch->rf_ch + 1))) {
            n += print_ch_stat(ch, opt, buff + n, size - n);
        }
    }
    return n;
}

// get receiver stream status --------------------------------------------------
void sdr_rcv_str_stat(sdr_rcv_t *rcv, int *stat)
{
    char msg[1024];
    for (int i = 0; i < 4; i++) {
        // stat[0]: NMEA, [1]: RTCM, [2]: log, [3]: IF data
        // -1: error, 0: close, 1: wait, 2: connect, 3: active
        stat[i] = rcv && rcv->strs[i] ? strstat(rcv->strs[i], msg) : 0;
    }
    stat[2] = sdr_log_stat();
}

// get receiver status as string -----------------------------------------------
int sdr_rcv_rcv_stat(sdr_rcv_t *rcv, char *buff, int size)
{
    static const char *src_str[] = {
        "---", "IF_Data", "PocketSDR_FE", "Stream", "SOAPY_FE"
    };
    static const char *fmt_str[] = {
        "---", "INT8", "INT8X2", "RAW8", "RAW16", "RAW16I", "RAW32", "CS8",
        "CS16"
    };
    static const char *IQ_str[] = {"--", "I", "IQ"};
    int n = 0;
    
    if (rcv && rcv->state) {
        char sys[16] = "", src[64] = "";
        int nch_trk = get_nch_trk(rcv, sys);
        if (rcv->dev == SDR_DEV_SOAPY) {
            snprintf(src, sizeof(src), "%s(%s)", src_str[rcv->dev],
                ((sdr_sdev_t *)rcv->dp)->driver);
        } else {
            snprintf(src, sizeof(src), "%s", src_str[rcv->dev]);
        }
        n += ap_str(buff + n, size - n, "%.3f %s %s/%d/%d ",
            get_buff_ix(rcv) * SDR_CYC, src, fmt_str[rcv->fmt], rcv->nrfch,
            rcv->narch);
        for (int i = 0; i < 8; i++) {
            double fo = i < num_rfch(rcv->fmt) ? rcv->rfch[i].fo * 1e-6 : 0.0;
            n += ap_str(buff + n, size - n, "%.2f%s", fo, i == 3 || i == 7 ? " " : ",");
        }
        for (int i = 0; i < 8; i++) {
            int IQ = i < num_rfch(rcv->fmt) ? rcv->rfch[i].IQ : 0;
            n += ap_str(buff + n, size - n, "%s%s", IQ_str[IQ],
                i == 7 ? " " : ",");
        }
        n += ap_str(buff + n, size - n, "%.3f %d/%d %.1f %.1f ", rcv->fs * 1e-6,
            nch_trk, rcv->nch, rcv->data_rate * 1e-6, rcv->buff_use);
        n += ap_str(buff + n, size - n, "%.3f %d/%d/%d %.1f", rcv->pvt->latency,
            rcv->pvt->count[0], rcv->pvt->count[1], rcv->pvt->count[2],
            rcv->data_sum);
    } else {
        n += ap_str(buff + n, size - n, "%.3f --- ---/-/- ", 0.0);
        for (int i = 0; i < 8; i++) {
            n += ap_str(buff + n, size - n, "%.2f%s", 0.0,
                i == 3 || i == 7 ? " " : ",");
        }
        for (int i = 0; i < 8; i++) {
            n += ap_str(buff + n, size - n, "--%s", i == 7 ? " " : ",");
        }
        n += ap_str(buff + n, size - n, "%.3f %d/%d %.1f %.1f ", 0.0, 0, 0, 0.0, 0.0);
        n += ap_str(buff + n, size - n, "%.3f %d/%d/%d %.1f", 0.0, 0, 0, 0, 0.0);
    }
    return n;
}

// get satellite status as string ----------------------------------------------
int sdr_rcv_sat_stat(sdr_rcv_t *rcv, const char *sat, char *buff, int size)
{
    int n = 0, satno = satid2no(sat);
    
    buff[0] = '\0';
    if (rcv && satno && sat[1] != '+') {
        sdr_mutex_lock(&rcv->pvt->mtx);
        
        ssat_t *ssat = rcv->pvt->ssat + satno - 1;
        sol_t *sol = rcv->pvt->sol;
        int prn, sys = satsys(satno, &prn), eph = 0, svh = 0, fcn = 0;
        
        if (sys == SYS_GLO) {
            geph_t *geph = rcv->pvt->nav->geph + prn - 1;
            eph = timediff(sol->time, geph->toe) <= 1800.0;
            svh = geph->svh;
            fcn = geph->frq;
        } else if (sys == SYS_SBS) {
            seph_t *seph = rcv->pvt->nav->seph + prn - MINPRNSBS;
            eph = timediff(sol->time, seph->t0) <= 360.0;
            svh = seph->svh;
        } else {
            eph_t *eph1 = rcv->pvt->nav->eph + satno - 1;
            eph_t *eph2 = rcv->pvt->nav->eph + MAXSAT + satno - 1;
            eph = timediff(sol->time, eph1->toe) <= 7200.0 ||
                  timediff(sol->time, eph2->toe) <= 7200.0;
            svh = eph1->svh | eph2->svh;
            if (sys == SYS_QZS) svh &= 0xFE; // mask L6 health
        }
        // sat az el pvt obs eph svh fcn
        n += ap_str(buff + n, size - n, "%s %.1f %.1f %d %d %d %d %d\n", sat,
            ssat->azel[0] * R2D, ssat->azel[1] * R2D, sol->stat && ssat->vs,
            ssat->snr[0] > 0.0, eph, svh, fcn);
        
        sdr_mutex_unlock(&rcv->pvt->mtx);
    }
    return n;
}

// select channel for correlator status ----------------------------------------
void sdr_rcv_sel_ch(sdr_rcv_t *rcv, int ch)
{
    if (!rcv || !rcv->state) return;
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_set_corr(rcv->th[i]->ch, i + 1 == ch ? SDR_N_CORRX : 0);
    }
}

// get correlator status -------------------------------------------------------
int sdr_rcv_corr_stat(sdr_rcv_t *rcv, int ch, double *stat, double *pos,
    sdr_cpx_t *C, double *P)
{
    if (!rcv || !rcv->state || ch < 1 || ch > rcv->nch) return 0;
    return sdr_ch_corr_stat(rcv->th[ch-1]->ch, stat, pos, C, P);
}

// get correlator history ------------------------------------------------------
int sdr_rcv_corr_hist(sdr_rcv_t *rcv, int ch, double tspan, double *stat,
    sdr_cpx_t *P)
{
    if (!rcv || !rcv->state || ch < 1 || ch > rcv->nch) return 0;
    return sdr_ch_corr_hist(rcv->th[ch-1]->ch, tspan, stat, P);
}

// get RF channel status -------------------------------------------------------
int sdr_rcv_rfch_stat(sdr_rcv_t *rcv, int ch, double *stat)
{
    if (!rcv || !rcv->state || ch < 1 || ch > rcv->nrfch + rcv->narch) return 0;
    stat[0] = rcv->dev;
    stat[1] = rcv->fmt;
    stat[2] = rcv->fs;
    stat[3] = rcv->rfch[ch-1].fo +
        (rcv->rtoc[ch-1] ? rcv->fs * 0.25 : 0.0); // rtoc: shift center by +fs/4
    stat[4] = rcv->buff[ch-1]->IQ;
    stat[5] = rcv->rfch[ch-1].bits;
    stat[6] = rcv->data_std;
    return 1;
}

// get RF channel PSD ----------------------------------------------------------
int sdr_rcv_rfch_psd(sdr_rcv_t *rcv, int ch, double tave, int N, float *psd)
{
    if (!rcv || !rcv->state || ch < 1 || ch > rcv->nrfch + rcv->narch) return 0;
    int n = (int)(rcv->fs * tave);
    int64_t ix = get_buff_ix(rcv);
    if (N < 0 || n < N || ix * rcv->N < n) return 0;
    sdr_cpx_t *buff = sdr_cpx_malloc(n);
    for (int i = 0; i < n; i++) {
        int j = (int)((ix * rcv->N - n + i) % rcv->buff[ch-1]->N);
        sdr_cpx8_t data = rcv->buff[ch-1]->data[j];
        buff[i][0] = (float)SDR_CPX8_I(data);
        buff[i][1] = (float)SDR_CPX8_Q(data);
    }
    sdr_psd_cpx(buff, n, N, rcv->fs, rcv->buff[ch-1]->IQ, psd);
    sdr_cpx_free(buff);
    return rcv->buff[ch-1]->IQ == 1 ? N / 2 : N; // PSD size
}

// get RF channel histogram ----------------------------------------------------
int sdr_rcv_rfch_hist(sdr_rcv_t *rcv, int ch, double tave, int *val,
    double *hist1, double *hist2)
{
    if (!rcv || !rcv->state || ch < 1 || ch > rcv->nrfch + rcv->narch) return 0;
    int n = (int)(rcv->fs * tave), cnt[2][256] = {{0}}, sum[2] = {0}, nval = 0;
    int64_t ix = get_buff_ix(rcv);
    if (ix * rcv->N < n) return 0;
    for (int i = 0; i < n; i++) {
        int j = (int)((ix * rcv->N - n + i) % rcv->buff[ch-1]->N);
        sdr_cpx8_t data = rcv->buff[ch-1]->data[j];
        cnt[0][SDR_CPX8_I(data)+128]++;
        cnt[1][SDR_CPX8_Q(data)+128]++;
    }
    for (int i = 0; i < 256; i++) {
        sum[0] += cnt[0][i];
        sum[1] += cnt[1][i];
    }
    for (int i = 0; i < 256; i++) {
        if (cnt[0][i] == 0 && cnt[1][i] == 0) continue;
        hist1[nval] = sum[0] > 0 ? (double)cnt[0][i] / sum[0] : 0.0;
        if (rcv->buff[ch-1]->IQ == 2) {
            hist2[nval] = sum[1] > 0 ? (double)cnt[1][i] / sum[1] : 0.0;
        }
        val[nval++] = i - 128;
    }
    return nval;
}

// get PVT solution ------------------------------------------------------------
int sdr_rcv_pvt_sol(sdr_rcv_t *rcv, char *buff, int size)
{
    if (!rcv || !rcv->state) return 0;
    sdr_pvt_solstr(rcv->pvt, buff, size);
    return 1;
}

//------------------------------------------------------------------------------
//  Output log $TIME (time information).
//
//  format:
//      $TIME,time,year,mon,day,hour,min,sec,tsys
//          time  receiver time (s)
//          year  year (2000-2099)
//          mon   month (1-12)
//          day   day (1-31)
//          hour  hour (0-23)
//          min   minute (0-59)
//          sec   second (0.000-59.999)
//          tsys  time system ("GPST" or "UTC")
//
static void out_log_time(double time)
{
    double t[6] = {0};
    time2epoch(utc2gpst(timeget()), t);
    sdr_log(3, "$TIME,%.3f,%.0f,%.0f,%.0f,%.0f,%.0f,%.3f,GPST", time, t[0],
       t[1], t[2], t[3], t[4], t[5]);
}

// new SDR receiver channel thread ---------------------------------------------
static sdr_ch_th_t *ch_th_new(const char *sig, int prn, double fi, double fs,
    sdr_rcv_t *rcv)
{
    sdr_ch_th_t *th = (sdr_ch_th_t *)sdr_malloc(sizeof(sdr_ch_th_t));
    
    if (!(th->ch = sdr_ch_new(sig, prn, fs, fi))) {
        sdr_free(th);
        return NULL;
    }
    th->rcv = rcv;
    return th;
}

// free SDR receiver channel ---------------------------------------------------
static void ch_th_free(sdr_ch_th_t *th)
{
    if (!th) return;
    sdr_ch_free(th->ch);
    sdr_free(th);
}

// SDR receiver channel thread ------------------------------------------------
static void *ch_thread(void *arg)
{
    sdr_ch_th_t *th = (sdr_ch_th_t *)arg;
    sdr_ch_t *ch = th->ch;
    int n = ch->N / th->rcv->N;
    
    while (th->state) {
        int64_t ix = get_buff_ix(th->rcv);
        for ( ; th->ix + 2 * n <= ix && th->state; th->ix += n) {
            
            // update SDR receiver channel
            sdr_ch_update(ch, th->ix * SDR_CYC, th->rcv->buff[ch->rf_ch],
                th->rcv->N * (int)(th->ix % MAX_BUFF));
            
            // update navigation data
            if (ch->nav->stat) {
                sdr_pvt_udnav(th->rcv->pvt, ch);
                ch->nav->stat = 0;
            }
            // update observation data
            sdr_pvt_udobs(th->rcv->pvt, th->ix, ch);
        }
        sdr_sleep_msec(TH_CYC);
    }
    return NULL;
}

// start SDR receiver channel ----------------------------------------------------
static int ch_th_start(sdr_ch_th_t *th)
{
    if (th->state) return 0;
    th->state = 1;
    return sdr_thread_create(&th->thread, ch_thread, th);
}

// stop SDR receiver channel ---------------------------------------------------
static void ch_th_stop(sdr_ch_th_t *th)
{
    th->state = 0;
}

// assign RF CH by option ------------------------------------------------------
static int opt_rfch(int nrfch, const char *sig, const char *opt, int *ch)
{
    const char *p;
    char str[64];
    if (!(p = strstr(opt, "-RFCH")) || !(p = strstr(p + 5, sig)) ||
        !sscanf(p + strlen(sig), ":%63[^ ]", str)) {
        return 0;
    }
    int chs[SDR_MAX_NPRN], n = sdr_parse_nums(str, chs), nch = 0;
    for (int i = 0; i < n && nch < SDR_MAX_RFCH; i++) {
        if (chs[i] >= 1 && chs[i] <= nrfch) ch[nch++] = chs[i] - 1;
    }
    return nch;
}

// set RF channel and IF frequency ---------------------------------------------
static int set_rfch(int fmt, double fs, const sdr_rfch_t *rfch, int nrfch,
    const char *sig, const char *opt, int *ch, double *fi)
{
    double freq = sdr_sig_freq(sig);
    int n, nch = 1;
    
    if ((n = opt_rfch(nrfch, sig, opt, ch)) > 0) { // RF CH assignment option
        nch = n;
    } else if (fmt == SDR_FMT_RAW8) { // FE 2CH
        ch[0] = freq > 1.4e9 ? 0 : 1;
    } else if (fmt == SDR_FMT_RAW16) { // FE 4CH
        for (int i = 1; i < 4; i++) {
            if (fabs(freq - rfch[i].fo) < fabs(freq - rfch[0].fo)) ch[0] = i;
        }
    } else if (fmt == SDR_FMT_RAW32) { // FE 8CH
        for (int i = 1; i < 8; i++) {
            if (fabs(freq - rfch[i].fo) < fabs(freq - rfch[ch[0]].fo)) ch[0] = i;
        }
    }
    for (int i = 0; i < nch; i++) {
        fi[i] = rfch[ch[i]].fo > 0.0 ? freq - rfch[ch[i]].fo :
            (rfch[ch[i]].IQ == 1 ? fs * 0.5 : 0.0);
    }
    return nch;
}

// add LPF to RF channel -------------------------------------------------------
static void add_lpf(const char *opt, int nrfch, double fs, sdr_lpf_t **lpf)
{
    const char *p;
    int rfch, n = 0;
    double fc;
    
    // option: -LPF=<rfch>:<MHz>[,<rfch>:<MHz>...])
    if (!(p = strstr(opt, "-LPF="))) return;
    
    for (p += 5; ; p++) {
        if (sscanf(p, "%d:%lf%n", &rfch, &fc, &n) < 2) break;
        if (rfch >= 1 && rfch <= nrfch && fc > 0.0) {
            sdr_lpf_free(lpf[rfch-1]);
            lpf[rfch-1] = sdr_lpf_new(fc * 1e6, fs);
        }
        p += n;
        if (*p != ',') break;
    }
}

// set signal search channels --------------------------------------------------
static void set_srch_ch(sdr_rcv_t *rcv, const char *opt)
{
    int rfch[SDR_MAX_RFCH] = {0}, nch;
    
    if (!(nch = opt_rfch(rcv->fmt, "SRCH", opt, rfch))) {
        return;
    }
    for (int i = 0; i < rcv->nch; i++) {
        int j;
        for (j = 0; j < nch; j++) {
            if (rcv->th[i]->ch->rf_ch == rfch[j]) break;
        }
        rcv->th[i]->ch->sig_srch = (j >= nch) ? 0 : 1;
    }
}

//------------------------------------------------------------------------------
//  Generate a new SDR receiver.
//
//  args:
//      sigs      (I)  signal types as a string array {sig1, sig2, ..., sign}
//      prns      (I)  PRN numbers as int array {prn1, prn2, ..., prnn}
//      n         (I)  number of sigs and prns
//      fmt       (I)  IF data format (SDR_FMT_???)
//      fs        (I)  sampling rate (sps)
//      fo        (I)  LO frequency for each RFCH (Hz)
//      IQ        (I)  sampling type for each RFCH (1:I, 2:IQ)
//      bits      (I)  number of bits for each RFCH
//      opt       (I)  receiver options
//                     -ARCH=<nch>: number of array channels
//
//  returns:
//      SDR receiver (NULL: error)
//
sdr_rcv_t *sdr_rcv_new(const char **sigs, const int *prns, int n, int fmt,
    double fs, const double *fo, const int *IQ, const int *bits,
    const char *opt)
{
    sdr_rcv_t *rcv = (sdr_rcv_t *)sdr_malloc(sizeof(sdr_rcv_t));
    const char *p;
    int nrfch, narch = 0;
    
    if ((p = strstr(opt, "-ARCH="))) sscanf(p, "-ARCH=%d", &narch);
    
    rcv->fmt = fmt;
    rcv->fs = fs;
    rcv->nrfch = num_rfch(fmt);
    rcv->narch = MIN(narch, SDR_MAX_RFCH - rcv->nrfch);
    nrfch = rcv->nrfch + rcv->narch;
    for (int i = 0; i < rcv->nrfch; i++) {
        rcv->rfch[i].fo = fo[i];
        rcv->rfch[i].IQ = IQ[i];
        rcv->rfch[i].bits = (fmt == SDR_FMT_INT8 || fmt == SDR_FMT_INT8X2 ||
            fmt == SDR_FMT_CS8) ? 8 : (fmt == SDR_FMT_CS16 ? 16 : bits[i]);
    }
    for (int i = rcv->nrfch; i < nrfch; i++) {
        rcv->rfch[i].fo = fo[0];
        rcv->rfch[i].IQ = IQ[0];
        rcv->rfch[i].bits = 4;
    }
    rcv->N = (int)(SDR_CYC * fs);

    // add LPF
    add_lpf(opt, rcv->nrfch, fs, rcv->lpf);
    
    // auto-enable rtoc for real-sampling RF CHs (fs/4 shift)
    for (int i = 0; i < rcv->nrfch; i++) {
        if (rcv->rfch[i].IQ == 1) rcv->rtoc[i] = 1;
    }
    for (int i = 0; i < n && rcv->nch < SDR_MAX_NCH; i++) {
        int ch[SDR_MAX_RFCH] = {0};
        double fi[SDR_MAX_RFCH] = {0};
        int nch = set_rfch(fmt, fs, rcv->rfch, nrfch, sigs[i], opt, ch, fi);
        for (int j = 0; j < nch; j++) {
            if (rcv->rtoc[ch[j]]) fi[j] -= fs * 0.25; // fs/4 shift
        }
        for (int j = 0; j < nch && rcv->nch < SDR_MAX_NCH; j++) {
            sdr_ch_th_t *th = ch_th_new(sigs[i], prns[i], fi[j], fs, rcv);
            if (th) {
                th->ch->no = rcv->nch + 1;
                th->ch->rf_ch = ch[j];
                th->ch->sig_srch = 1;
                rcv->th[rcv->nch++] = th;
            } else {
                fprintf(stderr, "signal / prn error: %s / %d\n", sigs[i],
                    prns[i]);
            }
        }
    }
    set_srch_ch(rcv, opt);
    for (int i = 0; i < rcv->nrfch + rcv->narch; i++) {
        int iq = rcv->rtoc[i] ? 2 : rcv->rfch[i].IQ;
        rcv->buff[i] = sdr_buff_new(rcv->N * MAX_BUFF, iq);
    }
    rcv->ich = -1;
    snprintf(rcv->opt, sizeof(rcv->opt), "%s", opt);
    sdr_mutex_init(&rcv->mtx);
    return rcv;
}

//------------------------------------------------------------------------------
//  Free a SDR receiver.
//
//  args:
//      rcv       (I)  SDR receiver generated by sdr_rcv_new()
//
//  returns:
//      none
//
void sdr_rcv_free(sdr_rcv_t *rcv)
{
    if (!rcv) return;
    
    for (int i = 0; i < rcv->nch; i++) {
        ch_th_free(rcv->th[i]);
    }
    for (int i = 0; i < rcv->nrfch + rcv->narch; i++) {
        sdr_buff_free(rcv->buff[i]);
    }
    for (int i = 0; i < SDR_MAX_RFCH; i++) {
        sdr_lpf_free(rcv->lpf[i]);
    }
    sdr_free(rcv);
}

// read IF data ----------------------------------------------------------------
static int read_data(sdr_rcv_t *rcv, uint8_t *raw, int N)
{
    if (rcv->dev == SDR_DEV_FILE) { // file input
        if (fread(raw, N, 1, (FILE *)rcv->dp) < 1) {
            return 0; // end of file
        }
    } else if (rcv->dev == SDR_DEV_USB) { // Pocket SDR FE
        while (!sdr_dev_read((sdr_dev_t *)rcv->dp, raw, N)) {
            if (!rcv->state) return 0;
            sdr_sleep_msec(1);
        }
    } else if (rcv->dev == SDR_DEV_SOAPY) { // SoapySDR device
        int n;
        while ((n = sdr_sdev_read((sdr_sdev_t *)rcv->dp, raw, N)) == 0) {
            if (!rcv->state) return 0;
            sdr_sleep_msec(1);
        }
        if (n < 0) return 0;
    } else { // SDR_DEV_STR
        for (int n = 0; n < N; ) {
            n += strread((stream_t *)rcv->dp, raw + n, N - n);
            if (!rcv->state) return 0;
            sdr_sleep_msec(1);
        }
    }
    return N;
}

// generate raw to sdr_cpx8_t LUT ----------------------------------------------
static sdr_cpx8_t *gen_LUTC(int fmt, int rfch, int IQ, int bits)
{
    static const int8_t valI_2b[] = {1, 3, -1, -3}, valQ_2b[] = {-1, -3, 1, 3};
    static const int8_t valI_3b[] = {1, 3, 5, 7, -1, -3, -5, -7};
    
    sdr_cpx8_t *LUTC = (sdr_cpx8_t *)sdr_malloc(sizeof(sdr_cpx8_t) * 256);
    
    for (int i = 0; i < 256; i++) {
        int8_t I, Q = 0;
        if (fmt == SDR_FMT_RAW16I) { // I-2bit-sampling
            I = valI_2b[(i>>((rfch % 4) * 2)) & 0x3];
        } else if (IQ == 1 && bits == 2) { // I-2bit-sampling
            I = valI_2b[(i>>(rfch % 2 ? 4 : 0)) & 0x3];
        } else if (IQ == 1 && bits == 3) { // I-3bit-sampling
            int val = (i>>(rfch % 2 ? 4 : 0)) & 0xF;
            I = valI_3b[((val << 1) & 6) + ((val >> 3) & 1)];
        } else { // IQ-sampling
            I = valI_2b[(i>>(rfch % 2 ? 4 : 0)) & 0x3];
            Q = valQ_2b[(i>>(rfch % 2 ? 6 : 2)) & 0x3];
        }
        LUTC[i] = SDR_CPX8(I, Q);
    }
    return LUTC;
}

// generate raw to int8_t LUT --------------------------------------------------
static int8_t *gen_LUT8(int fmt, double off, double scale)
{
    int N = fmt == SDR_FMT_CS16 ? 65536 : 256, val;
    int8_t *LUT8 = (int8_t *)sdr_malloc(sizeof(int8_t) * N);
    
    for (int i = -N / 2; i < N / 2; i++) {
        val = (int)floor((i - off) / scale + 0.5);
        int j = fmt == SDR_FMT_CS16 ? (uint16_t)i : (uint8_t)i;
        LUT8[j] = (int8_t)CLIP(val, -7, 7);
    }
    return LUT8;
}

// generate raw to sdr_cpx_t LUT -----------------------------------------------
static sdr_cpx_t *gen_LUTS(int fmt, const int *IQ, const int *bits,
    const sdr_cpx_t *weight)
{
    static const int8_t valI_2b[] = {1, 3, -1, -3}, valQ_2b[] = {-1, -3, 1, 3};
    static const int8_t valI_3b[] = {1, 3, 5, 7, -1, -3, -5, -7};
    
    if (fmt != SDR_FMT_RAW32) return NULL;
    int N = 65536;
    sdr_cpx_t *LUTS = (sdr_cpx_t *)sdr_malloc(sizeof(sdr_cpx_t) * N * 2);
    
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < 4; j++) {
            int8_t I, Q;
            if (IQ[j] == 2) {
                I = valI_2b[(i>>(j*4  ))&3];
                Q = valQ_2b[(i>>(j*4+2))&3];
            } else if (bits[j] == 2) {
                I = valI_2b[(i>>(j*4))&3];
                Q = 0;
            } else {
                int val = (i>>(j*4)) & 0xF;
                I = valI_3b[((val << 1) & 6) + ((val >> 3) & 1)];
                Q = 0;
            }
            LUTS[i][0] += I * weight[j][0] - Q * weight[j][1];
            LUTS[i][1] += I * weight[j][1] + Q * weight[j][0];
            if (IQ[j+4] == 2) {
                I = valI_2b[(i>>(j*4  ))&3];
                Q = valQ_2b[(i>>(j*4+2))&3];
            } else if (bits[j+4] == 2) {
                I = valI_2b[(i>>(j*4))&3];
                Q = 0;
            } else {
                int val = (i>>(j*4)) & 0xF;
                I = valI_3b[((val << 1) & 6) + ((val >> 3) & 1)];
                Q = 0;
            }
            LUTS[i+N][0] += I * weight[j+4][0] - Q * weight[j+4][1];
            LUTS[i+N][1] += I * weight[j+4][1] + Q * weight[j+4][0];
        }
    }
    return LUTS;
}

// generate raw to sdr_cpx8_t LUT with fs/4 real-to-complex mixing -------------
static sdr_cpx8_t *gen_LUTC_rtoc(int rfch)
{
    static const int8_t valI_3b[] = {1, 3, 5, 7, -1, -3, -5, -7};
    sdr_cpx8_t *LUT = (sdr_cpx8_t *)sdr_malloc(sizeof(sdr_cpx8_t) * 256 * 4);

    for (int i = 0; i < 256; i++) {
        int val = (i >> (rfch % 2 ? 4 : 0)) & 0xF;
        int8_t I = valI_3b[((val << 1) & 6) + ((val >> 3) & 1)];
        LUT[0*256+i] = SDR_CPX8( I,  0); // phase 0
        LUT[1*256+i] = SDR_CPX8( 0, -I); // phase 1
        LUT[2*256+i] = SDR_CPX8(-I,  0); // phase 2
        LUT[3*256+i] = SDR_CPX8( 0,  I); // phase 3
    }
    return LUT;
}

// per-CH write macro: rtoc (fs/4 mix LUT) only; LPF is applied in batch later
#define WR_CH(k, byte, ph) do { \
    rcv->buff[(k)]->data[i] = rcv->rtoc[(k)] \
        ? LUTC[(k)][((ph) << 8) | (byte)] \
        : LUTC[(k)][(byte)]; \
} while (0)

// write IF data buffer --------------------------------------------------------
static void write_buff(sdr_rcv_t *rcv, const uint8_t *raw, int64_t ix)
{
    int base = rcv->N * (int)(ix % MAX_BUFF);
    int i = base;
    int8_t *LUT8 = (int8_t *)rcv->LUT[0];
    sdr_cpx8_t *LUTC[8];
    for (int j = 0; j < 8; j++) LUTC[j] = (sdr_cpx8_t *)rcv->LUT[j];
    int ph0 = (int)((ix * rcv->N) & 3); // rtoc phase at start of this cycle
    int nch_lpf = 0; // number of channels eligible for LPF (WR_CH paths only)

    if (rcv->fmt == SDR_FMT_INT8) { // int8
        for (int j = 0; j < rcv->N; i++, j++) {
            rcv->buff[0]->data[i] = SDR_CPX8(LUT8[raw[j]], 0);
        }
    } else if (rcv->fmt == SDR_FMT_INT8X2) { // int8 x 2 complex (Q-inverted)
        for (int j = 0; j < rcv->N * 2; i++, j += 2) {
            rcv->buff[0]->data[i] = SDR_CPX8(LUT8[raw[j]], -LUT8[raw[j+1]]);
        }
    } else if (rcv->fmt == SDR_FMT_CS8) { // int8 x 2 complex
        for (int j = 0; j < rcv->N * 2; i++, j += 2) {
            rcv->buff[0]->data[i] = SDR_CPX8(LUT8[raw[j]], LUT8[raw[j+1]]);
        }
    } else if (rcv->fmt == SDR_FMT_CS16) { // int16 x 2 complex
        for (int j = 0; j < rcv->N * 4; i++, j += 4) {
            int8_t I = LUT8[*(uint16_t *)(raw + j    )];
            int8_t Q = LUT8[*(uint16_t *)(raw + j + 2)];
            rcv->buff[0]->data[i] = SDR_CPX8(I, Q);
        }
    } else if (rcv->fmt == SDR_FMT_RAW8) { // Pocket SDR FE 2CH raw
        for (int j = 0; j < rcv->N; i++, j++) {
            int ph = (ph0 + j) & 3;
            WR_CH(0, raw[j], ph);
            WR_CH(1, raw[j], ph);
        }
        nch_lpf = 2;
    } else if (rcv->fmt == SDR_FMT_RAW16) { // Pocket SDR FE 4CH raw
        for (int j = 0, k = 0; j < rcv->N * 2; i++, j += 2, k++) {
            int ph = (ph0 + k) & 3;
            WR_CH(0, raw[j  ], ph);
            WR_CH(1, raw[j  ], ph);
            WR_CH(2, raw[j+1], ph);
            WR_CH(3, raw[j+1], ph);
        }
        nch_lpf = 4;
    } else if (rcv->fmt == SDR_FMT_RAW16I) { // Spider SDR FE 8CH raw
        for (int j = 0; j < rcv->N * 2; i++, j += 2) {
            rcv->buff[0]->data[i] = LUTC[0][raw[j  ]];
            rcv->buff[1]->data[i] = LUTC[1][raw[j  ]];
            rcv->buff[2]->data[i] = LUTC[2][raw[j  ]];
            rcv->buff[3]->data[i] = LUTC[3][raw[j  ]];
            rcv->buff[4]->data[i] = LUTC[4][raw[j+1]];
            rcv->buff[5]->data[i] = LUTC[5][raw[j+1]];
            rcv->buff[6]->data[i] = LUTC[6][raw[j+1]];
            rcv->buff[7]->data[i] = LUTC[7][raw[j+1]];
        }
    } else if (rcv->fmt == SDR_FMT_RAW32) { // Pocket SDR FE 8CH raw
        for (int j = 0, k = 0; j < rcv->N * 4; i++, j += 4, k++) {
            int ph = (ph0 + k) & 3;
            WR_CH(0, raw[j  ], ph);
            WR_CH(1, raw[j  ], ph);
            WR_CH(2, raw[j+1], ph);
            WR_CH(3, raw[j+1], ph);
            WR_CH(4, raw[j+2], ph);
            WR_CH(5, raw[j+2], ph);
            WR_CH(6, raw[j+3], ph);
            WR_CH(7, raw[j+3], ph);

            // composite antenna array
            for (int m = 0; m < rcv->narch; m++) {
                sdr_cpx_t *LUT1 = (sdr_cpx_t *)rcv->LUT[m+8];
                sdr_cpx_t *LUT2 = (sdr_cpx_t *)rcv->LUT[m+8] + 65536;
                uint16_t raw1 = *((uint16_t *)(raw + j));     // CH1-4
                uint16_t raw2 = *((uint16_t *)(raw + j + 2)); // CH5-8
                int8_t I = (int8_t)floorf(LUT1[raw1][0] + LUT2[raw2][0] + 0.5f);
                int8_t Q = (int8_t)floorf(LUT1[raw1][1] + LUT2[raw2][1] + 0.5f);
                rcv->buff[8+m]->data[i] = SDR_CPX8(I, Q);
            }
        }
        nch_lpf = 8;
    }
    // apply LPF in batch (in-place) for channels that have it enabled
    for (int k = 0; k < nch_lpf; k++) {
        if (rcv->lpf[k]) {
            sdr_lpf_apply(rcv->lpf[k], rcv->buff[k]->data + base, rcv->N);
        }
    }
    set_buff_ix(rcv, ix); // update IF data buffer write pointer
}
#undef WR_CH

// re-acquisition --------------------------------------------------------------
static int re_acq(sdr_rcv_t *rcv, sdr_ch_t *ch)
{
    if (ch->lock * ch->T < MIN_LOCK) return 0;
    if (get_buff_ix(rcv) * SDR_CYC < ch->time + TO_REACQ) {
        ch->acq->fd_ext = (float)ch->fd;
        return 1;
    }
    ch->acq->fd_ext = 0.0; // re-acquisition timeout
    return 0;
}

// assisted acquisition --------------------------------------------------------
static int assist_acq(sdr_rcv_t *rcv, sdr_ch_t *ch)
{
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_t *ch_i = rcv->th[i]->ch;
        if (strcmp(ch->sat, ch_i->sat) || ch_i->state != SDR_STATE_LOCK ||
            ch_i->lock * ch_i->T < MIN_LOCK) continue;
        ch->acq->fd_ext = (float)(ch_i->fd * ch->fc / ch_i->fc);
        return 1;
    }
    return 0;
}

// update IF data rate ---------------------------------------------------------
static uint32_t update_data_rate(sdr_rcv_t *rcv, uint32_t tick, int sum_size)
{
    uint32_t tick_a = sdr_get_tick();
    if (tick_a == tick) return tick;
    rcv->data_rate = (double)sum_size / ((tick_a - tick) * 1e-3);
    return tick_a;
}

// update IF data stats --------------------------------------------------------
static void update_data_stats(sdr_rcv_t *rcv, const uint8_t *raw)
{
    int N = SAMPLES_STATS;
    
    for (int i = 0; i < N; i++) {
        if (rcv->fmt == SDR_FMT_INT8 || rcv->fmt == SDR_FMT_INT8X2 ||
            rcv->fmt == SDR_FMT_CS8) {
            rcv->data_stats[0] += (int8_t)raw[i];
            rcv->data_stats[1] += SQR((int8_t)raw[i]);
        } else if (rcv->fmt == SDR_FMT_CS16) {
            rcv->data_stats[0] += *(int16_t *)(raw + i * 2);
            rcv->data_stats[1] += SQR(*(int16_t *)(raw + i * 2));
        } else {
            return;
        }
    }
    rcv->data_cnt += N;
}

// update scale ----------------------------------------------------------------
static void update_scale(sdr_rcv_t *rcv)
{
    double ave, std, scale = 0.0;
    const char *p;
    
    if ((rcv->fmt != SDR_FMT_INT8  && rcv->fmt != SDR_FMT_INT8X2 &&
        rcv->fmt != SDR_FMT_CS8 && rcv->fmt != SDR_FMT_CS16) ||
        rcv->data_cnt <= 0) {
        return;
    }
    ave = rcv->data_stats[0] / rcv->data_cnt;
    std = sqrt(rcv->data_stats[1] / rcv->data_cnt - SQR(ave));
    rcv->data_stats[0] = rcv->data_stats[1] = 0.0;
    rcv->data_cnt = 0;
    rcv->data_std = std;
    
    if ((p = strstr(rcv->opt, "-SCALE"))) {
        sscanf(p, "-SCALE=%lf", &scale);
    }
    if (scale <= 0.0) { // auto gain control
        sdr_free(rcv->LUT[0]);
        rcv->LUT[0] = (void *)gen_LUT8(rcv->fmt, ave, std / AGC_LEVEL);
    }
}

// update IF data buffer usage rate --------------------------------------------
static void update_buff_use(sdr_rcv_t *rcv)
{
    rcv->buff_use = 0.0;
    int64_t ix = get_buff_ix(rcv);
    for (int i = 0; i < rcv->nch; i++) {
        double use = (ix - rcv->th[i]->ix) * 100.0 / MAX_BUFF;
        if (use > rcv->buff_use) rcv->buff_use = use;
    }
}

// update signal search channel ------------------------------------------------
static void update_srch_ch(sdr_rcv_t *rcv)
{
    if (rcv->buff_use > MAX_BUFF_USE) { // IF data buffer full ?
        return;
    }
    // signal search channel busy ?
    if (rcv->ich >= 0 && rcv->th[rcv->ich]->ch->state == SDR_STATE_SRCH) {
        return;
    }
    for (int i = 0; i < rcv->nch; i++) {
        // search next IDLE channel
        rcv->ich = (rcv->ich + 1) % rcv->nch;
        sdr_ch_t *ch = rcv->th[rcv->ich]->ch;
        if (ch->state != SDR_STATE_IDLE) continue;
        
        // re-acquisition, assisted-acquisition or short code cycle
        if (re_acq(rcv, ch) || assist_acq(rcv, ch) ||
            (ch->T <= sdr_max_acq * 1e-3 && ch->sig_srch)) {
            ch->state = SDR_STATE_SRCH;
            break;
        }
    }
}

// SDR receiver thread ---------------------------------------------------------
static void *rcv_thread(void *arg)
{
    sdr_rcv_t *rcv = (sdr_rcv_t *)arg;
    int ns = sample_byte(rcv->fmt), size, sum_size = 0;
    uint32_t tick = sdr_get_tick(), tick_r = tick;
    uint8_t *raw = (uint8_t *)sdr_malloc(ns * rcv->N);
    
    sdr_log(3, "$LOG,%.3f,%s,%d,START NCH=%d FMT=%d", 0.0, "", 0, rcv->nch,
        rcv->fmt);
    
    if (rcv->dev == SDR_DEV_USB) {
        sdr_dev_start((sdr_dev_t *)rcv->dp);
    } else if (rcv->dev == SDR_DEV_SOAPY) {
        sdr_sdev_start((sdr_sdev_t *)rcv->dp);
    }
    rcv->data_sum = 0.0;
    
    for (int64_t ix = 0; rcv->state; ix++) {
        if (ix % LOG_CYC == 0) {
            update_buff_use(rcv);
            tick_r = update_data_rate(rcv, tick_r, sum_size);
            sum_size = 0;
            out_log_time(ix * SDR_CYC);
        }
        // read raw IF data
        if (!(size = read_data(rcv, raw, ns * rcv->N))) {
            sdr_sleep_msec(500);
            rcv->state = 0;
            continue;
        }
        sum_size += size;
        
        // write IF data buffer
        write_buff(rcv, raw, ix);
        
        // write IF data log stream
        rcv->data_sum += sdr_str_write(rcv->strs[3], raw, size) * 1e-6;
        
        // update signal search channel
        update_srch_ch(rcv);
        
        // update PVT solution
        sdr_pvt_udsol(rcv->pvt, ix);
        
        // update IF data stats and scale
        update_data_stats(rcv, raw);
        if (ix % SCALE_CYC == 0 && ix > 0) {
            update_scale(rcv);
        }
        // sleep if reading file
        if (rcv->dev == SDR_DEV_FILE) {
            sdr_sleep_msec((int)(ix - (sdr_get_tick() - tick) * rcv->tscale));
        }
    }
    if (rcv->dev == SDR_DEV_USB) {
        sdr_dev_stop((sdr_dev_t *)rcv->dp);
    } else if (rcv->dev == SDR_DEV_SOAPY) {
        sdr_sdev_stop((sdr_sdev_t *)rcv->dp);
    }
    sdr_log(3, "$LOG,%.3f,%s,%d,STOP", get_buff_ix(rcv) * SDR_CYC, "", 0);
    sdr_free(raw);
    return NULL;
}

//------------------------------------------------------------------------------
//  Start a SDR receiver.
//
//  args:
//      rcv       (I)  SDR receiver
//      dev       (I)  SDR device type (SDR_DEV_???)
//      dp        (I)  SDR device pointer
//      paths     (I)  output stream paths ("": no output)
//                       paths[0]: NMEA PVT solutions stream
//                       paths[1]: RTCM3 OBS and NAV data stream
//                       paths[2]: log stream
//                       paths[3]: IF data log stream
//
//  returns:
//      Status (1:OK, 0:error)
//
int sdr_rcv_start(sdr_rcv_t *rcv, int dev, void *dp, const char **paths)
{
    char *p;
    double scale = 1.0;
    sdr_cpx_t weight[8] = {{0}};
    int level = TRACE_LEVEL, IQ[8] = {0}, bits[8] = {0};
    
    if (!rcv || rcv->state) return 0;
    
    if ((p = strstr(rcv->opt, "-SCALE"))) {
        sscanf(p, "-SCALE=%lf", &scale);
    }
    if ((p = strstr(rcv->opt, "-TRACE"))) {
        sscanf(p, "-TRACE=%d", &level);
        traceopen(TRACE_FILE);
        tracelevel(level);
    }
    sdr_log_open(paths[2]);
    
    for (int i = 0; i < rcv->nrfch; i++) {
        IQ[i] = rcv->rfch[i].IQ;
        bits[i] = rcv->rfch[i].bits;
    }
    for (int i = 0; i < 3; i++) {
        weight[i][0] = (float)(cos(i * 60.0 * D2R) / sqrt(3.0));
        weight[i][1] = (float)(sin(i * 60.0 * D2R) / sqrt(3.0));
    }
    // initialize raw to IF data LUT
    if (rcv->fmt == SDR_FMT_INT8 || rcv->fmt == SDR_FMT_INT8X2 ||
        rcv->fmt == SDR_FMT_CS8  || rcv->fmt == SDR_FMT_CS16) {
        rcv->LUT[0] = (void *)gen_LUT8(rcv->fmt, 0.0, scale);
    } else {
        for (int i = 0; i < rcv->nrfch; i++) {
            rcv->LUT[i] = rcv->rtoc[i] ?
                (void *)gen_LUTC_rtoc(i) :
                (void *)gen_LUTC(rcv->fmt, i, IQ[i], bits[i]);
        }
    }
    for (int i = 0; i < rcv->narch; i++) {
        rcv->LUT[i+8] = (void *)gen_LUTS(rcv->fmt, IQ, bits, weight);
    }
    for (int i = 0; i < rcv->nch; i++) {
        ch_th_start(rcv->th[i]);
    }
    rcv->dev = dev;
    rcv->dp = dp;
    rcv->pvt = sdr_pvt_new(rcv);
    for (int i = 0; i < 4; i++) {
        if (i == 3 && (rcv->dev != SDR_DEV_USB && rcv->dev != SDR_DEV_SOAPY)) {
            continue;
        }
        if (i != 2 && *paths[i] && !(rcv->strs[i] = sdr_str_open(paths[i]))) {
            fprintf(stderr, "stream open error: %s", paths[i]);
        }
    }
    rcv->start_time = utc2gpst(timeget());
    rcv->state = 1;
    if (!sdr_thread_create(&rcv->thread, rcv_thread, rcv)) {
        rcv->state = 0;
        for (int i = 0; i < rcv->nch; i++) ch_th_stop(rcv->th[i]);
        for (int i = 0; i < rcv->nch; i++) sdr_thread_join(rcv->th[i]->thread);
        return 0;
    }
    return 1;
}

// get file path ---------------------------------------------------------------
static int get_file_path(stream_t *str, char *file, int size)
{
    char buff[4096], *p = buff, *q;
    
    strstatx(str, buff);
    if (!(p = strstr(p, "openpath= ")) || !(q = strstr(p + 10, "\n"))) {
        return 0;
    }
    *q = '\0';
    return snprintf(file, size, "%s", p + 10) > 0;
}

//------------------------------------------------------------------------------
//  Stop a SDR receiver.
//
//  args:
//      rcv       (I)  SDR receiver
//
//  returns:
//      none
//
void sdr_rcv_stop(sdr_rcv_t *rcv)
{
    double fo[SDR_MAX_RFCH];
    int IQ[SDR_MAX_RFCH], bits[SDR_MAX_RFCH];
    char file[1024];
    
    for (int i = 0; i < rcv->nch; i++) {
        ch_th_stop(rcv->th[i]);
    }
    for (int i = 0; i < rcv->nch; i++) {
        sdr_thread_join(rcv->th[i]->thread);
    }
    rcv->state = 0;
    sdr_thread_join(rcv->thread);
    
    // write tag file for raw IF data
    if ((rcv->dev == SDR_DEV_USB || rcv->dev == SDR_DEV_SOAPY) &&
        rcv->strs[3] && rcv->strs[3]->type == STR_FILE &&
        get_file_path(rcv->strs[3], file, sizeof(file))) {
        for (int i = 0; i < SDR_MAX_RFCH; i++) {
            fo[i] = rcv->rfch[i].fo;
            IQ[i] = rcv->rfch[i].IQ;
            bits[i] = rcv->rfch[i].bits;
        }
        sdr_tag_write(file, SDR_DEV_NAME, rcv->start_time, rcv->fmt, rcv->fs,
            fo, IQ, bits);
    }
    for (int i = 0; i < 4; i++) {
        sdr_str_close(rcv->strs[i]);
    }
    for (int i = 0; i < SDR_MAX_RFCH; i++) {
        sdr_free(rcv->LUT[i]);
        rcv->LUT[i] = NULL;
    }
    sdr_pvt_free(rcv->pvt);
    sdr_log_close();
    if (strstr(rcv->opt, "-TRACE")) {
        traceclose();
    }
}

// get and set LNA gain of RF frontend -----------------------------------------
int sdr_rcv_get_gain(sdr_rcv_t *rcv, int ch)
{
    if (!rcv || !rcv->state || rcv->dev != SDR_DEV_USB) return -1;
    return sdr_dev_get_gain((sdr_dev_t *)rcv->dp, ch);
}

int sdr_rcv_set_gain(sdr_rcv_t *rcv, int ch, int gain)
{
    if (!rcv || !rcv->state || rcv->dev != SDR_DEV_USB) return 0;
    return sdr_dev_set_gain((sdr_dev_t *)rcv->dp, ch, gain);
}

// get and set IF filter of RF frontend ----------------------------------------
int sdr_rcv_get_filt(sdr_rcv_t *rcv, int ch, double *bw, double *freq,
    int *order)
{
    if (!rcv || !rcv->state || rcv->dev != SDR_DEV_USB) return 0;
    return sdr_dev_get_filt((sdr_dev_t *)rcv->dp, ch, bw, freq, order);
}

int sdr_rcv_set_filt(sdr_rcv_t *rcv, int ch, double bw, double freq, int order)
{
    if (!rcv || !rcv->state || rcv->dev != SDR_DEV_USB) return 0;
    return sdr_dev_set_filt((sdr_dev_t *)rcv->dp, ch, bw, freq, order);
}

//------------------------------------------------------------------------------
//  Generate a new SDR receiver by Pocket SDR FE device and start receiver.
//
//  args:
//      sigs      (I)  signal types as a string array {sig1, sig2, ..., sign}
//      prns      (I)  PRN numbers as int array {prn1, prn2, ..., prnn}
//      n         (I)  number of sigs and prns
//      bus       (I)  USB bus number of SDR device (-1:any)
//      port      (I)  USB port number of SDR device (-1:any)
//      conf_file (I)  configuration file for SDR device ("": no config)
//      paths     (I)  output stream paths as same as sdr_rcv_start()
//      opt       (I)  receiver options (string sparated by spaces)
//                     -RFCH <sig>:<ch>[{,|-}<ch>...]
//                        assign signal to specific RF CH(s)
//                     -TRACE<=level>
//                        enable debug trace level <level>
//
//  returns:
//      SDR receiver (NULL: error)
//
sdr_rcv_t *sdr_rcv_open_dev(const char **sigs, int *prns, int n, int bus,
    int port, const char *conf_file, const char **paths, const char *opt)
{
    sdr_dev_t *dev;
    double fs, fo[SDR_MAX_RFCH] = {0};
    int fmt, nch, IQ[SDR_MAX_RFCH] = {0}, bits[SDR_MAX_RFCH] = {0};
    
    if (!(dev = sdr_dev_open(bus, port))) {
        return NULL;
    }
    if (*conf_file) {
        if (!sdr_conf_write(dev, conf_file, 0)) {
            sdr_dev_close(dev);
            return NULL;
        }
        sdr_sleep_msec(50);
    }
    if (!(nch = sdr_dev_get_info(dev, &fmt, &fs, fo, IQ, bits))) {
        sdr_dev_close(dev);
        return NULL;
    }
    sdr_rcv_t *rcv = sdr_rcv_new(sigs, prns, n, fmt, fs, fo, IQ, bits, opt);
    if (!sdr_rcv_start(rcv, SDR_DEV_USB, (void *)dev, paths)) {
        sdr_dev_close(dev);
        sdr_rcv_free(rcv);
        return NULL;
    }
    return rcv;
}

//------------------------------------------------------------------------------
//  Generate a new SDR receiver by Soapy device and start receiver.
//
//  args:
//      sigs      (I)  signal types as a string array {sig1, sig2, ..., sign}
//      prns      (I)  PRN numbers as int array {prn1, prn2, ..., prnn}
//      n         (I)  number of sigs and prns
//      driver    (I)  SoapySDR driver
//      fmt       (I)  Sampling data format (CS8 or CS16)
//      rate      (I)  Sampling rate (sps)
//      freq      (I)  Carrier center frequency (Hz)
//      paths     (I)  output stream paths as same as sdr_rcv_start()
//      opt       (I)  receiver options (string sparated by spaces)
//                     -RFCH <sig>:<ch>[{,|-}<ch>...]
//                        assign signal to specific RF CH(s)
//                     -SCALE=<scale>
//                        scale of input bits
//                     -TRACE<=level>
//                        enable debug trace level <level>
//
//  returns:
//      SDR receiver (NULL: error)
//
sdr_rcv_t *sdr_rcv_open_sdev(const char **sigs, int *prns, int n,
    const char *driver, int fmt, double rate, double freq, const char **paths,
    const char *opt)
{
    sdr_sdev_t *sdev;
    const char *p;
    double fs, fo[SDR_MAX_RFCH] = {0}, bw = 0.0, gain = 0.0;
    int IQ[SDR_MAX_RFCH] = {0}, bits[SDR_MAX_RFCH] = {0};
    
    if ((p = strstr(opt, "-GAIN="))) sscanf(p, "-GAIN=%lf", &gain);
    if ((p = strstr(opt, "-BW="))) sscanf(p, "-BW=%lf", &bw);
    if (!(sdev = sdr_sdev_open(driver, fmt, rate, freq, bw * 1e6, gain))) {
        return NULL;
    }
    fs = rate;
    fo[0] = freq;
    IQ[0] = 2;
    bits[0] = 16;
    sdr_rcv_t *rcv = sdr_rcv_new(sigs, prns, n, fmt, fs, fo, IQ, bits, opt);
    if (!sdr_rcv_start(rcv, SDR_DEV_SOAPY, (void *)sdev, paths)) {
        sdr_sdev_close(sdev);
        sdr_rcv_free(rcv);
        return NULL;
    }
    return rcv;
}

// open local file and start receiver ------------------------------------------
static sdr_rcv_t *rcv_open_file(const char **sigs, int *prns, int n, int fmt,
    double fs, const double *fo, const int *IQ, const int *bits, double toff,
    double tscale, const char *file, const char **paths, const char *opt)
{
    FILE *fp;
    double fo_t[SDR_MAX_RFCH] = {0};
    int IQ_t[SDR_MAX_RFCH] = {0}, bits_t[SDR_MAX_RFCH];
    
    if (!(fp = fopen(file, "rb"))) {
        fprintf(stderr, "file open error: %s\n", file);
        return NULL;
    }
    // read tag file
    memcpy(fo_t, fo, sizeof(double) * SDR_MAX_RFCH);
    memcpy(IQ_t, IQ, sizeof(int) * SDR_MAX_RFCH);
    memcpy(bits_t, bits, sizeof(int) * SDR_MAX_RFCH);
    sdr_tag_read(file, NULL, NULL, &fmt, &fs, fo_t, IQ_t, bits_t);
    int64_t off = (int64_t)(fs * toff * sample_byte(fmt));
#if defined(WIN32)
    _fseeki64(fp, off, SEEK_SET);
#else
    fseeko(fp, (off_t)off, SEEK_SET);
#endif
    sdr_rcv_t *rcv = sdr_rcv_new(sigs, prns, n, fmt, fs, fo_t, IQ_t, bits_t,
        opt);
    rcv->tscale = tscale;
    if (!sdr_rcv_start(rcv, SDR_DEV_FILE, (void *)fp, paths)) {
        fclose(fp);
        sdr_rcv_free(rcv);
        return NULL;
    }
    return rcv;
}

// open stream and start receiver ----------------------------------------------
static sdr_rcv_t *rcv_open_str(const char **sigs, int *prns, int n, int fmt,
    double fs, const double *fo, const int *IQ, const int *bits,
    const char *path, const char **paths, const char *opt)
{
    stream_t *str = strnew();
    
    if (!stropen(str, STR_TCPCLI, STR_MODE_R, path)) {
        fprintf(stderr, "stream open error %s\n", path);
        strfree(str);
        return NULL;
    }
    sdr_rcv_t *rcv = sdr_rcv_new(sigs, prns, n, fmt, fs, fo, IQ, bits, opt);
    if (!sdr_rcv_start(rcv, SDR_DEV_STR, (void *)str, paths)) {
        strclose(str);
        strfree(str);
        sdr_rcv_free(rcv);
        return NULL;
    }
    return rcv;
}

//------------------------------------------------------------------------------
//  Generate a new SDR receiver by IF data path and start receiver.
//
//  args:
//      sigs      (I)  signal types as a string array {sig1, sig2, ..., sign}
//      prns      (I)  PRN numbers as int array {prn1, prn2, ..., prnn}
//      n         (I)  number of sigs and prns
//      fmt       (I)  IF data format as same as sdr_rcv_new()
//      fs        (I)  sampling rate (sps)
//      fo        (I)  LO frequency for each RFCH (Hz)
//      IQ        (I)  sampling type for each RFCH (1:I, 2:IQ)
//      bits      (I)  number of sample bits
//      toff      (I)  time offset of IF data file (s)
//      tscale    (I)  time scale of replay IF data file
//      path      (I)  input IF data path (local file or addr:port)
//      paths     (I)  output stream paths as same as sdr_rcv_start()
//      opt       (I)  receiver options (same as sdr_rcv_open_dev())
//
//  returns:
//      SDR receiver (NULL: error)
//
//  notes:
//      If the tag file exists, fmt, fs, fo, IQ are obtained from the tag file.
//
sdr_rcv_t *sdr_rcv_open_file(const char **sigs, int *prns, int n, int fmt,
    double fs, const double *fo, const int *IQ, const int *bits, double toff,
    double tscale, const char *path, const char **paths, const char *opt)
{
    const char *p;
    int port;
    
    if ((p = strrchr(path, ':')) && sscanf(p, ":%d", &port)) { // stream
        return rcv_open_str(sigs, prns, n, fmt, fs, fo, IQ, bits, path, paths,
            opt);
    } else { // local file
        return rcv_open_file(sigs, prns, n, fmt, fs, fo, IQ, bits, toff, tscale,
            path, paths, opt);
    }
}

//------------------------------------------------------------------------------
//  Stop and free SDR receiver opened by sdr_rcv_open_dev() or
//  sdr_rcv_open_file().
//
//  args:
//      rcv       (I)  SDR receiver
//
//  returns:
//      none
//
void sdr_rcv_close(sdr_rcv_t *rcv)
{
    if (!rcv) return;
    
    sdr_rcv_stop(rcv);
    
    if (rcv->dev == SDR_DEV_USB) {
        sdr_dev_close((sdr_dev_t *)rcv->dp);
    } else if (rcv->dev == SDR_DEV_SOAPY) {
        sdr_sdev_close((sdr_sdev_t *)rcv->dp);
    } else if (rcv->dev == SDR_DEV_FILE) {
        fclose((FILE *)rcv->dp);
    } else { // SDR_DEV_STR
        strclose((stream_t *)rcv->dp);
        strfree((stream_t *)rcv->dp);
    }
    sdr_rcv_free(rcv);
}

//------------------------------------------------------------------------------
//  Set SDR receiver options.
//
//  args:
//      opt       (I)  option string
//      value     (I)  value
//
//  returns:
//      none
//
void sdr_rcv_setopt(const char *opt, double value)
{
    extern double sdr_epoch, sdr_lag_epoch, sdr_el_mask, sdr_sp_corr, sdr_t_acq;
    extern double sdr_t_dll, sdr_b_dll, sdr_b_pll, sdr_b_fll_w, sdr_b_fll_n;
    extern double sdr_max_dop, sdr_thres_cn0_l, sdr_thres_cn0_u;
    extern int sdr_bump_jump;
    if      (!strcmp(opt, "epoch"      )) sdr_epoch       = value;
    else if (!strcmp(opt, "lag_epoch"  )) sdr_lag_epoch   = value;
    else if (!strcmp(opt, "el_mask"    )) sdr_el_mask     = value;
    else if (!strcmp(opt, "sp_corr"    )) sdr_sp_corr     = value;
    else if (!strcmp(opt, "t_acq"      )) sdr_t_acq       = value;
    else if (!strcmp(opt, "t_dll"      )) sdr_t_dll       = value;
    else if (!strcmp(opt, "b_dll"      )) sdr_b_dll       = value;
    else if (!strcmp(opt, "b_pll"      )) sdr_b_pll       = value;
    else if (!strcmp(opt, "b_fll_w"    )) sdr_b_fll_w     = value;
    else if (!strcmp(opt, "b_fll_n"    )) sdr_b_fll_n     = value;
    else if (!strcmp(opt, "max_dop"    )) sdr_max_dop     = value;
    else if (!strcmp(opt, "thres_cn0_l")) sdr_thres_cn0_l = value;
    else if (!strcmp(opt, "thres_cn0_u")) sdr_thres_cn0_u = value;
    else if (!strcmp(opt, "bump_jump"  )) sdr_bump_jump = (int)value;
    else if (!strcmp(opt, "max_acq"    )) sdr_max_acq     = value;
    else fprintf(stderr, "sdr_rcv_setopt error opt=%s\n", opt);
}
