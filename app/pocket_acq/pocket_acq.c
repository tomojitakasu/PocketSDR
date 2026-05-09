//
//  Pocket SDR C AP - GNSS Signal Acquisition.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-07-05  1.0  port pocket_acq.py to C
//  2022-08-08  1.1  add option -w, modify option -d
//  2024-07-02  1.2  support tag file input for auto-configuration
//
#include "pocket_sdr.h"

// constants -------------------------------------------------------------------
#define PROG_NAME "pocket_acq" // program name
#define T_AQC     0.010      // non-coherent integration time for acquisition (s)
#define THRES_CN0 38.0       // threshold to lock (dB-Hz)
#define ESC_COL   "\033[34m" // ANSI escape color = blue
#define ESC_RES   "\033[0m"  // ANSI escape reset
#define FFTW_WISDOM "../python/fftw_wisdom.txt"

// print version ---------------------------------------------------------------
static void print_ver(void)
{
     printf("%s ver.%s\n", PROG_NAME, sdr_get_ver());
     exit(0);
}

// show usage ------------------------------------------------------------------
static void show_usage(void)
{
    printf("Usage: pocket_acq [-sig sig] [-prn prn[,...]] [-tint tint]\n");
    printf("       [-toff toff] [-f freq] [-fi freq] [-d freq[,freq]] [-nz] file\n");
    exit(0);
}

// search signal ---------------------------------------------------------------
int search_sig(const char *sig, int prn, const sdr_buff_t *buff, double fs,
    double fi, float ref_dop, float max_dop, const int *opt, double *dop,
    double *coff, float *cn0)
{
    int8_t *code;
    int len_code;
    
    // generate code
    if (!(code = sdr_gen_code(sig, prn, &len_code))) {
        return 0;
    }
    // shift IF frequency for GLONASS FDMA
    fi = sdr_shift_freq(sig, prn, fi);
    
    // generate code FFT
    double T = sdr_code_cyc(sig);
    int N = (int)(fs * T), Nz = opt[2] ? 0 : N;
    sdr_cpx_t *code_fft = sdr_cpx_malloc(2 * N);
    sdr_gen_code_fft(code, NULL, len_code, T, 0.0, fs, N, Nz, code_fft);
    
    // doppler search bins
    int len_fds;
    float *fds = sdr_dop_bins(T, ref_dop, max_dop, &len_fds);
    
    // parallel code search and non-coherent integration
    float *P = (float *)sdr_malloc(sizeof(float) * (N + Nz) * len_fds);
    for (int i = 0; i < buff->N - 2 * N + 1; i += N) {
        sdr_search_code(code_fft, T, buff, i, N + Nz, fs, fi, fds, len_fds, P);
    }
    // max correlation power and C/N0
    int ix[2] = {0};
    *cn0 = sdr_corr_max(P, N + Nz, N, len_fds, T, ix);
    
    *dop = sdr_fine_dop(P, N + Nz, fds, len_fds, ix);
    *coff = ix[1] / fs;
    
    sdr_cpx_free(code_fft);
    sdr_free(fds);
    sdr_free(P);
    return 1;
}

// main (see doc/command_ref.md) -----------------------------------------------
int main(int argc, char **argv)
{
    const char *sig = "L1CA", *file = "", *fftw_wisdom = FFTW_WISDOM;
    double fs = 12e6, fi = 0.0, T = T_AQC, toff = 0.0, ref_dop = 0.0;
    double max_dop = 5000.0;
    int prns[SDR_MAX_NPRN], nprn = 0, opt[5] = {0}, fmt = SDR_FMT_INT8X2, IQ = 2;
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-sig") && i + 1 < argc) {
            sig = argv[++i];
        }
        else if (!strcmp(argv[i], "-prn") && i + 1 < argc) {
            nprn = sdr_parse_nums(argv[++i], prns);
        }
        else if (!strcmp(argv[i], "-tint") && i + 1 < argc) {
            T = atof(argv[++i]) * 1e-3;
        }
        else if (!strcmp(argv[i], "-toff") && i + 1 < argc) {
            toff = atof(argv[++i]) * 1e-3;
        }
        else if (!strcmp(argv[i], "-fmt") && i + 1 < argc) {
            const char *format = argv[++i];
            if      (!strcmp(format, "INT8"  )) fmt = SDR_FMT_INT8;
            else if (!strcmp(format, "INT8X2")) fmt = SDR_FMT_INT8X2;
            else if (!strcmp(format, "RAW8"  )) fmt = SDR_FMT_RAW8;
            else if (!strcmp(format, "RAW16" )) fmt = SDR_FMT_RAW16;
            else if (!strcmp(format, "RAW16I")) fmt = SDR_FMT_RAW16I;
            else if (!strcmp(format, "RAW32" )) fmt = SDR_FMT_RAW32;
            else {
                fprintf(stderr, "unrecognized format: %s\n", format);
                exit(-1);
            }
        }
        else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            fs = atof(argv[++i]) * 1e6;
        }
        else if (!strcmp(argv[i], "-fi") && i + 1 < argc) {
            fi = atof(argv[++i]) * 1e6;
            if (fi != 0.0) IQ = 1;
        }
        else if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            fftw_wisdom = argv[++i];
        }
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            sscanf(argv[++i], "%lf,%lf", &ref_dop, &max_dop);
        }
        else if (!strcmp(argv[i], "-nz")) {
            opt[2] = 1;
        }
        else if (!strcmp(argv[i], "-v")) {
            print_ver();
        }
        else if (argv[i][0] == '-') {
            show_usage();
        }
        else {
            file = argv[i];
        }
    }
    if (!*file) {
        fprintf(stderr, "Specify input file.\n");
        exit(-1);
    }
    double Tcode = sdr_code_cyc(sig); // code cycle (s)
    
    if (Tcode <= 0.0) {
        fprintf(stderr, "Invalid signal %s.\n", sig);
        exit(-1);
    }
    // integration time (s)
    if (T < Tcode) {
        T = Tcode;
    }
    sdr_func_init(fftw_wisdom);
    
    // read tag file
    double fo[SDR_MAX_RFCH];
    int IQ_t[SDR_MAX_RFCH], bits_t[SDR_MAX_RFCH];
    if (sdr_tag_read(file, NULL, NULL, &fmt, &fs, fo, IQ_t, bits_t)) {
        if (fmt != SDR_FMT_INT8 && fmt != SDR_FMT_INT8X2) {
            fprintf(stderr, "Unsupported format: %s\n", file);
            exit(-1);
        }
        fi = sdr_sig_freq(sig) - fo[0];
        IQ = IQ_t[0];
    }
    // read IF data
    sdr_buff_t *buff;
    if (!(buff = sdr_read_data(file, fs, IQ, T + Tcode, toff))) {
        exit(-1);
    }
    uint32_t tick = sdr_get_tick();
    
    // search signal
    for (int i = 0; i < nprn; i++) {
        double dop, coff;
        float cn0;
        
        if (!search_sig(sig, prns[i], buff, fs, fi, (float)ref_dop,
            (float)max_dop, opt, &dop, &coff, &cn0)) {
            continue;
        }
        printf("%sSIG= %-4s, %s= %3d, COFF= %8.5f ms, DOP= %5.0f Hz, C/N0= %4.1f dB-Hz%s\n",
            (cn0 >= THRES_CN0) ? ESC_COL : "", sig, "PRN", prns[i], coff * 1e3,
            dop, cn0, (cn0 >= THRES_CN0) ? ESC_RES : "");
        fflush(stdout);
    }
    printf("TIME = %.3f s\n", (sdr_get_tick() - tick) * 1e-3);
    return 0;
}
