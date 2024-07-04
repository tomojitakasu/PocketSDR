// 
//  Pocket SDR C Library - Fundamental GNSS SDR Functions
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
//  2022-07-08  1.4  port sdr_func.py to C
//  2022-08-04  1.5  ftell(),fseek() -> fgetpos(),fsetpos()
//  2022-08-15  1.6  ensure thread-safety of sdr_corr_fft_()
//  2023-12-28  1.7  support API changes
//                   enable escape sequence for Windows console
//  2024-01-03  1.8  fix AVX2 codes in dot_cpx_real()
//  2024-01-18  1.9  add API sdr_cpx_mul()
//  2024-03-20  1.10 add API sdr_buff_new(), sdr_buff_free()
//                   modify API sdr_read_data(), sdr_search_code(),
//                   sdr_mix_carr(), sdr_corr_std(), sdr_corr_fft()
//  2024-03-25  1.11 optimized
//  2024-03-29  1.12 add API sdr_corr_std_cpx(), sdr_corr_fft_cpx()
//                   support ARM and NEON
//  2024-05-13  1.13 add API sdr_str_open(), sdr_str_close()
//  2024-06-29  1.14 add API sdr_psd_cpx()
//
#include <math.h>
#include <stdarg.h>
#include "pocket_sdr.h"

#if defined(WIN32)
#include <io.h>
#endif
#if defined(AVX2)
#include <immintrin.h>
#elif defined(NEON)
#include <arm_neon.h>
#endif

// constants and macros --------------------------------------------------------
#define NTBL          256   // carrier-mixed-data LUT size
#define DOP_STEP      0.5   // Doppler frequency search step (* 1 / code cycle)
#define MAX_FFTW_PLAN 32    // max number of FFTW plans
#define MAX_LOG_BUFF  32768 // max sizeof log buffer
#define FFTW_FLAG     FFTW_ESTIMATE // FFTW flag

#define SQR(x)        ((x) * (x))
#define MIN(x, y)     ((x) < (y) ? (x) : (y))
#define ROUND(x)      floor(x + 0.5)

// global variables ------------------------------------------------------------
static sdr_cpx16_t mix_tbl[NTBL*256] = {{0,0}}; // carrier-mixed-data LUT
static fftwf_plan fftw_plans[MAX_FFTW_PLAN][2] = {{0}}; // FFTW plan buffer
static int fftw_size[MAX_FFTW_PLAN] = {0}; // FFTW plan sizes
static int log_lvl = 3;           // log level
static stream_t *log_str = NULL;  // log stream
static char log_buff[MAX_LOG_BUFF]; // log buffer
static int log_buff_p = 0;        // log buffer pointer
static pthread_mutex_t log_buff_mtx = PTHREAD_MUTEX_INITIALIZER;

// enable escape sequence for Windows console ----------------------------------
static void enable_console_esc(void)
{
#ifdef WIN32
    HANDLE h = (HANDLE)_get_osfhandle(1); // stdout
    DWORD mode = 0;
    
    if (!GetConsoleMode(h, &mode) ||
        !SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        //fprintf(stderr, "SetConsoleMode() error (%ld)\n", GetLastError());
    }
#endif
}

// initialize GNSS SDR functions -----------------------------------------------
void sdr_func_init(const char *file)
{
    // initilize log stream
    strinitcom();
    
    // import FFTW wisdom 
    if (*file && !fftwf_import_wisdom_from_filename(file)) {
        fprintf(stderr, "FFTW wisdom import error %s\n", file);
    }
    // generate carrier-mixed-data LUT
    for (int i = 0; i < NTBL; i++) {
        int8_t carr_I = (int8_t)ROUND(cos(-2.0 * PI * i / NTBL) / SDR_CSCALE);
        int8_t carr_Q = (int8_t)ROUND(sin(-2.0 * PI * i / NTBL) / SDR_CSCALE);
        for (int j = 0; j < 256; j++) {
            int8_t I = SDR_CPX8_I(j), Q = SDR_CPX8_Q(j);
            mix_tbl[(j << 8) + i].I = I * carr_I - Q * carr_Q;
            mix_tbl[(j << 8) + i].Q = I * carr_Q + Q * carr_I;
        }
    }
    // enable escape sequence for Windows console
    enable_console_esc();
}

