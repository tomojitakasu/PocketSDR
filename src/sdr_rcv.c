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
//
#include "pocket_sdr.h"

// constants and macros ---------------------------------------------------------
#define MAX_BUFF   8000         // max number of IF data buffer (* SDR_CYC)
#define LOG_CYC    1000         // receiver channel log cycle (* SDR_CYC)
#define TH_CYC     10           // receiver channel thread cycle (ms)
#define TO_REACQ   60.0         // re-acquisition timeout (s)
#define MIN_LOCK   2.0          // min lock time to show channel status (s)
#define NUM_COL    110          // number of channel status columns
#define MAX_ACQ    4e-3         // max code length w/o acqusition assist (s)
#define MAX_BUFF_USE 90         // max buffer usage rate (%)

#define MIN(x, y)  ((x) < (y) ? (x) : (y))

// global variables ------------------------------------------------------------
static char rcv_rcv_stat_buff[2048];
static char rcv_ch_stat_buff[120 * (SDR_MAX_NCH + 2)];
static char rcv_sat_stat_buff[1024];

// get IF data buffer pointer --------------------------------------------------
static int64_t get_buff_ix(sdr_rcv_t *rcv)
{
    pthread_mutex_lock(&rcv->mtx);
    int64_t ix = rcv->ix;
    pthread_mutex_unlock(&rcv->mtx);
    return ix;
}

// set IF data buffer pointer --------------------------------------------------
static void set_buff_ix(sdr_rcv_t *rcv, int64_t ix)
{
    pthread_mutex_lock(&rcv->mtx);
    rcv->ix = ix;
    pthread_mutex_unlock(&rcv->mtx);
}

// C/N0 bar --------------------------------------------------------------------
static void cn0_bar(float cn0, char *bar)
{
    int n = (int)((cn0 - 30.0) / 1.5);
    strcpy(bar, "|");
    for (int i = 0; i < n && i < 13; i++) {
        sprintf(bar + i, "|");
    }
}

