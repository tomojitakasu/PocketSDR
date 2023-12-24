//
//  Pocket SDR C AP - GNSS Signal Acquisition.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-07-05  1.0  port pocket_acq.py to C
//  2022-08-08  1.1  add option -w, modify option -d
//
#include "pocket_sdr.h"

// constants -------------------------------------------------------------------
#define T_AQC     0.010      // non-coherent integration time for acquisition (s)
#define THRES_CN0 38.0       // threshold to lock (dB-Hz)
#define ESC_COL   "\033[34m" // ANSI escape color = blue
#define ESC_RES   "\033[0m"  // ANSI escape reset
#define FFTW_WISDOM "../python/fftw_wisdom.txt"

// show usage ------------------------------------------------------------------
static void show_usage(void)
{
    printf("Usage: pocket_acq [-sig sig] [-prn prn[,...]] [-tint tint]\n");
    printf("       [-toff toff] [-f freq] [-fi freq] [-d freq[,freq]] [-nz] file\n");
    exit(0);
}

// search signal ---------------------------------------------------------------
int search_sig(const char *sig, int prn, const sdr_cpx_t *data, int len_data,
    double fs, double fi, float ref_dop, float max_dop, const int *opt,
    double *dop, double *coff, float *cn0)
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
    sdr_gen_code_fft(code, len_code, T, 0.0, fs, N, Nz, code_fft);
    
    // doppler search bins
    int len_fds;
    float *fds = sdr_dop_bins(T, ref_dop, max_dop, &len_fds);
    
    // parallel code search and non-coherent integration
    float *P = (float *)sdr_malloc(sizeof(float) * (N + Nz) * len_fds);
    for (int i = 0; i < len_data - 2 * N + 1; i += N) {
        sdr_search_code(code_fft, T, data, len_data, i, N + Nz, fs, fi, fds,
            len_fds, P);
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

//------------------------------------------------------------------------------
//
//   Synopsis
//
//     pocket_aqc [-sig sig] [-prn prn[,...]] [-tint tint] [-toff toff]
//         [-f freq] [-fi freq] [-d freq] [-nz] file
//
//   Description
//
//     Search GNSS signals in digital IF data and plot signal search results.
//     If single PRN number by -prn option, it plots correlation power and
//     correlation shape of the specified GNSS signal. If multiple PRN numbers
//     specified by -prn option, it plots C/N0 for each PRN.
// 
//   Options ([]: default)
//  
//     -sig sig
//         GNSS signal type ID (L1CA, L2CM, ...). See below for details. [L1CA]
// 
//     -prn prn[,...]
//         PRN numbers of the GNSS signal separated by ','. A PRN number can be a
//         PRN number range like 1-32 with start and end PRN numbers. For GLONASS
//         FDMA signals (G1CA, G2CA), the PRN number is treated as FCN (frequency
//         channel number). [1]
// 
//     -tint tint
//         Integration time in ms to search GNSS signals. [code cycle]
// 
//     -toff toff
//         Time offset from the start of digital IF data in ms. [0.0]
// 
//     -f freq
//         Sampling frequency of digital IF data in MHz. [12.0]
//
//     -fi freq
//         IF frequency of digital IF data in MHz. The IF frequency equals 0, the
//         IF data is treated as IQ-sampling (zero-IF). [0.0]
//
//     -d freq[,freq]
//         Reference and max Doppler frequency to search the signal in Hz.
//         [0.0,5000.0]
//
//     -nz
//         Disalbe zero-padding for circular colleration to search the signal.
//         [enabled]
//
//     -h
//         Show usage and signal type IDs
//
//     file
//         File path of the input digital IF data. The format should be a series of
//         int8_t (signed byte) for real-sampling (I-sampling) or interleaved int8_t
//         for complex-sampling (IQ-sampling). PocketSDR and AP pocket_dump can be
//         used to capture such digital IF data.
//
int main(int argc, char **argv)
{
    const char *sig = "L1CA", *file = "", *fftw_wisdom = FFTW_WISDOM;
    double fs = 12e6, fi = 0.0, T = T_AQC, toff = 0.0, ref_dop = 0.0;
    double max_dop = 5000.0;
    int prns[SDR_MAX_NPRN], nprn = 0, opt[5] = {0};
    
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
        else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            fs = atof(argv[++i]) * 1e6;
        }
        else if (!strcmp(argv[i], "-fi") && i + 1 < argc) {
            fi = atof(argv[++i]) * 1e6;
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
    
    // read IF data
    sdr_cpx_t *data;
    int len_data;
    if (!(data = sdr_read_data(file, fs, (fi > 0) ? 1 : 2, T + Tcode, toff,
        &len_data))) {
        exit(-1);
    }
    uint32_t tick = sdr_get_tick();
    
    // search signal
    for (int i = 0; i < nprn; i++) {
        double dop, coff;
        float cn0;
        
        if (!search_sig(sig, prns[i], data, len_data, fs, fi, ref_dop, max_dop,
                opt, &dop, &coff, &cn0)) {
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