//------------------------------------------------------------------------------
//  Allocate memory for complex array. If no memory allocated, it exits the AP
//  immediately with an error message.
//  
//  args:
//      N        (I)  Size of complex array
//
//  return:
//      Complex array allocated.
//
sdr_cpx_t *sdr_cpx_malloc(int N)
{
    sdr_cpx_t *cpx;
    
    if (!(cpx = (sdr_cpx_t *)fftwf_malloc(sizeof(sdr_cpx_t) * N))) {
        fprintf(stderr, "sdr_cpx_t memory allocation error N=%d\n", N);
        exit(-1);
    }
    return cpx;
}

//------------------------------------------------------------------------------
//  Free memory allocated by sdr_cpx_malloc().
//  
//  args:
//      cpx      (I)  Complex array
//
//  return:
//      None
//
void sdr_cpx_free(sdr_cpx_t *cpx)
{
    fftwf_free(cpx);
}

//------------------------------------------------------------------------------
//  Absolute value of a complex.
//  
//  args:
//      cpx      (I)  Complex value
//
//  return:
//      | cpx |
//
float sdr_cpx_abs(sdr_cpx_t cpx)
{
    return sqrtf(SQR(cpx[0]) + SQR(cpx[1]));
}

//------------------------------------------------------------------------------
//  Multiplication of two complex arrays.
//  
//  args:
//      a, b     (I)  Complex arrays
//      N        (I)  Size of complex arrays
//      s        (I)  Scale
//      c        (O)  Multiplication of a and b (c[i] = a[i] * b[i] * s)
//
//  return:
//      None
//
void sdr_cpx_mul(const sdr_cpx_t *a, const sdr_cpx_t *b, int N, float s,
    sdr_cpx_t *c)
{
    int i = 0;
#if defined(AVX2)
    __m256 yr = _mm256_set_ps(-1, 1, -1, 1, -1, 1, -1, 1);
    __m256 ys = _mm256_set1_ps(s);
    
    for ( ; i < N - 3; i += 4) {
         __m256 ya = _mm256_loadu_ps((float *)(a + i));
         __m256 yb = _mm256_loadu_ps((float *)(b + i));
         __m256 yc = _mm256_mul_ps(ya, _mm256_mul_ps(yb, yr));
         __m256 yd = _mm256_mul_ps(ya, _mm256_permute_ps(yb, 0xB1));
         __m256 ye = _mm256_permute_ps(_mm256_hadd_ps(yc, yd), 0xD8);
         _mm256_storeu_ps((float *)(c + i), _mm256_mul_ps(ye, ys));
    }
#endif
    for ( ; i < N; i++) {
        c[i][0] = (a[i][0] * b[i][0] - a[i][1] * b[i][1]) * s;
        c[i][1] = (a[i][0] * b[i][1] + a[i][1] * b[i][0]) * s;
    }
}

//------------------------------------------------------------------------------
//  Generate a new IF data buffer.
//
//  args:
//      N        (I)  Size of IF data buffer
//      IQ       (I)  Sampling type (1: I-sampling, 2: IQ-sampling)
//
//  return:
//      IF data buffer
//
sdr_buff_t *sdr_buff_new(int N, int IQ)
{
    sdr_buff_t *buff = (sdr_buff_t *)sdr_malloc(sizeof(sdr_buff_t));
    buff->data = (sdr_cpx8_t *)sdr_malloc(sizeof(sdr_cpx8_t *) * N);
    buff->N = N;
    buff->IQ = IQ;
    return buff;
}

