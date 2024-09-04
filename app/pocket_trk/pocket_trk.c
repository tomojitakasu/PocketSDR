//
//  Pocket SDR C AP -  GNSS Signal Tracking and PVT Generation.
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
//  2024-02-08  1.8  separate SDR receiver functions to sdr_rcv.c
//  2024-02-20  1.9  modify -fi option, add -I option
//  2024-04-04  1.10 add -u, -p and -c options
//  2024-04-28  1.11 add -r, -rx, -s options, implement -rtcm, -nmea options
//  2024-05-28  1.12 delete -u, -r, -rx, -s options, add -IQ option
//                   delete input from stdin
//  2024-07-02  1.8  add -fo, -raw option, delete -fi option
//                   support tag file input for auto-configuration
//
#include <math.h>
#include <signal.h>
#include "pocket_sdr.h"

// constants and macros ---------------------------------------------------------
#define TRACE_LEVEL 2           // debug trace level
#define FFTW_WISDOM "../python/fftw_wisdom.txt"
#define NUM_COL    110          // number of channel status columns
#define MAX_ROW    108          // max number of channel status rows
#define ESC_COL    "\033[34m"   // ANSI escape color blue
#define ESC_RES    "\033[0m"    // ANSI escape reset
#define ESC_UCUR   "\033[A"     // ANSI escape cursor up
#define ESC_VCUR   "\033[?25h"  // ANSI escape show cursor
#define ESC_HCUR   "\033[?25l"  // ANSI escape hide cursor

// usage text ------------------------------------------------------------------
static const char *usage_text[] = {
    "Usage: pocket_trk [-sig sig -prn prn[,...] ...] [-fmt {INT8|INT8X2|RAW8|RAW16}]",
    "       [-f freq] [-fo freq[,...]] [-IQ {1|2}[,...]] [-toff toff] [-ti tint]",
    "       [-p bus,[,port] [-c conf_file] [-log path] [-nmea path] [-rtcm path]",
    "       [-raw path] [-w file] [file]", NULL
};

// interrupt flag --------------------------------------------------------------
static volatile uint8_t intr = 0;

// signal handler --------------------------------------------------------------
static void sig_func(int sig)
{
    intr = 1;
    signal(sig, sig_func);
}

// show usage ------------------------------------------------------------------
static void show_usage(void)
{
    for (int i = 0; usage_text[i]; i++) {
        printf("%s\n", usage_text[i]);
    }
    exit(0);
}

// print receiver status -------------------------------------------------------
static int print_rcv_stat(sdr_rcv_t *rcv, int nrow)
{
    char *stat, *p, *q;
    int n = 0;
    
    for (int i = 0; i < nrow; i++) {
        printf("%s", ESC_UCUR);
    }
    // get SDR receiver channel status
    stat = sdr_rcv_ch_stat(rcv, "ALL", 0);
    
    for (p = q = stat; (q = strchr(p, '\n')); p = q + 1) {
        if (n < MAX_ROW) {
            printf("%s%.*s%s", n < 2 ? "" : ESC_COL, (int)(q - p) + 1, p,
                ESC_RES);
            n++;
        }
        else if (n == MAX_ROW) {
            printf("... ..\n");
            n++;
        }
    }
    for ( ; n < nrow; n++) {
        printf("%*s\n", NUM_COL, "");
    }
    fflush(stdout);
    return n;
}

