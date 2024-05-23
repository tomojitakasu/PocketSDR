//
//  Pocket SDR - Dump digital IF data of SDR device.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2021-10-08  0.1  new
//  2021-12-01  1.0  add default output file paths
//  2022-01-04  1.1  data capture cycle: 1 -> 50 (ms)
//  2022-01-06  1.2  add output to stdout with "-" as file path
//                   add option -q to suppress output of status
//  2022-08-31  1.3  support max 8 CH inputs
//                   DATA_CYC: 50 -> 10 (ms)
//  2023-12-25  1.4  insert wait after writing receiver settings
//  2023-12-28  1.5  set binary mode to stdout for Windows
//  2023-04-28  1.6  support Pocket SDR FE 4CH
//
#include <signal.h>
#include <time.h>
#ifdef WIN32
#include <fcntl.h>
#endif
#include "pocket_sdr.h"
#include "pocket_dev.h"

// constants and macros --------------------------------------------------------
#define PROG_NAME       "pocket_dump" // program name
#define DATA_CYC        10      // data capture cycle (ms)
#define STAT_CYC        50      // status update cycle (ms)
#define RATE_CYC        1000    // data rate update cycle (ms)
#define F2BIT(buff,ch,IQ) (((buff)>>(4*(ch)+2*(IQ)))&3)

// interrupt flag --------------------------------------------------------------
static volatile uint8_t intr = 0;

// signal handler --------------------------------------------------------------
static void sig_func(int sig)
{
    intr = 1;
    signal(sig, sig_func);
}

// print usage -----------------------------------------------------------------
static void print_usage(void)
{
    printf("Usage: %s [-t tsec] [-r] [-p bus[,port]] [-c conf_file] [-q]\n"
        "    [file [file ...]]\n", PROG_NAME);
    exit(0);
}

// generate lookup table -------------------------------------------------------
static void gen_LUT(int8_t LUT[][256])
{
    static const int8_t val[] = {1, 3, -1, -3}; // sign + magnitude
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 256; j++) {
            LUT[i][j] = val[(j >> (i * 2)) & 3];
        }
    }
}

// write IF data to file -------------------------------------------------------
static int write_file(int fmt, const uint8_t *buff, int size, int ch, int IQ,
    FILE *fp)
{
    static int8_t LUT[4][256] = {{0}};
    
    if (!fp) return 0;
    
    if (fmt == 0) { // output raw
        return (int)fwrite(buff, 1, size, fp);
    }
    if (!LUT[0][0]) {
        gen_LUT(LUT);
    }
    int8_t data[size * IQ];
    
    if (fmt == SDR_FMT_RAW8) { // packed 8(4x2) bits raw
        int pos = ch * 2;
        for (int i = 0; i < size; i++) {
            if (IQ == 1) {
                data[i] = LUT[pos][buff[i]];
            }
            else {
                data[i*2  ] = LUT[pos  ][buff[i]];
                data[i*2+1] = LUT[pos+1][buff[i]];
            }
        }
    }
    else if (fmt == SDR_FMT_RAW16) { // packed 16(4x4) bits raw
        int pos = ch % 2 * 2;
        for (int i = 0, j = ch / 2; i < size; i++, j += 2) {
            if (IQ == 1) {
                data[i] = LUT[pos][buff[j]];
            }
            else {
                data[i*2  ] = LUT[pos  ][buff[j]];
                data[i*2+1] = LUT[pos+1][buff[j]];
            }
        }
    }
    else { // packed 16(2x8) bits raw
        int pos = ch % 4;
        for (int i = 0, j = ch / 4; i < size; i++, j += 2) {
            data[i] = LUT[pos][buff[j]];
        }
    }
    return (int)fwrite(data, 1, size * IQ, fp);
}

// print header ----------------------------------------------------------------
static void print_head(int nch, FILE **fp)
{
    fprintf(stderr, "%9s ", "TIME(s)");
    for (int i = 0; i < nch; i++) {
        if (fp[i]) fprintf(stderr, " %3s   CH%d(Bytes)", "T", i + 1);
    }
    fprintf(stderr, " %12s\n", "RATE(Ks/s)");
}

// print status ----------------------------------------------------------------
static void print_stat(int nch, const int *IQ, FILE **fp, double time,
    const double *byte, double rate)
{
    static const char *str_IQ[] = {"-", "I", "IQ", "I"};
    
    fprintf(stderr, "%9.1f ", time);
    for (int i = 0; i < nch; i++) {
        if (!fp[i]) continue;
        fprintf(stderr, " %3s %12.0f", str_IQ[IQ[i]], byte[i]);
    }
    fprintf(stderr, " %12.1f\r", rate * 1e-3);
    fflush(stderr);
}

