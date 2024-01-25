//
//  Pocket SDR C AP - GNSS Signal Tracking and NAV data decoding
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-07-05  1.0  port pocket_trk.py to C.
//  2022-08-08  1.1  add option -w, -d
//  2023-12-15  1.2  support multi-threading
//  2023-12-25  1.3  add -r option for raw SDR device data
//  2023-12-28  1.4  set binary mode to stdin for Windows
//  2024-01-12  1.5  update receiver status style
//  2024-01-16  1.6  add -l option, delete -q option
//  2024-01-25  1.7  add acquisition assist
//
#include <signal.h>
#ifdef WIN32
#include <fcntl.h>
#endif
#include "pocket_sdr.h"

// constants --------------------------------------------------------------------
#define SP_CORR    0.5        // correlator spacing (chip) 
#define T_CYC      1e-3       // data read cycle (s)
#define LOG_CYC    1000       // receiver channel log cycle (* T_CYC)
#define TH_CYC     10         // receiver channel thread cycle (ms)
#define T_REACQ    60.0       // re-acquisition timeout (s)
#define MIN_LOCK   2.0        // min lock time to print channel status (s)
#define MAX_BUFF   8000       // max number of IF data buffer
#define MAX_DOP    5000.0     // default max Doppler for acquisition (Hz) 
#define NCOL       106        // nummber of status columns

#define ESC_COL    "\033[34m" // ANSI escape color blue
#define ESC_RES    "\033[0m"  // ANSI escape reset
#define ESC_UCUR   "\033[A"   // ANSI escape cursor up
#define ESC_VCUR   "\033[?25h" // ANSI escape show cursor
#define ESC_HCUR   "\033[?25l" // ANSI escape hide cursor

#define FFTW_WISDOM "../python/fftw_wisdom.txt"

#define MIN(x, y)  ((x) < (y) ? x : y)

// type definitions -------------------------------------------------------------
struct rcv_tag;

typedef struct {              // receiver channel type
    int state;                // state (0:stop,1:run)
    sdr_ch_t *ch;             // SDR receiver channel
    int if_ch;                // IF data channel
    int64_t ix;               // IF buffer read pointer (cyc)
    struct rcv_tag *rcv;      // pointer to receiver
    pthread_t thread;         // receiver channel thread
} rcv_ch_t;

typedef struct rcv_tag {      // receiver type
    int nch;                  // number of receiver channels
    int ich;                  // signal search channel index
    rcv_ch_t *ch[SDR_MAX_NCH]; // receiver channels
    int64_t ix;               // IF buffer write pointer (cyc)
    sdr_cpx_t *buff[2];       // IF buffers
    int N, len_buff;          // cycle and total length of IF buffer (bytes)
    int fmt, IQ[2];           // IF format (1:raw) and sampling types (1:I,2:IQ)
    int8_t *raw;              // input data buffer
} rcv_t;

// usage text ------------------------------------------------------------------
static const char *usage_text[] = {
    "Usage: pocket_trk [-sig sig -prn prn[,...] ...] [-r] [-toff toff] [-f freq]",
    "       [-log path[,level]] [-nmea path[,tint]] [-rtcm path[,tint]] [-w file]",
    "       [file]", NULL
};

// interrupt flag --------------------------------------------------------------
static volatile uint8_t intr = 0;

// signal handler --------------------------------------------------------------
static void sig_func(int sig)
{
    intr = 1;
    signal(sig, sig_func);
}

