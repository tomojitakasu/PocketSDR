//
//  Pocket SDR - Dump digital IF data of Pocket SDR FE device.
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
//  2024-04-28  1.6  support Pocket SDR FE 4CH
//  2024-06-29  1.7  support API change in sdr_dev.c
//  2024-07-02  1.8  support tag file output
//  2024-11-23  1.9  support Pocket SDR FE 8CH
//
#include <signal.h>
#ifdef WIN32
#include <fcntl.h>
#endif
#include "pocket_sdr.h"

// constants and macros --------------------------------------------------------
#define PROG_NAME       "pocket_dump" // program name
#define DATA_CYC        10      // data capture cycle (ms)
#define STAT_CYC        50      // status update cycle (ms)
#define RATE_CYC        1000    // data rate update cycle (ms)

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

// bytes per sample ------------------------------------------------------------
static int sample_byte(int fmt)
{
    switch (fmt) {
        case SDR_FMT_RAW16 :
        case SDR_FMT_RAW16I: return 2;
        case SDR_FMT_RAW32 : return 4;
    }
    return 1;
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
    
    if (!LUT[0][0]) {
        gen_LUT(LUT);
    }
    int8_t *data = (int8_t *)sdr_malloc(size * IQ);
    
    if (fmt == SDR_FMT_RAW8 || fmt == SDR_FMT_RAW16 || fmt == SDR_FMT_RAW32) {
        int ns = sample_byte(fmt), pos = ch % 2 * 2;
        for (int i = 0, j = ch / 2; i < size; i++, j += ns) {
            if (IQ == 1) {
                data[i] = LUT[pos][buff[j]];
            }
            else {
                data[i*2  ] = LUT[pos  ][buff[j]];
                data[i*2+1] = LUT[pos+1][buff[j]];
            }
        }
    }
    else { // SDR_FMT_RAW16I
        int pos = ch % 4;
        for (int i = 0, j = ch / 4; i < size; i++, j += 2) {
            data[i] = LUT[pos][buff[j]];
        }
    }
    int bytes = (int)fwrite(data, 1, size * IQ, fp);
    sdr_free(data);
    return bytes;
}

// print header ----------------------------------------------------------------
static void print_head(int raw, int fmt, int nfile, const int *IQ, FILE **fp)
{
    static const char *str_IQ[] = {"- ", "I ", "IQ", "I "};
    static const char *str_fmt[] = {
        "-", "-", "RAW8", "RAW16", "RAW16I", "RAW32"
    };
    fprintf(stderr, "%8s", "TIME(s)");
    if (raw) {
        if (fp[0]) fprintf(stderr, "    %6s(B)", str_fmt[fmt-1]);
    }
    else {
        for (int i = 0; i < nfile; i++) {
            if (fp[i]) fprintf(stderr, "    CH%d:%s(B)", i + 1, str_IQ[IQ[i]]);
        }
    }
    fprintf(stderr, " %10s\n", "RATE(Ks/s)");
}

// print status ----------------------------------------------------------------
static void print_stat(int nch, const int *IQ, FILE **fp, double time,
    const double *byte, double rate)
{
    fprintf(stderr, "%8.1f", time);
    for (int i = 0; i < nch; i++) {
        if (!fp[i]) continue;
        fprintf(stderr, "%13.0f", byte[i]);
    }
    fprintf(stderr, " %10.1f\r", rate * 1e-3);
    fflush(stderr);
}