// dump digital IF data --------------------------------------------------------
static void dump_data(sdr_dev_t *dev, double tsec, int quiet, int fmt,
    double fs, int nch, const double *fo, const int *IQ, FILE **fp)
{
    double time = 0.0, time_p = 0.0, sample = 0.0, sample_p = 0.0;
    double rate = 0.0, byte[SDR_MAX_CH] = {0};
    int ns = (fmt == 0 || fmt == SDR_FMT_RAW8) ? 1 : 2;
    uint8_t buff[SDR_SIZE_BUFF * ns];
    
    if (!quiet) {
        print_head(nch, fp);
    }
    uint32_t tick = sdr_get_tick();
    
    if (!sdr_dev_start(dev)) return;
    
    for (int i = 0; !intr && (tsec <= 0.0 || time < tsec); i++) {
        if (!quiet) {
            time = (sdr_get_tick() - tick) * 1e-3;
        }
        while (sdr_dev_read(dev, buff, SDR_SIZE_BUFF * ns) && !intr) {
            for (int j = 0; j < nch; j++) {
                byte[j] += write_file(fmt, buff, SDR_SIZE_BUFF, j, IQ[j], fp[j]);
            }
            sample += SDR_SIZE_BUFF;
        }
        if (!quiet && time - time_p > RATE_CYC * 1e-3) {
            rate = (sample - sample_p) / (time - time_p);
            time_p = time;
            sample_p = sample;
        }
        if (!quiet && i % (STAT_CYC / DATA_CYC) == 0) {
            print_stat(nch, IQ, fp, time, byte, rate);
        }
        sdr_sleep_msec(DATA_CYC);
    }
    (void)sdr_dev_stop(dev);
    
    if (!quiet) {
        rate = time > 0.0 ? sample / time : 0.0;
        print_stat(nch, IQ, fp, time, byte, rate);
        fprintf(stderr, "\n");
    }
}

//------------------------------------------------------------------------------
//  Synopsis
//
//    pocket_dump [-t tsec] [-r] [-p bus[,port]] [-c conf_file] [-q]
//                [file [file ...]]
//
//  Description
//
//    Capture and dump digital IF (DIF) data of a SDR device to output files.
//    To stop capturing, press Ctr-C.
//
//  Options
//    -t tsec
//        Data capturing time in seconds.
//
//    -r
//        Dump raw SDR device data without channel separation and quantization.
//
//    -p bus[,port]
//        USB bus and port number of the SDR device. Without the option, the
//        command selects the SDR device firstly found.
//
//    -c conf_file
//        Configure the SDR device with a device configuration file before 
//        capturing.
//
//    -q 
//        Suppress showing data dump status.
//
//    [file [file ...]]
//        Output digital IF data file paths. The first path is for CH1,
//        the second one is for CH2 and so on. The second one or the later
//        can be omitted. With option -r, only the first path is used. If
//        the file path is "", data are not output to anywhere. If the file
//        path is "-", data are output to stdout. If all of the file paths
//        omitted, the following default file paths are used.
//        
//        CH1: ch1_YYYYMMDD_hhmmss.bin
//        CH2: ch2_YYYYMMDD_hhmmss.bin
//        ...
//        (YYYYMMDD: dump start date in UTC, hhmmss: dump start time in UTC)
//
int main(int argc, char **argv)
{
    FILE *fp[SDR_MAX_CH] = {0};
    sdr_dev_t *dev;
    char *files[SDR_MAX_CH] = {0}, path[SDR_MAX_CH][64];
    const char *conf_file = "";
    time_t dump_time;
    double tsec = 0.0, fs = 0.0, fo[SDR_MAX_CH] = {0};
    int i, n = 0, bus = -1, port = -1, raw = 0, quiet = 0;
    int fmt = 0, nch = 1, IQ[SDR_MAX_CH] = {0};
    
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-t") && i + 1 < argc) {
            tsec = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-r")) {
            raw = 1; // raw output
        }
        else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            sscanf(argv[++i], "%d,%d", &bus, &port);
        }
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            conf_file = argv[++i];
        }
        else if (!strcmp(argv[i], "-q")) {
            quiet = 1;
        }
        else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            print_usage();
        }
        else if (n < SDR_MAX_CH) {
            files[n++] = argv[i];
        }
    }
    if (*conf_file) {
        if (!sdr_write_settings(conf_file, bus, port, 0)) {
            return -1;
        }
        sdr_sleep_msec(100);
    }
    if (!(dev = sdr_dev_open(bus, port))) {
        return -1;
    }
    if (!raw) {
        if (!(fmt = sdr_dev_info(dev, &fs, fo, IQ))) {
            sdr_dev_close(dev);
            return -1;
        }
        nch = fmt == SDR_FMT_RAW8 ? 2 : (fmt == SDR_FMT_RAW16 ? 4 : 8);
    }
    if (n == 0) { // set default file paths
        dump_time = time(NULL);
        for (i = 0; i < nch; i++) {
            char *p = path[i];
            p += sprintf(p, "ch%d_", i + 1);
            p += strftime(p, 32, "%Y%m%d_%H%M%S.bin", gmtime(&dump_time));
            files[i] = path[i];
        }
    }
    for (i = 0; i < nch; i++) {
        if (!files[i]) continue;
        if (!strcmp(files[i], "-")) {
#ifdef WIN32 // set binary mode for Windows
            _setmode(_fileno(stdout), _O_BINARY);
#endif
            fp[i] = stdout;
        }
        else if (*files[i] && !(fp[i] = fopen(files[i], "wb"))) {
            fprintf(stderr, "file open error %s\n", files[i]);
            sdr_dev_close(dev);
            return -1;
        }
    }
    signal(SIGTERM, sig_func);
    signal(SIGINT, sig_func);
    
    dump_data(dev, tsec, quiet, fmt, fs, nch, fo, IQ, fp);
    
    for (i = 0; i < nch; i++) {
        if (fp[i]) fclose(fp[i]);
    }
    sdr_dev_close(dev);
    
    return 0;
}
