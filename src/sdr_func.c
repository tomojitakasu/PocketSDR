// 
//  Pocket SDR C Library - Fundamental GNSS SDR Functions.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-01-25  1.0  new
//  2022-05-11  1.1  add API: search_code(), corr_max(), fine_dop(),
//                   shift_freq(), dop_bins(), add_buff(), xor_bits()
//  2022-05-18  1.2  change API: *() -> sdr_*()
//  2022-05-23  1.3  add API: sdr_read_data(), sdr_parse_nums()
//
#include <math.h>
#include "rtklib.h"
#include "pocket_sdr.h"

#ifdef AVX2
#include <immintrin.h>
#endif

// constants and macros --------------------------------------------------------
#define NTBL      256     // carrier lookup table size 
#define FFTW_FLAG FFTW_ESTIMATE // FFTW flag 

#define MAX_DOP   5000.0  // default max Doppler frequency to search signals (Hz)
#define DOP_STEP  0.5     // Doppler frequency search step (* 1 / code cycle)

#define SQR(x)    ((x) * (x))

// initialize library ----------------------------------------------------------
void sdr_init_lib(const char *file)
{
    // import FFTW wisdom 
    if (*file && !fftwf_import_wisdom_from_filename(file)) {
        fprintf(stderr, "FFTW wisdom import error %s\n", file);
    }
}

// mix carrier (N = 2 * n) -----------------------------------------------------
void sdr_mix_carr(const sdr_cpx_t *buff, int ix, int N, double fs, double fc,
    double phi, sdr_cpx_t *data)
{
    static float cos_tbl[NTBL] = {0}, sin_tbl[NTBL] = {0};
    
    // generate carrier lookup table 
    if (cos_tbl[0] == 0.0f) {
        for (int i = 0; i < NTBL; i++) {
            cos_tbl[i] = cosf(-2.0f * (float)PI * i / NTBL);
            sin_tbl[i] = sinf(-2.0f * (float)PI * i / NTBL);
        }
    }
    double step = fc / fs * NTBL;
    phi = fmod(phi, 1.0) * NTBL;
    
    for (int i = 0, j = ix; i < N; i += 2, j += 2) {
        double p = phi + step * i;
        uint8_t k = (uint8_t)p;
        uint8_t m = (uint8_t)(p + step);
        
        data[i  ][0] = buff[j  ][0] * cos_tbl[k] - buff[j  ][1] * sin_tbl[k];
        data[i  ][1] = buff[j  ][0] * sin_tbl[k] + buff[j  ][1] * cos_tbl[k];
        data[i+1][0] = buff[j+1][0] * cos_tbl[m] - buff[j+1][1] * sin_tbl[m];
        data[i+1][1] = buff[j+1][0] * sin_tbl[m] + buff[j+1][1] * cos_tbl[m];
    }
}

// inner product of complex and real -------------------------------------------
static void dot_cpx_real(const sdr_cpx_t *a, const sdr_cpx_t *b, int N, float s,
    sdr_cpx_t *c)
{
#ifdef AVX2
    int i;
    
    __m256 ymm1 = _mm256_setzero_ps();
    __m256 ymm2 = _mm256_setzero_ps();
    
    for (i = 0; i < N - 7; i += 8) {
        __m256 ymm3 = _mm256_loadu_ps(a[i]);
        __m256 ymm4 = _mm256_loadu_ps(a[i + 4]);
        __m256 ymm5 = _mm256_shuffle_ps(ymm3, ymm4, 0x88); // a.real 
        __m256 ymm6 = _mm256_shuffle_ps(ymm3, ymm4, 0xDD); // a.imag 
        ymm3 = _mm256_loadu_ps(b[i]);
        ymm4 = _mm256_loadu_ps(b[i + 4]);
        __m256 ymm7 = _mm256_shuffle_ps(ymm3, ymm4, 0x88); // b.real 
        ymm1 = _mm256_fmadd_ps(ymm5, ymm7, ymm1);   // c.real 
        ymm2 = _mm256_fmadd_ps(ymm6, ymm7, ymm2);   // c.imag 
    }
    float d[8], e[8];
    _mm256_storeu_ps(d, ymm1);
    _mm256_storeu_ps(e, ymm2);
    (*c)[0] = (d[0] + d[1] + d[2] + d[3] + d[4] + d[5] + d[6] + d[7]) * s;
    (*c)[1] = (e[0] + e[1] + e[2] + e[3] + e[4] + e[5] + e[6] + e[7]) * s;
    
    for ( ; i < N; i++) {
        (*c)[0] += a[i][0] * b[i][0] * s;
        (*c)[1] += a[i][1] * b[i][0] * s;
    }
#else
    (*c)[0] = (*c)[1] = 0.0f;
    
    for (int i = 0; i < N; i++) {
        (*c)[0] += a[i][0] * b[i][0];
        (*c)[1] += a[i][1] * b[i][0];
    }
    (*c)[0] *= s;
    (*c)[1] *= s;
#endif // AVX2 
}

