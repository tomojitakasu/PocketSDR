//
//  Pocket SDR C Library - GNSS SDR Receiver Functions
//
//  Author:
//  T.TAKASU
//
//  History:
//  2024-02-08  1.0  separated from pocket_trk.c
//  2024-03-25  1.1  modify API sdr_rcv_new(), sdr_rcv_start()
//
#include "rtklib.h"
#include "pocket_sdr.h"

// constants --------------------------------------------------------------------
#define SP_CORR    0.5          // correlator spacing (chip) 
#define T_CYC      1e-3         // data read cycle (s)
#define LOG_CYC    1000         // receiver channel log cycle (* T_CYC)
#define TH_CYC     50           // receiver channel thread cycle (ms)
#define T_REACQ    60.0         // re-acquisition timeout (s)
#define MIN_LOCK   2.0          // min lock time to print channel status (s)
#define MAX_BUFF   8000         // max number of IF data buffer
#define MAX_ROW    110          // max number of status rows
#define NCOL       102          // nummber of status columns

#define ESC_COL    "\033[34m"   // ANSI escape color blue
#define ESC_RES    "\033[0m"    // ANSI escape reset
#define ESC_UCUR   "\033[A"     // ANSI escape cursor up
#define ESC_VCUR   "\033[?25h"  // ANSI escape show cursor
#define ESC_HCUR   "\033[?25l"  // ANSI escape hide cursor

// test IF buffer full ---------------------------------------------------------
static int buff_full(sdr_rcv_t *rcv)
{
    for (int i = 0; i < rcv->nch; i++) {
        if (rcv->ix + 1 - rcv->th[i]->ix >= MAX_BUFF) return 1;
    }
    return 0;
}

// C/N0 bar --------------------------------------------------------------------
static void cn0_bar(float cn0, char *bar)
{
    int n = (int)((cn0 - 30.0) / 1.5);
    bar[0] = '\0';
    for (int i = 0; i < n && i < 13; i++) {
        sprintf(bar + i, "|");
    }
}

// channel sync status ---------------------------------------------------------
static void sync_stat(sdr_ch_t *ch, char *stat)
{
    sprintf(stat, "%s%s%s%s", (ch->trk->sec_sync > 0) ? "S" : "-",
        (ch->nav->ssync > 0) ? "B" : "-", (ch->nav->fsync > 0) ? "F" : "-",
        (ch->nav->rev) ? "R" : "-");
}

// print SDR receiver status header --------------------------------------------
static int print_head(sdr_rcv_t *rcv)
{
    int nch = 0, ncol = NCOL;
    
    for (int i = 0; i < rcv->nch; i++) {
        if (rcv->th[i]->ch->state == STATE_LOCK) nch++;
    }
    printf("\r TIME(s):%10.2f %*s%10s  SRCH:%4d  LOCK:%3d/%3d", rcv->ix * T_CYC,
        ncol - 55, "", buff_full(rcv) ? "BUFF-FULL" : "", rcv->ich + 1, nch,
        rcv->nch);
#ifdef DEBUG
    int week;
    double tow = time2gpst(utc2gpst(timeget()), &week);
    printf("        %4d-%11.3f", week, tow);
#endif
    printf("\n%3s %4s %5s %3s %8s %4s %-12s %11s %7s %11s %4s %5s %4s %4s %3s",
        "CH", "SAT", "SIG", "PRN", "LOCK(s)", "C/N0", "(dB-Hz)", "COFF(ms)",
        "DOP(Hz)", "ADR(cyc)", "SYNC", "#NAV", "#ERR", "#LOL", "NER");
#ifdef DEBUG
    printf(" %3s %3s %3s %11s", "ERP", "ERC", "MT", "TOW(s)");
#endif
    printf("\n");
    return 2;
}

