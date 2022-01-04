/**
 *  Pocket SDR - Dump digital IF data of SDR device.
 *
 *  Author:
 *  T.TAKASU
 *
 *  History:
 *  2021-10-08  0.1  new
 *  2021-12-01  1.0  add default output file paths
 *  2022-01-04  1.1  data capture cycle: 1 -> 50 (ms)
 *
 **/
#include <signal.h>
#include <time.h>
#include "pocket.h"

/* constants and macros ------------------------------------------------------*/
#define PROG_NAME       "pocket_dump" /* program name */
#define DATA_CYC        50      /* data capture cycle (ms) */
#define RATE_CYC        1000    /* data rate update cycle (ms) */

/* interrupt flag ------------------------------------------------------------*/
static volatile uint8_t intr = 0;

/* signal handler ------------------------------------------------------------*/
static void sig_func(int sig)
{
    intr = 1;
    signal(sig, sig_func);
}

/* print usage ---------------------------------------------------------------*/
static void print_usage(void)
{
    printf("Usage: %s [-t tsec] [-r] [-p bus[,port]] [-c conf_file] [file [file]]\n",
        PROG_NAME);
    exit(0);
}

/* dump digital IF data ------------------------------------------------------*/
static void dump_data(int bus, int port, double tsec, int raw, FILE **fp)
{
    static const char *str_IQ[] = {"-", "I", "IQ"};
    sdr_dev_t *dev;
    double time = 0.0, time_p = 0.0, sample = 0.0, sample_p = 0.0;
    double rate = 0.0, byte[SDR_MAX_CH] = {0};
    uint32_t tick;
    int8_t *buff[SDR_MAX_CH];
    int i, j, size, n[SDR_MAX_CH];
    
    if (!(dev = sdr_dev_open(bus, port))) {
        return;
    }
    if (raw) {
        dev->IQ[0] = dev->IQ[1] = 0;
    }
    for (i = 0; i < SDR_MAX_CH; i++) {
        size = SDR_SIZE_BUFF * SDR_MAX_BUFF * (dev->IQ[i] == 2 ? 2 : 1);
        buff[i] = (int8_t *)sdr_malloc(size);
    }
    printf("%9s  %3s %12s %3s %12s %12s\n", "TIME(s)", "T", "CH1(Bytes)", "T",
        "CH2(Bytes)", "RATE(Ks/s)");
    
    tick = sdr_get_tick();
    
    for (i = 0; !intr && (tsec <= 0.0 || time < tsec); i++) {
        time = (sdr_get_tick() - tick) * 1e-3;
        
        if ((size = sdr_dev_data(dev, buff, n)) > 0) {
            for (j = 0; j < SDR_MAX_CH; j++) {
                if (!fp[j]) continue;
                fwrite(buff[j], n[j], 1, fp[j]);
                byte[j] += n[j];
            }
            sample += size;
        }
        if (time - time_p > RATE_CYC * 1e-3) {
            rate = (sample - sample_p) / (time - time_p);
            time_p = time;
            sample_p = sample;
        }
        printf("%9.1f  %3s %12.0f %3s %12.0f %12.1f\r", time, str_IQ[dev->IQ[0]],
            byte[0], str_IQ[dev->IQ[1]], byte[1], rate * 1e-3);
        fflush(stdout);
        
        sdr_sleep_msec(DATA_CYC);
    }
    rate = time > 0.0 ? sample / time : 0.0;
    printf("%9.1f  %3s %12.0f %3s %12.0f %12.1f\n", time, str_IQ[dev->IQ[0]],
        byte[0], str_IQ[dev->IQ[1]], byte[1], rate * 1e-3);
    
    for (i = 0; i < SDR_MAX_CH; i++) {
        free(buff[i]);
    }
    sdr_dev_close(dev);
}

/*----------------------------------------------------------------------------*/
/**
 *  Synopsis
 *
 *    pocket_dump [-t tsec] [-r] [-p bus[,port]] [-c conf_file] [file [file]]
 *
 *  Description
 *
 *    Capture and dump digital IF data of a SDR device to output files. To stop
 *    capturing, press Ctr-C.
 *
 *  Options
 *
 *    -t tsec
 *        Data capturing time in seconds.
 *
 *    -r
 *        Dump raw SDR device data without channel separation and quantization.
 *
 *    -p bus[,port]
 *        USB bus and port number of the SDR device. Without the option, the
 *        command selects the SDR device firstly found.
 *
 *    -c conf_file
 *        Configure the SDR device with a device configuration file before 
 *        capturing.
 *
 *    [file [file]]
 *        Output digital IF data file paths. The first path is for CH1 and
 *        the second one is for CH2. The second one can be omitted. With
 *        option -r, only the first path is used. If the file paths omitted,
 *        default output file paths are used as follows:
 *        
 *        CH1: ch1_YYYYMMDD_hhmmss.bin
 *        CH2: ch2_YYYYMMDD_hhmmss.bin
 *        (YYYYMMDD: dump start date in UTC, hhmmss: dump start time in UTC)
 *
 */
int main(int argc, char **argv)
{
    FILE *fp[SDR_MAX_CH] = {0};
    char *files[SDR_MAX_CH], path[2][32];
    const char *conf_file = "";
    time_t dump_time;
    double tsec = 0.0;
    int i, n = 0, bus = -1, port = -1, raw = 0;
    
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
        else if (!strncmp(argv[i], "-", 1)) {
            print_usage();
        }
        else if (n < SDR_MAX_CH) {
            files[n++] = argv[i];
        }
    }
    if (n == 0) { /* set default file paths */
        dump_time = time(NULL);
        strftime(path[0], 32, "ch1_%Y%m%d_%H%M%S.bin", gmtime(&dump_time));
        strftime(path[1], 32, "ch2_%Y%m%d_%H%M%S.bin", gmtime(&dump_time));
        files[n++] = path[0];
        files[n++] = path[1];
    }
    for (i = 0; i < n; i++) {
        if (raw && i > 0) continue;
        if (!(fp[i] = fopen(files[i], "wb"))) {
            fprintf(stderr, "file open error %s\n", files[i]);
            return -1;
        }
    }
    signal(SIGTERM, sig_func);
    signal(SIGINT, sig_func);
    
    if (*conf_file) {
        sdr_write_settings(conf_file, bus, port, 0);
    }
    dump_data(bus, port, tsec, raw, fp);
    
    for (i = 0; i < n; i++) {
        if (fp[i]) fclose(fp[i]);
    }
    return 0;
}
