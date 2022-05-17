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
//
#include <math.h>
#include "pocket.h"

#ifdef AVX2
#include <immintrin.h>
#endif

// constants and macros --------------------------------------------------------
#define NTBL      256     // carrier lookup table size 
#define FFTW_FLAG FFTW_ESTIMATE // FFTW flag 

#define MAX_DOP   5000.0  // default max Doppler frequency to search signals (Hz)
#define DOP_STEP  0.5     // Doppler frequency search step (* 1 / code cycle)

#define SQR(x)    ((x) * (x))

// global variables ------------------------------------------------------------
static float cos_tbl[NTBL] = {0}; // carrier lookup table 
static float sin_tbl[NTBL] = {0};

// initialize library ----------------------------------------------------------
void init_lib(const char *file)
{
    int i;
    
    // generate carrier lookup table 
    for (i = 0; i < NTBL; i++) {
        cos_tbl[i] = cosf(-2.0f * (float)PI * i / NTBL);
        sin_tbl[i] = sinf(-2.0f * (float)PI * i / NTBL);
    }
    // import FFTW wisdom 
    if (*file && !fftwf_import_wisdom_from_filename(file)) {
        fprintf(stderr, "FFTW wisdom import error %s\n", file);
    }
}

// mix carrier (N = 2 * n) -----------------------------------------------------
void mix_carr(const sdr_cpx_t *buff, int ix, int N, double fs, double fc,
    double phi, sdr_cpx_t *data)
{
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
void dot_cpx_real(const sdr_cpx_t *a, const sdr_cpx_t *b, int N, float s,
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
        __m256 ymm7 = _mm256_shuffle_ps(ymm3, ymm4, 0x88); // b.real 
        ymm3 = _mm256_loadu_ps(b[i]);
        ymm4 = _mm256_loadu_ps(b[i + 4]);
        ymm1 = _mm256_fmadd_ps(ymm5, ymm7, ymm1);   // c.real 
        ymm2 = _mm256_fmadd_ps(ymm6, ymm7, ymm2);   // c.imag 
    }
    float d[8], e[8];
    _mm256_storeu_ps(d, ymm1);
    _mm256_storeu_ps(e, ymm2);
    *c[0] = (d[0] + d[1] + d[2] + d[3] + d[4] + d[5] + d[6] + d[7]) * s;
    *c[1] = (e[0] + e[1] + e[2] + e[3] + e[4] + e[5] + e[6] + e[7]) * s;
    
    for ( ; i < N; i++) {
        *c[0] += a[i][0] * b[i][0] * s;
        *c[1] += a[i][1] * b[i][0] * s;
    }
#else
    *c[0] = *c[1] = 0.0f;
    
    for (int i = 0; i < N; i++) {
        *c[0] += a[i][0] * b[i][0];
        *c[1] += a[i][1] * b[i][0];
    }
    *c[0] *= s;
    *c[1] *= s;
#endif // AVX2 
}

