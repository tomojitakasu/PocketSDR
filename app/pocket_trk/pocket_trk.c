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
//  2025-02-18  1.9  fix bug on output files
//                   add -rfch option
//  2025-03-16  1.10 add -opt option, delete -w option
//  2026-07-24  1.11 support up to 8 output streams by repeating -nmea, -rtcm,
//                   -log and -raw options
//  2026-08-02  1.12 add -arch and -geom options for antenna array
//  2026-08-07  1.13 close the SDR device on console close for Windows
//
#include <math.h>
#include <signal.h>
#include <ctype.h>
#include "pocket_sdr.h"

// constants and macros ---------------------------------------------------------
#define PROG_NAME "pocket_trk"  // program name
#define TRACE_LEVEL 3           // debug trace level
#define FFTW_WISDOM "../python/fftw_wisdom.txt"
#define NUM_COL    110          // number of channel status columns
#define MAX_ROW    64           // max number of channel status rows
#define MIN_LOCK   2.0          // min lock time for channel status (s)
#define EXIT_WAIT  400          // max wait cycles (10 ms) for device close
#define ESC_COL    "\033[34m"   // ANSI escape color blue
#define ESC_RES    "\033[0m"    // ANSI escape reset
#define ESC_VCUR   "\033[?25h"  // ANSI escape show cursor
#define ESC_HCUR   "\033[?25l"  // ANSI escape hide cursor
#define ESC_EOL    "\033[K"     // ANSI escape clear to end of line

// usage text ------------------------------------------------------------------
static const char *usage_text[] = {
    "Usage: pocket_trk [-sig sig -prn prn[,...] [-rfch ch[,...]] ...]",
    "       [-fmt {INT8|INT8X2|RAW8|RAW16|RAW32|CS8|CS16}] [-f freq]",
    "       [-fo freq[,...]] [-IQ {1|2}[,...]] [-bits {2|3}[,...]",
    "       [-toff toff] [-ti tint] [-p bus,[,port] [-c conf_file]",
    "       [-driver name] [-gain gain] [-bw bw] [-gpio val] [-EXT_CLOCK]",
    "       [-NO_SYNC] [-fd dopp] [-log path] [-nmea path] [-rtcm path]",
    "       [-raw path] ... [-opt file] [-arch nch] [-geom file]",
    "       [-sens name[,...]] [file]",
    NULL
};

// system options --------------------------------------------------------------
static char fftw_wisdom[1024] = "";

static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return s;
    char *e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e)) *e-- = '\0';
    return s;
}

static int load_opt_file(const char *file)
{
    FILE *fp = fopen(file, "r");
    char buff[2048];
    int n = 0;
    
    if (!fp) return 0;
    
    while (fgets(buff, sizeof(buff), fp)) {
        n++;
        char *line = trim(buff), *eq, *sharp;
        if (!*line || *line == '#') continue;
        if ((sharp = strchr(line, '#'))) *sharp = '\0';
        line = trim(line);
        if (!*line) continue;
        if (!(eq = strchr(line, '='))) {
            fprintf(stderr, "invalid option %s (%s:%d)\n", line, file, n);
            continue;
        }
        *eq++ = '\0';
        char *key = trim(line);
        char *val = trim(eq);
        if (!strcmp(key, "fftw_wisdom")) {
            snprintf(fftw_wisdom, sizeof(fftw_wisdom), "%s", val);
        } else {
            sdr_rcv_setopt(key, atof(val));
        }
    }
    fclose(fp);
    return 1;
}

// add output stream -------------------------------------------------------------
static void add_str(int *types, const char **paths, int type, const char *path)
{
    for (int i = 0; i < SDR_MAX_STR; i++) {
        if (types[i] != SDR_STR_NONE) continue;
        types[i] = type;
        paths[i] = path;
        return;
    }
    fprintf(stderr, "too many output streams (max %d)\n", SDR_MAX_STR);
    exit(-1);
}

// interrupt and shutdown completion flags -------------------------------------
static volatile uint8_t intr = 0;
static volatile uint8_t done = 0;