// standard correlator ---------------------------------------------------------
static void corr_std_(const sdr_cpx_t *data, const sdr_cpx_t *code, int N,
    const int *pos, int n, sdr_cpx_t *corr)
{
    for (int i = 0; i < n; i++) {
        if (pos[i] > 0) {
            int M = N - pos[i];
            dot_cpx_real(data + pos[i], code, M, 1.0f / M, corr + i);
        }
        else if (pos[i] < 0) {
            int M = N + pos[i];
            dot_cpx_real(data, code - pos[i], M, 1.0f / M, corr + i);
        }
        else {
            dot_cpx_real(data, code, N, 1.0f / N, corr + i);
        }
    }
}

// multiplication of complex64 (N = 2 * n) -------------------------------------
static void mul_cpx(const sdr_cpx_t *a, const sdr_cpx_t *b, int N, float s,
    sdr_cpx_t *c)
{
    for (int i = 0; i < N; i += 2) {
        c[i  ][0] = (a[i  ][0] * b[i  ][0] - a[i  ][1] * b[i  ][1]) * s;
        c[i  ][1] = (a[i  ][0] * b[i  ][1] + a[i  ][1] * b[i  ][0]) * s;
        c[i+1][0] = (a[i+1][0] * b[i+1][0] - a[i+1][1] * b[i+1][1]) * s;
        c[i+1][1] = (a[i+1][0] * b[i+1][1] + a[i+1][1] * b[i+1][0]) * s;
    }
}

// FFT correlator (N = 2 * n) --------------------------------------------------
static void corr_fft_(const sdr_cpx_t *data, const sdr_cpx_t *code_fft, int N,
    sdr_cpx_t *corr)
{
    static fftwf_plan plan[2] = {0};
    static int N_plan = 0;
    sdr_cpx_t *cpx1 = sdr_cpx_malloc(N);
    sdr_cpx_t *cpx2 = sdr_cpx_malloc(N);
    
    // generate FFTW plan 
    if (N != N_plan) {
        if (plan[0]) {
            fftwf_destroy_plan(plan[0]);
            fftwf_destroy_plan(plan[1]);
        }
        plan[0] = fftwf_plan_dft_1d(N, cpx1, cpx2, FFTW_FORWARD,  FFTW_FLAG);
        plan[1] = fftwf_plan_dft_1d(N, cpx2, cpx1, FFTW_BACKWARD, FFTW_FLAG);
        N_plan = N;
    }
    // ifft(fft(data) * code_fft) / N^2 
    fftwf_execute_dft(plan[0], (sdr_cpx_t *)data, cpx1);
    mul_cpx(cpx1, code_fft, N, 1.0f / N / N, cpx2);
    fftwf_execute_dft(plan[1], cpx2, corr);
    
    sdr_cpx_free(cpx1);
    sdr_cpx_free(cpx2);
}

// mix carrier and standard correlator (N = 2 * n) -----------------------------
void sdr_corr_std(const sdr_cpx_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx_t *code, const int *pos, int n, sdr_cpx_t *corr)
{
    sdr_cpx_t *data = sdr_cpx_malloc(N);
    
    sdr_mix_carr(buff, ix, N, fs, fc, phi, data);
    corr_std_(data, code, N, pos, n, corr);
    
    sdr_cpx_free(data);
}

// mix carrier and FFT correlator (N = 2 * n) ----------------------------------
void sdr_corr_fft(const sdr_cpx_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx_t *code_fft, sdr_cpx_t *corr)
{
    sdr_cpx_t *data = sdr_cpx_malloc(N);

    sdr_mix_carr(buff, ix, N, fs, fc, phi, data);
    corr_fft_(data, code_fft, N, corr);
    
    sdr_cpx_free(data);
}

