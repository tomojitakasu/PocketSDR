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
//
#include <signal.h>
#include <time.h>
#ifdef WIN32
#include <fcntl.h>
#endif
#include "pocket_dev.h"

// constants and macros --------------------------------------------------------
#define PROG_NAME       "pocket_dump" // program name
#define DATA_CYC        10      // data capture cycle (ms)
#define STAT_CYC        50      // status update cycle (ms)
#define RATE_CYC        1000    // data rate update cycle (ms)

// interrupt flag --------------------------------------------------------------
static volatile uint8_t intr = 0;

// get system tick (ms) --------------------------------------------------------
static uint32_t get_tick(void)
{
#ifdef WIN32
    return (uint32_t)timeGetTime();
#else
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000u + tv.tv_usec / 1000u;
#endif
}

// sleep milli-sec -------------------------------------------------------------
static void sleep_msec(int msec)
{
#ifdef WIN32
    Sleep(msec < 5 ? 1 : msec);
#else
    struct timespec ts = {0};
    if (msec <= 0) return;
    ts.tv_nsec = (long)(msec * 1000000);
    nanosleep(&ts, NULL);
#endif
}

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

// dump digital IF data --------------------------------------------------------
static void dump_data(sdr_dev_t *dev, double tsec, int raw, int quiet,
    FILE **fp)
{
    static const char *str_IQ[] = {"-", "I", "IQ", "I"};
    double time = 0.0, time_p = 0.0, sample = 0.0, sample_p = 0.0;
    double rate = 0.0, byte[SDR_MAX_CH] = {0};
    int8_t *buff[SDR_MAX_CH];
    int size, n[SDR_MAX_CH];
    
    if (raw) {
        for (int i = 0; i < dev->max_ch; i++) {
            dev->IQ[i] = 0;
        }
    }
    for (int i = 0; i < dev->max_ch; i++) {
        size = SDR_SIZE_BUFF * SDR_MAX_BUFF * (dev->IQ[i] == 2 ? 2 : 1);
        buff[i] = (int8_t *)malloc(size);
    }
    if (!quiet) {
        fprintf(stderr, "%9s ", "TIME(s)");
        for (int i = 0; i < dev->max_ch; i++) {
            if (!fp[i]) continue;
            fprintf(stderr, " %3s   CH%d(Bytes)", "T", i + 1);
        }
        fprintf(stderr, " %12s\n", "RATE(Ks/s)");
    }
    uint32_t tick = get_tick();
    
    for (int i = 0; !intr && (tsec <= 0.0 || time < tsec); i++) {
        if (!quiet) {
            time = (get_tick() - tick) * 1e-3;
        }
        if ((size = sdr_dev_data(dev, buff, n)) > 0) {
            for (int j = 0; j < dev->max_ch; j++) {
                if (!fp[j]) continue;
                fwrite(buff[j], n[j], 1, fp[j]);
                fflush(fp[j]);
                byte[j] += n[j];
            }
            sample += size;
        }
        if (!quiet && time - time_p > RATE_CYC * 1e-3) {
            rate = (sample - sample_p) / (time - time_p);
            time_p = time;
            sample_p = sample;
        }
        if (!quiet && i % (STAT_CYC / DATA_CYC) == 0) {
            fprintf(stderr, "%9.1f ", time);
            for (int j = 0; j < dev->max_ch; j++) {
                if (!fp[j]) continue;
                fprintf(stderr, " %3s %12.0f", str_IQ[dev->IQ[j]], byte[j]);
            }
            fprintf(stderr, " %12.1f\r", rate * 1e-3);
            fflush(stderr);
        }
        sleep_msec(DATA_CYC);
    }
    if (!quiet) {
        rate = time > 0.0 ? sample / time : 0.0;
        fprintf(stderr, "%9.1f ", time);
        for (int j = 0; j < dev->max_ch; j++) {
            if (!fp[j]) continue;
            fprintf(stderr, " %3s %12.0f", str_IQ[dev->IQ[j]], byte[j]);
        }
        fprintf(stderr, " %12.1f\n", rate * 1e-3);
    }
    for (int i = 0; i < dev->max_ch; i++) {
        free(buff[i]);
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
    double tsec = 0.0;
    int i, n = 0, bus = -1, port = -1, raw = 0, quiet = 0;
    
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-t") && i + 1 < argc) {
            tsec = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-r")) {
            raw = 1; /* raw */
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
        sleep_msec(100);
    }
    if (!(dev = sdr_dev_open(bus, port))) {
        return -1;
    }
    if (n == 0) { // set default file paths
        dump_time = time(NULL);
        for (i = 0; i < dev->max_ch; i++) {
            char *p = path[i];
            p += sprintf(p, "ch%d_", i + 1);
            p += strftime(p, 32, "%Y%m%d_%H%M%S.bin", gmtime(&dump_time));
            files[i] = path[i];
        }
    }
    for (i = 0; i < dev->max_ch; i++) {
        if (!files[i] || (raw && i > 0)) continue;
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
    
    dump_data(dev, tsec, raw, quiet, fp);
    
    for (i = 0; i < dev->max_ch; i++) {
        if (fp[i]) fclose(fp[i]);
    }
    sdr_dev_close(dev);
    
    return 0;
}