//------------------------------------------------------------------------------
//
//   Synopsis
//
//     pocket_trk [-sig sig -prn prn[,...] ...] [-fmt {INT8|INT8X2|RAW8|RAW16}]
//         [-f freq] [-fo freq[,...]] [-IQ {1|2}[,...]] [-toff toff] [-ti tint]
//         [-p bus,[,port] [-c conf_file] [-log path] [-nmea path] [-rtcm path]
//         [-raw path] [-w file] [file]
//
//   Description
//
//     It searchs and tracks GNSS signals in the input digital IF data, extract
//     observation data, decode navigation data and generate PVT solutions. The
//     observation and navigation data can be output as a RTCM3 stream. The PVT
//     solutions can be output as a NMEA stream. The observation data and
//     raw navigation data and some event logs can be output as a log stream.
//
//   Options ([]: default)
//
//     -sig sig [-fi freq] -prn prn[,...] ...
//         A GNSS signal type ID (L1CA, L2CM, ...) and a PRN number list of the
//         signal. For signal type IDs, refer pocket_acq.py manual. The PRN
//         number list shall be PRN numbers or PRN number ranges like 1-32 with
//         the start and the end numbers. They are separated by ",". For
//         GLONASS FDMA signals (G1CA, G2CA), the PRN number is treated as the
//         FCN (frequency channel number). The pair of a signal type ID and a PRN
//         number list can be repeated for multiple GNSS signals to be tracked.
//
//     -fmt {INT8|INT8X2|RAW8|RAW16}
//         Specify IF data format as follows: INT8 = int8 (I-sampling), INT8X2 =
//         interleaved int8 (IQ-sampling), RAW8 = Pocket SDR FE 2CH raw (packed
//         8 bits), RAW16 = Pocket SDR FE 4CH raw (packed 16 bits) [INT8X2]
//
//     -f freq
//         Specify the sampling frequency of the IF data in MHz. [12.0]
//
//     -fo freq[,...]
//         Specify LO frequency for each RF channel in MHz. In case of the
//         IF data format as RAW8 or RAW16, multiple (2 or 4) frequencies have
//         to be specified separated by ",". If the LO frequency is specified
//         as 0, the IF frequency is assumed as 0 for IQ-sampling and 1/2 of
//         the sampling frequency for I-sampling. [0,0,0,0]
//
//     -IQ {1|2}[,...]
//         Specify the sampling type (1 = I-sampline, 2 = IQ-sampling) for each
//         RF channel separated by "," in case of the IF data foramt as RAW8 or
//         RAW16. [2,2,2,2]
//
//     -toff toff
//         Time offset from the start of the IF data in s. [0.0]
//
//     -tscale scale
//         Time scale to replay the IF data file. [1.0]
//
//     -ti tint
//         Update interval of the signal tracking status in seconds. If 0
//         specified, the signal tracking status is suppressed. [0.1]
//
//     -p bus[,port]
//         USB bus and port number of the Pocket SDR FE device in case of IF data
//         input from the device.
//
//     -c conf_file
//         Configure the Pocket SDR FE device with a device configuration file
//         before signal acquisition and tracking.
//
//     -log path
//         A stream path to write the signal tracking log. The log includes
//         observation data, navigation data, PVT solutions and some event logs.
//         The stream path should be one of the followings.
//
//         (1) local file file path without ':'. The file path can be contain
//             time keywords (%Y, %m, %d, %h, %M) as same as the RTKLIB stream.
//         (2) TCP server  :port
//         (3) TCP client  address:port
//
//     -nmea path
//         A stream path to write PVT solutions as NMEA GNRMC, GNGGA and GNGSV
//         sentences. The stream path is as same as the -log option.
//         
//     -rtcm path
//         A stream path to write raw observation and navigation data as RTCM3.3
//         messages. The stream path is as same as the -log option.
//
//     -raw path
//         A stream path to write raw IF data. The stream path is as same as the
//         -log option.
//
//     -w file
//         Specify the FFTW wisdowm file. [../python/fftw_wisdom.txt]
//
//     [file]
//         A file path of the input IF data. The Pocket SDR FE deveice and
//         pocket_dump can be used to capture such digitized IF data.
//
//         If the tag file <file>.tag of the input IF data exists, the format,
//         the sampling frequency, the LO frequencies and the sampling types
//         are automatically recognized by the tag file and the options -fmt,
//         -f, -fo, and -IQ are ignored.
//
//         If the file path omitted, the input is taken from a Pocket SDR FE
//         device directly. In this case, the sampling frequency, the sampling
//         types of IF data, the RF channel assignments and the IF freqencies
//         for each signals are automatically configured according to the
//         device information.
//
int main(int argc, char **argv)
{
    sdr_rcv_t *rcv;
    int prns[SDR_MAX_NCH], nch = 0, fmt = SDR_FMT_INT8X2;
    int IQ[SDR_MAX_RFCH] = {2, 2, 2, 2, 2, 2, 2, 2};
    int dev_type = SDR_DEV_FILE, bus = -1, port = -1, nrow = 0;
    double fs = 12.0, fo[SDR_MAX_RFCH] = {0}, toff = 0.0, tscale = 1.0;
    double tint = 0.1;
    const char *sig = "L1CA", *sigs[SDR_MAX_NCH];
    const char *file = "", *fftw_wisdom = FFTW_WISDOM, *conf_file = "";
    const char *paths[4] = {"", "", "", ""}, *debug_file = "";
    
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
        else if (!strcmp(argv[i], "-tscale") && i + 1 < argc) {
            tscale = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-fmt") && i + 1 < argc) {
            const char *format = argv[++i];
            if      (!strcmp(format, "INT8"  )) fmt = SDR_FMT_INT8;
            else if (!strcmp(format, "INT8X2")) fmt = SDR_FMT_INT8X2;
            else if (!strcmp(format, "RAW8"  )) fmt = SDR_FMT_RAW8;
            else if (!strcmp(format, "RAW16" )) fmt = SDR_FMT_RAW16;
            else if (!strcmp(format, "RAW16I")) fmt = SDR_FMT_RAW16I;
            else {
                fprintf(stderr, "unrecognized format: %s\n", format);
                exit(-1);
            }
        }
        else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            fs = atof(argv[++i]) * 1e6;
        }
        else if (!strcmp(argv[i], "-fo") && i + 1 < argc) {
            int n = sscanf(argv[++i], "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", fo,
                fo + 1, fo + 2, fo + 3, fo + 4, fo + 5, fo + 6, fo + 7);
            for (int j = 0; j < n; j++) fo[j] *= 1e6;
        }
        else if (!strcmp(argv[i], "-IQ") && i + 1 < argc) {
            sscanf(argv[++i], "%d,%d,%d,%d,%d,%d,%d,%d", IQ, IQ + 1, IQ + 2,
                IQ + 3, IQ + 4, IQ + 5, IQ + 6, IQ + 7);
        }
        else if (!strcmp(argv[i], "-ti") && i + 1 < argc) {
            tint = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            sscanf(argv[++i], "%d,%d", &bus, &port);
        }
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            conf_file = argv[++i];
        }
        else if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            fftw_wisdom = argv[++i];
        }
        else if (!strcmp(argv[i], "-nmea") && i + 1 < argc) {
            paths[0] = argv[++i];
        }
        else if (!strcmp(argv[i], "-rtcm") && i + 1 < argc) {
            paths[1] = argv[++i];
        }
        else if (!strcmp(argv[i], "-log") && i + 1 < argc) {
            paths[2] = argv[++i];
        }
        else if (!strcmp(argv[i], "-raw") && i + 1 < argc) {
            paths[3] = argv[++i];
        }
        else if (!strcmp(argv[i], "-debug") && i + 1 < argc) {
            debug_file = argv[++i];
        }
        else if (argv[i][0] == '-') {
            show_usage();
        }
        else {
            file = argv[i];
        }
    }
    if (*debug_file) {
        traceopen(debug_file);
        tracelevel(TRACE_LEVEL);
    }
    sdr_func_init(fftw_wisdom);
    
    signal(SIGTERM, sig_func);
    signal(SIGINT, sig_func);
#ifndef WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    uint32_t tt = sdr_get_tick();
    
    if (*file) {
        rcv = sdr_rcv_open_file(sigs, prns, nch, fmt, fs, fo, IQ, toff,
            tscale, file, paths);
    }
    else {
        rcv = sdr_rcv_open_dev(sigs, prns, nch, bus, port, conf_file, paths);
    }
    if (!rcv) {
        return -1;
    }
    if (tint > 0.0) {
        printf("%s", ESC_HCUR);
    }
    while (!intr && rcv->state) { // wait for interrupt or file end
        if (tint > 0.0) {
            nrow = print_rcv_stat(rcv, nrow);
        }
        sdr_sleep_msec(tint > 0.0 ? (int)(tint * 1000) : 100);
    }
    if (tint > 0.0) {
        print_rcv_stat(rcv, nrow);
        printf("  TIME(s) = %.3f\n", (sdr_get_tick() - tt) * 1e-3);
        printf("%s", ESC_VCUR);
    }
    sdr_rcv_close(rcv);
    
    if (*debug_file) {
        traceclose();
    }
    return 0;
}