// test IF buffer full ---------------------------------------------------------
static int buff_full(rcv_t *rcv)
{
    for (int i = 0; i < rcv->nch; i++) {
        if (rcv->ix + 1 - rcv->ch[i]->ix >= MAX_BUFF) return 1;
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

// print receiver status header ------------------------------------------------
static void print_head(rcv_t *rcv)
{
    int nch = 0;
    for (int i = 0; i < rcv->nch; i++) {
        if (rcv->ch[i]->ch->state == STATE_LOCK) nch++;
    }
    printf("\r TIME(s):%10.2f%*s%10s  SRCH:%4d  LOCK:%3d/%3d\n",
        rcv->ix * T_CYC, NCOL - 54, "", buff_full(rcv) ? "BUFF-FULL" : "",
        rcv->ich + 1, nch, rcv->nch);
    printf("%3s %4s %5s %3s %8s %4s %-12s %11s %7s %11s %4s %5s %4s %4s %3s %3s\n",
        "CH", "SAT", "SIG", "PRN", "LOCK(s)", "C/N0", "(dB-Hz)", "COFF(ms)",
        "DOP(Hz)", "ADR(cyc)", "SYNC", "#NAV", "#ERR", "#LOL", "NER", "SEQ");
}

// print receiver channel status -----------------------------------------------
static void print_ch_stat(sdr_ch_t *ch)
{
    char bar[16], stat[16];
    cn0_bar(ch->cn0, bar);
    sync_stat(ch, stat);
    printf("%s%3d %4s %5s %3d %8.2f %4.1f %-13s%11.7f %7.1f %11.1f %s %5d %4d %4d %3d %3d%s\n",
        ESC_COL, ch->no, ch->sat, ch->sig, ch->prn, ch->lock * ch->T, ch->cn0,
        bar, ch->coff * 1e3, ch->fd, ch->adr, stat, ch->nav->count[0],
        ch->nav->count[1], ch->lost, ch->nav->nerr, ch->nav->seq % 1000,
        ESC_RES);
}

// print receiver status -------------------------------------------------------
static int rcv_print_stat(rcv_t *rcv, int nrow)
{
    int n = 2;
    for (int i = 0; i < nrow; i++) {
        printf("%s", ESC_UCUR);
    }
    print_head(rcv);
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_t *ch = rcv->ch[i]->ch;
        if (ch->state != STATE_LOCK || ch->lock * ch->T < MIN_LOCK) continue;
        print_ch_stat(ch);
        n++;
    }
    for (int i = n; i < nrow; i++) {
        printf("%*s\n", NCOL, "");
        n++;
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
    sdr_log(3, "$CH,%.3f,%s,%d,%d,%.1f,%.9f,%.3f,%.3f,%d,%d", ch->time, ch->sig,
        ch->prn, ch->lock, ch->cn0, ch->coff * 1e3, ch->fd, ch->adr,
        ch->nav->count[0], ch->nav->count[1]);
}

// show usage ------------------------------------------------------------------
static void show_usage(void)
{
    for (int i = 0; usage_text[i]; i++) {
        printf("%s\n", usage_text[i]);
    }
    exit(0);
}

// receiver channel thread -----------------------------------------------------
static void *rcv_ch_thread(void *arg)
{
    rcv_ch_t *ch = (rcv_ch_t *)arg;
    int n = ch->ch->N / ch->rcv->N;
    int64_t ix = 0;
    
    while (ch->state) {
        for ( ; ix + 2 * n <= ch->rcv->ix + 1; ix += n) {
            
            // update SDR receiver channel
            sdr_ch_update(ch->ch, ix * T_CYC, ch->rcv->buff[ch->if_ch],
                ch->rcv->len_buff, ch->rcv->N * (ix % MAX_BUFF));
            
            if (ch->ch->state == STATE_LOCK && ix % LOG_CYC == 0) {
                out_log_ch(ch->ch);
            }
            ch->ix = ix; // IF buffer read pointer
        }
        sdr_sleep_msec(TH_CYC);
    }
    return NULL;
}

// new receiver channel --------------------------------------------------------
static rcv_ch_t *rcv_ch_new(const char *sig, int prn, double fs, double fi,
    const double *dop, rcv_t *rcv, int if_ch)
{
    rcv_ch_t *ch = (rcv_ch_t *)sdr_malloc(sizeof(rcv_ch_t));
    double sp = SP_CORR;
    
    if (!(ch->ch = sdr_ch_new(sig, prn, fs, fi, sp, 0, dop[0], dop[1], ""))) {
        sdr_free(ch);
        return NULL;
    }
    ch->if_ch = rcv->fmt ? if_ch : 0;
    ch->rcv = rcv;
    ch->state = 1;
    if (pthread_create(&ch->thread, NULL, rcv_ch_thread, ch)) {
        sdr_ch_free(ch->ch);
        sdr_free(ch);
        return NULL;
    }
    return ch;
}

// stop receiver channel -------------------------------------------------------
static void rcv_ch_stop(rcv_ch_t *ch)
{
    ch->state = 0;
    pthread_join(ch->thread, NULL);
}

// free receiver channel -------------------------------------------------------
static void rcv_ch_free(rcv_ch_t *ch)
{
    sdr_ch_free(ch->ch);
    sdr_free(ch);
}

// generate new receiver -------------------------------------------------------
static rcv_t *rcv_new(char **sigs, const int *prns, int n, double fs,
    const double *fi, const double *dop, int fmt, const int *IQ)
{
    rcv_t *rcv = (rcv_t *)sdr_malloc(sizeof(rcv_t));
    
    rcv->ich = -1;
    rcv->N = (int)(T_CYC * fs);
    rcv->len_buff = rcv->N * MAX_BUFF;
    rcv->fmt = fmt;
    for (int i = 0; i < (fmt ? 2 : 1); i++) {
        rcv->buff[i] = sdr_cpx_malloc(rcv->len_buff);
        rcv->IQ[i] = IQ[i];
    }
    rcv->raw = (int8_t *)sdr_malloc(rcv->N * (fmt || IQ[0] == 1 ? 1 : 2));
    
    for (int i = 0; i < n && rcv->nch < SDR_MAX_NCH; i++) {
        int j = (sdr_sig_freq(sigs[i]) >= 1.5e9) ? 0 : 1; // 0:L1,1:L2/L5/L6
        rcv_ch_t *ch;
        if ((ch = rcv_ch_new(sigs[i], prns[i], fs, fi[j], dop, rcv, j))) {
            ch->ch->no = rcv->nch + 1;
            rcv->ch[rcv->nch++] = ch;
        }
        else {
            fprintf(stderr, "signal / prn error: %s / %d\n", sigs[i], prns[i]);
        }
    }
    return rcv;
}

// stop receiver ---------------------------------------------------------------
static void rcv_stop(rcv_t *rcv)
{
    for (int i = 0; i < rcv->nch; i++) {
        rcv_ch_stop(rcv->ch[i]);
    }
}

// free receiver ---------------------------------------------------------------
static void rcv_free(rcv_t *rcv)
{
    for (int i = 0; i < rcv->nch; i++) {
        rcv_ch_free(rcv->ch[i]);
    }
    for (int i = 0; i < 2; i++) {
        sdr_cpx_free(rcv->buff[i]);
    }
    sdr_free(rcv->raw);
    sdr_free(rcv);
}

// generate lookup table -------------------------------------------------------
static void gen_LUT(const int *IQ, sdr_cpx_t LUT[][256])
{
    static const float val[] = {1.0, 3.0, -1.0, -3.0};
    
    for (int i = 0; i < 256; i++) {
        LUT[0][i][0] = val[(i>>0) & 0x3];
        LUT[0][i][1] = IQ[0] == 1 ? 0.0 : -val[(i>>2) & 0x3];
        LUT[1][i][0] = val[(i>>4) & 0x3];
        LUT[1][i][1] = IQ[1] == 1 ? 0.0 : -val[(i>>6) & 0x3];
    }
}

// read IF data ----------------------------------------------------------------
static int rcv_read_data(rcv_t *rcv, int64_t ix, FILE *fp)
{
    static sdr_cpx_t LUT[2][256] = {{{0}}};
    int i = rcv->N * (ix % MAX_BUFF);
    int N = rcv->N * ((rcv->fmt || rcv->IQ[0] == 1) ? 1 : 2);
    
    if (fread(rcv->raw, N, 1, fp) < 1) {
        return 0; // end of file
    }
    if (rcv->fmt) { // raw SDR device format
        if (LUT[0][0][0] == 0.0) {
            gen_LUT(rcv->IQ, LUT);
        }
        for (int j = 0; j < rcv->N; i++, j++) {
            uint8_t raw = (uint8_t)rcv->raw[j];
            rcv->buff[0][i][0] = LUT[0][raw][0];
            rcv->buff[0][i][1] = LUT[0][raw][1];
            rcv->buff[1][i][0] = LUT[1][raw][0];
            rcv->buff[1][i][1] = LUT[1][raw][1];
        }
    }
    else if (rcv->IQ[0] == 1) { // I
        for (int j = 0; j < rcv->N; i++, j++) {
            rcv->buff[0][i][0] = rcv->raw[j];
            rcv->buff[0][i][1] = 0.0f;
        }
    }
    else { // IQ
        for (int j = 0; j < rcv->N; i++, j++) {
            rcv->buff[0][i][0] =  rcv->raw[j*2  ];
            rcv->buff[0][i][1] = -rcv->raw[j*2+1];
        }
    }
    rcv->ix = ix; // IF buffer write pointer
    return 1;
}

// re-acquisition --------------------------------------------------------------
static int re_acq(rcv_t *rcv, sdr_ch_t *ch)
{
    if (ch->lock * ch->T >= MIN_LOCK && rcv->ix * T_CYC - ch->time + T_REACQ) {
        ch->acq->fd_ext = ch->fd;
        return 1;
    }
    return 0;
}

// assisted-acquisition --------------------------------------------------------
static int assist_acq(rcv_t *rcv, sdr_ch_t *ch)
{
    for (int i = 0; i < rcv->nch; i++) {
        sdr_ch_t *ch_i = rcv->ch[i]->ch;
        if (strcmp(ch->sat, ch_i->sat) || ch_i->state != STATE_LOCK ||
            ch_i->lock * ch_i->T < 2.0) continue;
        ch->acq->fd_ext = ch_i->fd * ch->fc / ch_i->fc;
        return 1;
    }
    return 0;
}

// update signal search channel ------------------------------------------------
static void rcv_update_srch(rcv_t *rcv)
{
    // signal search channel busy ?
    if (rcv->ich >= 0 && rcv->ch[rcv->ich]->ch->state == STATE_SRCH) {
        return;
    }
    for (int i = 0; i < rcv->nch; i++) {
        // search next IDLE channel
        rcv->ich = (rcv->ich + 1) % rcv->nch;
        sdr_ch_t *ch = rcv->ch[rcv->ich]->ch;
        if (ch->state != STATE_IDLE) continue;
        
        // re-acquisition, assisted-acquisition or short code cycle
        if (re_acq(rcv, ch) || assist_acq(rcv, ch) || ch->T <= 5e-3) {
        //if (re_acq(rcv, ch) || assist_acq(rcv, ch)) {
            ch->state = STATE_SRCH;
            break;
        }
    }
}

// wait for receiver channels --------------------------------------------------
static void rcv_wait(rcv_t *rcv)
{
    for (int i = 0; i < rcv->nch; i++) {
        while (rcv->ix + 1 - rcv->ch[i]->ix >= MAX_BUFF - 10) {
            sdr_sleep_msec(TH_CYC);
        }
    }
}

// execute receiver ------------------------------------------------------------
static void rcv_exec(rcv_t *rcv, FILE *fp, const double *tint)
{
    int nrow = 0;
    
    // set all channels search for file input
    if (fp != stdin) {
        for (int i = 0; i < rcv->nch; i++) {
            rcv->ch[i]->ch->state = STATE_SRCH;
        }
    }
    if (tint[0] > 0) {
        printf("%s", ESC_HCUR);
    }
    for (int64_t ix = 0; !intr ; ix++) {
        if (ix % LOG_CYC == 0) {
            out_log_time(ix * T_CYC);
        }
        // read IF data
        if (!rcv_read_data(rcv, ix, fp)) break;
        
        // update signal search channel
        rcv_update_srch(rcv);
        
        // print receiver status
        if (tint[0] > 0.0 && ix % (int)(tint[0] / T_CYC) == 0) {
            nrow = rcv_print_stat(rcv, nrow);
        }
        // suspend data reading for file input
        if (fp != stdin) rcv_wait(rcv);
    }
    // stop receiver
    rcv_stop(rcv);
    
    if (tint[0] > 0.0) {
        rcv_print_stat(rcv, nrow);
        printf("%s", ESC_VCUR);
    }
}

//------------------------------------------------------------------------------
//
//   Synopsis
//
//     pocket_trk [-sig sig -prn prn[,...] ...] [-r] [-toff toff] [-f freq]
//         [-fi freq[,freq]] [-d freq[,freq]] [-IQ|-IQI|-IIQ] [-ti tint]
//         [-log path[,level]] [-nmea path[,tint]] [-rtcm path[,tint]] [-w file]
//         [file]
//
//   Description
//
//     It tracks GNSS signals in digital IF data and decode navigation data in
//     the signals.
//
//   Options ([]: default)
//
//     -sig sig -prn prn[,...] ...
//         A GNSS signal type ID (L1CA, L2CM, ...) and a PRN number list of the
//         signal. For signal type IDs, refer pocket_acq.py manual. The PRN
//         number list shall be PRN numbers or PRN number ranges like 1-32 with
//         the start and the end numbers. They are separated by ",". For
//         GLONASS FDMA signals (G1CA, G2CA), the PRN number is treated as the
//         FCN (frequency channel number). The pair of a signal type ID and a PRN
//         number list can be repeated for multiple GNSS signals to be tracked.
//
//     -r
//         Input raw SDR device data format generated by Pocket SDR RF-frontend.
//         The data can be captured by pocket_dump with the -r option.
//         Without the option, input data are handled as a series of int8_t
//         (I-sampling) or interleaved int8_t (IQ-sampling) as the format.
//
//     -toff toff
//         Time offset from the start of the digital IF data in s. [0.0]
//
//     -f freq
//         Sampling frequency of the digital IF data in MHz. [12.0]
//
//     -fi freq[,freq]
//         IF frequency of the digital IF data in MHz. The IF frequency is equal
//         0, the digital IF data is treated as IQ-sampling without -IQ, -IIQ,
//         or -IQI option (zero-IF). The second frequency is for the CH2
//         (L2/L5/L6) in the raw SDR device data format. [0.0, 0.0]
//
//     -d freq[,freq]
//         Reference and max Doppler frequencies to search the signal in Hz.
//         As default, the range of the Doppler frequencies is -5 kHz to +5 kHz.
//         [0.0, 5000.0]
//
//     -IQ|-IQI|-IIQ
//         IQ-sampling even if the IF frequency is not equal 0. (-IQ: CH1 and
//         CH2, -IQI: CH1 only, -IIQ: CH2 only)
//
//     -ti tint
//         Update interval of the signal tracking status in s. If 0 specified,
//         the signal tracking status suppressed. [0.1]
//
//     -log path[,level]
//         A stream path to write the signal tracking log. The log includes
//         decoded navigation data and code offset, including navigation data
//         decoded. To specify the log level, add ",<level>" after the path.
//         The default log level is 3.
//         The stream path should be one of the followings.
//
//         (1) local file  file path without ':'. The file path can be contain
//             time keywords (%Y, %m, %d, %h, %M) as same as the RTKLIB stream.
//         (2) TCP server  :port
//         (3) TCP client  address:port
//
//     -nmea path[,tint]
//         A stream path to write PVT solutions as NMEA GNRMC, GNGGA and GNGSV
//         sentences. To specify the interval of the PVT solutions in seconds,
//         add ",<tint>" after the path. The default interval is 1 sec. The
//         stream path is as same as the -log option.
//         
//         To be implemented.
//
//     -rtcm path[,tint]
//         A stream path to write raw observation and navigation data as RTCM3.3
//         messages. To specify the interval of the raw observation data in
//         seconds, add ",<tint>" after the path. The stream path is as same
//         as the -log option.
//
//         To be implemented.
//
//     -w file
//         Specify the FFTW wisdowm file. [../python/fftw_wisdom.txt]
//
//     [file]
//         A file path of the input digital IF data. The format should be a
//         series of int8_t (signed byte) for real-sampling (I-sampling),
//         interleaved int8_t for complex-sampling (IQ-sampling) or raw SDR
//         device data. The Pocket SDR RF-frontend and pocket_dump can be used
//         to capture such digital IF data. If the option omitted, the input
//         is taken from stdin.
//
int main(int argc, char **argv)
{
    FILE *fp = stdin;
    int prns[SDR_MAX_NCH], nch = 0, fmt = 0, IQ[2] = {1, 1}, log_lvl = 3;
    double fs = 12e6, fi[2] = {0}, toff = 0.0, tint[3] = {0.1, 1.0, 1.0};
    double dop[2] = {0.0, MAX_DOP};
    char *sig = "L1CA", *sigs[SDR_MAX_NCH];
    char *file = "", paths[3][1024] = {{0}}, *fftw_wisdom = FFTW_WISDOM;
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-sig") && i + 1 < argc) {
            sig = argv[++i];
        }
        else if (!strcmp(argv[i], "-prn") && i + 1 < argc) {
            int nums[SDR_MAX_NCH];
            int n = sdr_parse_nums(argv[++i], nums);
            for (int j = 0; j < n && nch < SDR_MAX_NCH; j++) {
                sigs[nch] = sig;
                prns[nch++] = nums[j];
            }
        }
        else if (!strcmp(argv[i], "-toff") && i + 1 < argc) {
            toff = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            fs = atof(argv[++i]) * 1e6;
        }
        else if (!strcmp(argv[i], "-fi") && i + 1 < argc) {
            sscanf(argv[++i], "%lf,%lf", fi, fi + 1);
            for (int j = 0; j < 2; j++) fi[j] *= 1e6;
        }
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            sscanf(argv[++i], "%lf,%lf", dop, dop + 1);
        }
        else if (!strcmp(argv[i], "-r")) {
            fmt = 1; // raw SDR device format
        }
        else if (!strcmp(argv[i], "-IQ")) {
            IQ[0] = IQ[1] = 2;
        }
        else if (!strcmp(argv[i], "-IIQ")) {
            IQ[0] = 1;
            IQ[1] = 2;
        }
        else if (!strcmp(argv[i], "-IQI")) {
            IQ[0] = 2;
            IQ[1] = 1;
        }
        else if (!strcmp(argv[i], "-ti") && i + 1 < argc) {
            tint[0] = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            fftw_wisdom = argv[++i];
        }
        else if (!strcmp(argv[i], "-log") && i + 1 < argc) {
            char *p = strrchr(argv[++i], ',');
            if (p) sscanf(p, ",%d", &log_lvl);
            strncpy(paths[0], argv[i], p ? MIN(p - argv[i], 1023) : 1023);
        }
        else if (!strcmp(argv[i], "-nmea") && i + 1 < argc) {
            char *p = strrchr(argv[++i], ',');
            if (p) sscanf(p, ",%lf", tint + 1);
            strncpy(paths[1], argv[i], p ? MIN(p - argv[i], 1023) : 1023);
        }
        else if (!strcmp(argv[i], "-rtcm") && i + 1 < argc) {
            char *p = strrchr(argv[++i], ',');
            if (p) sscanf(p, ",%lf", tint + 2);
            strncpy(paths[2], argv[i], p ? MIN(p - argv[i], 1023) : 1023);
        }
        else if (argv[i][0] == '-') {
            show_usage();
        }
        else {
            file = argv[i];
        }
    }
    for (int i = 0; i < 2; i++) {
        IQ[i] = (IQ[i] == 1 && fi[i] > 0.0) ? 1 : 2;
    }
    if (*file) {
        if (!(fp = fopen(file, "rb"))) {
            fprintf(stderr, "file open error: %s\n", file);
            exit(-1);
        }
        fseek(fp, (long)(toff * fs * (fmt || IQ[0] == 1 ? 1 : 2)), SEEK_SET);
    }