// generate FFTW wisdom --------------------------------------------------------
int sdr_gen_fftw_wisdom(const char *file, int N)
{
    fftwf_plan plan[2] = {0};
    
    sdr_cpx_t *cpx1 = sdr_cpx_malloc(N);
    sdr_cpx_t *cpx2 = sdr_cpx_malloc(N);
    plan[0] = fftwf_plan_dft_1d(N, cpx1, cpx2, FFTW_FORWARD,  FFTW_PATIENT);
    plan[1] = fftwf_plan_dft_1d(N, cpx2, cpx1, FFTW_BACKWARD, FFTW_PATIENT);
    
    int stat = fftwf_export_wisdom_to_filename(file);
    
    fftwf_destroy_plan(plan[0]);
    fftwf_destroy_plan(plan[1]);
    sdr_cpx_free(cpx1);
    sdr_cpx_free(cpx2);
    
    return stat;
}

//------------------------------------------------------------------------------
//  Read digitalized IF (inter-frequency) data from file. Supported file format
//  is signed byte (int8) for I-sampling (real-sampling) or interleaved singned
//  byte for IQ-sampling (complex-sampling).
//
//  args:
//      file     (I) Digitalized IF data file path
//      fs       (I) Sampling frequency (Hz)
//      IQ       (I) Sampling type (1: I-sampling, 2: IQ-sampling)
//      T        (I) Sample period (s) (0: all samples)
//      toff     (I) Time offset from the beginning (s)
//      len_data (O) length of data
//
//  return:
//      Digitized IF data as complex array (NULL: read error)
//
sdr_cpx_t *sdr_read_data(const char *file, double fs, int IQ, double T,
    double toff, int *len_data)
{
    size_t cnt = (T > 0.0) ? (size_t)(fs * T * IQ) : 0;
    size_t off = (size_t)(fs * toff * IQ);
    FILE *fp;
    
    if (!(fp = fopen(file, "rb"))) {
        fprintf(stderr, "data read error %s\n", file);
        return NULL;
    }
    // get file size
    fseek(fp, 0, SEEK_END);
    size_t size = (size_t)ftell(fp);
    rewind(fp);
    
    if (cnt <= 0) {
        cnt = size - off;
    }
    if (size < off + cnt) {
        fprintf(stderr, "data size error %s\n", file);
        fclose(fp);
        return NULL;
    }
    int8_t *raw = (int8_t *)sdr_malloc(cnt);
    fseek(fp, off, SEEK_SET);
    
    if (fread(raw, 1, cnt, fp) < cnt) {
        fprintf(stderr, "data read error %s\n", file);
        fclose(fp);
        return NULL;
    }
    sdr_cpx_t *data;
    *len_data = (IQ == 1) ? cnt : cnt / 2;
    data = sdr_cpx_malloc(*len_data);
    
    if (IQ == 1) { // I-sampling
        for (int i = 0; i < *len_data; i++) {
            data[i][0] = raw[i];
            data[i][1] = 0.0;
        }
    }
    else { // IQ-sampling
        for (int i = 0; i < *len_data; i++) {
            data[i][0] =  raw[i*2  ];
            data[i][1] = -raw[i*2+1];
        }
    }
    sdr_free(raw);
    fclose(fp);
    return data;
}

// parse numbers list and range ------------------------------------------------
int sdr_parse_nums(const char *str, int *prns)
{
    int n = 0, prn, prn1, prn2;
    char buff[1024], *p, *q;
    
    sprintf(buff, "%*s", (int)sizeof(buff) - 1, str);
    
    for (p = buff; ; p = q + 1) {
        if ((q = strchr(p, ','))) {
            *q = '\0';
        }
        if (sscanf(p, "%d-%d", &prn1, &prn2) == 2) {
            for (prn = prn1; prn <= prn2 && n < SDR_MAX_NPRN; prn++) {
                prns[n++] = prn;
            }
        }
        else if (sscanf(p, "%d", &prn) == 1 && n < SDR_MAX_NPRN) {
             prns[n++] = prn;
        }
        if (!q) break;
    }
    return n;
}

//------------------------------------------------------------------------------
//  Parallel code search in digitized IF data.
//
//  args:
//      code_fft (I) Code DFT (with or w/o zero-padding) as complex array
//      T        (I) Code cycle (period) (s)
//      buff     (I) Buffer of IF data as complex array
//      ix       (I) Index of buffer
//      N        (I) length of buffer
//      fs       (I) Sampling frequency (Hz)
//      fi       (I) IF frequency (Hz)
//      fds      (I) Doppler frequency bins as ndarray (Hz)
//      len_fds  (I) length of Doppler frequency bins
//      P        (IO) Correlation powers in the Doppler frequencies - Code offset
//                   space as float 2D-array (N x len_fs, N = (int)(fs * T))
//
//  return:
//      none
//
void sdr_search_code(const sdr_cpx_t *code_fft, double T, const sdr_cpx_t *buff,
    int ix, int N, double fs, double fi, const float *fds, int len_fds,
    float *P)
{
    sdr_cpx_t *C = sdr_cpx_malloc(N);
    
    for (int i = 0; i < len_fds; i++) {
        
        // FFT correlator
        sdr_corr_fft(buff, ix, N, fs, fi + fds[i], 0.0, code_fft, C);
        
        // add correlation power
        for (int j = 0; j < N; j++) {
            P[i*N+j] += SQR(C[j][0]) + SQR(C[j][1]); // abs(C[j]) ** 2
        }
    }
    sdr_cpx_free(C);
}