// dump digital IF data --------------------------------------------------------
static void dump_data(sdr_dev_t *dev, double tsec, int quiet, int raw, int fmt,
    int nfile, const int *IQ, FILE **fp)
{
    double time = 0.0, time_p = 0.0, sample = 0.0, sample_p = 0.0;
    double rate = 0.0, byte[SDR_MAX_RFCH] = {0};
    int ns = sample_byte(fmt);
    uint8_t *buff = (uint8_t *)sdr_malloc(SDR_SIZE_BUFF * ns);
    
    if (!quiet) {
        print_head(raw, fmt, nfile, IQ, fp);
    }
    uint32_t tick = sdr_get_tick();
    
    if (!sdr_dev_start(dev)) return;
    
    for (int i = 0; !intr && (tsec <= 0.0 || time < tsec); i++) {
        if (!quiet) {
            time = (sdr_get_tick() - tick) * 1e-3;
        }
        while (sdr_dev_read(dev, buff, SDR_SIZE_BUFF * ns) && !intr) {
            for (int j = 0; j < nfile; j++) {
                if (!fp[j]) continue;
                if (raw) {
                    byte[j] += fwrite(buff, 1, SDR_SIZE_BUFF * ns, fp[j]);
                }
                else {
                    byte[j] += write_file(fmt, buff, SDR_SIZE_BUFF, j, IQ[j],
                        fp[j]);
                }
            }
            sample += SDR_SIZE_BUFF;
        }
        if (!quiet && time - time_p > RATE_CYC * 1e-3) {
            rate = (sample - sample_p) / (time - time_p);
            time_p = time;
            sample_p = sample;
        }
        if (!quiet && i % (STAT_CYC / DATA_CYC) == 0) {
            print_stat(nfile, IQ, fp, time, byte, rate);
        }
        sdr_sleep_msec(DATA_CYC);
    }
    sdr_dev_stop(dev);
    
    if (!quiet) {
        rate = time > 0.0 ? sample / time : 0.0;
        print_stat(nfile, IQ, fp, time, byte, rate);
        fprintf(stderr, "\n");
    }
    sdr_free(buff);
}

// write tag file --------------------------------------------------------------
static void write_tag_files(gtime_t time, int raw, int fmt, double fs,
    const double *fo, const int *IQ, int nch, char **files)
{
    for (int i = 0; i < (raw ? 1 : nch); i++) {
        if (!files[i] || !*files[i] || !strcmp(files[i], "-")) continue;
        
        if (raw) {
            sdr_tag_write(files[i], PROG_NAME, time, fmt, fs, fo, IQ);
        }
        else {
            int fmt_i = IQ[i] == 1 ? SDR_FMT_INT8 : SDR_FMT_INT8X2;
            sdr_tag_write(files[i], PROG_NAME, time, fmt_i, fs, fo + i, IQ + i);
        }
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
//    Capture and dump digital IF (DIF) data of a Pocket SDR FE device to output
//    files. To stop capturing, press Ctr-C.
//
//  Options
//    -t tsec
//        Data capturing time in seconds.
//
//    -r
//        Dump raw data of the Pocket SDR FE device without channel separation
//        and quantization.
//
//    -p bus[,port]
//        USB bus and port number of the Pocket SDR FE device. Without the
//        option, the command selects the device firstly found.
//
//    -c conf_file
//        Configure the Pocket SDR FE device with a device configuration file
//        before capturing.
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
    FILE *fp[SDR_MAX_RFCH] = {0};
    sdr_dev_t *dev;
    char *files[SDR_MAX_RFCH] = {0}, path[SDR_MAX_RFCH][64];
    const char *conf_file = "";
    gtime_t dump_time;
    double tsec = 0.0, fs, fo[SDR_MAX_RFCH];
    int n = 0, bus = -1, port = -1, raw = 0, quiet = 0;
    int nch, fmt, IQ[SDR_MAX_RFCH], nfile;
    
    for (int i = 1; i < argc; i++) {
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
        else if (n < SDR_MAX_RFCH) {
            files[n++] = argv[i];
        }
    }
    if (!(dev = sdr_dev_open(bus, port))) {
        return -1;
    }
    if (*conf_file) {
        if (!sdr_conf_write(dev, conf_file, 0)) {
            sdr_dev_close(dev);
            return -1;
        }
        sdr_sleep_msec(50);
    }
    if (!(nch = sdr_dev_get_info(dev, &fmt, &fs, fo, IQ))) {
        sdr_dev_close(dev);
        return -1;
    }
    nfile = raw ? 1 : nch;
    dump_time = utc2gpst(timeget());
    
    if (n == 0) { // set default file paths
        for (int i = 0; i < nfile; i++) {
            double ep[6] = {0};
            char *p = path[i];
            time2epoch(dump_time, ep);
            p += sprintf(p, "ch%d_", i + 1);
            p += sprintf(p, "%04.0f%02.0f%02.0f_%02.0f%02.0f%02.0f.bin", ep[0],
                ep[1], ep[2], ep[3], ep[4], ep[5]);
            files[i] = path[i];
        }
    }
    for (int i = 0; i < nfile; i++) {
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
    
    dump_data(dev, tsec, quiet, raw, fmt, nfile, IQ, fp);
    
    for (int i = 0; i < nfile; i++) {
        if (fp[i]) fclose(fp[i]);
    }
    sdr_dev_close(dev);
    
    write_tag_files(dump_time, raw, fmt, fs, fo, IQ, nch, files);
    
    return 0;
}