// signal handler --------------------------------------------------------------
static void sig_func(int sig)
{
    intr = 1;
    signal(sig, sig_func);
}

#ifdef WIN32

// console control handler -----------------------------------------------------
//   The C runtime raises SIGINT only for Ctrl-C and Ctrl-Break. Console close,
//   logoff and shutdown terminate the process as soon as the handler returns,
//   so wait here until the main thread has closed the SDR device.
static BOOL WINAPI ctrl_func(DWORD type)
{
    intr = 1;
    for (int i = 0; i < EXIT_WAIT && !done; i++) {
        sdr_sleep_msec(10);
    }
    return TRUE;
}
#endif // WIN32

// print version ---------------------------------------------------------------
static void print_ver(void)
{
     printf("%s ver.%s\n", PROG_NAME, sdr_get_ver());
     exit(0);
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
static int print_rcv_stat(sdr_rcv_t *rcv, int nrow, int max_row)
{
    static char stat[(NUM_COL+10)*MAX_ROW];
    char *p, *q;
    int n = 0;

    if (nrow > 0) {
        printf("\033[%dA\r", nrow);
    } else {
        printf("\r%s", ESC_EOL);
    }
    // get SDR receiver channel status
    (void)sdr_rcv_ch_stat(rcv, "ALL", 0, MIN_LOCK, 0, 0, stat, sizeof(stat));
    
    for (p = q = stat; (q = strchr(p, '\n')); p = q + 1) {
        if (n < max_row) {
            printf("%s%.*s%s%s\n", n < 2 ? "" : ESC_COL, (int)(q - p), p,
                ESC_EOL, ESC_RES);
            n++;
        } else if (n == max_row) {
            printf("... ..%s\n", ESC_EOL);
            n++;
        }
    }
    for ( ; n < nrow; n++) {
        printf("%s\n", ESC_EOL);
    }
    fflush(stdout);
    return n;
}

// main ------------------------------------------------------------------------
// Command spec: see doc/command_ref.md.
int main(int argc, char **argv)
{
    sdr_rcv_t *rcv;
    int prns[SDR_MAX_NCH], nch = 0, fmt = SDR_FMT_INT8X2;
    int IQ[SDR_MAX_RFCH] = {2, 2, 2, 2, 2, 2, 2, 2};
    int bits[SDR_MAX_RFCH] = {2, 2, 2, 2, 2, 2, 2, 2};
    int dev_type = SDR_DEV_FILE, bus = -1, port = -1, nrow = 0;
    int max_row = MAX_ROW;
    double fs = 12e6, fo[SDR_MAX_RFCH] = {0}, toff = 0.0, tscale = 1.0;
    double tint = 0.1;
    const char *sig = "L1CA", *sigs[SDR_MAX_NCH];
    const char *file = "", *conf_file = "";
    int types[SDR_MAX_STR] = {0};
    const char *paths[SDR_MAX_STR] = {"", "", "", "", "", "", "", ""};
    const char *opt_file = "";
    const char *debug_file = "";
    const char *driver = "";
    double gain = 0.0, bw = 0.0, max_dop = 0.0;
    char rfch_opt[1024] = "-RFCH";
    const char *geom_file = "";

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-sig") && i + 1 < argc) {
            sig = argv[++i];
        } else if (!strcmp(argv[i], "-prn") && i + 1 < argc) {
            int nums[SDR_MAX_NCH];
            int n = sdr_parse_nums(argv[++i], nums);
            for (int j = 0; j < n && nch < SDR_MAX_NCH; j++) {
                sigs[nch] = sig;
                prns[nch++] = nums[j];
            }
        } else if (!strcmp(argv[i], "-rfch") && i + 1 < argc) {
            size_t len = strlen(rfch_opt);
            snprintf(rfch_opt + len, sizeof(rfch_opt) - len, " %s:%s", sig,
                argv[++i]);
        } else if (!strcmp(argv[i], "-toff") && i + 1 < argc) {
            toff = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-tscale") && i + 1 < argc) {
            tscale = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-fmt") && i + 1 < argc) {
            const char *str = argv[++i];
            if      (!strcmp(str, "INT8"  )) fmt = SDR_FMT_INT8;
            else if (!strcmp(str, "INT8X2")) fmt = SDR_FMT_INT8X2;
            else if (!strcmp(str, "RAW8"  )) fmt = SDR_FMT_RAW8;
            else if (!strcmp(str, "RAW16" )) fmt = SDR_FMT_RAW16;
            else if (!strcmp(str, "RAW16I")) fmt = SDR_FMT_RAW16I;
            else if (!strcmp(str, "RAW32" )) fmt = SDR_FMT_RAW32;
            else if (!strcmp(str, "CS8"   )) fmt = SDR_FMT_CS8;
            else if (!strcmp(str, "CS16"  )) fmt = SDR_FMT_CS16;
            else {
                fprintf(stderr, "unrecognized format: %s\n", str);
                exit(-1);
            }
        } else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            fs = atof(argv[++i]) * 1e6;
        } else if (!strcmp(argv[i], "-fo") && i + 1 < argc) {
            sscanf(argv[++i], "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", fo, fo + 1,
                fo + 2, fo + 3, fo + 4, fo + 5, fo + 6, fo + 7);
            for (int j = 0; j < 8; j++) fo[j] *= 1e6;
        } else if (!strcmp(argv[i], "-IQ") && i + 1 < argc) {
            sscanf(argv[++i], "%d,%d,%d,%d,%d,%d,%d,%d", IQ, IQ + 1, IQ + 2,
                IQ + 3, IQ + 4, IQ + 5, IQ + 6, IQ + 7);
        } else if (!strcmp(argv[i], "-bits") && i + 1 < argc) {
            sscanf(argv[++i], "%d,%d,%d,%d,%d,%d,%d,%d", bits, bits + 1,
                bits + 2, bits + 3, bits + 4, bits + 5, bits + 6, bits + 7);
        } else if (!strcmp(argv[i], "-ti") && i + 1 < argc) {
            tint = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            sscanf(argv[++i], "%d,%d", &bus, &port);
        } else if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            conf_file = argv[++i];
        } else if (!strcmp(argv[i], "-nmea") && i + 1 < argc) {
            add_str(types, paths, SDR_STR_NMEA, argv[++i]);
        } else if (!strcmp(argv[i], "-rtcm") && i + 1 < argc) {
            add_str(types, paths, SDR_STR_RTCM3, argv[++i]);
        } else if (!strcmp(argv[i], "-log") && i + 1 < argc) {
            add_str(types, paths, SDR_STR_LOG, argv[++i]);
        } else if (!strcmp(argv[i], "-raw") && i + 1 < argc) {
            add_str(types, paths, SDR_STR_IFDATA, argv[++i]);
        } else if (!strcmp(argv[i], "-h") && i + 1 < argc) {
            max_row = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-opt") && i + 1 < argc) {
            opt_file = argv[++i];
        } else if (!strcmp(argv[i], "-debug") && i + 1 < argc) {
            debug_file = argv[++i];
        } else if (!strcmp(argv[i], "-driver") && i + 1 < argc) {
            driver = argv[++i];
        } else if (!strcmp(argv[i], "-gain") && i + 1 < argc) {
            gain = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-bw") && i + 1 < argc) {
            bw = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-gpio") && i + 1 < argc) {
            size_t len = strlen(rfch_opt);
            snprintf(rfch_opt + len, sizeof(rfch_opt) - len, " -GPIO=%d",
                atoi(argv[++i]));
        } else if (!strcmp(argv[i], "-EXT_CLOCK")) {
            size_t len = strlen(rfch_opt);
            snprintf(rfch_opt + len, sizeof(rfch_opt) - len, " -EXT_CLOCK");
        } else if (!strcmp(argv[i], "-NO_SYNC")) {
            size_t len = strlen(rfch_opt);
            snprintf(rfch_opt + len, sizeof(rfch_opt) - len, " -NO_SYNC");
        } else if (!strcmp(argv[i], "-fd") && i + 1 < argc) {
            max_dop = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-arch") && i + 1 < argc) {
            size_t len = strlen(rfch_opt);
            snprintf(rfch_opt + len, sizeof(rfch_opt) - len, " -ARCH=%d",
                atoi(argv[++i]));
        } else if (!strcmp(argv[i], "-geom") && i + 1 < argc) {
            geom_file = argv[++i];
        } else if (!strcmp(argv[i], "-sens") && i + 1 < argc) {
            size_t len = strlen(rfch_opt);
            snprintf(rfch_opt + len, sizeof(rfch_opt) - len, " -SENS=%s",
                argv[++i]);
        } else if (!strcmp(argv[i], "-v")) {
            print_ver();
        } else if (argv[i][0] == '-') {
            show_usage();
        } else {
            file = argv[i];
        }
    }
    if (*debug_file) {
        traceopen(debug_file);
        tracelevel(TRACE_LEVEL);
    }
    if (*opt_file && !load_opt_file(opt_file)) {
        fprintf(stderr, "options file read error: %s\n", opt_file);
    }
    if (max_dop > 0.0) { // -fd overrides max_dop from -opt file
        sdr_rcv_setopt("max_dop", max_dop);
    }
    sdr_func_init(fftw_wisdom);
    
    signal(SIGTERM, sig_func);
    signal(SIGINT, sig_func);