//------------------------------------------------------------------------------
//  Free IF data buffer.
//
//  args:
//      buff     (I)  IF data buffer
//
//  return:
//      none
//
void sdr_buff_free(sdr_buff_t *buff)
{
    if (!buff) return;
    sdr_free(buff->data);
    sdr_free(buff);
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
//
//  return:
//      IF data buffer (NULL: read error)
//
sdr_buff_t *sdr_read_data(const char *file, double fs, int IQ, double T,
    double toff)
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
#if defined(WIN32) || defined(MACOS)
    fpos_t pos = 0;
    fgetpos(fp, &pos);
    size_t size = (size_t)pos;
#else
    fpos_t pos;
    fgetpos(fp, &pos);
    size_t size = (size_t)(pos.__pos);
#endif
    rewind(fp);
    
    if (cnt <= 0) {
        cnt = size - off;
    }
    if (size < off + cnt) {
        fclose(fp);
        return NULL;
    }
    int8_t *raw = (int8_t *)sdr_malloc(cnt);
#if defined(WIN32) || defined(MACOS)
    pos = (size_t)off;
    fsetpos(fp, &pos);
#else
    pos.__pos = (__off_t)off;
    fsetpos(fp, &pos);
#endif
    
    if (fread(raw, 1, cnt, fp) < cnt) {
        fprintf(stderr, "data read error %s\n", file);
        fclose(fp);
        return NULL;
    }
    sdr_buff_t *buff = sdr_buff_new(cnt / IQ, IQ);
    
    if (IQ == 1) { // I-sampling
        for (int i = 0; i < buff->N; i++) {
            buff->data[i] = SDR_CPX8(raw[i], 0);
        }
    }
    else { // IQ-sampling
        for (int i = 0; i < buff->N; i++) {
            buff->data[i] = SDR_CPX8(raw[i*2], raw[i*2+1]);
        }
    }
    sdr_free(raw);
    fclose(fp);
    return buff;
}

//------------------------------------------------------------------------------
//  Parallel code search in digitized IF data.
//
//  args:
//      code_fft (I) Code DFT (with or w/o zero-padding) as complex array
//      T        (I) Code cycle (period) (s)
//      buff     (I) IF data buffer
//      ix       (I) Index of sample data
//      N        (I) length of sample data
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
void sdr_search_code(const sdr_cpx_t *code_fft, double T,
    const sdr_buff_t *buff, int ix, int N, double fs, double fi,
    const float *fds, int len_fds, float *P)
{
    sdr_cpx_t *C = sdr_cpx_malloc(N);
    
    for (int i = 0; i < len_fds; i++) {
        
        // FFT correlator
        sdr_corr_fft(buff, ix, N, fs, fi + fds[i], 0.0, code_fft, C);
        
        // add correlation power
        for (int j = 0; j < N; j++) {
            P[i*N+j] += SQR(C[j][0]) + SQR(C[j][1]); // abs(C[j]) ** 2
        }
        if (i % 22 == 21) { // release cpu
            sdr_sleep_msec(1);
        }
    }
    sdr_cpx_free(C);
}

