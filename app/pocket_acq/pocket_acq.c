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
//  2026-06-01  1.3  support CS8 and CS16 formats
//
#include "pocket_sdr.h"

// constants -------------------------------------------------------------------
#define PROG_NAME "pocket_acq" // program name
#define T_AQC     0.010      // non-coherent integration time for acquisition (s)
#define THRES_CN0 38.0       // threshold to lock (dB-Hz)
#define ESC_COL   "\033[34m" // ANSI escape color = blue
#define ESC_RES   "\033[0m"  // ANSI escape reset
#define FFTW_WISDOM "../python/fftw_wisdom.txt"
#define AGC_LEVEL 2.3

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
    printf("       [-toff toff] [-fmt fmt] [-f freq] [-fi freq] [-d freq[,freq]]\n");
    printf("       [-nz] file\n");
    exit(0);
}

// sample bytes per IF sample --------------------------------------------------
static int sample_byte(int fmt, int IQ)
{
    switch (fmt) {
        case SDR_FMT_CS8   : return 2;
        case SDR_FMT_CS16  : return 4;
        default            : return IQ;
    }
}

// clip int to 4-bit IF sample -------------------------------------------------
static int8_t clip4(double x)
{
    int v = (int)floor(x + (x >= 0.0 ? 0.5 : -0.5));
    if (v < -7) return -7;
    if (v >  7) return  7;
    return (int8_t)v;
}

// read CS8/CS16 IF data -------------------------------------------------------
static sdr_buff_t *read_cs_data(const char *file, int fmt, double fs, double T,
    double toff)
{
    int bs = sample_byte(fmt, 2);
    size_t cnt = (T > 0.0) ? (size_t)(fs * T) : 0;
    size_t off = (size_t)(fs * toff) * bs, size;
    FILE *fp;

    if (!(fp = fopen(file, "rb"))) {
        fprintf(stderr, "data read error %s\n", file);
        return NULL;
    }
#if defined(WIN32)
    _fseeki64(fp, 0, SEEK_END);
    size = (size_t)_ftelli64(fp);
    _fseeki64(fp, 0, SEEK_SET);
#else
    fseeko(fp, 0, SEEK_END);
    size = (size_t)ftello(fp);
    fseeko(fp, 0, SEEK_SET);
#endif
    if (off > size) {
        fclose(fp);
        return NULL;
    }
    if (cnt <= 0) cnt = (size - off) / bs;
    if (cnt * bs > size - off) {
        fclose(fp);
        return NULL;
    }
    uint8_t *raw = (uint8_t *)sdr_malloc(cnt * bs);
#if defined(WIN32)
    _fseeki64(fp, (long long)off, SEEK_SET);
#else
    fseeko(fp, (off_t)off, SEEK_SET);
#endif
    if (fread(raw, bs, cnt, fp) < cnt) {
        fprintf(stderr, "data read error %s\n", file);
        fclose(fp);
        sdr_free(raw);
        return NULL;
    }
    fclose(fp);

    double sum[2] = {0}, sumsq[2] = {0}, ave[2], std[2], scale[2];
    for (size_t i = 0; i < cnt; i++) {
        double I, Q;
        if (fmt == SDR_FMT_CS8) {
            I = (int8_t)raw[i*2];
            Q = (int8_t)raw[i*2+1];
        } else {
            I = *(int16_t *)(raw + i*4);
            Q = *(int16_t *)(raw + i*4 + 2);
        }
        sum[0] += I;
        sum[1] += Q;
        sumsq[0] += I * I;
        sumsq[1] += Q * Q;
    }
    for (int i = 0; i < 2; i++) {
        ave[i] = cnt > 0 ? sum[i] / cnt : 0.0;
        std[i] = cnt > 0 ? sqrt(sumsq[i] / cnt - ave[i] * ave[i]) : 0.0;
        scale[i] = std[i] > 0.0 ? std[i] / AGC_LEVEL : 1.0;
    }
    sdr_buff_t *buff = sdr_buff_new((int)cnt, 2);
    for (int i = 0; i < buff->N; i++) {
        double I, Q;
        if (fmt == SDR_FMT_CS8) {
            I = (int8_t)raw[i*2];
            Q = (int8_t)raw[i*2+1];
        } else {
            I = *(int16_t *)(raw + i*4);
            Q = *(int16_t *)(raw + i*4 + 2);
        }
        buff->data[i] = SDR_CPX8(clip4((I - ave[0]) / scale[0]),
            clip4((Q - ave[1]) / scale[1]));
    }
    sdr_free(raw);
    return buff;
}

// read IF data ----------------------------------------------------------------
static sdr_buff_t *read_data(const char *file, int fmt, double fs, int IQ,
    double T, double toff)
{
    if (fmt == SDR_FMT_CS8 || fmt == SDR_FMT_CS16) {
        return read_cs_data(file, fmt, fs, T, toff);
    }
    return sdr_read_data(file, fs, IQ, T, toff);
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
            else if (!strcmp(format, "CS8"   )) fmt = SDR_FMT_CS8;
            else if (!strcmp(format, "CS16"  )) fmt = SDR_FMT_CS16;
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
        if (fmt != SDR_FMT_INT8 && fmt != SDR_FMT_INT8X2 &&
            fmt != SDR_FMT_CS8 && fmt != SDR_FMT_CS16) {
            fprintf(stderr, "Unsupported format: %s\n", file);
            exit(-1);
        }
        fi = sdr_sig_freq(sig) - fo[0];
        IQ = IQ_t[0];
    }
    // read IF data
    sdr_buff_t *buff;
    if (!(buff = read_data(file, fmt, fs, IQ, T + Tcode, toff))) {
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
