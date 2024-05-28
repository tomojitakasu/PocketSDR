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
//
#include <math.h>
#include <signal.h>
#include "pocket_sdr.h"
#include "pocket_dev.h"

// constants and macros ---------------------------------------------------------
#define TRACE_LEVEL 2           // debug trace level
#define FFTW_WISDOM "../python/fftw_wisdom.txt"

// usage text ------------------------------------------------------------------
static const char *usage_text[] = {
    "Usage: pocket_trk [-sig sig -prn prn[,...] ...] [-toff toff] [-f freq]",
    "    [-fi freq] [-IQ] [-ti tint] [-p bus,[,port] [-c conf_file]",
    "    [-log path] [-nmea path] [-rtcm path] [-w file] [file]", NULL
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

// set IF channel and IF frequency ---------------------------------------------
static void set_if_ch(int fmt, const double *fo, const char *sig, int *if_ch,
    double *fi)
{
    int nch = fmt == SDR_FMT_RAW8 ? 2 : (fmt == SDR_FMT_RAW16 ? 4 : 8);
    double freq = sdr_sig_freq(sig);
    *if_ch = 0;
    for (int i = 1; i < nch; i++) {
        if (fabs(fo[i] - freq) < fabs(fo[*if_ch] - freq)) *if_ch = i;
    }
    *fi = freq - fo[*if_ch];
}

//------------------------------------------------------------------------------
//
//   Synopsis
//
//     pocket_trk [-sig sig -prn prn[,...] ...] [-toff toff] [-f freq]
//         [-fi freq] [-IQ] [-ti tint] [-p bus,[,port] [-c conf_file]
//         [-log path] [-nmea path] [-rtcm path] [-w file] [file]
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
//     -toff toff
//         Time offset from the start of the digital IF data in s. [0.0]
//
//     -f freq
//         Sampling frequency of the digital IF data in MHz. [12.0]
//
//     -fi freq
//         IF frequency of digital IF data in MHz. The IF frequency is equal 0,
//         the IF data is treated as IQ-sampling without -IQ option (zero-IF).
//         [0.0]
//
//     -IQ
//        IQ-sampling even if the IF frequency is not equal 0.
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
//     -w file
//         Specify the FFTW wisdowm file. [../python/fftw_wisdom.txt]
//
//     [file]
//         A file path of the input digital IF data. The format should be a
//         series of int8_t (signed byte) for real-sampling (I-sampling),
//         interleaved int8_t for complex-sampling (IQ-sampling).
//         The Pocket SDR FE deveice and pocket_dump can be used to capture
//         such digital IF data.
//
//         If the file path omitted, the input is taken from a Pocket SDR FE
//         device directly. In this case, the sampling frequency, the sampling
//         types of IF data, the RF channel assignments and the IF freqencies
//         for each signals are automatically configured according to the
//         device information. In this case, the specified options -toff,
//         -f, -fi and -IQ are ignored.
//
int main(int argc, char **argv)
{
    void *dp = NULL;
    int prns[SDR_MAX_NCH], if_ch[SDR_MAX_NCH] = {0}, nch = 0;
    int fmt = SDR_FMT_INT8, IQ[4] = {2, 2, 2, 2};
    int dev_type = SDR_DEV_FILE, bus = -1, port = -1;
    double fs = 12e6, fif = 0.0, fi[SDR_MAX_NCH] = {0}, toff = 0.0, tint = 0.1;
    const char *sig = "L1CA", *sigs[SDR_MAX_NCH];
    const char *file = "", *fftw_wisdom = FFTW_WISDOM, *conf_file = "";
    const char *paths[3] = {"", "", ""}, *debug_file = "";
    
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
            fif = atof(argv[++i]) * 1e6;
            IQ[0] = 1;
        }
        else if (!strcmp(argv[i], "-IQ")) {
            IQ[0] = 2;
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
        else if (!strcmp(argv[i], "-log") && i + 1 < argc) {
            paths[0] = argv[++i];
        }
        else if (!strcmp(argv[i], "-nmea") && i + 1 < argc) {
            paths[1] = argv[++i];
        }
        else if (!strcmp(argv[i], "-rtcm") && i + 1 < argc) {
            paths[2] = argv[++i];
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
    
    if (*file) { // input from IF data file
        FILE *fp = fopen(file, "rb");
        if (!fp) {
            fprintf(stderr, "file open error: %s\n", file);
            exit(-1);
        }
        dp = (void *)fp;
        fmt = (IQ[0] == 1) ? SDR_FMT_INT8 : SDR_FMT_CPX16;
        for (int i = 0; i < nch; i++) fi[i] = fif;
        fseek(fp, (long)(toff * fs * (IQ[0] == 1 ? 1 : 2)), SEEK_SET);
    }
    else { // input from the SDR USB device
        dev_type = SDR_DEV_USB;
        if (*conf_file) {
            (void)sdr_write_settings(conf_file, bus, port, 0);
            sdr_sleep_msec(100);
        }
        sdr_dev_t *dev = sdr_dev_open(bus, port);
        if (!dev) {
            exit(-1);
        }
        dp = (void *)dev;
        fmt = dev->fmt;
        fs = dev->fs;
        memcpy(IQ, dev->IQ, sizeof(int) * 4);
        
        // set IF channel and IF frequency
        for (int i = 0; i < nch; i++) {
            set_if_ch(fmt, dev->fo, sigs[i], if_ch + i, fi + i);
        }
    }
    signal(SIGTERM, sig_func);
    signal(SIGINT, sig_func);
    
    uint32_t tt = sdr_get_tick();
    
    // new and start SDR receiver
    sdr_rcv_t *rcv = sdr_rcv_new(sigs, prns, if_ch, fi, nch, fs, fmt, IQ);
    sdr_rcv_start(rcv, dev_type, dp, paths, tint);
    
    while (!intr && rcv->state) { // wait for interrupt or file end
        sdr_sleep_msec(10);
    }
    // stop and free SDR receiver
    sdr_rcv_stop(rcv);
    sdr_rcv_free(rcv);
    
    if (dev_type == SDR_DEV_USB) {
        sdr_dev_close((sdr_dev_t *)dp);
    }
    else if (*file) {
        fclose((FILE *)dp);
    }
    if (tint > 0.0) {
        printf("  TIME(s) = %.3f\n", (sdr_get_tick() - tt) * 1e-3);
    }
    if (*debug_file) {
        traceclose();
    }
    return 0;
}