// max correlation power and C/N0 ----------------------------------------------
float sdr_corr_max(const float *P, int N, int Nmax, int M, double T, int *ix)
{
    double P_max = 0.0, P_ave = 0.0;
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
    return (P_ave > 0.0) ?
        (float)(10.0 * log10((P_max - P_ave) / P_ave / T)) : 0.0f;
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
double sdr_fine_dop(const float *P, int N, const float *fds, int len_fds,
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
    return -p[1] / (2.0 * p[2]);
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

// mix carrier -----------------------------------------------------------------
static void mix_carr(const sdr_buff_t *buff, int ix, int N, double phi,
    double step, sdr_cpx16_t *IQ)
{
    const uint8_t *data = buff->data + ix;
    double scale = (double)(1 << 24) * NTBL;
    uint32_t p = (uint32_t)((phi - floor(phi)) * scale);
    uint32_t s = (uint32_t)(int)(step * scale);
    int i = 0;
#if defined(AVX2)
    __m256i yp = _mm256_set_epi32(p+s*7, p+s*6, p+s*5, p+s*4, p+s*3, p+s*2, p+s, p);
    __m256i ys = _mm256_set1_epi32(s*8);
    
    for ( ; i < N - 16; i += 8) {
        int idx[8];
        __m128i xdas = _mm_loadu_si128((__m128i *)(data + i));
        __m256i ydat = _mm256_cvtepu8_epi32(xdas);
        __m256i yidx = _mm256_add_epi32(_mm256_slli_epi32(ydat, 8),
            _mm256_srli_epi32(yp, 24));
        _mm256_storeu_si256((__m256i *)idx, yidx);
        IQ[i  ] = mix_tbl[idx[0]];
        IQ[i+1] = mix_tbl[idx[1]];
        IQ[i+2] = mix_tbl[idx[2]];
        IQ[i+3] = mix_tbl[idx[3]];
        IQ[i+4] = mix_tbl[idx[4]];
        IQ[i+5] = mix_tbl[idx[5]];
        IQ[i+6] = mix_tbl[idx[6]];
        IQ[i+7] = mix_tbl[idx[7]];
        yp = _mm256_add_epi32(yp, ys);
    }
#endif
    for (p += s * i; i < N; i++, p += s) {
        int idx = ((int)data[i] << 8) + (p >> 24);
        IQ[i] = mix_tbl[idx];
    }
}

void sdr_mix_carr(const sdr_buff_t *buff, int ix, int N, double fs, double fc,
    double phi, sdr_cpx16_t *IQ)
{
    double step = fc / fs;
    
    if (ix + N <= buff->N) {
        mix_carr(buff, ix, N, phi, step, IQ);
    }
    else { // across IF buffer boundary
        int n = buff->N - ix;
        mix_carr(buff, ix, n, phi, step, IQ);
        mix_carr(buff, 0, N - n, phi + step * n, step, IQ + n);
    }
}

// mix carrier for complex buffer ----------------------------------------------
static void mix_carr_cpx(const sdr_cpx_t *buff, int len_buff, int ix, int N,
    double fs, double fc, double phi, sdr_cpx16_t *IQ)
{
    sdr_buff_t *buff_cpx8 = sdr_buff_new(N, 2);
    for (int i = 0, j = ix; i < N; i++, j = (j + 1) % len_buff) {
        buff_cpx8->data[i] = SDR_CPX8((int8_t)buff[j][0], (int8_t)buff[j][1]);
    }
    mix_carr(buff_cpx8, 0, N, phi, fc / fs, IQ);
    sdr_buff_free(buff_cpx8);
}

// sum of int16 fields ---------------------------------------------------------
#if defined(AVX2)
#define sum_s16(ymm, sum) { \
    int16_t s[16]; \
    _mm256_storeu_si256((__m256i *)s, ymm); \
    ymm = _mm256_setzero_si256(); \
    sum += s[0] + s[1] + s[2] + s[3] + s[4] + s[5] + s[6] + s[7] + s[8] + s[9] \
        + s[10] + s[11] + s[12] + s[13] + s[14] + s[15]; \
}
#elif defined(NEON)
#define sum_s16(ymm, sum) { \
    int16_t s[8]; \
    vst1q_s16(s, ymm); \
    ymm = vdupq_n_s16(0); \
    sum += s[0] + s[1] + s[2] + s[3] + s[4] + s[5] + s[6] + s[7]; \
}
#endif