#ifdef WIN32 // set binary mode to stdin for Windows
    else {
        _setmode(_fileno(stdin), _O_BINARY);
    }
#endif
    sdr_func_init(fftw_wisdom);
    if (*paths[0]) {
        sdr_log_open(paths[0]);
        sdr_log_level(log_lvl);
    }
    signal(SIGTERM, sig_func);
    signal(SIGINT, sig_func);
    
    uint32_t tt = sdr_get_tick();
    sdr_log(3, "$LOG,%.3f,%s,%d,START FILE=%s FS=%.3f FMT=%d IQ=%d,%d", 0.0, "",
        0, file, fs * 1e-6, fmt, IQ[0], IQ[1]);
    
    // generate new receiver
    rcv_t *rcv = rcv_new(sigs, prns, nch, fs, fi, dop, fmt, IQ);
    
    // execute receiver
    rcv_exec(rcv, fp, tint);
    
    // free receiver
    rcv_free(rcv);
    
    tt = sdr_get_tick() - tt;
    sdr_log(3, "$LOG,%.3f,%s,%d,END FILE=%s", tt * 1e-3, "", 0, file);
    if (tint[0] > 0.0) {
        printf("  TIME(s) = %.3f\n", tt * 1e-3);
    }
    sdr_log_close();
    fclose(fp);
    return 0;
}
