//
//  Pocket SDR C AP - GNSS Signal Tracking
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-07-05  1.0  port pocket_trk.py to C.
//
#include "pocket_sdr.h"

// constants --------------------------------------------------------------------
#define SP_CORR    0.5        // default correlator spacing (chip) 
#define MAX_DOP    5000.0     // default max Doppler for acquisition (Hz) 
#define CYC_SRCH   10.0       // signal search cycle (s)
#define MAX_BUFF   32         // max number of IF data buffer
#define NCORR_PLOT 40         // numober of additional correlators for plot
#define ESC_UP     "\033[%dF" // ANSI escape cursor up
#define ESC_COL    "\033[34m" // ANSI escape color blue
#define ESC_RES    "\033[0m"  // ANSI escape reset

#define FFTW_WISDOM "../python/fftw_wisdom.txt"

// read IF data -----------------------------------------------------------------
static int read_data(FILE *fp, int N, int IQ, sdr_cpx_t * buff, int ix)
{
    int8_t *raw = (int8_t *)sdr_malloc(N * IQ);
    
    if (fread(raw, N * IQ, 1, fp) < 1) {
        sdr_free(raw);
        return 0;
    }
    if (IQ == 1) { // I
        for (int i = 0; i < N; i++) {
            buff[ix+i][0] = raw[i];
            buff[ix+i][1] = 0.0;
        }
    }
    else { // IQ (Q sign inverted in MAX2771)
        for (int i = 0; i < N; i++) {
            buff[ix+i][0] =  raw[i*2  ];
            buff[ix+i][1] = -raw[i*2+1];
        }
    }
    sdr_free(raw);
    return 1;
}
// update receiver channels state -----------------------------------------------
static int update_state(sdr_ch_t **ch, int n, int ix)
{
    for (int i = 0; i < n; i++) {
        ix = (ix + 1) % n;
        if (!strcmp(ch[ix]->state, "IDLE")) {
            ch[ix]->state = "SRCH";
            break;
        }
    }
    return ix;
}

// C/N0 bar ---------------------------------------------------------------------
static void cn0_bar(float cn0, char *bar)
{
    int n = (int)((cn0 - 30.0) / 1.5);
    bar[0] = '\0';
    for (int i = 0; i < n && i < 13; i++) {
        sprintf(bar + i, "|");
    }
}

// receiver channel sync status -------------------------------------------------
static void sync_stat(const sdr_ch_t *ch, char *stat)
{
    sprintf(stat, "%s%s%s%s", (ch->trk->sec_sync > 0) ? "S" : "-",
        (ch->nav->ssync > 0) ? "B" : "-", (ch->nav->fsync > 0) ? "F" : "-",
        (ch->nav->rev) ? "R" : "-");
}

// print receiver channels status header ----------------------------------------
static void print_head(void)
{
    printf("%9s %5s %3s %5s %8s %4s %-12s %10s %7s %11s %4s %4s %4s %4s %3s\n",
        "TIME(s)", "SIG", "PRN", "STATE", "LOCK(s)", "C/N0", "(dB-Hz)",
        "COFF(ms)", "DOP(Hz)", "ADR(cyc)", "SYNC", "#NAV", "#ERR", "#LOL",
        "NER");
}

// print receiver channel status -----------------------------------------------
static void print_stat(const sdr_ch_t *ch)
{
    char bar[16], stat[16];
    cn0_bar(ch->cn0, bar);
    sync_stat(ch, stat);
    printf("%s%9.2f %5s %3d %5s %8.2f %4.1f %-13s%10.7f %7.1f %11.1f %s %4d %4d %4d %3d%s\n",
        !strcmp(ch->state, "LOCK") ? ESC_COL : "", ch->time, ch->sig, ch->prn,
        ch->state, ch->lock * ch->T, ch->cn0, bar, ch->coff * 1e3, ch->fd,
        ch->adr, stat, ch->nav->count[0], ch->nav->count[1], ch->lost,
        ch->nav->nerr, !strcmp(ch->state, "LOCK") ? ESC_RES : "");
}

