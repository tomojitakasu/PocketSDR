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
//
#include "pocket_sdr.h"
#include "pocket_dev.h"

// constants --------------------------------------------------------------------
#define SP_CORR    0.25         // correlator spacing (chip) 
#define MAX_DOP    5000.0       // max Doppler frequency for acquisition (Hz) 
#define MAX_BUFF   8000         // max number of IF data buffer (* SDR_CYC)
#define LOG_CYC    1000         // receiver channel log cycle (* SDR_CYC)
#define TH_CYC     10           // receiver channel thread cycle (ms)
#define TO_REACQ   60.0         // re-acquisition timeout (s)
#define MIN_LOCK   2.0          // min lock time to show channel status (s)
#define MAX_ROW    108          // max number of channel status rows
#define NUM_COL    110          // number of channel status columns
#define MAX_ACQ    4e-3         // max code length w/o acqusition assist (s)
#define MAX_BUFF_USE 90         // max buffer usage rate (%)
#define ESC_COL    "\033[34m"   // ANSI escape color blue
#define ESC_RES    "\033[0m"    // ANSI escape reset
#define ESC_UCUR   "\033[A"     // ANSI escape cursor up
#define ESC_VCUR   "\033[?25h"  // ANSI escape show cursor
#define ESC_HCUR   "\033[?25l"  // ANSI escape hide cursor