// print SDR receiver channel status -------------------------------------------
static int print_ch_stat(sdr_ch_t *ch)
{
    char bar[16], stat[16];
    cn0_bar(ch->cn0, bar);
    sync_stat(ch, stat);
    printf("%s%3d %4s %5s %3d %8.2f %4.1f %-13s%11.7f %7.1f %11.1f %s %5d %4d %4d %3d",
        ESC_COL, ch->no, ch->sat, ch->sig, ch->prn, ch->lock * ch->T, ch->cn0,
        bar, ch->coff * 1e3, ch->fd, ch->adr, stat, ch->nav->count[0],
        ch->nav->count[1], ch->lost, ch->nav->nerr);
#ifdef DEBUG
    printf(" %3.0f %3.0f %3d %11.3f", ch->trk->err_phas * 100.0,
        ch->trk->err_code * 1e8, ch->nav->mt, ch->tow);
#endif
    printf("%s\n", ESC_RES);
    return 1;
}

// print SDR receiver status ---------------------------------------------------
static int print_rcv_stat(sdr_rcv_t *rcv, int nrow)
{
    int n = 0, ncol = NCOL;
    for (int i = 0; i < nrow; i++) {
        printf("%s", ESC_UCUR);
    }
    n += print_head(rcv);
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_t *ch = rcv->th[i]->ch;
        if (ch->state != STATE_LOCK || ch->lock * ch->T < MIN_LOCK) continue;
        if (n < MAX_ROW - 1) {
            n += print_ch_stat(ch);
        }
        else if (n == MAX_ROW - 1) {
            printf("...\n");
            n++;
        }
    }
#ifdef DEBUG
    ncol += 27;