// standard correlator ---------------------------------------------------------
static void corr_std_(const sdr_cpx_t *data, const sdr_cpx_t *code, int N,
    const int *pos, int n, sdr_cpx_t *corr)
{
    for (int i = 0, j = 0; i < n; i++, j++) {
        if (pos[i] > 0) {
            int M = N - pos[i];
            dot_cpx_real(data + pos[i], code, M, 1.0f / M, corr + j);
        }
        else if (pos[i] < 0) {
            int M = N + pos[i];
            dot_cpx_real(data, code - pos[i], M, 1.0f / M, corr + j);
        }
        else {
            dot_cpx_real(data, code, N, 1.0f / N, corr + j);
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
    mul_cpx(cpx1, code_fft, N, 1.0f / SQR(N), cpx2);
    fftwf_execute_dft(plan[1], cpx2, corr);
    
    sdr_cpx_free(cpx1);
    sdr_cpx_free(cpx2);
}

// mix carrier and standard correlator (N = 2 * n) -----------------------------
void corr_std(const sdr_cpx_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx_t *code, const int *pos, int n, sdr_cpx_t *corr)
{
    sdr_cpx_t *data = sdr_cpx_malloc(N);
    
    mix_carr(buff, ix, N, fs, fc, phi, data);
    corr_std_(data, code, N, pos, n, corr);
    
    sdr_cpx_free(data);
}

// mix carrier and FFT correlator (N = 2 * n) ----------------------------------
void corr_fft(const sdr_cpx_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx_t *code_fft, sdr_cpx_t *corr)
{
    sdr_cpx_t *data = sdr_cpx_malloc(N);
    
    mix_carr(buff, ix, N, fs, fc, phi, data);
    corr_fft_(data, code_fft, N, corr);
    
    sdr_free(data);
}

// generate FFTW wisdom --------------------------------------------------------
int gen_fftw_wisdom(const char *file, int N)
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
//  Parallel code search in digitized IF data.
//
//  args:
//      code_fft (I) Code DFT (with or w/o zero-padding) as complex array
//      T        (I) Code cycle (period) (s)
//      buff     (I) Buffer of IF data as complex array
//      ix       (I) Index of buffer
//      fs       (I) Sampling frequency (Hz)
//      fi       (I) IF frequency (Hz)
//      fds      (I) Doppler frequency bins as ndarray (Hz)
//      len_fds  (I) length of Doppler frequency bins
//      P        (O) Correlation powers in the Doppler frequencies - Code offset
//                   space as float 2D-array (N x len_fs, N = (int)(fs * T))
//
//  return:
//      none
//
void search_code(const sdr_cpx_t *code_fft, double T, const sdr_cpx_t *buff,
    int ix, double fs, double fi, const float *fds, int len_fds, float *P)
{
    int N = (int)(fs * T);
    sdr_cpx_t *C = sdr_cpx_malloc(len_fds);
    
    for (int i = 0; i < len_fds; i++) {
        corr_fft(buff, ix, N, fs, fi + fds[i], 0.0, code_fft, C);
        for (int j = 0; j < N; j++) {
            P[i*N+j] = SQR(C[j][0]) + SQR(C[j][1]); // abs(C) ** 2
        }
    }
    sdr_free(C);
}

// max correlation power and C/N0 ----------------------------------------------
float corr_max(const float *P, int N, int len_fds, double T, int *ix,
    float *cn0)
{
    float P_max = 0.0, P_ave = 0.0;
    int n = 0;
    
    for (int i = 0; i < len_fds; i++) {
        for (int j = 0; j < N; j++) {
            P_ave += (P[i*N+j] - P_ave) / ++n;
            if (P[i*N+j] <= P_max) continue;
            P_max = P[i*N+j];
            ix[0] = i;
            ix[1] = j;
        }
    }
    *cn0 = (P_ave > 0.0) ? 10.0 * log10f((P_max - P_ave) / P_ave / T) : 0.0;
    return P_max;
}

// polynomial fitting ----------------------------------------------------------
static int polyfit(const double *x, const double *y, int nx, int np, double *p)
{
#if 0
    if (nt < np) {
        return 0;
    }
    double V = mat(np, nx);
    double Q = mat(np, np);
    
    for (int i = 0; i < nx; i++) { // Vandermonde matrix
        for (int j = 0; j < np; j++) {
            V[i*np+j] = (j == 0) ? 1.0 : V[i*np+j-1] * x[i];
        }
    }
    int stat = lsq(V, y, np, nx, p, Q);
    free(V);
    free(Q);
    return !stat;
#else
    return 0;
#endif
}

// fine Doppler frequency by quadratic fitting ---------------------------------
float fine_dop(const float *P, const float *fds, int len_fds, int ix)
{
    if (ix == 0 || ix == len_fds - 1) {
        return fds[ix];
    }
    double x[3], y[3], p[3];
    
    for (int i = 0; i < 3; i++) {
        x[i] = fds[ix - 1 + i];
        y[i] = P[ix - 1 + i];
    }
    if (!polyfit(x, y, 3, 3, p)) {
        return fds[ix];
    }
    return (float)(-p[1] / (2.0 * p[0]));
}

// shift IF frequency for GLONASS FDMA -----------------------------------------
double shift_freq(const char *sig, int fcn, double fi)
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
float *dop_bins(double T, float dop, float max_dop, int *len_fds)
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
void add_buff(void *buff, int len_buff, void *item, size_t size_item)
{
    memmove(buff, buff + size_item, size_item * (len_buff - 1));
    memcpy(buff + size_item * (len_buff - 1), item, size_item);
}

// exclusive-or of all bits ----------------------------------------------------
uint8_t xor_bits(uint32_t X)
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