// SDR receiver channel sync status --------------------------------------------
static void sync_stat(sdr_ch_t *ch, char *stat)
{
    sprintf(stat, "%s%s%s%s", (ch->trk->sec_sync > 0) ? "S" : "-",
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
static int print_head(char *buff, sdr_rcv_t *rcv)
{
    char *p = buff, solstr[128] = "", sys[16] = "";
    int nch_bb = 0, nch_trk = 0, ch_srch = 0;
    double buff_use = 0.0;
    
    if (rcv) {
        nch_bb = rcv->nch;
        nch_trk = get_nch_trk(rcv, sys);
        ch_srch = rcv->ich + 1;
        buff_use = rcv->buff_use;
        sdr_pvt_solstr(rcv->pvt, solstr);
    }
    p += sprintf(p, " %-*s BUFF:%3.0f%% SRCH:%3d LOCK:%3d/%3d\n", NUM_COL - 38,
        solstr, buff_use, ch_srch, nch_trk, nch_bb);
    p += sprintf(p, "%3s %2s %4s %5s %3s %8s %4s %-12s %11s %7s %11s %4s %5s "
        "%4s %4s %3s\n", "CH", "RF", "SAT", "SIG", "PRN", "LOCK(s)", "C/N0",
        "(dB-Hz)", "COFF(ms)", "DOP(Hz)", "ADR(cyc)", "SYNC", "#NAV", "#ERR",
        "#LOL", "FEC");
    return (int)(p - buff);
}

// print SDR receiver channel status -------------------------------------------
static int print_ch_stat(char *buff, sdr_ch_t *ch)
{
    char *p = buff, bar[16], stat[16];
    cn0_bar(ch->cn0, bar);
    sync_stat(ch, stat);
    p += sprintf(p, "%3d %2d %4s %5s %3d %8.2f %4.1f %-13s%11.7f %7.1f %11.1f"
        " %s %5d %4d %4d %3d\n", ch->no, ch->rf_ch + 1, ch->sat, ch->sig,
        ch->prn, ch->lock * ch->T, ch->cn0, bar, ch->coff * 1e3, ch->fd,
        ch->adr, stat, ch->nav->count[0], ch->nav->count[1], ch->lost,
        ch->nav->nerr);
    return (int)(p - buff);
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
//      sys       (I)  system
//      all       (I)  all channel including IDLE channel
//
//  returns:
//      channel status string
//
char *sdr_rcv_ch_stat(sdr_rcv_t *rcv, const char *sys, int all)
{
    char *p = rcv_ch_stat_buff;
    
    p += print_head(p, rcv);
    for (int i = 0; rcv && i < rcv->nch; i++) {
        sdr_ch_t *ch = rcv->th[i]->ch;
        if (!sat_select(ch->sat, sys)) continue;
        if (all || (ch->state == SDR_STATE_LOCK &&
            ch->lock * ch->T >= MIN_LOCK)) {
            p += print_ch_stat(p, ch);
        }
    }
    return rcv_ch_stat_buff;
}

// get receiver status as sting ------------------------------------------------
char *sdr_rcv_rcv_stat(sdr_rcv_t *rcv)
{
    static const char *src_str[] = {"---", "IF Data", "RF Frontend"};
    static const char *fmt_str[] = {"---", "INT8", "INT8X2", "RAW8", "RAW16"};
    static const char *IQ_str[] = {"---", "I", "IQ"};
    char *p = rcv_rcv_stat_buff;
    
    if (rcv && rcv->state) {
        char solstr[128] = "", sys[16] = "";
        int nch_trk = get_nch_trk(rcv, sys);
        sdr_pvt_solstr(rcv->pvt, solstr);
        p += sprintf(p, "%.3f,%s,%s,%d,%.3f/%.3f,%.3f/%.3f,%s/%s/%s/%s,%.3f,"
            "%d/%d,%.3f,%.1f,", get_buff_ix(rcv) * SDR_CYC, src_str[rcv->dev],
            fmt_str[rcv->fmt], rcv->nbuff, rcv->fo[0] * 1e-6, rcv->fo[1] * 1e-6,
            rcv->fo[2] * 1e-6, rcv->fo[3] * 1e-6, IQ_str[rcv->IQ[0]],
            IQ_str[rcv->IQ[1]], IQ_str[rcv->IQ[2]], IQ_str[rcv->IQ[3]],
            rcv->fs * 1e-6, nch_trk, rcv->nch, rcv->data_rate * 1e-6,
            rcv->buff_use);
        p += sprintf(p, "%.21s,%.3s,%.11s,%.12s,%.8s,%s,%.5s,,%d,%d/%d,%.1f,",
            solstr, solstr + 64, solstr + 24, solstr + 36, solstr + 49, sys,
            solstr + 58, rcv->pvt->count[0], rcv->pvt->count[1],
            rcv->pvt->count[2], rcv->data_sum);
       }
    else {
        p += sprintf(p, "%.3f,---,---,%d,%.3f/%.3f,%.3f/%.3f,---/---/---/---,"
            "%.3f,%d/%d,%.3f,%.1f,", 0.0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0.0,
            0.0);
        p += sprintf(p, "1970-01-01 00:00:00.0,---,%.7f,%.7f,%.2f,,%d/%d,,%d,"
            "%d/%d,%.1f,", 0.0, 0.0, 0.0, 0, 0, 0, 0, 0, 0.0);
    }
    return rcv_rcv_stat_buff;
}

// get satellite status as string ----------------------------------------------
char *sdr_rcv_sat_stat(sdr_rcv_t *rcv, const char *sys)
{
    char *p = rcv_sat_stat_buff;
    
    *p = '\0';
    if (rcv) {
        pthread_mutex_lock(&rcv->pvt->mtx);
        
        for (int i = 0; i < MAXSAT; i++) {
            char sat[16];
            satno2id(i+1, sat);
            if (!sat_select(sat, sys)) continue;
            ssat_t *ssat = rcv->pvt->ssat + i;
            if (ssat->azel[1] <= 0.0) continue;
            p += sprintf(p, "%s %.1f %.1f %d\n", sat, ssat->azel[0] * R2D,
                ssat->azel[1] * R2D, ssat->vs);
        }
        pthread_mutex_unlock(&rcv->pvt->mtx);
    }
    return rcv_sat_stat_buff;
}

// select channel for correlator status ----------------------------------------
void sdr_rcv_sel_ch(sdr_rcv_t *rcv, int ch)
{
    if (!rcv || !rcv->state) return;
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_set_corr(rcv->th[i]->ch, i + 1 == ch ? SDR_N_CORR : 4);
    }
}

// get correlator status -------------------------------------------------------
int sdr_rcv_corr_stat(sdr_rcv_t *rcv, int ch, double *stat, int *pos,
    sdr_cpx_t *C)
{
    if (!rcv || !rcv->state || ch < 1 || ch > rcv->nch) return 0;
    return sdr_ch_corr_stat(rcv->th[ch-1]->ch, stat, pos, C);
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
    if (!rcv || !rcv->state || ch < 1 || ch > rcv->nbuff) return 0;
    stat[0] = rcv->dev;
    stat[1] = rcv->fmt;
    stat[2] = rcv->fs;
    stat[3] = rcv->fo[ch-1];
    stat[4] = rcv->IQ[ch-1];
    return 1;
}

// get RF channel PSD ----------------------------------------------------------
int sdr_rcv_rfch_psd(sdr_rcv_t *rcv, int ch, double tave, int N, float *psd)
{
    if (!rcv || !rcv->state || ch < 1 || ch > rcv->nbuff) return 0;
    int n = (int)(rcv->fs * tave);
    int64_t ix = get_buff_ix(rcv);
    if (ix * rcv->N < n) return 0;
    sdr_cpx_t *buff = sdr_cpx_malloc(n);
    for (int i = 0; i < n; i++) {
        int j = (int)((ix * rcv->N - n + i) % rcv->buff[ch-1]->N);
        sdr_cpx8_t data = rcv->buff[ch-1]->data[j];
        buff[i][0] = SDR_CPX8_I(data);
        buff[i][1] = SDR_CPX8_Q(data);
    }
    sdr_psd_cpx(buff, n, N, rcv->fs, rcv->buff[ch-1]->IQ, psd);
    sdr_cpx_free(buff);
    return rcv->buff[ch-1]->IQ == 1 ? N / 2 : N; // PSD size
}

// get RF channel histgram -----------------------------------------------------
int sdr_rcv_rfch_hist(sdr_rcv_t *rcv, int ch, double tave, int *val,
    double *hist1, double *hist2)
{
    if (!rcv || !rcv->state || ch < 1 || ch > rcv->nbuff) return 0;
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
        if (rcv->IQ[ch-1] == 2) {
            hist2[nval] = sum[1] > 0 ? (double)cnt[1][i] / sum[1] : 0.0;
        }
        val[nval++] = i - 128;
    }
    return nval;
}

// output log $TIME ------------------------------------------------------------
static void out_log_time(double time)
{
    double t[6] = {0};
    sdr_get_time(t);
    sdr_log(3, "$TIME,%.3f,%.0f,%.0f,%.0f,%.0f,%.0f,%.6f,UTC", time, t[0], t[1],
       t[2], t[3], t[4], t[5]);
}

// output log $CH --------------------------------------------------------------
static void out_log_ch(sdr_ch_t *ch)
{
    sdr_log(4, "$CH,%.3f,%s,%d,%d,%.1f,%.9f,%.3f,%.3f,%d,%d", ch->time, ch->sig,
        ch->prn, ch->lock, ch->cn0, ch->coff * 1e3, ch->fd, ch->adr,
        ch->nav->count[0], ch->nav->count[1]);
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
            
            // output channel log
            if (ch->state == SDR_STATE_LOCK && th->ix % LOG_CYC == 0) {
                out_log_ch(ch);
            }
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
    return !pthread_create(&th->thread, NULL, ch_thread, th);
}

// stop SDR receiver channel ---------------------------------------------------
static void ch_th_stop(sdr_ch_th_t *th)
{
    th->state = 0;
}

// set RF channel and IF frequency ---------------------------------------------
static int set_rfch(int fmt, double fs, const double *fo, const int *IQ,
    const char *sig, double *fi)
{
    double freq = sdr_sig_freq(sig);
    int rfch = 0;
    
    if (fmt == SDR_FMT_RAW8) { // FE 2CH
        rfch = freq > 1.4e9 ? 0 : 1;
    }
    else if (fmt == SDR_FMT_RAW16) { // FE 4CH
        for (int i = 1; i < 4; i++) {
            if (fabs(freq - fo[i]) < fabs(freq - fo[rfch])) rfch = i;
        }
    }
    *fi = fo[rfch] > 0.0 ? freq - fo[rfch] : (IQ[rfch] == 1 ? fs * 0.5 : 0.0);
    return rfch;
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
//
//  returns:
//      SDR receiver (NULL: error)
//
sdr_rcv_t *sdr_rcv_new(const char **sigs, const int *prns, int n, int fmt,
    double fs, const double *fo, const int *IQ)
{
    sdr_rcv_t *rcv = (sdr_rcv_t *)sdr_malloc(sizeof(sdr_rcv_t));
    
    rcv->fmt = fmt;
    rcv->fs = fs;
    for (int i = 0; i < SDR_MAX_RFCH; i++) {
        rcv->fo[i] = fo[i];
        rcv->IQ[i] = IQ[i];
    }
    rcv->N = (int)(SDR_CYC * fs);
    for (int i = 0; i < n && rcv->nch < SDR_MAX_NCH; i++) {
        double fi = 0.0;
        int rfch = set_rfch(fmt, fs, fo, IQ, sigs[i], &fi);
        sdr_ch_th_t *th = ch_th_new(sigs[i], prns[i], fi, fs, rcv);
        if (th) {
            th->ch->no = rcv->nch + 1;
            th->ch->rf_ch = rfch;
            rcv->th[rcv->nch++] = th;
        }
        else {
            fprintf(stderr, "signal / prn error: %s / %d\n", sigs[i], prns[i]);
        }
    }
    rcv->nbuff = fmt == SDR_FMT_RAW16 ? 4 : (fmt == SDR_FMT_RAW8 ? 2 : 1);
    for (int i = 0; i < rcv->nbuff; i++) {
        rcv->buff[i] = sdr_buff_new(rcv->N * MAX_BUFF, rcv->IQ[i]);
    }
    rcv->ich = -1;
    pthread_mutex_init(&rcv->mtx, NULL);
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
    for (int i = 0; i < rcv->nbuff; i++) {
        sdr_buff_free(rcv->buff[i]);
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
    }
    else { // USB device
        while (!sdr_dev_read((sdr_dev_t *)rcv->dp, raw, N)) {
            if (!rcv->state) return 0;
            sdr_sleep_msec(1);
        }
    }
    return N;
}

// generate lookup table -------------------------------------------------------
static void gen_LUT(sdr_buff_t **buff, int nbuff, sdr_cpx8_t LUT[][256])
{
    static const int8_t valI[] = {1, 3, -1, -3}, valQ[] = {-1, -3, 1, 3};
    
    for (int i = 0; i < 256; i++) {
        int8_t I[] = {valI[(i>>0) & 0x3], valI[(i>>4) & 0x3]};
        int8_t Q[] = {valQ[(i>>2) & 0x3], valQ[(i>>6) & 0x3]};
        for (int j = 0; j < nbuff; j++) {
            LUT[j][i] = SDR_CPX8(I[j%2], buff[j]->IQ == 1 ? 0 : Q[j%2]);
        }
    }
}

// write IF data buffer ---------------------------------------------------------
static void write_buff(sdr_rcv_t *rcv, const uint8_t *raw, int64_t ix)
{
    static sdr_cpx8_t LUT[4][256] = {{0}};
    int i = rcv->N * (int)(ix % MAX_BUFF);
    
    if (!LUT[0][0] && (rcv->fmt == SDR_FMT_RAW8 || rcv->fmt == SDR_FMT_RAW16)) {
        gen_LUT(rcv->buff, rcv->nbuff, LUT);
    }
    if (rcv->fmt == SDR_FMT_INT8) { // int8
        for (int j = 0; j < rcv->N; i++, j++) {
            rcv->buff[0]->data[i] = SDR_CPX8(raw[j], 0);
        }
    }
    else if (rcv->fmt == SDR_FMT_INT8X2) { // int8 x 2 complex
        for (int j = 0; j < rcv->N * 2; i++, j += 2) {
            rcv->buff[0]->data[i] = SDR_CPX8(raw[j], -raw[j+1]);
        }
    }
    else if (rcv->fmt == SDR_FMT_RAW8) { // packed 8 bit raw (2CH)
        for (int j = 0; j < rcv->N; i++, j++) {
            rcv->buff[0]->data[i] = LUT[0][raw[j]];
            rcv->buff[1]->data[i] = LUT[1][raw[j]];
        }
    }
    else if (rcv->fmt == SDR_FMT_RAW16) { // packed 16 bit raw (4CH)
        for (int j = 0; j < rcv->N * 2; i++, j += 2) {
            rcv->buff[0]->data[i] = LUT[0][raw[j  ]];
            rcv->buff[1]->data[i] = LUT[1][raw[j  ]];
            rcv->buff[2]->data[i] = LUT[2][raw[j+1]];
            rcv->buff[3]->data[i] = LUT[3][raw[j+1]];
        }
    }
    set_buff_ix(rcv, ix); // update IF data buffer write pointer
}

// re-acquisition --------------------------------------------------------------
static int re_acq(sdr_rcv_t *rcv, sdr_ch_t *ch)
{
    if (ch->lock * ch->T >= MIN_LOCK &&
        get_buff_ix(rcv) * SDR_CYC < ch->time + TO_REACQ) {
        ch->acq->fd_ext = ch->fd;
        return 1;
    }
    return 0;
}

// assisted acquisition --------------------------------------------------------
static int assist_acq(sdr_rcv_t *rcv, sdr_ch_t *ch)
{
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_t *ch_i = rcv->th[i]->ch;
        if (strcmp(ch->sat, ch_i->sat) || ch_i->state != SDR_STATE_LOCK ||
            ch_i->lock * ch_i->T < MIN_LOCK) continue;
        ch->acq->fd_ext = ch_i->fd * ch->fc / ch_i->fc;
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
        if (re_acq(rcv, ch) || assist_acq(rcv, ch) || ch->T <= MAX_ACQ) {
            ch->state = SDR_STATE_SRCH;
            break;
        }
    }
}

// SDR receiver thread ---------------------------------------------------------
static void *rcv_thread(void *arg)
{
    sdr_rcv_t *rcv = (sdr_rcv_t *)arg;
    int ns = (rcv->fmt == SDR_FMT_INT8 || rcv->fmt == SDR_FMT_RAW8) ? 1 : 2;
    int size, sum_size = 0;
    uint8_t *raw = (uint8_t *)sdr_malloc(ns * rcv->N);
    uint32_t tick = sdr_get_tick(), tick_r = tick;
    
    sdr_log(3, "$LOG,%.3f,%s,%d,START NCH=%d FMT=%d", 0.0, "", 0, rcv->nch,
        rcv->fmt);
    
    if (rcv->dev == SDR_DEV_USB) {
        sdr_dev_start((sdr_dev_t *)rcv->dp);
    }
    rcv->data_sum = 0.0;
    
    for (int64_t ix = 0; rcv->state; ix++) {
        if (ix % LOG_CYC == 0) {
            update_buff_use(rcv);
            tick_r = update_data_rate(rcv, tick_r, sum_size);
            sum_size = 0;
            out_log_time(ix * SDR_CYC);
        }
        // read IF data
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
        
        // sleep if reading file
        if (rcv->dev == SDR_DEV_FILE) {
            sdr_sleep_msec((int)(ix - (sdr_get_tick() - tick) * rcv->tscale));
        }
    }
    if (rcv->dev == SDR_DEV_USB) {
        sdr_dev_stop((sdr_dev_t *)rcv->dp);
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
    if (rcv->state) return 0;
    
    sdr_log_open(paths[2]);
    
    for (int i = 0; i < rcv->nch; i++) {
        ch_th_start(rcv->th[i]);
    }
    rcv->dev = dev;
    rcv->dp = dp;
    rcv->pvt = sdr_pvt_new(rcv);
    for (int i = 0; i < 4; i++) {
        if (i == 3 && rcv->dev != SDR_DEV_USB) continue;
        if (i != 2 && *paths[i] && !(rcv->strs[i] = sdr_str_open(paths[i]))) {
            fprintf(stderr, "stream open error: %s", paths[i]);
        }
    }
    rcv->state = 1;
    return !pthread_create(&rcv->thread, NULL, rcv_thread, rcv);
}

// get file path and start time ------------------------------------------------
static int get_path_time(stream_t *str, char *path, char *tstr)
{
    char buff[4096], *p = buff, *q;
    
    strstatx(str, buff);
    if (!(p = strstr(p, "openpath= ")) || !(q = strstr(p + 10, "\n"))) {
        return 0;
    }
    *q = '\0';
    sprintf(path, "%.1023s", p + 10);
    if (!(p = strstr(q + 1, "time    = ")) || !(q = strstr(p + 10, "\n"))) {
        return 0;
    }
    *q = '\0';
    sprintf(tstr, "%.31s", p + 10);
    return 1;
}

// write tag file --------------------------------------------------------------
static void write_tag(sdr_rcv_t *rcv, stream_t *str)
{
    static const char *fstr[] = {
        "-", "INT8", "INT8X2", "RAW8", "RAW16", "RAW16I"
    };
    FILE *fp;
    char path[1024+4], tstr[32];
    
    // get file path and start time
    if (!get_path_time(str, path, tstr)) return;
    strcat(path, ".tag");
    
    if (!(fp = fopen(path, "w"))) return;
    fprintf(fp, "PROG = %s\n", SDR_DEV_NAME);
    fprintf(fp, "TIME = %s\n", tstr);
    fprintf(fp, "FMT  = %s\n", fstr[rcv->fmt]);
    fprintf(fp, "F_S  = %.6g\n", rcv->fs * 1e-6);
    int nch = rcv->fmt == SDR_FMT_RAW8 ? 2 : (rcv->fmt == SDR_FMT_RAW16 ? 4 : 8);
    fprintf(fp, "F_LO = ");
    for (int j = 0; j < nch; j++) {
        fprintf(fp, "%.6g%s", rcv->fo[j] * 1e-6, j < nch - 1 ? "," : "\n");
    }
    fprintf(fp, "IQ   = ");
    for (int j = 0; j < nch; j++) {
        fprintf(fp, "%d%s", rcv->IQ[j], j < nch - 1 ? "," : "\n");
    }
    fclose(fp);
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
    for (int i = 0; i < rcv->nch; i++) {
        ch_th_stop(rcv->th[i]);
    }
    for (int i = 0; i < rcv->nch; i++) {
        pthread_join(rcv->th[i]->thread, NULL);
    }
    rcv->state = 0;
    pthread_join(rcv->thread, NULL);
    
    if (rcv->dev == SDR_DEV_USB && rcv->strs[3] && rcv->strs[3]->type == STR_FILE) {
        write_tag(rcv, rcv->strs[3]);
    }
    for (int i = 0; i < 4; i++) {
        sdr_str_close(rcv->strs[i]);
    }
    sdr_pvt_free(rcv->pvt);
    sdr_log_close();
}

// get and set LNA gain of RF frontend -----------------------------------------
int sdr_rcv_get_gain(sdr_rcv_t *rcv, int ch)
{
    if (!rcv || !rcv->state || rcv->dev != SDR_DEV_USB) return -1;
    return sdr_dev_get_gain((sdr_dev_t *)rcv->dp, ch);
}

int sdr_rcv_set_gain(sdr_rcv_t *rcv, int ch, int gain)
{
    if (!rcv || !rcv->state || rcv->dev != SDR_DEV_USB) return -1;
    return sdr_dev_set_gain((sdr_dev_t *)rcv->dp, ch, gain);
}

//------------------------------------------------------------------------------
//  Generate a new SDR receiver by SDR device and start receiver.
//
//  args:
//      sigs      (I)  signal types as a string array {sig1, sig2, ..., sign}
//      prns      (I)  PRN numbers as int array {prn1, prn2, ..., prnn}
//      n         (I)  number of sigs and prns
//      bus       (I)  USB bus number of SDR device (-1:any)
//      port      (I)  USB port number of SDR device (-1:any)
//      conf_file (I)  configration file for SDR device ("": no config)
//      paths     (I)  output stream paths as same as sdr_rcv_start()
//
//  returns:
//      SDR receiver (NULL: error)
//
sdr_rcv_t *sdr_rcv_open_dev(const char **sigs, int *prns, int n, int bus,
    int port, const char *conf_file, const char **paths)
{
    sdr_dev_t *dev;
    double fs, fo[SDR_MAX_RFCH] = {0};
    int fmt, nch, IQ[SDR_MAX_RFCH] = {0};
    
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
    if (!(nch = sdr_dev_get_info(dev, &fmt, &fs, fo, IQ))) {
        sdr_dev_close(dev);
        return NULL;
    }
    sdr_rcv_t *rcv = sdr_rcv_new(sigs, prns, n, fmt, fs, fo, IQ);
    sdr_rcv_start(rcv, SDR_DEV_USB, (void *)dev, paths);
    
    return rcv;
}

// read tag for IF data dump file ----------------------------------------------
static void read_tag(const char *file, int *fmt, double *fs, double *fo,
    int *IQ)
{
    FILE *fp;
    char path[1024+4], buff[256], *p;
    
    snprintf(path, sizeof(path), "%s.tag", file);
    
    if (!(fp = fopen(path, "r"))) return;
    
    while (fgets(buff, sizeof(buff), fp)) {
        if (!(p = strchr(buff, '='))) continue;
        if (strstr(buff, "FMT") == buff) {
            if      (!strncmp(p + 2, "INT8X2", 6)) *fmt = SDR_FMT_INT8X2;
            else if (!strncmp(p + 2, "INT8"  , 4)) *fmt = SDR_FMT_INT8;
            else if (!strncmp(p + 2, "RAW16I", 6)) *fmt = SDR_FMT_RAW16I;
            else if (!strncmp(p + 2, "RAW16" , 5)) *fmt = SDR_FMT_RAW16;
            else if (!strncmp(p + 2, "RAW8"  , 4)) *fmt = SDR_FMT_RAW8;
        }
        else if (strstr(buff, "F_S") == buff) {
            if (sscanf(p + 2, "%lf", fs)) *fs *= 1e6;
        }
        else if (strstr(buff, "F_LO") == buff) {
            int n = sscanf(p + 2, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", fo, fo + 1,
                fo + 2, fo + 3, fo + 4, fo + 5, fo + 6, fo + 7);
            for (int i = 0; i < n; i++) fo[i] *= 1e6;
        }
        else if (strstr(buff, "IQ") == buff) {
            sscanf(p + 2, "%d,%d,%d,%d,%d,%d,%d,%d", IQ, IQ + 1, IQ + 2, IQ + 3,
                IQ + 4, IQ + 5, IQ + 6, IQ + 7);
        }
    }
    int nch = (*fmt == SDR_FMT_INT8 || *fmt == SDR_FMT_INT8X2) ? 1 :
        (*fmt == SDR_FMT_RAW8 ? 2 : (*fmt == SDR_FMT_RAW16 ? 4 : 8));
    for (int i = nch; i < SDR_MAX_RFCH; i++) {
        fo[i] = 0.0;
        IQ[i] = 0;
    }
    fclose(fp);
}

//------------------------------------------------------------------------------
//  Generate a new SDR receiver by IF data file and start receiver.
//
//  args:
//      sigs      (I)  signal types as a string array {sig1, sig2, ..., sign}
//      prns      (I)  PRN numbers as int array {prn1, prn2, ..., prnn}
//      n         (I)  number of sigs and prns
//      fmt       (I)  IF data format as same as sdr_rcv_new()
//      fs        (I)  sampling rate (sps)
//      fo        (I)  LO frequency for each RFCH (Hz)
//      IQ        (I)  sampling type for each RFCH (1:I, 2:IQ)
//      toff      (I)  time offset of IF data file (s)
//      tscale    (I)  time scale of replay IF data file
//      file      (I)  IF data file
//      paths     (I)  output stream paths as same as sdr_rcv_start()
//
//  returns:
//      SDR receiver (NULL: error)
//
//  notes:
//      If the tag file exists, fmt, fs, fo, IQ are obtained from the tag file.
//
//
sdr_rcv_t *sdr_rcv_open_file(const char **sigs, int *prns, int n, int fmt,
    double fs, const double *fo, const int *IQ, double toff, double tscale,
    const char *file, const char **paths)
{
    FILE *fp;
    double fo_t[SDR_MAX_RFCH] = {0};
    int IQ_t[SDR_MAX_RFCH] = {0};
    
    if (!(fp = fopen(file, "rb"))) {
        fprintf(stderr, "file open error: %s\n", file);
        return NULL;
    }
    // read tag file
    memcpy(fo_t, fo, sizeof(double) * SDR_MAX_RFCH);
    memcpy(IQ_t, IQ, sizeof(int) * SDR_MAX_RFCH);
    read_tag(file, &fmt, &fs, fo_t, IQ_t);
    
    int ns = (fmt == SDR_FMT_INT8 || fmt == SDR_FMT_RAW8) ? 1 : 2;
#if defined(WIN32) || defined(MACOS)
    fpos_t pos = (fpos_t)(fs * toff * ns);
#else
    fpos_t pos = {(__off_t)(fs * toff * ns)};
#endif
    fsetpos(fp, &pos);
    
    sdr_rcv_t *rcv = sdr_rcv_new(sigs, prns, n, fmt, fs, fo_t, IQ_t);
    rcv->tscale = tscale;
    sdr_rcv_start(rcv, SDR_DEV_FILE, (void *)fp, paths);
    
    return rcv;
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
    }
    else {
        fclose((FILE *)rcv->dp);
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
    else fprintf(stderr, "sdr_rcv_setopt error opt=%s\n", opt);
}