// C/N0 bar --------------------------------------------------------------------
static void cn0_bar(float cn0, char *bar)
{
    int n = (int)((cn0 - 30.0) / 1.5);
    bar[0] = '\0';
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

// print SDR receiver status header --------------------------------------------
static int print_head(FILE *fp, sdr_rcv_t *rcv)
{
    char solstr[128];
    int nch = 0;
    for (int i = 0; i < rcv->nch; i++) {
        if (rcv->th[i]->ch->state != SDR_STATE_LOCK) continue;
        nch++;
    }
    sdr_pvt_solstr(rcv->pvt, solstr);
    fprintf(fp, "\r %-*s BUFF:%3d%% SRCH:%3d LOCK:%3d/%3d\n", NUM_COL - 38,
        solstr, rcv->buff_use, rcv->ich + 1, nch, rcv->nch);
    fprintf(fp, "%3s %2s %4s %5s %3s %8s %4s %-12s %11s %7s %11s %4s %5s %4s "
        "%4s %3s\n", "CH", "RF", "SAT", "SIG", "PRN", "LOCK(s)", "C/N0",
        "(dB-Hz)", "COFF(ms)", "DOP(Hz)", "ADR(cyc)", "SYNC", "#NAV", "#ERR",
        "#LOL", "FEC");
    return 2;
}

// print SDR receiver channel status -------------------------------------------
static int print_ch_stat(FILE *fp, sdr_ch_t *ch)
{
    char bar[16], stat[16];
    cn0_bar(ch->cn0, bar);
    sync_stat(ch, stat);
    fprintf(fp, "%s%3d %2d %4s %5s %3d %8.2f %4.1f %-13s%11.7f %7.1f %11.1f %s "
        "%5d %4d %4d %3d%s\n", ESC_COL, ch->no, ch->if_ch + 1, ch->sat, ch->sig,
        ch->prn, ch->lock * ch->T, ch->cn0, bar, ch->coff * 1e3, ch->fd,
        ch->adr, stat, ch->nav->count[0], ch->nav->count[1], ch->lost,
        ch->nav->nerr, ESC_RES);
    return 1;
}

// print SDR receiver status ---------------------------------------------------
static int print_rcv_stat(FILE *fp, sdr_rcv_t *rcv, int nrow)
{
    int n = 0;
    
    for (int i = 0; i < nrow; i++) {
        fprintf(fp, "%s", ESC_UCUR);
    }
    n += print_head(fp, rcv);
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_t *ch = rcv->th[i]->ch;
        if (ch->state != SDR_STATE_LOCK || ch->lock * ch->T < MIN_LOCK) continue;
        if (n < MAX_ROW - 1) {
            n += print_ch_stat(fp, ch);
        }
        else if (n == MAX_ROW - 1) {
            fprintf(fp, "... ....\n");
            n++;
        }
    }
    for ( ; n < nrow; n++) {
        fprintf(fp, "%*s\n", NUM_COL, "");
    }
    fflush(fp);
    return n;
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
    double sp = SP_CORR;
    
    if (!(th->ch = sdr_ch_new(sig, prn, fs, fi, sp, 0, 0.0, MAX_DOP))) {
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
        for ( ; th->ix + 2 * n <= th->rcv->ix && th->state; th->ix += n) {
            
            // update SDR receiver channel
            sdr_ch_update(ch, th->ix * SDR_CYC, th->rcv->buff[ch->if_ch],
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

//------------------------------------------------------------------------------
//  Generate a new SDR receiver.
//
//  args:
//      sigs     (I) signal types as a string array {sig_1, sig_2, ..., sig_n}
//      prns     (I) PRN numbers as int array {prn_1, prn_2, ..., prn_n}
//      if_ch    (I) IF CHs (0:CH1,1:CH2, ...) {if_ch_1, if_ch_2, ..., if_ch_n}
//      fi       (I) IF frequencies (Hz) {fi_1, fi_2, ..., fi_n}
//      n        (I) number of signal types, PRN numbers and IF frequencies
//      fs       (I) sampling frequency of IF data (Hz)
//      dop      (I) Doppler search range (dop[0]-dop[1] ... dop[0]+dop[1])
//                     dop[0]: center frequency to search (Hz)
//                     dop[1]: lower and upper limits to search (Hz)
//      fmt      (I) IF data format
//                     SDR_FMT_INT8 : 8 bits int
//                     SDR_FMT_CPX16: 16(8+8) bits complex
//                     SDR_FMT_RAW8 : packed 8(4x2) bits raw
//                     SDR_FMT_RAW16: packed 16(4x4) bits raw
//      IQ       (I) sampling types of IF data (1:I-sampling, 2:IQ-sampling)
//                     IQ[0]: CH1
//                     IQ[1]: CH2 (SDR_FMT_RAW8 or SDR_FMT_RAW16)
//                     IQ[2]: CH3 (SDR_FMT_RAW16)
//                     IQ[3]: CH4 (SDR_FMT_RAW16)
//
//  returns:
//      SDR receiver (NULL: error)
//
sdr_rcv_t *sdr_rcv_new(const char **sigs, const int *prns, const int *if_ch, 
    const double *fi, int n, double fs, int fmt, const int *IQ)
{
    int nbuff = (fmt == SDR_FMT_RAW16) ? 4 : ((fmt == SDR_FMT_RAW8) ? 2 : 1);
    
    sdr_rcv_t *rcv = (sdr_rcv_t *)sdr_malloc(sizeof(sdr_rcv_t));
    rcv->ich = -1;
    rcv->N = (int)(SDR_CYC * fs);
    rcv->fmt = fmt;
    rcv->nbuff = nbuff;
    for (int i = 0; i < nbuff; i++) {
        rcv->buff[i] = sdr_buff_new(rcv->N * MAX_BUFF, IQ[i]);
    }
    for (int i = 0; i < n && rcv->nch < SDR_MAX_NCH; i++) {
        sdr_ch_th_t *th = ch_th_new(sigs[i], prns[i], fi[i], fs, rcv);
        if (th) {
            th->ch->no = rcv->nch + 1;
            th->ch->if_ch = if_ch[i];
            rcv->th[rcv->nch++] = th;
        }
        else {
            fprintf(stderr, "signal / prn error: %s / %d\n", sigs[i], prns[i]);
        }
    }
    return rcv;
}

//------------------------------------------------------------------------------
//  Free a SDR receiver.
//
//  args:
//      rcv      (I) SDR receiver generated by sdr_rcv_new()
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
    if (rcv->fmt == SDR_FMT_INT8) { // 8 bits int
        for (int j = 0; j < rcv->N; i++, j++) {
            rcv->buff[0]->data[i] = SDR_CPX8(raw[j], 0);
        }
    }
    else if (rcv->fmt == SDR_FMT_CPX16) { // 16(8x2) bits complex
        for (int j = 0; j < rcv->N * 2; i++, j += 2) {
            rcv->buff[0]->data[i] = SDR_CPX8(raw[j], -raw[j+1]);
        }
    }
    else if (rcv->fmt == SDR_FMT_RAW8) { // packed 8(4x2) bits raw
        for (int j = 0; j < rcv->N; i++, j++) {
            rcv->buff[0]->data[i] = LUT[0][raw[j]];
            rcv->buff[1]->data[i] = LUT[1][raw[j]];
        }
    }
    else if (rcv->fmt == SDR_FMT_RAW16) { // packed 16(4x4) bits raw
        for (int j = 0; j < rcv->N * 2; i++, j += 2) {
            rcv->buff[0]->data[i] = LUT[0][raw[j  ]];
            rcv->buff[1]->data[i] = LUT[1][raw[j  ]];
            rcv->buff[2]->data[i] = LUT[2][raw[j+1]];
            rcv->buff[3]->data[i] = LUT[3][raw[j+1]];
        }
    }
    rcv->ix = ix; // update IF data buffer write pointer
}

// re-acquisition --------------------------------------------------------------
static int re_acq(sdr_rcv_t *rcv, sdr_ch_t *ch)
{
    if (ch->lock * ch->T >= MIN_LOCK &&
        rcv->ix * SDR_CYC < ch->time + TO_REACQ) {
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

// update IF data buffer usage rate --------------------------------------------
static void update_buff_use(sdr_rcv_t *rcv)
{
    rcv->buff_use = 0;
    for (int i = 0; i < rcv->nch; i++) {
        int use = (rcv->ix + 1 - rcv->th[i]->ix) * 100 / MAX_BUFF + 1;
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

// wait for receiver channels --------------------------------------------------
static void rcv_wait(sdr_rcv_t *rcv)
{
    for (int i = 0; i < rcv->nch; i++) {
        while (rcv->ix + 1 - rcv->th[i]->ix >= MAX_BUFF - 10) {
            sdr_sleep_msec(1);
        }
    }
}

// SDR receiver thread ---------------------------------------------------------
static void *rcv_thread(void *arg)
{
    FILE *fp = stdout; // SDR receiver status output
    sdr_rcv_t *rcv = (sdr_rcv_t *)arg;
    int ns = (rcv->fmt == SDR_FMT_INT8 || rcv->fmt == SDR_FMT_RAW8) ? 1 : 2;
    int nrow = 0;
    uint8_t raw[ns * rcv->N];
    
    sdr_log(3, "$LOG,%.3f,%s,%d,START NCH=%d FMT=%d", 0.0, "", 0, rcv->nch,
        rcv->fmt);
    
    if (rcv->dev == SDR_DEV_FILE && (FILE *)rcv->dp == stdin) {
        fflush((FILE *)rcv->dp);
    }
    else if (rcv->dev == SDR_DEV_USB) {
        sdr_dev_start((sdr_dev_t *)rcv->dp);
    }
    if (rcv->tint > 0.0) {
        fprintf(fp, "%s", ESC_HCUR);
    }
    for (int64_t ix = 0; rcv->state; ix++) {
        if (ix % LOG_CYC == 0) {
            update_buff_use(rcv);
            out_log_time(ix * SDR_CYC);
        }
        // read IF data
        if (!read_data(rcv, raw, ns * rcv->N)) {
            sdr_sleep_msec(500);
            rcv->state = 0;
            break;
        }
        // write IF data buffer
        write_buff(rcv, raw, ix);
        
        // update signal search channel
        update_srch_ch(rcv);
        
        // update PVT solution
        sdr_pvt_udsol(rcv->pvt, ix);
        
        // print receiver status
        if (rcv->tint > 0.0 && ix % (int)(rcv->tint / SDR_CYC) == 0) {
            nrow = print_rcv_stat(fp, rcv, nrow);
        }
        // suspend data if reading file
        if (rcv->dev == SDR_DEV_FILE && (FILE *)(rcv->dp) != stdin) {
            rcv_wait(rcv);
        }
    }
    if (rcv->dev == SDR_DEV_USB) {
        sdr_dev_stop((sdr_dev_t *)rcv->dp);
    }
    if (rcv->tint > 0.0) {
        print_rcv_stat(fp, rcv, nrow);
        fprintf(fp, "%s", ESC_VCUR);
    }
    sdr_log(3, "$LOG,%.3f,%s,%d,STOP", rcv->ix * SDR_CYC, "", 0);
    return NULL;
}

//------------------------------------------------------------------------------
//  Start a SDR receiver.
//
//  args:
//      rcv      (I) SDR receiver
//      dev      (I) SDR device type
//                     SDR_DEV_FILE: file
//                     SDR_DEV_USB : USB device
//      dp       (I) SDR device pointer
//                     file pointer       (dev = SDR_DEV_FILE)
//                     SDR device pointer (dev = SDR_DEV_USB)
//      paths    (I) output stream paths ("": no output)
//                     paths[0]: log stream
//                     paths[1]: NMEA PVT solutions stream
//                     paths[2]: RTCM3 OBS and NAV data stream
//      tint     (I) status print intervals (s) (0: no output)
//
//  returns:
//      Status (1:OK, 0:error)
//
int sdr_rcv_start(sdr_rcv_t *rcv, int dev, void *dp, const char **paths,
    double tint)
{
    if (rcv->state) return 0;
    
    sdr_log_open(paths[0]);
    
    for (int i = 0; i < rcv->nch; i++) {
        // set all channels search for file input
        if (dev == SDR_DEV_FILE && (FILE *)dp != stdin) {
            rcv->th[i]->ch->state = SDR_STATE_SRCH;
        }
        ch_th_start(rcv->th[i]);
    }
    rcv->dev = dev;
    rcv->dp = dp;
    rcv->pvt = sdr_pvt_new(rcv);
    rcv->strs[0] = sdr_str_open(paths[1]);
    rcv->strs[1] = sdr_str_open(paths[2]);
    rcv->tint = tint;
    rcv->state = 1;
    return !pthread_create(&rcv->thread, NULL, rcv_thread, rcv);
}

//------------------------------------------------------------------------------
//  Stop a SDR receiver.
//
//  args:
//      rcv      (I) SDR receiver
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
    sdr_str_close(rcv->strs[0]);
    sdr_str_close(rcv->strs[1]);
    sdr_pvt_free(rcv->pvt);
    sdr_log_close();
}