// inner product of IQ data and code -------------------------------------------
static void dot_IQ_code(const sdr_cpx16_t *IQ, const sdr_cpx16_t *code, int N,
    float s, sdr_cpx_t *c)
{
    int i = 0;
    (*c)[0] = (*c)[1] = 0.0f;
#if defined(AVX2)
    __m256i ysumI = _mm256_setzero_si256();
    __m256i ysumQ = _mm256_setzero_si256();
    __m256i yextI = _mm256_set_epi8(0,1,0,1,0,1,0,1, 0,1,0,1,0,1,0,1,
        0,1,0,1,0,1,0,1, 0,1,0,1,0,1,0,1);
    __m256i yextQ = _mm256_set_epi8(1,0,1,0,1,0,1,0, 1,0,1,0,1,0,1,0,
        1,0,1,0,1,0,1,0, 1,0,1,0,1,0,1,0);
    
    for ( ; i < N - 15; i += 16) {
        __m256i ydata = _mm256_loadu_si256((__m256i *)(IQ + i));
        __m256i ycode = _mm256_loadu_si256((__m256i *)(code + i)); // {-1,0,1}
        __m256i ycorr = _mm256_sign_epi8(ydata, ycode); // IQ * code
        ysumI = _mm256_add_epi16(ysumI, _mm256_maddubs_epi16(yextI, ycorr));
        ysumQ = _mm256_add_epi16(ysumQ, _mm256_maddubs_epi16(yextQ, ycorr));
        if (i % (16 * 256) == 0) {
            sum_s16(ysumI, (*c)[0])
            sum_s16(ysumQ, (*c)[1])
        }
    }
    sum_s16(ysumI, (*c)[0])
    sum_s16(ysumQ, (*c)[1])
#elif defined(NEON)
    int16x8_t ysumI = vdupq_n_s16(0);
    int16x8_t ysumQ = vdupq_n_s16(0);
    
    for ( ; i < N - 7; i += 8) {
        int8x8x2_t ydata = vld2_s8((int8_t *)(IQ + i));
        int8x8x2_t ycode = vld2_s8((int8_t *)(code + i));
        ysumI = vmlal_s8(ysumI, ydata.val[0], ycode.val[0]);
        ysumQ = vmlal_s8(ysumQ, ydata.val[1], ycode.val[1]);
        if (i % (8 * 256) == 0) {
            sum_s16(ysumI, (*c)[0])
            sum_s16(ysumQ, (*c)[1])
        }
    }
    sum_s16(ysumI, (*c)[0])
    sum_s16(ysumQ, (*c)[1])
#endif
    for ( ; i < N; i++) {
        (*c)[0] += IQ[i].I * code[i].I;
        (*c)[1] += IQ[i].Q * code[i].Q;
    }
    (*c)[0] *= s * SDR_CSCALE;
    (*c)[1] *= s * SDR_CSCALE;
}

// standard correlator ---------------------------------------------------------
static void corr_std(const sdr_cpx16_t *IQ, const sdr_cpx16_t *code, int N,
    const int *pos, int n, sdr_cpx_t *corr)
{
    for (int i = 0; i < n; i++) {
        if (pos[i] > 0) {
            int M = N - pos[i];
            dot_IQ_code(IQ + pos[i], code, M, 1.0f / M, corr + i);
        }
        else if (pos[i] < 0) {
            int M = N + pos[i];
            dot_IQ_code(IQ, code - pos[i], M, 1.0f / M, corr + i);
        }
        else {
            dot_IQ_code(IQ, code, N, 1.0f / N, corr + i);
        }
    }
}

// mix carrier and standard correlator -----------------------------------------
void sdr_corr_std(const sdr_buff_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx16_t *code, const int *pos, int n, sdr_cpx_t *corr)
{
    sdr_cpx16_t *IQ = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N);
    sdr_mix_carr(buff, ix, N, fs, fc, phi, IQ);
    corr_std(IQ, code, N, pos, n, corr);
    sdr_free(IQ);
}

// mix carrier and standard correlator for complex buffer ----------------------
void sdr_corr_std_cpx(const sdr_cpx_t *buff, int len_buff, int ix, int N,
    double fs, double fc, double phi, const float *code, const int *pos, int n,
    sdr_cpx_t *corr)
{
    sdr_cpx16_t *IQ = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N * 2);
    mix_carr_cpx(buff, len_buff, ix, N, fs, fc, phi, IQ);
    for (int i = 0; i < N; i++) {
        IQ[N+i].I = IQ[N+i].Q = (int8_t)code[i];
    }
    corr_std(IQ, IQ + N, N, pos, n, corr);
    sdr_free(IQ);
}