// print receiver channels status ----------------------------------------------
static int print_stats(sdr_ch_t **ch, int n, int ncol)
{
    if (ncol > 0) {
        printf(ESC_UP, ncol);
    }
    for (int i = 0; i < n; i++) {
        print_stat(ch[i]);
    }
    fflush(stdout);
    return n;
}

// log receiver channel status -------------------------------------------------
static void log_stat(const sdr_ch_t *ch)
{
    sdr_log(3, "$CH,%.3f,%s,%d,%d,%.1f,%.9f,%.3f,%.3f,%d,%d", ch->time, ch->sig,
        ch->prn, ch->lock, ch->cn0, ch->coff * 1e3, ch->fd, ch->adr,
        ch->nav->count[0], ch->nav->count[1]);
}

// show usage -------------------------------------------------------------------
static void show_usage(void)
{
    printf("Usage: pocket_trk [-sig sig] [-prn prn[,...]] [-toff toff] [-f freq]\n");
    printf("       [-fi freq] [-IQ] [-ti tint] [-log path] [-out path] [-q] [file]\n");
    exit(0);
}

//-------------------------------------------------------------------------------
//
//   Synopsis
// 
//     pocket_trk [-sig sig] [-prn prn[,...]] [-toff toff] [-f freq]
//         [-fi freq] [-IQ] [-ti tint] [-log path] [-out path] [-q] [file]
// 
//   Description
// 
//     It tracks GNSS signals in digital IF data and decode navigation data in
//     the signals.
//     If single PRN number by -prn option, it plots correlation power and
//     correlation shape of the specified GNSS signal. If multiple PRN numbers
//     specified by -prn option, it plots C/N0 for each PRN.
// 
//   Options ([]: default)
//  
//     -sig sig
//         GNSS signal type ID (L1CA, L2CM, ...). Refer pocket_acq.py manual for
//         details. [L1CA]
// 
//     -prn prn[,...]
//         PRN numbers of the GNSS signal separated by ','. A PRN number can be a
//         PRN number range like 1-32 with start and end PRN numbers. For GLONASS
//         FDMA signals (G1CA, G2CA), the PRN number is treated as FCN (frequency
//         channel number). [1]
// 
//     -toff toff
//         Time offset from the start of digital IF data in s. [0.0]
// 
//     -f freq
//         Sampling frequency of digital IF data in MHz. [12.0]
//
//     -fi freq
//         IF frequency of digital IF data in MHz. The IF frequency is equal 0,
//         the IF data is treated as IQ-sampling without -IQ option (zero-IF).
//         [0.0]
//
//     -IQ
//         IQ-sampling even if the IF frequency is not equal 0.
//
//     -ti tint
//         Update interval of signal tracking status, plot and log in s. [0.05]
//
//     -log path
//         Log stream path to write signal tracking status. The log includes
//         decoded navigation data and code offset, including navigation data
//         decoded. The stream path should be one of the followings.
//         (1) local file  file path without ':'. The file path can be contain
//             time keywords (%Y, %m, %d, %h, %M) as same as RTKLIB stream.
//         (2) TCP server  :port
//         (3) TCP client  address:port
//
//     -out path
//         Output stream path to write special messages. Currently only UBX-RXM-
//         QZSSL6 message is supported as a special message,
//
//     -q
//         Suppress showing signal tracking status.
//
//     [file]
//         File path of the input digital IF data. The format should be a series of
//         int8_t (signed byte) for real-sampling (I-sampling) or interleaved int8_t
//         for complex-sampling (IQ-sampling). PocketSDR and AP pocket_dump can be
//         used to capture such digital IF data. If the option omitted, the input
//         is taken from stdin.
//
int main(int argc, char **argv)
{
    FILE *fp = stdin;
    sdr_ch_t *ch[SDR_MAX_NPRN];
    int prns[SDR_MAX_NPRN], nprn = 0, IQ = 1, log_lvl = 4, quiet = 0, ix = 0;
    double fs = 12e6, fi = 0.0, toff = 0.0, tint = 0.1;
    char *sig = "L1CA", *file = "", *log_file = "";
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-sig") && i + 1 < argc) {
            sig = argv[++i];
        }
        else if (!strcmp(argv[i], "-prn") && i + 1 < argc) {
            nprn = sdr_parse_nums(argv[++i], prns);
        }
        else if (!strcmp(argv[i], "-toff") && i + 1 < argc) {
            toff = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            fs = atof(argv[++i]) * 1e6;
        }
        else if (!strcmp(argv[i], "-fi") && i + 1 < argc) {
            fi = atof(argv[++i]) * 1e6;
        }
        else if (!strcmp(argv[i], "-IQ")) {
            IQ = 2;
        }
        else if (!strcmp(argv[i], "-ti") && i + 1 < argc) {
            tint = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-log") && i + 1 < argc) {
            log_file = argv[++i];
        }
        else if (!strcmp(argv[i], "-q")) {
            quiet = 1;
        }
        else if (argv[i][0] == '-') {
            show_usage();
        }
        else {
            file = argv[i];
        }
    }
    double T = sdr_code_cyc(sig); // code cycle
    if (T <= 0.0) {
        fprintf(stderr, "Invalid signal %s.\n", sig);
        exit(-1);
    }
    IQ = (IQ == 1 && fi > 0.0) ? 1 : 2;
    
    if (*file) {
        if (!(fp = fopen(file, "rb"))) {
            fprintf(stderr, "file open error: %s\n", file);
            exit(-1);
        }
        fseek(fp, (long)(toff * fs * IQ), SEEK_SET);
    }
    sdr_func_init(FFTW_WISDOM);
    
    for (int i = 0; i < nprn; i++) {
        ch[i] = sdr_ch_new(sig, prns[i], fs, fi, MAX_DOP, SP_CORR, 0, "");
        ch[i]->state = "SRCH";
    }
    if (!quiet) {
        print_head();
    }
    if (*log_file) {
        sdr_log_open(log_file);
        sdr_log_level(log_lvl);
    }
    int N = (int)(T * fs), ncol = 0;
    sdr_cpx_t *buff = sdr_cpx_malloc(N * (MAX_BUFF + 1));
    
    uint32_t tick = sdr_get_tick();
    sdr_log(3, "$LOG,%.3f,%s,%d,START FILE=%s FS=%.3f FI=%.3f IQ=%d TOFF=%.3f",
        0.0, "", 0, file, fs * 1e-6, fi * 1e-6, IQ, toff);
    
    for (int i = 0; ; i++) {
        double time_rcv = toff + T * (i - 1); // receiver time
        
        // read IF data to buffer
        if (!read_data(fp, N, IQ, buff, N * (i % MAX_BUFF))) {
            break;
        }
        if (i == 0) {
            continue;
        }
        else if (i % MAX_BUFF == 0) {
            memcpy(buff + N * MAX_BUFF, buff, sizeof(sdr_cpx_t) * N);
        }
        // update receiver channels
        for (int j = 0; j < nprn; j++) {
            sdr_ch_update(ch[j], time_rcv, buff, N * ((i - 1) % MAX_BUFF));
        }
        // update receiver channels state
        if (i % (int)(CYC_SRCH / T) == 0) {
            ix = update_state(ch, nprn, ix);
        }
        if ((i - 1) % (int)(tint / T) != 0) {
            continue;
        }
        // print receiver channels status
        if (!quiet) {
            ncol = print_stats(ch, nprn, ncol);
        }
        // update log
        double t[6] = {0};
        sdr_get_time(t);
        sdr_log(3, "$TIME,%.0f,%.0f,%.0f,%.0f,%.0f,%.6f", t[0], t[1], t[2],
            t[3], t[4], t[5]);
        
        for (int j = 0; j < nprn; j++) {
            if (!strcmp(ch[j]->state, "LOCK")) {
                log_stat(ch[j]);
            }
        }
    }
    double tt = (sdr_get_tick() - tick) * 1e-3;
    sdr_log(3, "$LOG,%.3f,%s,%d,END FILE=%s", tt, "", 0, file);
    
    if (!quiet) {
        printf("  TIME(s) = %.3f\n", tt);
    }
    for (int i = 0; i < nprn; i++) {
        sdr_ch_free(ch[i]);
    }
    sdr_cpx_free(buff);
    if (*file) {
        fclose(fp);
    }
    sdr_log_close();
    return 0;
}