#endif
    for ( ; n < nrow; n++) {
        printf("%*s\n", ncol, "");
    }
    fflush(stdout);
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
    const double *dop, sdr_rcv_t *rcv)
{
    sdr_ch_th_t *th = (sdr_ch_th_t *)sdr_malloc(sizeof(sdr_ch_th_t));
    double sp = SP_CORR;
    
    if (!(th->ch = sdr_ch_new(sig, prn, fs, fi, sp, 0, dop[0], dop[1], ""))) {
        sdr_free(th);
        return NULL;
    }
    th->if_ch = (rcv->fmt == SDR_FMT_RAW8 && sdr_sig_freq(sig) < 1.5e9) ? 1 : 0;
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
    int n = th->ch->N / th->rcv->N;
    int64_t ix = 0;
    
    while (th->state) {
        for ( ; ix + 2 * n <= th->rcv->ix + 1; ix += n) {
            
            // update SDR receiver channel
            sdr_ch_update(th->ch, ix * T_CYC, th->rcv->buff[th->if_ch],
                th->rcv->N * (ix % MAX_BUFF));
            
            if (th->ch->state == STATE_LOCK && ix % LOG_CYC == 0) {
                out_log_ch(th->ch);
            }
            th->ix = ix; // IF buffer read pointer
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
//      dev      (I) SDR device type
//                     SDR_DEV_FILE: file
//                     SDR_DEV_USB : USB device
//      dp       (I) SDR device pointer
//                     file pointer       (dev = SDR_DEV_FILE)
//                     SDR device pointer (dev = SDR_DEV_USB)
//      sigs     (I) signal types as a string array {sig_1, sig_2, ..., sig_n}
//      prns     (I) PRN numbers as int array {prn_1, prn_2, ..., prn_n}
//      fi       (I) IF frequencies (Hz) {fi_1, fi_2, ..., fi_n}
//      n        (I) number of signal types, PRN numbers and IF frequencies
//      fs       (I) sampling freqency of IF data (Hz)
//      dop      (I) Doppler search range (dop[0]-dop[1] ... dop[0]+dop[1])
//                     dop[0]: center frequency to search (Hz)
//                     dop[1]: lower and upper limits to search (Hz)
//      fmt      (I) IF data format
//                     SDR_FMT_INT8 : int8
//                     SDR_FMT_RAW8 : packed 8 bit raw
//                     SDR_FMT_RAW16: packed 16 bit raw
//      IQ       (I) sampling types of IF data (1:I-sampling, 2:IQ-sampling)
//                     IQ[0]: CH1
//                     IQ[1]: CH2 (packed 8 or 16 bit raw format only)
//                     IQ[2]: CH3 (packed 16 bit raw format only)
//                     IQ[3]: CH4 (packed 16 bit raw format only)
//
//  returns:
//      SDR receiver (NULL: error)
//
sdr_rcv_t *sdr_rcv_new(int dev, void *dp, char **sigs, const int *prns,
    const double *fi, int n, double fs, const double *dop, int fmt,
    const int *IQ)
{
    sdr_rcv_t *rcv = (sdr_rcv_t *)sdr_malloc(sizeof(sdr_rcv_t));
    
    rcv->dev = dev;
    rcv->dp = dp;
    rcv->ich = -1;
    rcv->N = (int)(T_CYC * fs);
    rcv->fmt = fmt;
    for (int i = 0; i < (fmt == SDR_FMT_RAW8 ? 2 : 1); i++) {
        rcv->buff[i] = sdr_buff_new(rcv->N * MAX_BUFF, IQ[i]);
    }
    int ns = (fmt == SDR_FMT_INT8 && IQ[0] == 2) ? 2 : 1;
    rcv->raw = (int8_t *)sdr_malloc(rcv->N * ns);
    
    for (int i = 0; i < n && rcv->nch < SDR_MAX_NCH; i++) {
        sdr_ch_th_t *th = ch_th_new(sigs[i], prns[i], fi[i], fs, dop, rcv);
        if (th) {
            th->ch->no = rcv->nch + 1;
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
    for (int i = 0; i < 2; i++) {
        sdr_buff_free(rcv->buff[i]);
    }
    sdr_free(rcv->raw);
    sdr_free(rcv);
}

// generate lookup table -------------------------------------------------------
static void gen_LUT(int IQ0, int IQ1, int8_t LUT[][256])
{
    static const int8_t val[] = {1, 3, -1, -3};
    
    for (int i = 0; i < 256; i++) {
        LUT[0][i] = CPX8(val[(i>>0) & 0x3], IQ0 == 1 ? 0 : -val[(i>>2) & 0x3]);
        LUT[1][i] = CPX8(val[(i>>4) & 0x3], IQ1 == 1 ? 0 : -val[(i>>6) & 0x3]);
    }
}

// read IF data ----------------------------------------------------------------
static int rcv_read_data(sdr_rcv_t *rcv, int64_t ix)
{
    static int8_t LUT[2][256] = {{0}};
    int i = rcv->N * (ix % MAX_BUFF);
    int ns = (rcv->fmt == SDR_FMT_RAW8 || rcv->buff[0]->IQ == 1) ? 1 : 2;
    
    if (rcv->dev == SDR_DEV_FILE) { // file input
        if (fread(rcv->raw, ns, rcv->N, (FILE *)(rcv->dp)) < (size_t)rcv->N) {
            return 0; // end of file
        }
    }
    else {
        return 0;
    }
    if (rcv->fmt == SDR_FMT_RAW8) { // packed 8 bit raw
        if (LUT[0][0] == 0) {
            gen_LUT(rcv->buff[0]->IQ, rcv->buff[1]->IQ, LUT);
        }
        for (int j = 0; j < rcv->N; i++, j++) {
            uint8_t raw = (uint8_t)rcv->raw[j];
            rcv->buff[0]->data[i] = LUT[0][raw];
            rcv->buff[1]->data[i] = LUT[1][raw];
        }
    }
    else if (rcv->buff[0]->IQ == 1) { // I-sampling
        for (int j = 0; j < rcv->N; i++, j++) {
            rcv->buff[0]->data[i] = CPX8(rcv->raw[j], 0);
        }
    }
    else if (rcv->buff[0]->IQ == 2) { // IQ-sampling
        for (int j = 0; j < rcv->N; i++, j++) {
            rcv->buff[0]->data[i] = CPX8(rcv->raw[j*2], -rcv->raw[j*2+1]);
        }
    }
    rcv->ix = ix; // IF buffer write pointer
    return 1;
}

// re-acquisition --------------------------------------------------------------
static int re_acq(sdr_rcv_t *rcv, sdr_ch_t *ch)
{
    if (ch->lock * ch->T >= MIN_LOCK && rcv->ix * T_CYC - ch->time + T_REACQ) {
        ch->acq->fd_ext = ch->fd;
        return 1;
    }
    return 0;
}

// assisted-acquisition --------------------------------------------------------
static int assist_acq(sdr_rcv_t *rcv, sdr_ch_t *ch)
{
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_t *ch_i = rcv->th[i]->ch;
        if (strcmp(ch->sat, ch_i->sat) || ch_i->state != STATE_LOCK ||
            ch_i->lock * ch_i->T < MIN_LOCK) continue;
        ch->acq->fd_ext = ch_i->fd * ch->fc / ch_i->fc;
        return 1;
    }
    return 0;
}

// update signal search channel ------------------------------------------------
static void rcv_update_srch(sdr_rcv_t *rcv)
{
    // signal search channel busy ?
    if (rcv->ich >= 0 && rcv->th[rcv->ich]->ch->state == STATE_SRCH) {
        return;
    }
    for (int i = 0; i < rcv->nch; i++) {
        // search next IDLE channel
        rcv->ich = (rcv->ich + 1) % rcv->nch;
        sdr_ch_t *ch = rcv->th[rcv->ich]->ch;
        if (ch->state != STATE_IDLE) continue;
        
        // re-acquisition, assisted-acquisition or short code cycle
        if (re_acq(rcv, ch) || assist_acq(rcv, ch) || ch->T <= 5e-3) {
            ch->state = STATE_SRCH;
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
    sdr_rcv_t *rcv = (sdr_rcv_t *)arg;
    int nrow = 0;
    
    sdr_log(3, "$LOG,%.3f,%s,%d,START NCH=%d FMT=%d", 0.0, "", 0, rcv->nch,
        rcv->fmt);
    
    if (rcv->tint[0] > 0.0) {
        printf("%s", ESC_HCUR);
    }
    for (int64_t ix = 0; rcv->state; ix++) {
        if (ix % LOG_CYC == 0) {
            out_log_time(ix * T_CYC);
        }
        // read IF data
        if (!rcv_read_data(rcv, ix)) {
            sdr_sleep_msec(500);
            rcv->state = 0;
            break;
        }
        // update signal search channel
        rcv_update_srch(rcv);
        
        // print receiver status
        if (rcv->tint[0] > 0.0 && ix % (int)(rcv->tint[0] / T_CYC) == 0) {
            nrow = print_rcv_stat(rcv, nrow);
        }
        // suspend data if reading file
        if (rcv->dev == SDR_DEV_FILE && (FILE *)(rcv->dp) != stdin) {
            rcv_wait(rcv);
        }
    }
    if (rcv->tint[0] > 0.0) {
        print_rcv_stat(rcv, nrow);
        printf("%s", ESC_VCUR);
    }
    sdr_log(3, "$LOG,%.3f,%s,%d,STOP", rcv->ix * T_CYC, "", 0);
    return NULL;
}

//------------------------------------------------------------------------------
//  Start a SDR receiver.
//
//  args:
//      rcv      (I) SDR receiver
//      tint     (I) output intervals (s) (0: no output)
//                     tint[0]: status print
//                     tint[1]: NMEA PVT solutions      (to be implemented)
//                     tint[2]: RTCM3 OBS and NAV data  (to be implemented)
//
//  returns:
//      Status (1:OK, 0:error)
//
int sdr_rcv_start(sdr_rcv_t *rcv, const double *tint)
{
    if (rcv->state) return 0;
    
    for (int i = 0; i < rcv->nch; i++) {
        // set all channels search for file input
        if (rcv->dev == SDR_DEV_FILE && (FILE *)rcv->dp != stdin) {
            rcv->th[i]->ch->state = STATE_SRCH;
        }
        ch_th_start(rcv->th[i]);
    }
    rcv->state = 1;
    rcv->tint[0] = tint[0];
    rcv->tint[1] = tint[1];
    rcv->tint[2] = tint[2];
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
}