// get FFTW plan ---------------------------------------------------------------
static int get_fftw_plan(int N, fftwf_plan *plan)
{
    static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    
    pthread_mutex_lock(&mtx);
    
    for (int i = 0; i < MAX_FFTW_PLAN; i++) {
        if (fftw_size[i] == 0) {
            sdr_cpx_t *cpx1 = sdr_cpx_malloc(N);
            sdr_cpx_t *cpx2 = sdr_cpx_malloc(N);
            fftw_plans[i][0] = fftwf_plan_dft_1d(N, cpx1, cpx2, FFTW_FORWARD,  FFTW_FLAG);
            fftw_plans[i][1] = fftwf_plan_dft_1d(N, cpx2, cpx1, FFTW_BACKWARD, FFTW_FLAG);
            fftw_size[i] = N;
            sdr_cpx_free(cpx1);
            sdr_cpx_free(cpx2);
        }
        if (fftw_size[i] == N) {
            plan[0] = fftw_plans[i][0];
            plan[1] = fftw_plans[i][1];
            pthread_mutex_unlock(&mtx);
            return 1;
        }
    }
    fprintf(stderr, "fftw plan buffer overflow N=%d\n", N);
    pthread_mutex_unlock(&mtx);
    return 0;
}

// FFT correlator --------------------------------------------------------------
static void corr_fft(const sdr_cpx16_t *IQ, const sdr_cpx_t *code_fft, int N,
    sdr_cpx_t *corr)
{
    fftwf_plan plan[2];
    
    if (!get_fftw_plan(N, plan)) return;
    sdr_cpx_t *cpx = sdr_cpx_malloc(N * 2);
    for (int i = 0; i < N; i++) {
        cpx[i][0] = IQ[i].I * SDR_CSCALE;
        cpx[i][1] = IQ[i].Q * SDR_CSCALE;
    }
    // ifft(fft(data) * code_fft) / N^2 
    fftwf_execute_dft(plan[0], cpx, cpx + N);
    sdr_cpx_mul(cpx + N, code_fft, N, 1.0f / N / N, cpx);
    fftwf_execute_dft(plan[1], cpx, corr);
    
    sdr_cpx_free(cpx);
}

// mix carrier and FFT correlator ----------------------------------------------
void sdr_corr_fft(const sdr_buff_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx_t *code_fft, sdr_cpx_t *corr)
{
    sdr_cpx16_t *IQ = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N);
    sdr_mix_carr(buff, ix, N, fs, fc, phi, IQ);
    corr_fft(IQ, code_fft, N, corr);
    sdr_free(IQ);
}

// mix carrier and FFT correlator for complex input ----------------------------
void sdr_corr_fft_cpx(const sdr_cpx_t *buff, int len_buff, int ix, int N,
    double fs, double fc, double phi, const sdr_cpx_t *code_fft,
    sdr_cpx_t *corr)
{
    sdr_cpx16_t *IQ = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N);
    mix_carr_cpx(buff, len_buff, ix, N, fs, fc, phi, IQ);
    corr_fft(IQ, code_fft, N, corr);
    sdr_free(IQ);
}

// hanning window function -----------------------------------------------------
static float hann_window(int N, float *w)
{
    float w_ave = 0.0;
    
    for (int i = 0; i < N; i++) {
        w[i] = 0.5 + 0.5 * cos(2.0 * PI * (i - 0.5 * (N - 1)) / (N - 1));
        w_ave += w[i] / N;
    }
    return w_ave;
}