// max correlation power and C/N0 ----------------------------------------------
float sdr_corr_max(const float *P, int N, int Nmax, int M, double T, int *ix)
{
    float P_max = 0.0, P_ave = 0.0;
    int n = 0;
    
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < Nmax; j++) {
            P_ave += (P[i*N+j] - P_ave) / ++n;
            if (P[i*N+j] <= P_max) continue;
            P_max = P[i*N+j];
            ix[0] = i; // index of doppler freq.
            ix[1] = j; // index of code offset
        }
    }
    return (P_ave > 0.0) ? 10.0 * log10f((P_max - P_ave) / P_ave / T) : 0.0;
}

// polynomial fitting ----------------------------------------------------------
static int poly_fit(const double *x, const double *y, int nx, int np, double *p)
{
    if (nx < np) {
        return 0;
    }
    double *V = mat(np, nx), *Q = mat(np, np);
    
    for (int i = 0; i < nx; i++) { // Vandermonde matrix
        for (int j = 0; j < np; j++) {
            V[i*np+j] = (j == 0) ? 1.0 : V[i*np+j-1] * x[i];
        }
    }
    int stat = lsq(V, y, np, nx, p, Q);
    free(V);
    free(Q);
    return !stat;
}

// fine Doppler frequency by quadratic fitting ---------------------------------
float sdr_fine_dop(const float *P, int N, const float *fds, int len_fds,
    const int *ix)
{
    if (ix[0] == 0 || ix[0] == len_fds - 1) {
        return fds[ix[0]];
    }
    double x[3], y[3], p[3];
    
    for (int i = 0; i < 3; i++) {
        x[i] = fds[ix[0]-1+i];
        y[i] = P[(ix[0]-1+i)*N+ix[1]];
    }
    if (!poly_fit(x, y, 3, 3, p)) {
        return fds[ix[0]];
    }
    return (float)(-p[1] / (2.0 * p[2]));
}

// shift IF frequency for GLONASS FDMA -----------------------------------------
double sdr_shift_freq(const char *sig, int fcn, double fi)
{
    if (!strcmp(sig, "G1CA")) {
        fi += 0.5625e6 * fcn;
    }
    else if (!strcmp(sig, "G2CA")) {
        fi += 0.4375e6 * fcn;
    }
    return fi;
}

// doppler search bins ---------------------------------------------------------
float *sdr_dop_bins(double T, float dop, float max_dop, int *len_fds)
{
    float *fds, step = DOP_STEP / T;
    
    *len_fds = (int)(2.0 * max_dop / step) + 1;
    fds = (float *)sdr_malloc(sizeof(float) * (*len_fds));
    
    for (int i = 0; i < *len_fds; i++) {
        fds[i] = dop - max_dop + i * step;
    }
    return fds;
}

// add item to buffer -----------------------------------------------------------
void sdr_add_buff(void *buff, int len_buff, void *item, size_t size_item)
{
    memmove(buff, buff + size_item, size_item * (len_buff - 1));
    memcpy(buff + size_item * (len_buff - 1), item, size_item);
}

// exclusive-or of all bits ----------------------------------------------------
uint8_t sdr_xor_bits(uint32_t X)
{
    static const uint8_t xor_8b[] = { // xor of 8 bits
        0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0, 1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1,
        1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1, 0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0,
        1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1, 0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0,
        0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0, 1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1,
        1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1, 0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0,
        0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0, 1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1,
        0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0, 1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1,
        1,0,0,1,0,1,1,0, 0,1,1,0,1,0,0,1, 0,1,1,0,1,0,0,1, 1,0,0,1,0,1,1,0
    };
    return xor_8b[(uint8_t)X] ^ xor_8b[(uint8_t)(X >> 8)] ^
        xor_8b[(uint8_t)(X >> 16)] ^ xor_8b[(uint8_t)(X >> 24)];
}