#ifdef WIN32
    SetConsoleCtrlHandler(ctrl_func, TRUE); // console close, logoff, shutdown
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    uint32_t tt = sdr_get_tick();

    if (gain > 0.0) {
        size_t len = strlen(rfch_opt);
        snprintf(rfch_opt + len, sizeof(rfch_opt) - len, " -GAIN=%.1f", gain);
    }
    if (bw > 0.0) {
        size_t len = strlen(rfch_opt);
        snprintf(rfch_opt + len, sizeof(rfch_opt) - len, " -BW=%.3f", bw);
    }
    if (*file) {
        rcv = sdr_rcv_open_file(sigs, prns, nch, fmt, fs, fo, IQ, bits, toff,
            tscale, file, types, paths, rfch_opt);
    } else if (*driver) {
        rcv = sdr_rcv_open_sdev(sigs, prns, nch, driver, fmt, fs, fo, types,
            paths, rfch_opt);
    } else {
        rcv = sdr_rcv_open_dev(sigs, prns, nch, bus, port, conf_file, types,
            paths, rfch_opt);
    }
    if (!rcv) {
        return -1;
    }
    if (*geom_file) { // array element positions
        double ant_pos[SDR_MAX_RFCH*3] = {0};
        int ant_ena[SDR_MAX_RFCH] = {0};
        int nant = sdr_array_geom_load(geom_file, ant_pos, SDR_MAX_RFCH);
        for (int i = 0; i < nant; i++) ant_ena[i] = 1;
        if (nant <= 0 || !sdr_rcv_array_ant_pos(rcv, ant_pos, ant_ena)) {
            fprintf(stderr, "array geometry load error: %s\n", geom_file);
        }
    }
    if (tint > 0.0) {
        printf("%s", ESC_HCUR);
    }
    while (!intr && rcv->state) { // wait for interrupt or file end
        if (tint > 0.0) {
            nrow = print_rcv_stat(rcv, nrow, max_row);
        }
        sdr_sleep_msec(tint > 0.0 ? (int)(tint * 1000) : 100);
    }
    if (tint > 0.0) {
        print_rcv_stat(rcv, nrow, max_row);
        printf("  TIME(s) = %.3f\n", (sdr_get_tick() - tt) * 1e-3);
        printf("%s", ESC_VCUR);
    }
    sdr_rcv_close(rcv);
    
    if (*debug_file) {
        traceclose();
    }
    done = 1; // release the console control handler
    return 0;
}