//------------------------------------------------------------------------------
//  Power spectral density of digitalized IF (inter-frequency) data.
//
//  args:
//      buff     (I) digitalized IF data buffer
//      len_buff (I) length of buffer
//      N        (I) FFT size
//      fs       (I) sampling frequency (Hz)
//      IQ       (I) sampling type (1: I-sampling, 2: IQ-sampling)
//      psd      (O) PSD (dB/Hz) size: N/2 (IQ=1), N (IQ=2)
//
//  return:
//      none
//
void sdr_psd_cpx(const sdr_cpx_t *buff, int len_buff, int N, double fs, int IQ,
    float *psd)
{
    fftwf_plan plan[2];
    
    if (!get_fftw_plan(N, plan)) return;
    
    float *p = (float *)sdr_malloc(sizeof(float) * N);
    float *w = (float *)sdr_malloc(sizeof(float) * N);
    sdr_cpx_t *cpx1 = sdr_cpx_malloc(N);
    sdr_cpx_t *cpx2 = sdr_cpx_malloc(N);
    
    float w_ave = hann_window(N, w);
    
    // Welch's method without overlap
    int M = len_buff / N;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            cpx1[j][0] = buff[N*i+j][0] * w[j];
            cpx1[j][1] = buff[N*i+j][1] * w[j];
        }
        fftwf_execute_dft(plan[0], cpx1, cpx2);
        for (int j = 0; j < N; j++) {
            p[j] += SQR(cpx2[j][0]) + SQR(cpx2[j][1]);
        }
    }
    // scale complies with matplotlib.psd()
    float scale = 1.333 / N / M / w_ave / fs;
    
    if (IQ == 1) { // I
        for (int i = 0; i < N / 2; i++) {
            psd[i] = 10.0f * log10f(p[i] * scale * 2.0);
        }
    }
    else { // IQ
        for (int i = 0; i < N / 2; i++) {
            psd[i] = 10.0f * log10f(p[N/2+i] * scale);
        }
        for (int i = N / 2; i < N; i++) {
            psd[i] = 10.0f * log10f(p[i-N/2] * scale);
        }
    }
    sdr_free(p);
    sdr_free(w);
    sdr_cpx_free(cpx1);
    sdr_cpx_free(cpx2);
}

// open stream -----------------------------------------------------------------
stream_t *sdr_str_open(const char *path)
{
    if (!*path) return NULL;
    
    stream_t *str = (stream_t *)sdr_malloc(sizeof(stream_t));
    const char *p = strchr(path, ':');
    int stat = 0, port = 0, str_opt[] = {30000, 30000, 1000, 1<<20, 0};
    
    strinit(str);
    strsetopt(str_opt);
    if (p == path) { // TCP server (path = :port)
        stat = stropen(str, STR_TCPSVR, STR_MODE_W, path);
    }
    else if (sscanf(p, ":%d", &port) == 1) { // TCP client (path = addr:port)
        stat = stropen(str, STR_TCPCLI, STR_MODE_W, path);
    }
    else { // file (path = file[::opt...])
#ifdef WIN32
        char buff[1024], *q;
        snprintf(buff, sizeof(buff), "%s", path);
        for (q = buff; *q; q++) if (*q == '/') *q = '\\';
        path = buff;
#endif
        stat = stropen(str, STR_FILE, STR_MODE_W, path);
    }
    if (!stat) {
        sdr_free(str);
        return NULL;
    }
    return str;
}

// close stream ----------------------------------------------------------------
void sdr_str_close(stream_t *str)
{
    if (!str) return;
    strclose(str);
}

// write stream ----------------------------------------------------------------
int sdr_str_write(stream_t *str, uint8_t *data, int size)
{
    if (!str) return 0;
    return strwrite(str, data, size);
}

// open log --------------------------------------------------------------------
int sdr_log_open(const char *path)
{
    if (!*path || log_str) return 0;
    
    if (!(log_str = sdr_str_open(path))) {
        fprintf(stderr, "log stream open error %s\n", path);
        return 0;
    }
    return 1;
}

// close log -------------------------------------------------------------------
void sdr_log_close(void)
{
    sdr_str_close(log_str);
    log_str = NULL;
}

// set log level ---------------------------------------------------------------
void sdr_log_level(int level)
{
    log_lvl = level;
}

// output log ------------------------------------------------------------------
void sdr_log(int level, const char *msg, ...)
{
    va_list ap;
    
    va_start(ap, msg);
    
    if (log_lvl == 0) {
        vprintf(msg, ap);
    }
    else if (level <= log_lvl) {
        char buff[1024];
        int len = vsnprintf(buff, sizeof(buff) - 2, msg, ap);
        len = MIN(len, (int)sizeof(buff) - 3);
        if (log_str) {
            sprintf(buff + len, "\r\n");
            strwrite(log_str, (uint8_t *)buff, len + 2);
        }
        pthread_mutex_lock(&log_buff_mtx);
        if (log_buff_p + len + 1 < MAX_LOG_BUFF) {
            log_buff_p += sprintf(log_buff + log_buff_p, "%s\n", buff);
        }
        pthread_mutex_unlock(&log_buff_mtx);
    }
    va_end(ap);
}

// get log buffer --------------------------------------------------------------
int sdr_get_log(char *buff, int size)
{
    pthread_mutex_lock(&log_buff_mtx);
    int out_size = snprintf(buff, size, "%s", log_buff);
    log_buff[0] = '\0';
    log_buff_p = 0;
    pthread_mutex_unlock(&log_buff_mtx);
    return out_size <= size ? out_size : size;
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

// add item to buffer ----------------------------------------------------------
void sdr_add_buff(void *buff, int len_buff, void *item, size_t size_item)
{
    memmove(buff, (uint8_t *)buff + size_item, size_item * (len_buff - 1));
    memcpy((uint8_t *)buff + size_item * (len_buff - 1), item, size_item);
}

// pack bit array to uint8_t array ---------------------------------------------
void sdr_pack_bits(const uint8_t *data, int nbit, int nz, uint8_t *buff)
{
    memset(buff, 0, (nz + nbit + 7) / 8);
    for (int i = nz; i < nz + nbit; i++) {
        buff[i / 8] |= data[i-nz] << (7 - i % 8);
    }
}

// unpack uint8_t array to bit array -------------------------------------------
void sdr_unpack_bits(const uint8_t *data, int nbit, uint8_t *buff)
{
    for (int i = 0; i < nbit * 8; i++) {
        buff[i] = (data[i / 8] >> (7 - i % 8)) & 1;
    }
}

// unpack data to bits ---------------------------------------------------------
void sdr_unpack_data(uint32_t data, int nbit, uint8_t *buff)
{
    for (int i = 0; i < nbit; i++) {
        buff[i] = (data >> (nbit - 1 - i)) & 1;
    }
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

// generate FFTW wisdom --------------------------------------------------------
int sdr_gen_fftw_wisdom(const char *file, int N)
{
    fftwf_plan plan[2] = {0};
    
    sdr_cpx_t *cpx1 = sdr_cpx_malloc(N);
    sdr_cpx_t *cpx2 = sdr_cpx_malloc(N);
#if defined(NEON)
    plan[0] = fftwf_plan_dft_1d(N, cpx1, cpx2, FFTW_FORWARD,  FFTW_MEASURE);
    plan[1] = fftwf_plan_dft_1d(N, cpx2, cpx1, FFTW_BACKWARD, FFTW_MEASURE);
#else
    plan[0] = fftwf_plan_dft_1d(N, cpx1, cpx2, FFTW_FORWARD,  FFTW_PATIENT);
    plan[1] = fftwf_plan_dft_1d(N, cpx2, cpx1, FFTW_BACKWARD, FFTW_PATIENT);
#endif
    int stat = fftwf_export_wisdom_to_filename(file);
    
    fftwf_destroy_plan(plan[0]);
    fftwf_destroy_plan(plan[1]);
    sdr_cpx_free(cpx1);
    sdr_cpx_free(cpx2);
    
    return stat;
}

