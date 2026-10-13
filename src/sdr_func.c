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
//  2024-08-26  1.15 modify API sdr_corr_std(), sdr_corr_fft()
//  2024-11-21  1.16 add API sdr_tag_read(), sdr_tag_write(), sdr_log_mask(),
//                   sdr_log_stat()
//                   modify API sdr_corr_std(), sdr_corr_std_cpx()
//  2025-11-15  1.17 add API sdr_cpx_fft()
//                   modify API sdr_tag_read(), sdr_tag_write()
//  2026-07-04  1.18 optimize NEON codes in sdr_cpx_mul(), dot_IQ_code()
//                   add NEON codes in mix_carr(), sdr_lpf_apply()
//  2026-07-05  1.19 integrate carrier mixing into sdr_corr_std()
//                   modify API sdr_corr_std()
//                   change real code bank to 1 byte/sample int8_t
//  2026-07-05  1.20 integrate carrier mixing into sdr_corr_fft()
//                   modify API sdr_corr_fft()
//                   use persistent scratch buffer in sdr_corr_fft()
//  2026-07-05  1.21 add API sdr_bin_csk()
//  2026-07-05  1.22 add polarity option to sdr_corr_std()
//                   modify API sdr_corr_std()
//  2026-07-08  1.23 support int8 code values in sdr_corr_std() for CBOC
//                   modify API sdr_corr_std()
//  2026-07-08  1.24 support int8 complex code values in sdr_corr_std_cpx_code()
//                   for E5 AltBOC
//                   modify API sdr_corr_std_cpx_code()
//  2026-07-14  1.25 support separate codes for before/after the code
//                   wrap-around in sdr_corr_std_cpx_code()
//                   modify API sdr_corr_std_cpx_code()
//  2026-07-17  1.26 support complex codes as I/Q real code banks and fixed
//                   negative polarity in sdr_corr_std()
//                   modify API sdr_corr_std()
//  2026-07-17  1.27 make sdr_corr_std() real-only; drop code_Q/complex kernel
//  2026-07-19  1.28 restore complex code path in sdr_corr_std() with fused
//                   dot_IQ_code2_acc() (shared carrier mixing and IQ load)
//                   modify API sdr_corr_std()
//  2026-07-19  1.29 add API sdr_corr_std2() with separate code banks for the
//                   part after the code wrap-around
//                   remove API sdr_corr_std_cpx_code()
//  2026-07-24  1.30 support multiple log streams
//                   add API sdr_log_add_str(), sdr_log_rm_str()
//  2026-08-08  1.31 fix memory leak of stream in sdr_str_close()
//  2026-09-03  1.32 support number of RF CHs in the tag file
//  2026-10-14  1.33 add AVX512 codes
//                   modify API sdr_tag_read(), sdr_tag_write()
//
#include <math.h>
#include <stdarg.h>
#include "pocket_sdr.h"

#if defined(WIN32)
#include <io.h>
#endif
#if defined(AVX512) || defined(AVX2)
#include <immintrin.h>
#elif defined(NEON)
#include <arm_neon.h>
#endif
#if defined(FFTW)
#include <fftw3.h>
#else
#include "pocketfft.h"
#endif

#define NTBL          256   // carrier-mixed-data LUT size
#define MIX_F32_CHUNK 4096  // mix-carrier chunk size for FFT correlator
#define DOP_STEP      0.5   // Doppler frequency search step (* 1 / code cycle)
#define MAX_FFTW_PLAN 32    // max number of FFTW plans
#define MAX_LOG_BUFF  (2<<18) // max sizeof log buffer
#define FFTW_FLAG     FFTW_ESTIMATE // FFTW flag
#define LPF_N_TAP     9     // number of LPF taps (symmetric, must be odd)
#define LPF_HIST_SIZE 16    // history circular buffer size (power of 2)
#define LPF_HIST_MASK (LPF_HIST_SIZE - 1)
#define LPF_H_SHIFT   13    // Q13 coefficient scale (1.0 = 8192)
#define LPF_N_CHUNK   1024  // LPF chunk size

#define MIX_CORR_CHUNK 4096 // chunk size of fused mix carrier + correlator

#define SQR(x)        ((x) * (x))
#define MIN(x, y)     ((x) < (y) ? (x) : (y))
#define MAX(x, y)     ((x) > (y) ? (x) : (y))
#define ROUND(x)      floor((x) + 0.5)
#define CLIP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

// type definitions ------------------------------------------------------------
struct sdr_lpf_tag {              // LPF state struct
    int h[LPF_HIST_SIZE];         // LPF coefficients (Q13)
    int I[LPF_HIST_SIZE], Q[LPF_HIST_SIZE]; // history
    int pos;                      // history write position
};

// global variables ------------------------------------------------------------
static sdr_cpx16_t mix_tbl[NTBL*256] = {{0,0}}; // carrier-mixed-data LUT
static sdr_cpx16_t mix_tbl_b[NTBL*256] = {{0,0}}; // LUT biased by +128 (u8)
#if defined(AVX512)
static int16_t carr_tbl[NTBL] = {0}; // carrier LUT (int8 cos | sin << 8)
#endif
static fftwf_plan fftw_plans[MAX_FFTW_PLAN][2] = {{0}}; // FFTW plan buffer
static int fftw_size[MAX_FFTW_PLAN] = {0}; // FFTW plan sizes
static int log_lvl = 3;            // log level
static const char *log_types[] = { // log types
    "$TIME,", "$POS,", "$ATT,", "$OBS,", "$NAV,", "$SAT,", "$CH,", "$EPH,",
    "$ALM,", "$LOG,", "$SENS,", NULL
};
static int log_mask[16] = {1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1}; // log mask
static stream_t *log_strs[SDR_MAX_STR] = {NULL}; // registered log streams
static stream_t *log_str_own = NULL; // log stream owned by sdr_log_open()
static char log_buff[MAX_LOG_BUFF]; // log buffer
static int log_buff_p = 0;        // log buffer pointer
static sdr_mutex_t log_buff_mtx = SDR_MUTEX_INIT;
static const char *fmt_str[] = { // IF data format string
    "-", "INT8", "INT8X2", "RAW8", "RAW16", "RAW16I", "RAW32", "CS8", "CS16",
    NULL
};
static const int fmt_nch[] = { // IF data format # of CH
    0, 1, 1, 2, 4, 8, 8, 1, 1
};

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
    // initialize log stream
    strinitcom();
    
    // import FFTW wisdom 
    if (*file && !fftwf_import_wisdom_from_filename(file)) {
        fprintf(stderr, "FFTW wisdom import error %s\n", file);
    }
    // generate carrier-mixed-data LUT
    for (int i = 0; i < NTBL; i++) {
        int8_t carr_I = (int8_t)ROUND(cos(-2.0 * PI * i / NTBL) / SDR_CSCALE);
        int8_t carr_Q = (int8_t)ROUND(sin(-2.0 * PI * i / NTBL) / SDR_CSCALE);
#if defined(AVX512)
        carr_tbl[i] = (int16_t)(((int)carr_Q << 8) | (carr_I & 0xFF));
#endif
        for (int j = 0; j < 256; j++) {
            int8_t I = SDR_CPX8_I(j), Q = SDR_CPX8_Q(j);
            mix_tbl[(j << 8) + i].I = I * carr_I - Q * carr_Q;
            mix_tbl[(j << 8) + i].Q = I * carr_Q + Q * carr_I;
            mix_tbl_b[(j << 8) + i].I = mix_tbl[(j << 8) + i].I ^ (int8_t)0x80;
            mix_tbl_b[(j << 8) + i].Q = mix_tbl[(j << 8) + i].Q ^ (int8_t)0x80;
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
#if defined(AVX512)
    __m512 zs = _mm512_set1_ps(s);

    for ( ; i < N - 7; i += 8) {
        __m512 za = _mm512_loadu_ps((const float *)(a + i));
        __m512 zb = _mm512_loadu_ps((const float *)(b + i));
        __m512 zc = _mm512_mul_ps(_mm512_movehdup_ps(za),
            _mm512_permute_ps(zb, 0xB1)); // (ai*bi, ai*br)
        zc = _mm512_fmaddsub_ps(_mm512_moveldup_ps(za), zb, zc);
        _mm512_storeu_ps((float *)(c + i), _mm512_mul_ps(zc, zs));
    }
#elif defined(AVX2)
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
#elif defined(NEON)
    float32x4_t scale = vdupq_n_f32(s);
    
    for (; i < N - 3; i += 4) {
        float32x4x2_t va = vld2q_f32((const float32_t *)(a + i));
        float32x4x2_t vb = vld2q_f32((const float32_t *)(b + i));
        float32x4x2_t vc;
        vc.val[0] = vfmsq_f32(vmulq_f32(va.val[0], vb.val[0]), va.val[1],
            vb.val[1]);
        vc.val[1] = vfmaq_f32(vmulq_f32(va.val[0], vb.val[1]), va.val[1],
            vb.val[0]);
        vc.val[0] = vmulq_f32(vc.val[0], scale);
        vc.val[1] = vmulq_f32(vc.val[1], scale);
        vst2q_f32((float32_t *)(c + i), vc);
    }
#endif
    for ( ; i < N; i++) {
        c[i][0] = (a[i][0] * b[i][0] - a[i][1] * b[i][1]) * s;
        c[i][1] = (a[i][0] * b[i][1] + a[i][1] * b[i][0]) * s;
    }
}

// FFT and IFFT of complex -----------------------------------------------------
int sdr_cpx_fft(sdr_cpx_t *cpx1, int N, int dir, sdr_cpx_t *cpx2)
{
    static sdr_mutex_t mtx = SDR_MUTEX_INIT;
    int i;
    
    if (dir != SDR_FFT_FORWARD && dir != SDR_FFT_BACKWARD) return 0;

    sdr_mutex_lock(&mtx);
    for (i = 0; i < MAX_FFTW_PLAN; i++) {
        if (fftw_size[i] == 0) {
            sdr_cpx_t *cpxa = sdr_cpx_malloc(N);
            sdr_cpx_t *cpxb = sdr_cpx_malloc(N);
            fftw_plans[i][0] = fftwf_plan_dft_1d(N, cpxa, cpxb, FFTW_FORWARD,  FFTW_FLAG);
            fftw_plans[i][1] = fftwf_plan_dft_1d(N, cpxb, cpxa, FFTW_BACKWARD, FFTW_FLAG);
            fftw_size[i] = N;
            sdr_cpx_free(cpxa);
            sdr_cpx_free(cpxb);
        }
        if (fftw_size[i] == N) break;
    }
    sdr_mutex_unlock(&mtx);
    
    if (i >= MAX_FFTW_PLAN) {
        fprintf(stderr, "fftw plan buffer overflow N=%d\n", N);
        return 0;
    }
    fftwf_execute_dft(fftw_plans[i][dir], cpx1, cpx2);
    return 1;
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
    buff->data = (sdr_cpx8_t *)sdr_malloc(sizeof(sdr_cpx8_t) * N);
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
//  Create a new LPF instance (symmetric FIR, 9-tap windowed sinc).
//
//  args:
//      fc       (I)  cutoff frequency (Hz)
//      fs       (I)  sampling frequency (Hz)
//
//  return:
//      LPF instance (NULL: error)
//
sdr_lpf_t *sdr_lpf_new(double fc, double fs)
{
    if (fc <= 0.0 || fs <= 0.0 || fc >= fs * 0.5) return NULL;

    sdr_lpf_t *lpf = (sdr_lpf_t *)sdr_malloc(sizeof(sdr_lpf_t));
    double fc_n = fc / fs, h[LPF_N_TAP], sum = 0.0;
    
    for (int i = 0; i < LPF_N_TAP; i++) {
        double t = i - (LPF_N_TAP - 1) / 2.0;
        double s = (t == 0.0) ? 2.0 * fc_n : sin(2.0 * PI * fc_n * t) / (PI * t);
#if 1 // hamming window
        h[i] = s * (0.54 - 0.46 * cos(2.0 * PI * i / (LPF_N_TAP - 1)));
#else // hann window
        h[i] = s * (0.5 - 0.5 * cos(2.0 * PI * i / (LPF_N_TAP - 1)));
#endif
        sum += h[i];
    }
    for (int i = 0; i < LPF_N_TAP; i++) {
        int v = (int)ROUND((1 << LPF_H_SHIFT) * h[i] / sum);
        lpf->h[i] = (int16_t)CLIP(v, -32768, 32767);
    }
    return lpf;
}

//------------------------------------------------------------------------------
//  Free an LPF instance (safe with NULL).
//
//  args:
//      lpf      (I)  LPF instance
//
//  return:
//      none
//
void sdr_lpf_free(sdr_lpf_t *lpf)
{
    if (!lpf) return;
    sdr_free(lpf);
}

//------------------------------------------------------------------------------
//  Apply LPF to n cpx8 samples (in-place).
//
//  args:
//      lpf      (IO) LPF instance (state updated)
//      data     (IO) input/output samples (cpx8 array, length N)
//      N        (I)  number of samples (>= 0)
//
//  return:
//      none
//
void sdr_lpf_apply(sdr_lpf_t *lpf, sdr_cpx8_t *data, int N)
{
    int32_t I[LPF_N_CHUNK+LPF_N_TAP], Q[LPF_N_CHUNK+LPF_N_TAP];
    int i, n = 0, HIST = LPF_N_TAP - 1;

    for ( ; N > 0; data += n, N -= n) {
        n = (N < LPF_N_CHUNK) ? N : LPF_N_CHUNK;

        for (i = 0; i < HIST; i++) {
            int j = (lpf->pos - HIST + i) & LPF_HIST_MASK;
            I[i] = lpf->I[j];
            Q[i] = lpf->Q[j];
        }
        i = 0;
#if defined(AVX512)
        for ( ; i + 16 <= n; i += 16) { // unpack 4-bit I/Q to int32
            __m512i d = _mm512_cvtepi8_epi32(
                _mm_loadu_si128((const __m128i *)(data + i)));
            _mm512_storeu_si512((__m512i *)(I + HIST + i),
                _mm512_srai_epi32(_mm512_slli_epi32(d, 28), 28));
            _mm512_storeu_si512((__m512i *)(Q + HIST + i),
                _mm512_srai_epi32(d, 4));
        }
#elif defined(NEON)
        for ( ; i + 16 <= n; i += 16) { // unpack 4-bit I/Q to int32
            int8x16_t d = vld1q_s8((const int8_t *)(data + i));
            int8x16_t dI = vshrq_n_s8(vshlq_n_s8(d, 4), 4);
            int8x16_t dQ = vshrq_n_s8(d, 4);
            int16x8_t wl = vmovl_s8(vget_low_s8(dI));
            int16x8_t wh = vmovl_s8(vget_high_s8(dI));
            vst1q_s32(I + HIST + i     , vmovl_s16(vget_low_s16 (wl)));
            vst1q_s32(I + HIST + i + 4 , vmovl_s16(vget_high_s16(wl)));
            vst1q_s32(I + HIST + i + 8 , vmovl_s16(vget_low_s16 (wh)));
            vst1q_s32(I + HIST + i + 12, vmovl_s16(vget_high_s16(wh)));
            wl = vmovl_s8(vget_low_s8(dQ));
            wh = vmovl_s8(vget_high_s8(dQ));
            vst1q_s32(Q + HIST + i     , vmovl_s16(vget_low_s16 (wl)));
            vst1q_s32(Q + HIST + i + 4 , vmovl_s16(vget_high_s16(wl)));
            vst1q_s32(Q + HIST + i + 8 , vmovl_s16(vget_low_s16 (wh)));
            vst1q_s32(Q + HIST + i + 12, vmovl_s16(vget_high_s16(wh)));
        }
#endif
        for ( ; i < n; i++) {
            I[HIST+i] = SDR_CPX8_I(data[i]);
            Q[HIST+i] = SDR_CPX8_Q(data[i]);
        }
        i = 0;
#if defined(AVX512)
        const __m512i round = _mm512_set1_epi32(1 << (LPF_H_SHIFT - 1));
        const __m512i cmask = _mm512_set1_epi32(0x0F);
        const __m512i clo = _mm512_set1_epi32(-7);
        const __m512i chi = _mm512_set1_epi32(7);

        for ( ; i + 16 <= n; i += 16) {
            // center tap
            __m512i xI = _mm512_loadu_si512((const __m512i *)(I + i + LPF_N_TAP / 2));
            __m512i xQ = _mm512_loadu_si512((const __m512i *)(Q + i + LPF_N_TAP / 2));
            __m512i hc = _mm512_set1_epi32(lpf->h[LPF_N_TAP/2]);
            __m512i accI = _mm512_mullo_epi32(xI, hc);
            __m512i accQ = _mm512_mullo_epi32(xQ, hc);

            for (int k = 0; k < LPF_N_TAP / 2; k++) { // side symmetric taps
                __m512i xIa = _mm512_loadu_si512((const __m512i *)(I + i + k));
                __m512i xIb = _mm512_loadu_si512((const __m512i *)(I + i + HIST - k));
                __m512i xQa = _mm512_loadu_si512((const __m512i *)(Q + i + k));
                __m512i xQb = _mm512_loadu_si512((const __m512i *)(Q + i + HIST - k));
                __m512i hk  = _mm512_set1_epi32(lpf->h[k]);
                accI = _mm512_add_epi32(accI, _mm512_mullo_epi32(_mm512_add_epi32(xIa, xIb), hk));
                accQ = _mm512_add_epi32(accQ, _mm512_mullo_epi32(_mm512_add_epi32(xQa, xQb), hk));
            }
            // round, right shift Q13, clip [-7, 7] and  pack to cpx8
            accI = _mm512_srai_epi32(_mm512_add_epi32(accI, round), LPF_H_SHIFT);
            accQ = _mm512_srai_epi32(_mm512_add_epi32(accQ, round), LPF_H_SHIFT);
            accI = _mm512_max_epi32(clo, _mm512_min_epi32(chi, accI));
            accQ = _mm512_max_epi32(clo, _mm512_min_epi32(chi, accQ));
            __m512i qm = _mm512_slli_epi32(_mm512_and_si512(accQ, cmask), 4);
            __m512i im = _mm512_and_si512(accI, cmask);
            _mm_storeu_si128((__m128i *)(data + i),
                _mm512_cvtepi32_epi8(_mm512_or_si512(qm, im)));
        }
#elif defined(AVX2)
        const __m256i round = _mm256_set1_epi32(1 << (LPF_H_SHIFT - 1));
        const __m256i cmask = _mm256_set1_epi32(0x0F);
        const __m256i clo = _mm256_set1_epi32(-7);
        const __m256i chi = _mm256_set1_epi32(7);
        
        for ( ; i + 8 <= n; i += 8) {
            // center tap
            __m256i xI = _mm256_loadu_si256((const __m256i *)(I + i + LPF_N_TAP / 2));
            __m256i xQ = _mm256_loadu_si256((const __m256i *)(Q + i + LPF_N_TAP / 2));
            __m256i hc = _mm256_set1_epi32(lpf->h[LPF_N_TAP/2]);
            __m256i accI = _mm256_mullo_epi32(xI, hc);
            __m256i accQ = _mm256_mullo_epi32(xQ, hc);

            for (int k = 0; k < LPF_N_TAP / 2; k++) { // side symmetric taps
                __m256i xIa = _mm256_loadu_si256((const __m256i *)(I + i + k));
                __m256i xIb = _mm256_loadu_si256((const __m256i *)(I + i + HIST - k));
                __m256i xQa = _mm256_loadu_si256((const __m256i *)(Q + i + k));
                __m256i xQb = _mm256_loadu_si256((const __m256i *)(Q + i + HIST - k));
                __m256i hk  = _mm256_set1_epi32(lpf->h[k]);
                accI = _mm256_add_epi32(accI, _mm256_mullo_epi32(_mm256_add_epi32(xIa, xIb), hk));
                accQ = _mm256_add_epi32(accQ, _mm256_mullo_epi32(_mm256_add_epi32(xQa, xQb), hk));
            }
            // round, right shift Q13, clip [-7, 7] and  pack to cpx8
            accI = _mm256_srai_epi32(_mm256_add_epi32(accI, round), LPF_H_SHIFT);
            accQ = _mm256_srai_epi32(_mm256_add_epi32(accQ, round), LPF_H_SHIFT);
            accI = _mm256_max_epi32(clo, _mm256_min_epi32(chi, accI));
            accQ = _mm256_max_epi32(clo, _mm256_min_epi32(chi, accQ));
            __m256i qm = _mm256_slli_epi32(_mm256_and_si256(accQ, cmask), 4);
            __m256i im = _mm256_and_si256(accI, cmask);
            __m256i packed = _mm256_or_si256(qm, im);

            int32_t tmp[8];
            _mm256_storeu_si256((__m256i *)tmp, packed);
            for (int j = 0; j < 8; j++) data[i+j] = (sdr_cpx8_t)tmp[j];
        }
#endif
        for ( ; i < n; i++) {
            int accI = lpf->h[LPF_N_TAP/2] * I[i+LPF_N_TAP/2];
            int accQ = lpf->h[LPF_N_TAP/2] * Q[i+LPF_N_TAP/2];
            for (int k = 0; k < LPF_N_TAP / 2; k++) {
                accI += lpf->h[k] * (I[i+k] + I[i+HIST-k]);
                accQ += lpf->h[k] * (Q[i+k] + Q[i+HIST-k]);
            }
            accI = (accI + (1 << (LPF_H_SHIFT - 1))) >> LPF_H_SHIFT;
            accQ = (accQ + (1 << (LPF_H_SHIFT - 1))) >> LPF_H_SHIFT;
            data[i] = SDR_CPX8(CLIP(accI, -7, 7), CLIP(accQ, -7, 7));
        }
        // only the last HIST samples survive in the history ring
        for (i = (n > HIST) ? n - HIST : 0; i < n; i++) {
            int j = (lpf->pos + i) & LPF_HIST_MASK;
            lpf->I[j] = I[HIST+i];
            lpf->Q[j] = Q[HIST+i];
        }
        lpf->pos = (lpf->pos + n) & LPF_HIST_MASK;
    }
}

//------------------------------------------------------------------------------
//  Read digitalized IF (inter-frequency) data from file. Supported file format
//  is signed byte (int8) for I-sampling (real-sampling) or interleaved signed
//  byte for IQ-sampling (complex-sampling).
//
//  args:
//      file     (I)  Digitalized IF data file path
//      fs       (I)  Sampling frequency (Hz)
//      IQ       (I)  Sampling type (1: I-sampling, 2: IQ-sampling)
//      T        (I)  Sample period (s) (0: all samples)
//      toff     (I)  Time offset from the beginning (s)
//
//  return:
//      IF data buffer (NULL: read error)
//
sdr_buff_t *sdr_read_data(const char *file, double fs, int IQ, double T,
    double toff)
{
    size_t cnt = (T > 0.0) ? (size_t)(fs * T * IQ) : 0;
    size_t off = (size_t)(fs * toff * IQ);
    size_t size;
    FILE *fp;
    
    if (!(fp = fopen(file, "rb"))) {
        fprintf(stderr, "data read error %s\n", file);
        return NULL;
    }
    // get file size
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
    if (cnt <= 0) {
        cnt = size - off;
    }
    if (cnt > size - off) {
        fclose(fp);
        return NULL;
    }
    int8_t *raw = (int8_t *)sdr_malloc(cnt);
#if defined(WIN32)
    _fseeki64(fp, (long long)off, SEEK_SET);
#else
    fseeko(fp, (off_t)off, SEEK_SET);
#endif
    if (fread(raw, 1, cnt, fp) < cnt) {
        fprintf(stderr, "data read error %s\n", file);
        fclose(fp);
        sdr_free(raw);
        return NULL;
    }
    sdr_buff_t *buff = sdr_buff_new((int)cnt / IQ, IQ);
    
    if (IQ == 1) { // I-sampling
        for (int i = 0; i < buff->N; i++) {
            buff->data[i] = SDR_CPX8(raw[i], 0);
        }
    } else { // IQ-sampling
        for (int i = 0; i < buff->N; i++) {
            buff->data[i] = SDR_CPX8(raw[i*2], -raw[i*2+1]); // flip Q-polarity
        }
    }
    sdr_free(raw);
    fclose(fp);
    return buff;
}

//------------------------------------------------------------------------------
//  Write the tag file for the IF data file. The tag file path will be
//  <file>.tag.
//
//  args:
//      file     (I)  IF data file path
//      prog     (I)  Program name
//      time     (I)  IF data file recording start time
//      fmt      (I)  IF data file format (SDR_FMT_???)
//      nch      (I)  Number of RF channels (<= 0: as defined by fmt)
//      fs       (I)  Sampling frequency (Hz)
//      fo       (I)  LO frequencies (Hz)
//      IQ       (I)  Sampling types (1: I-sampling, 2: IQ-sampling)
//      bits     (I)  Number of sample bits
//
//  return:
//      Status (1: OK, 0: error)
//
//  notes:
//      If nch exceeds the number of RF channels defined by fmt, the IF data
//      file is a sequence of channel-planar blocks of SDR_CYC each, so T_CYC
//      is written to reject a replay with a different processing cycle.
//
int sdr_tag_write(const char *file, const char *prog, gtime_t time, int fmt,
    int nch, double fs, const double *fo, const int *IQ, const int *bits)
{
    FILE *fp;
    char path[1024+4], tstr[32];
    
    if (fmt <= 0 || fmt > SDR_FMT_CS16) return 0;
    int n = nch > 0 ? nch : fmt_nch[fmt];
    if (n > SDR_MAX_RFCH) n = SDR_MAX_RFCH;
    snprintf(path, sizeof(path), "%.1023s.tag", file);
    time2str(time, tstr, 3);
    
    if (!(fp = fopen(path, "w"))) {
        fprintf(stderr, "tag file open error %s\n", path);
        return 0;
    }
    fprintf(fp, "PROG = %s\n", prog);
    fprintf(fp, "TIME = %s\n", tstr);
    fprintf(fp, "FMT  = %s\n", fmt_str[fmt]);
    fprintf(fp, "NCH  = %d\n", n);
    if (n > fmt_nch[fmt]) {
        fprintf(fp, "T_CYC = %.9g\n", SDR_CYC);
    }
    fprintf(fp, "F_S  = %.9g\n", fs * 1e-6);
    fprintf(fp, "F_LO = ");
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%.9g%s", fo[i] * 1e-6, i < n - 1 ? "," : "");
    }
    fprintf(fp, "\n");
    fprintf(fp, "IQ   = ");
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%d%s", IQ[i], i < n - 1 ? "," : "");
    }
    fprintf(fp, "\n");
    fprintf(fp, "BITS = ");
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%d%s", bits[i], i < n - 1 ? "," : "");
    }
    fprintf(fp, "\n");
    fclose(fp);
    return 1;
}

//------------------------------------------------------------------------------
//  Read the tag file for the IF data file. The tag file path will be
//  <file>.tag.
//
//  args:
//      file     (I)  IF data file path
//      prog     (IO) Program name (NULL: not read)
//      time     (IO) IF data file recording start time (NULL: not read)
//      fmt      (O)  IF data file format (SDR_FMT_???)
//      nch      (O)  Number of RF channels (NULL: not read)
//      fs       (O)  Sampling frequency (Hz)
//      fo       (O)  LO frequencies (Hz)
//      IQ       (O)  Sampling types (1: I-sampling, 2: IQ-sampling)
//      bits     (O)  Number of sample bits
//
//  return:
//      Status (1: OK, 0: no readable tag file, -1: IF data unusable)
//
int sdr_tag_read(const char *file, char *prog, gtime_t *time, int *fmt,
    int *nch, double *fs, double *fo, int *IQ, int *bits)
{
    FILE *fp;
    char path[1024+4], buff[256], *p;
    double t_cyc = 0.0;
    int n = 0;
    
    snprintf(path, sizeof(path), "%s.tag", file);
    
    if (!(fp = fopen(path, "r"))) {
        fprintf(stderr, "tag file open error %s\n", path);
        return 0;
    }
    *fmt = 0;
    *fs = 0.0;
    for (int i = 0; i < SDR_MAX_RFCH; i++) { // set default
        fo[i] = 0.0;
        IQ[i] = bits[i] = 2;
    }
    while (fgets(buff, sizeof(buff), fp)) {
        if (!(p = strchr(buff, '='))) continue;
        
        if (strstr(buff, "PROG") == buff && prog) {
            sscanf(p + 2, "%16s", prog);
        } else if (strstr(buff, "TIME") == buff && time) {
            double ep[6] = {0};
            sscanf(p + 2, "%lf/%lf/%lf %lf:%lf:%lf", ep, ep + 1, ep + 2, ep + 3,
                ep + 4, ep + 5);
            *time = epoch2time(ep);
        } else if (strstr(buff, "FMT") == buff) {
            char str[32] = "";
            sscanf(p + 2, "%16s", str);
            for (int i = 0; fmt_str[i]; i++) {
                if (strcmp(str, fmt_str[i])) continue;
                *fmt = i;
                break;
            }
        } else if (strstr(buff, "NCH") == buff) {
            sscanf(p + 2, "%d", &n);
        } else if (strstr(buff, "T_CYC") == buff) {
            sscanf(p + 2, "%lf", &t_cyc);
        } else if (strstr(buff, "F_S") == buff) {
            if (sscanf(p + 2, "%lf", fs)) *fs *= 1e6;
        } else if (strstr(buff, "F_LO") == buff) {
            int n = sscanf(p + 2, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", fo, fo + 1,
                fo + 2, fo + 3, fo + 4, fo + 5, fo + 6, fo + 7);
            for (int i = 0; i < n; i++) fo[i] *= 1e6;
        } else if (strstr(buff, "IQ") == buff) {
            sscanf(p + 2, "%d,%d,%d,%d,%d,%d,%d,%d", IQ, IQ + 1, IQ + 2, IQ + 3,
                IQ + 4, IQ + 5, IQ + 6, IQ + 7);
        } else if (strstr(buff, "BITS") == buff) {
            sscanf(p + 2, "%d,%d,%d,%d,%d,%d,%d,%d", bits, bits + 1, bits + 2,
                bits + 3, bits + 4, bits + 5, bits + 6, bits + 7);
        }
    }
    fclose(fp);
    if (*fmt <= 0 || *fmt > SDR_FMT_CS16) { // format error
        return 0;
    }
    if (n <= 0 || n > SDR_MAX_RFCH) n = fmt_nch[*fmt];
    // channel-planar IF data cannot be split into cycles of a different length
    if (t_cyc > 0.0 && fabs(t_cyc - SDR_CYC) > 1e-12) {
        fprintf(stderr, "tag file cycle unmatch %.9g (%.9g expected)\n", t_cyc,
            SDR_CYC);
        return -1;
    }
    for (int i = n; i < SDR_MAX_RFCH; i++) {
        fo[i] = 0.0;
        IQ[i] = bits[i] = 0;
    }
    if (nch) *nch = n;
    return 1;
}

//------------------------------------------------------------------------------
//  Parallel code search in digitized IF data.
//
//  args:
//      code_fft (I)  Code DFT (with or w/o zero-padding) as complex array
//      T        (I)  Code cycle (period) (s)
//      buff     (I)  IF data buffer
//      ix       (I)  Index of sample data
//      N        (I)  length of sample data
//      fs       (I)  Sampling frequency (Hz)
//      fi       (I)  IF frequency (Hz)
//      fds      (I)  Doppler frequency bins as ndarray (Hz)
//      len_fds  (I)  length of Doppler frequency bins
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
        
        // mix carrier and FFT correlator
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
    } else if (!strcmp(sig, "G2CA")) {
        fi += 0.4375e6 * fcn;
    }
    return fi;
}

// doppler search bins ---------------------------------------------------------
float *sdr_dop_bins(double T, float dop, float max_dop, int *len_fds)
{
    float *fds, step = (float)(DOP_STEP / T);
    
    *len_fds = (int)(2.0 * max_dop / step) + 1;
    fds = (float *)sdr_malloc(sizeof(float) * (*len_fds));
    
    for (int i = 0; i < *len_fds; i++) {
        fds[i] = dop - max_dop + i * step;
    }
    return fds;
}

// mix carrier with fixed-point phase and phase step ----------------------------
static void mix_carr_p(const sdr_buff_t *buff, int ix, int N, uint32_t p,
    uint32_t s, const sdr_cpx16_t *tbl, sdr_cpx16_t *IQ)
{
    const uint8_t *data = buff->data + ix;
    int i = 0;
#if defined(AVX512)
    __m512i c0 = _mm512_loadu_si512(carr_tbl), c1 = _mm512_loadu_si512(carr_tbl + 32);
    __m512i c2 = _mm512_loadu_si512(carr_tbl + 64), c3 = _mm512_loadu_si512(carr_tbl + 96);
    __m512i c4 = _mm512_loadu_si512(carr_tbl + 128), c5 = _mm512_loadu_si512(carr_tbl + 160);
    __m512i c6 = _mm512_loadu_si512(carr_tbl + 192), c7 = _mm512_loadu_si512(carr_tbl + 224);
    __m512i b6 = _mm512_set1_epi16(0x40), b7 = _mm512_set1_epi16(0x80);
    __m512i lo8 = _mm512_set1_epi16(0xFF);
    __m512i xbias = _mm512_set1_epi8(tbl == mix_tbl_b ? (char)0x80 : 0);
    // phases in the lane order of _mm512_packus_epi32() (lane 4L+m <- sample 8L+m)
    __m512i zpa = _mm512_add_epi32(_mm512_set1_epi32(p), _mm512_mullo_epi32(
        _mm512_set1_epi32(s), _mm512_setr_epi32(0, 1, 2, 3, 8, 9, 10, 11, 16,
        17, 18, 19, 24, 25, 26, 27)));
    __m512i zpb = _mm512_add_epi32(zpa, _mm512_set1_epi32(s * 4));
    __m512i zs = _mm512_set1_epi32(s * 32);

    for ( ; i < N; i += 32) { // (I + jQ) * (cI + jcQ) in int16, no LUT of data
        __mmask32 m = (N - i >= 32) ? 0xFFFFFFFFu : (1u << (N - i)) - 1;
        __m512i d = _mm512_cvtepi8_epi16(_mm256_maskz_loadu_epi8(m, data + i));
        __m512i dI = _mm512_srai_epi16(_mm512_slli_epi16(d, 12), 12);
        __m512i dQ = _mm512_srai_epi16(d, 4);
        // carrier (cI | cQ << 8) of 8-bit phase by 4 x 64-entry LUT
        __m512i ix = _mm512_packus_epi32(_mm512_srli_epi32(zpa, 24),
            _mm512_srli_epi32(zpb, 24));
        __mmask32 k6 = _mm512_test_epi16_mask(ix, b6);
        __mmask32 k7 = _mm512_test_epi16_mask(ix, b7);
        __m512i c = _mm512_mask_blend_epi16(k7,
            _mm512_mask_blend_epi16(k6, _mm512_permutex2var_epi16(c0, ix, c1),
                _mm512_permutex2var_epi16(c2, ix, c3)),
            _mm512_mask_blend_epi16(k6, _mm512_permutex2var_epi16(c4, ix, c5),
                _mm512_permutex2var_epi16(c6, ix, c7)));
        __m512i cI = _mm512_srai_epi16(_mm512_slli_epi16(c, 8), 8);
        __m512i cQ = _mm512_srai_epi16(c, 8);
        zpa = _mm512_add_epi32(zpa, zs);
        zpb = _mm512_add_epi32(zpb, zs);
        __m512i re = _mm512_sub_epi16(_mm512_mullo_epi16(dI, cI),
            _mm512_mullo_epi16(dQ, cQ));
        __m512i im = _mm512_add_epi16(_mm512_mullo_epi16(dI, cQ),
            _mm512_mullo_epi16(dQ, cI));
        __m512i out = _mm512_or_si512(_mm512_and_si512(re, lo8),
            _mm512_slli_epi16(im, 8));
        _mm512_mask_storeu_epi16(IQ + i, m, _mm512_xor_si512(out, xbias));
    }
#elif defined(AVX2)
    __m256i yp = _mm256_set_epi32(p+s*7, p+s*6, p+s*5, p+s*4, p+s*3, p+s*2, p+s, p);
    __m256i ys = _mm256_set1_epi32(s*8);
    
    for ( ; i < N - 15; i += 16) {
        int idx[16];
        __m128i xdas = _mm_loadu_si128((__m128i *)(data + i));
        __m256i ydat = _mm256_cvtepu8_epi32(xdas);
        __m256i yidx = _mm256_add_epi32(_mm256_slli_epi32(ydat, 8),
            _mm256_srli_epi32(yp, 24));
        yp = _mm256_add_epi32(yp, ys);
        _mm256_storeu_si256((__m256i *)idx, yidx);
        xdas = _mm_srli_si128(xdas, 8);
        ydat = _mm256_cvtepu8_epi32(xdas);
        yidx = _mm256_add_epi32(_mm256_slli_epi32(ydat, 8),
            _mm256_srli_epi32(yp, 24));
        yp = _mm256_add_epi32(yp, ys);
        _mm256_storeu_si256((__m256i *)(idx + 8), yidx);
        for (int j = 0; j < 16; j++) {
            IQ[i+j] = tbl[idx[j]];
        }
    }
#elif defined(NEON)
    uint32_t p4[4] = {p, p + s, p + s * 2, p + s * 3};
    uint32x4_t yp0 = vld1q_u32(p4);
    uint32x4_t yp1 = vaddq_u32(yp0, vdupq_n_u32(s * 4));
    uint32x4_t ys8 = vdupq_n_u32(s * 8);
    
    for ( ; i < N - 15; i += 16) {
        uint16_t idx[16];
        uint8x16_t ydat = vld1q_u8(data + i);
        uint16x8_t yd0 = vshll_n_u8(vget_low_u8(ydat), 8);
        uint16x8_t yd1 = vshll_n_u8(vget_high_u8(ydat), 8);
        uint16x4_t yph0 = vmovn_u32(vshrq_n_u32(yp0, 24));
        uint16x4_t yph1 = vmovn_u32(vshrq_n_u32(yp1, 24));
        yp0 = vaddq_u32(yp0, ys8);
        yp1 = vaddq_u32(yp1, ys8);
        uint16x4_t yph2 = vmovn_u32(vshrq_n_u32(yp0, 24));
        uint16x4_t yph3 = vmovn_u32(vshrq_n_u32(yp1, 24));
        yp0 = vaddq_u32(yp0, ys8);
        yp1 = vaddq_u32(yp1, ys8);
        vst1q_u16(idx, vorrq_u16(yd0, vcombine_u16(yph0, yph1)));
        vst1q_u16(idx + 8, vorrq_u16(yd1, vcombine_u16(yph2, yph3)));
        for (int j = 0; j < 16; j++) {
            IQ[i+j] = tbl[idx[j]];
        }
    }
#endif
    for (p += s * i; i < N; i++, p += s) {
        int idx = ((int)data[i] << 8) + (p >> 24);
        IQ[i] = tbl[idx];
    }
}

// mix carrier -----------------------------------------------------------------
static void mix_carr(const sdr_buff_t *buff, int ix, int N, double phi,
    double step, sdr_cpx16_t *IQ)
{
    double scale = (double)(1 << 24) * NTBL;
    uint32_t p = (uint32_t)((phi - floor(phi)) * scale);
    uint32_t s = (uint32_t)(int)(step * scale);

    mix_carr_p(buff, ix, N, p, s, mix_tbl, IQ);
}

//------------------------------------------------------------------------------
//  Mix IF carrier to IF data.
//
//  args:
//      buff     (I)  IF data buffer
//      ix       (I)  Start index of IF data in IF data buffer
//      N        (I)  length of IF data
//      fs       (I)  IF data sampling frequency (Hz)
//      fc       (I)  IF carrier frequency (Hz)
//      phi      (I)  IF carrier phase offset (cyc)
//      IQ       (O)  IF-carrier-mixed IF data (N x 1)
//
//  return:
//      none
//
void sdr_mix_carr(const sdr_buff_t *buff, int ix, int N, double fs, double fc,
    double phi, sdr_cpx16_t *IQ)
{
    double step = fc / fs;
    
    if (ix + N <= buff->N) {
        mix_carr(buff, ix, N, phi, step, IQ);
    } else { // across IF buffer boundary
        int n = buff->N - ix;
        mix_carr(buff, ix, n, phi, step, IQ);
        mix_carr(buff, 0, N - n, phi + step * n, step, IQ + n);
    }
}

// convert carrier-mixed IF data to complex float --------------------------------
static void cvt_cpx16(const sdr_cpx16_t *IQ, int N, sdr_cpx_t *cpx)
{
    int i = 0;
#if defined(AVX512)
    __m512 zs = _mm512_set1_ps(SDR_CSCALE);

    for ( ; i < N - 7; i += 8) { // 8 samples = 16 x int8
        __m128i xiq = _mm_loadu_si128((const __m128i *)(IQ + i));
        __m512 ziq = _mm512_cvtepi32_ps(_mm512_cvtepi8_epi32(xiq));
        _mm512_storeu_ps((float *)(cpx + i), _mm512_mul_ps(ziq, zs));
    }
#elif defined(AVX2)
    __m256 ys = _mm256_set1_ps(SDR_CSCALE);
    
    for ( ; i < N - 3; i += 4) { // 4 samples = 8 x int8
        __m128i xiq = _mm_loadl_epi64((const __m128i *)(IQ + i));
        __m256 yiq = _mm256_cvtepi32_ps(_mm256_cvtepi8_epi32(xiq));
        _mm256_storeu_ps((float *)(cpx + i), _mm256_mul_ps(yiq, ys));
    }
#elif defined(NEON)
    float32x4_t ys = vdupq_n_f32(SDR_CSCALE);
    
    for ( ; i < N - 3; i += 4) { // 4 samples = 8 x int8
        int16x8_t yiq = vmovl_s8(vld1_s8((const int8_t *)(IQ + i)));
        float32x4_t y0 = vcvtq_f32_s32(vmovl_s16(vget_low_s16(yiq)));
        float32x4_t y1 = vcvtq_f32_s32(vmovl_s16(vget_high_s16(yiq)));
        vst1q_f32((float *)(cpx + i), vmulq_f32(y0, ys));
        vst1q_f32((float *)(cpx + i) + 4, vmulq_f32(y1, ys));
    }
#endif
    for ( ; i < N; i++) {
        cpx[i][0] = IQ[i].I * SDR_CSCALE;
        cpx[i][1] = IQ[i].Q * SDR_CSCALE;
    }
}

// mix carrier as complex float (chunk-wise to keep IQ cache-resident) -----------
static void mix_carr_f32(const sdr_buff_t *buff, int ix, int N, double phi,
    double step, sdr_cpx_t *cpx)
{
    sdr_cpx16_t IQ[MIX_F32_CHUNK];
    double scale = (double)(1 << 24) * NTBL;
    uint32_t p = (uint32_t)((phi - floor(phi)) * scale);
    uint32_t s = (uint32_t)(int)(step * scale);
    
    for (int i = 0; i < N; i += MIX_F32_CHUNK) {
        int n = MIN(MIX_F32_CHUNK, N - i);
        mix_carr_p(buff, ix + i, n, p + s * (uint32_t)i, s, mix_tbl, IQ);
        cvt_cpx16(IQ, n, cpx + i);
    }
}

// accumulate IF data of a code chip -----------------------------------------
static void bin_csk_chip(const sdr_cpx16_t *IQ, int j0, int j1, sdr_cpx_t *bin)
{
    int32_t sI = 0, sQ = 0;
    
    for (int j = j0; j < j1; j++) {
        sI += IQ[j].I;
        sQ += IQ[j].Q;
    }
    (*bin)[0] = (float)sI;
    (*bin)[1] = (float)sQ;
}

//------------------------------------------------------------------------------
//  Bin carrier-mixed IF data to code chips for L6 CSK decoding. IF data are
//  accumulated chip by chip on one of the two TDM sub-chip slots of each code
//  chip. (L6D and L6E signals are TDMed slot by slot: L6D=0, L6E=1)
//
//  args:
//      IQ       (I) IF-carrier-mixed IF data as sdr_cpx16_t array (N x 1)
//      N        (I) length of IF data
//      len_code (I) length of code as TDM slots (2 x chips)
//      slot     (I) TDM slot of the signal in a code chip (0 or 1)
//      coff     (I) fractional code offset (samples)
//      bins     (O) chip-binned IF data as sdr_cpx_t array (len_code / 2 x 1)
//
//  return:
//      none
//
void sdr_bin_csk(const sdr_cpx16_t *IQ, int N, int len_code, int slot,
    double coff, sdr_cpx_t *bins)
{
    int L = len_code / 2;
    double spc = (double)N / len_code, spc2 = spc * 2.0;
    coff += slot * spc;
    
    for (int k = 0; k < L; k++) {
        double t = coff + k * spc2, t1 = t + spc;
        int j0 = (int)t, j1 = (int)t1;
        if (j0 < t) j0++;
        if (j1 < t1) j1++;
        if (j1 > N) j1 = N;
        bin_csk_chip(IQ, j0, j1, bins + k);
    }
}

// convert complex buffer to IF data buffer (for python) ------------------------
static sdr_buff_t *buff_new_cpx(const sdr_cpx_t *buff, int len_buff, int ix,
    int N)
{
    sdr_buff_t *buff_cpx8 = sdr_buff_new(N, 2);
    for (int i = 0, j = ix; i < N; i++, j = (j + 1) % len_buff) {
        buff_cpx8->data[i] = SDR_CPX8((int8_t)buff[j][0], (int8_t)buff[j][1]);
    }
    return buff_cpx8;
}

#if defined(AVX512)
// accumulate u8 x s8 products of 64 bytes to int32 lanes ---------------------
static __m512i dot_u8s8(__m512i acc, __m512i a, __m512i b)
{
#if defined(__AVX512VNNI__)
    return _mm512_dpbusd_epi32(acc, a, b);
#else
    return _mm512_add_epi32(acc, _mm512_madd_epi16(_mm512_maddubs_epi16(a, b),
        _mm512_set1_epi16(1)));
#endif
}
#endif

// accumulate inner product of +128-biased IQ data and int8 code (csum debias) --
static void dot_IQ_code_acc(const sdr_cpx16_t *IQ, const int8_t *code,
    const int32_t *csum, int N, int32_t *sumI, int32_t *sumQ)
{
    int i = 0;
#if defined(AVX512)
    __m512i zsumI = _mm512_setzero_si512(), zsumQ = _mm512_setzero_si512();

    for ( ; i < N; i += 32) { // tail by masked loads
        __mmask32 m = (N - i >= 32) ? 0xFFFFFFFFu : (1u << (N - i)) - 1;
        __m512i zdata = _mm512_maskz_loadu_epi16(m, IQ + i); // biased u8
        __m512i zcodeI = _mm512_cvtepu8_epi16(
            _mm256_maskz_loadu_epi8(m, code + i));           // (c,0) pairs
        zsumI = dot_u8s8(zsumI, zdata, zcodeI);
        zsumQ = dot_u8s8(zsumQ, zdata, _mm512_slli_epi16(zcodeI, 8)); // (0,c)
    }
    // remove data bias: sum((d+128)*c) = sum(d*c) + 128*sum(c)
    int32_t b = 128 * (csum[N] - csum[0]);
    *sumI += _mm512_reduce_add_epi32(zsumI) - b;
    *sumQ += _mm512_reduce_add_epi32(zsumQ) - b;
#elif defined(AVX2)
    __m256i ysumI = _mm256_setzero_si256();
    __m256i ysumQ = _mm256_setzero_si256();
    __m256i yones = _mm256_set1_epi16(1);
    __m256i ydupI = _mm256_setr_epi8(0,-1,1,-1,2,-1,3,-1, 4,-1,5,-1,6,-1,7,-1,
        8,-1,9,-1,10,-1,11,-1, 12,-1,13,-1,14,-1,15,-1);
    
    for ( ; i < N - 15; i += 16) {
        __m256i ydata = _mm256_loadu_si256((__m256i *)(IQ + i)); // biased u8
        __m256i ycode8 = _mm256_broadcastsi128_si256(
            _mm_loadu_si128((const __m128i *)(code + i)));
        __m256i ycodeI = _mm256_shuffle_epi8(ycode8, ydupI); // (c,0) pairs
        __m256i ycodeQ = _mm256_slli_epi16(ycodeI, 8);       // (0,c) pairs
        __m256i ycorrI = _mm256_maddubs_epi16(ydata, ycodeI);
        __m256i ycorrQ = _mm256_maddubs_epi16(ydata, ycodeQ);
        ysumI = _mm256_add_epi32(ysumI, _mm256_madd_epi16(ycorrI, yones));
        ysumQ = _mm256_add_epi32(ysumQ, _mm256_madd_epi16(ycorrQ, yones));
    }
    int32_t sI[8], sQ[8];
    _mm256_storeu_si256((__m256i *)sI, ysumI);
    _mm256_storeu_si256((__m256i *)sQ, ysumQ);
    // remove data bias: sum((d+128)*c) = sum(d*c) + 128*sum(c)
    int32_t b = 128 * (csum[i] - csum[0]);
    *sumI += sI[0] + sI[1] + sI[2] + sI[3] + sI[4] + sI[5] + sI[6] + sI[7] - b;
    *sumQ += sQ[0] + sQ[1] + sQ[2] + sQ[3] + sQ[4] + sQ[5] + sQ[6] + sQ[7] - b;
#elif defined(NEON) && defined(__ARM_FEATURE_DOTPROD)
    int32x4_t ysumI = vdupq_n_s32(0);
    int32x4_t ysumQ = vdupq_n_s32(0);
    int8x16_t ybias = vdupq_n_s8(-128); // biased u8 ^ 0x80 -> signed
    
    for ( ; i < N - 15; i += 16) {
        int8x16x2_t ydata = vld2q_s8((const int8_t *)(IQ + i));
        int8x16_t ycode = vld1q_s8(code + i); // int8, I=Q
        ysumI = vdotq_s32(ysumI, veorq_s8(ydata.val[0], ybias), ycode);
        ysumQ = vdotq_s32(ysumQ, veorq_s8(ydata.val[1], ybias), ycode);
    }
    *sumI += vaddvq_s32(ysumI);
    *sumQ += vaddvq_s32(ysumQ);
#elif defined(NEON)
    int32x4_t ysumI = vdupq_n_s32(0);
    int32x4_t ysumQ = vdupq_n_s32(0);
    int8x8_t ybias = vdup_n_s8(-128); // biased u8 ^ 0x80 -> signed
    
    for ( ; i < N - 7; i += 8) {
        int8x8x2_t ydata = vld2_s8((int8_t *)(IQ + i));
        int8x8_t ycode = vld1_s8(code + i); // int8, I=Q
        ysumI = vpadalq_s16(ysumI, vmull_s8(veor_s8(ydata.val[0], ybias),
            ycode));
        ysumQ = vpadalq_s16(ysumQ, vmull_s8(veor_s8(ydata.val[1], ybias),
            ycode));
    }
    int32_t sI[4], sQ[4];
    vst1q_s32(sI, ysumI);
    vst1q_s32(sQ, ysumQ);
    *sumI += sI[0] + sI[1] + sI[2] + sI[3];
    *sumQ += sQ[0] + sQ[1] + sQ[2] + sQ[3];
#endif
    (void)csum;
    for ( ; i < N; i++) { // biased u8 -> signed
        *sumI += ((uint8_t)IQ[i].I - 128) * code[i];
        *sumQ += ((uint8_t)IQ[i].Q - 128) * code[i];
    }
}

// accumulate 4 inner products of biased IQ data and 2 int8 codes (shared load) -
static void dot_IQ_code2_acc(const sdr_cpx16_t *IQ, const int8_t *codeI,
    const int8_t *codeQ, const int32_t *csumI, const int32_t *csumQ, int N,
    int32_t *sII, int32_t *sIQ, int32_t *sQI, int32_t *sQQ)
{
    int i = 0;
#if defined(AVX512)
    __m512i zII = _mm512_setzero_si512(), zIQ = _mm512_setzero_si512();
    __m512i zQI = _mm512_setzero_si512(), zQQ = _mm512_setzero_si512();

    for ( ; i < N; i += 32) { // tail by masked loads
        __mmask32 m = (N - i >= 32) ? 0xFFFFFFFFu : (1u << (N - i)) - 1;
        __m512i zdata = _mm512_maskz_loadu_epi16(m, IQ + i); // biased u8
        __m512i cI = _mm512_cvtepu8_epi16(_mm256_maskz_loadu_epi8(m, codeI + i));
        __m512i cQ = _mm512_cvtepu8_epi16(_mm256_maskz_loadu_epi8(m, codeQ + i));
        zII = dot_u8s8(zII, zdata, cI);                        // (cI,0) pairs
        zIQ = dot_u8s8(zIQ, zdata, _mm512_slli_epi16(cI, 8)); // (0,cI) pairs
        zQI = dot_u8s8(zQI, zdata, cQ);
        zQQ = dot_u8s8(zQQ, zdata, _mm512_slli_epi16(cQ, 8));
    }
    int32_t bI = 128 * (csumI[N] - csumI[0]), bQ = 128 * (csumQ[N] - csumQ[0]);
    *sII += _mm512_reduce_add_epi32(zII) - bI;
    *sIQ += _mm512_reduce_add_epi32(zIQ) - bI;
    *sQI += _mm512_reduce_add_epi32(zQI) - bQ;
    *sQQ += _mm512_reduce_add_epi32(zQQ) - bQ;
#elif defined(AVX2)
    __m256i yII = _mm256_setzero_si256(), yIQ = _mm256_setzero_si256();
    __m256i yQI = _mm256_setzero_si256(), yQQ = _mm256_setzero_si256();
    __m256i yones = _mm256_set1_epi16(1);
    __m256i ydup = _mm256_setr_epi8(0,-1,1,-1,2,-1,3,-1, 4,-1,5,-1,6,-1,7,-1,
        8,-1,9,-1,10,-1,11,-1, 12,-1,13,-1,14,-1,15,-1);
    
    for ( ; i < N - 15; i += 16) {
        __m256i ydata = _mm256_loadu_si256((__m256i *)(IQ + i)); // biased u8
        __m256i cI = _mm256_broadcastsi128_si256(
            _mm_loadu_si128((const __m128i *)(codeI + i)));
        __m256i cQ = _mm256_broadcastsi128_si256(
            _mm_loadu_si128((const __m128i *)(codeQ + i)));
        __m256i cII = _mm256_shuffle_epi8(cI, ydup); // (cI,0) pairs
        __m256i cQI = _mm256_shuffle_epi8(cQ, ydup); // (cQ,0) pairs
        yII = _mm256_add_epi32(yII, _mm256_madd_epi16(
            _mm256_maddubs_epi16(ydata, cII), yones));
        yIQ = _mm256_add_epi32(yIQ, _mm256_madd_epi16(
            _mm256_maddubs_epi16(ydata, _mm256_slli_epi16(cII, 8)), yones));
        yQI = _mm256_add_epi32(yQI, _mm256_madd_epi16(
            _mm256_maddubs_epi16(ydata, cQI), yones));
        yQQ = _mm256_add_epi32(yQQ, _mm256_madd_epi16(
            _mm256_maddubs_epi16(ydata, _mm256_slli_epi16(cQI, 8)), yones));
    }
    int32_t aII[8], aIQ[8], aQI[8], aQQ[8];
    _mm256_storeu_si256((__m256i *)aII, yII);
    _mm256_storeu_si256((__m256i *)aIQ, yIQ);
    _mm256_storeu_si256((__m256i *)aQI, yQI);
    _mm256_storeu_si256((__m256i *)aQQ, yQQ);
    int32_t bI = 128 * (csumI[i] - csumI[0]), bQ = 128 * (csumQ[i] - csumQ[0]);
    for (int k = 0; k < 8; k++) {
        *sII += aII[k]; *sIQ += aIQ[k]; *sQI += aQI[k]; *sQQ += aQQ[k];
    }
    *sII -= bI; *sIQ -= bI; *sQI -= bQ; *sQQ -= bQ;
#elif defined(NEON) && defined(__ARM_FEATURE_DOTPROD)
    int32x4_t yII = vdupq_n_s32(0), yIQ = vdupq_n_s32(0);
    int32x4_t yQI = vdupq_n_s32(0), yQQ = vdupq_n_s32(0);
    int8x16_t ybias = vdupq_n_s8(-128); // biased u8 ^ 0x80 -> signed
    
    for ( ; i < N - 15; i += 16) {
        int8x16x2_t ydata = vld2q_s8((const int8_t *)(IQ + i));
        int8x16_t dI = veorq_s8(ydata.val[0], ybias);
        int8x16_t dQ = veorq_s8(ydata.val[1], ybias);
        int8x16_t cI = vld1q_s8(codeI + i), cQ = vld1q_s8(codeQ + i);
        yII = vdotq_s32(yII, dI, cI);
        yIQ = vdotq_s32(yIQ, dQ, cI);
        yQI = vdotq_s32(yQI, dI, cQ);
        yQQ = vdotq_s32(yQQ, dQ, cQ);
    }
    *sII += vaddvq_s32(yII); *sIQ += vaddvq_s32(yIQ);
    *sQI += vaddvq_s32(yQI); *sQQ += vaddvq_s32(yQQ);
#elif defined(NEON)
    int32x4_t yII = vdupq_n_s32(0), yIQ = vdupq_n_s32(0);
    int32x4_t yQI = vdupq_n_s32(0), yQQ = vdupq_n_s32(0);
    int8x8_t ybias = vdup_n_s8(-128); // biased u8 ^ 0x80 -> signed
    
    for ( ; i < N - 7; i += 8) {
        int8x8x2_t ydata = vld2_s8((int8_t *)(IQ + i));
        int8x8_t dI = veor_s8(ydata.val[0], ybias);
        int8x8_t dQ = veor_s8(ydata.val[1], ybias);
        int8x8_t cI = vld1_s8(codeI + i), cQ = vld1_s8(codeQ + i);
        yII = vpadalq_s16(yII, vmull_s8(dI, cI));
        yIQ = vpadalq_s16(yIQ, vmull_s8(dQ, cI));
        yQI = vpadalq_s16(yQI, vmull_s8(dI, cQ));
        yQQ = vpadalq_s16(yQQ, vmull_s8(dQ, cQ));
    }
    int32_t aII[4], aIQ[4], aQI[4], aQQ[4];
    vst1q_s32(aII, yII); vst1q_s32(aIQ, yIQ);
    vst1q_s32(aQI, yQI); vst1q_s32(aQQ, yQQ);
    for (int k = 0; k < 4; k++) {
        *sII += aII[k]; *sIQ += aIQ[k]; *sQI += aQI[k]; *sQQ += aQQ[k];
    }
#endif
    (void)csumI; (void)csumQ;
    for ( ; i < N; i++) { // biased u8 -> signed
        int dI = (uint8_t)IQ[i].I - 128, dQ = (uint8_t)IQ[i].Q - 128;
        *sII += dI * codeI[i];
        *sIQ += dQ * codeI[i];
        *sQI += dI * codeQ[i];
        *sQQ += dQ * codeQ[i];
    }
}

//------------------------------------------------------------------------------
//  Standard correlator. Mix IF carrier to IF data and make multiple
//  correlations with resampled spreading codes in a single fused pass. The
//  shifts of the spreading codes are specified as pos in unit of samples.
//
//  args:
//      buff     (I) IF data buffer
//      ix       (I) start index of IF data in IF data buffer
//      N        (I) length of IF data
//      fs       (I) IF data sampling frequency (Hz)
//      fc       (I) IF carrier frequency (Hz)
//      phi      (I) IF carrier phase offset (cyc)
//      code     (I) resampled code bank (N x SDR_N_CODES)
//      code_sum (I) prefix sums of code bank ((N + 1) x SDR_N_CODES)
//      code_Q   (I) imag part of complex code bank (NULL for real code)
//      code_sum_Q (I) prefix sums of imag code bank (NULL for real code)
//      scale    (I) code amplitude scale (1 for -1/0/+1 codes)
//      coff     (I) code offset (samples)
//      pos      (I) correlator shift positions (n x 1) (samples)
//      n        (I) size of pos (number of correlators)
//      pol      (I) code polarity over wrap-around (0:detect flip,+1/-1:fixed)
//      corr     (O) correlations as sdr_cpx_t array (n x 1)
//      C        (O) correlations before and after bit transition
//
//  return:
//      none
//
//  notes:
//      The value of spreading codes shall be int8_t. Correlations are
//      normalized by the code amplitude scale. To guarantee no overflow of
//      the int32_t accumulators, N * 127 * max|code| shall be < 2^31.
//      The code bank is stored as int8_t (1 byte/sample, I=Q implied).
//      code_sum[k*(N+1)+s] shall be the sum of the first s values of the
//      code bank slice k (used to remove the +128 data bias of the
//      uint8 x int8 inner products - see dot_IQ_code_acc()).
//      For a complex code (code_Q != NULL), the carrier is mixed once and both
//      real (code) and imag (code_Q) banks are correlated over the shared data
//      as corr = sum(IQ * conj(code + j * code_Q)).
//      code2* are the code banks for the part after the code wrap-around
//      (NULL = same as code*). They allow a replica whose sideband combination
//      sign differs across the wrap-around, which pol cannot express.
//
void sdr_corr_std2(const sdr_buff_t *buff, int ix, int N, double fs,
    double fc, double phi, const int8_t *code, const int32_t *code_sum,
    const int8_t *code_Q, const int32_t *code_sum_Q, const int8_t *code2,
    const int32_t *code2_sum, const int8_t *code2_Q,
    const int32_t *code2_sum_Q, int scale, double coff, const double *pos,
    int n, int pol, sdr_cpx_t *corr, sdr_cpx_t *C)
{
    sdr_cpx16_t IQ[MIX_CORR_CHUNK];
    sdr_cpx_t corr1[SDR_MAX_CORR], corr2[SDR_MAX_CORR];
    int32_t s1I[SDR_MAX_CORR] = {0}, s1Q[SDR_MAX_CORR] = {0};
    int32_t s2I[SDR_MAX_CORR] = {0}, s2Q[SDR_MAX_CORR] = {0};
    int32_t q1I[SDR_MAX_CORR] = {0}, q1Q[SDR_MAX_CORR] = {0};
    int32_t q2I[SDR_MAX_CORR] = {0}, q2Q[SDR_MAX_CORR] = {0};
    const int8_t *slice[SDR_MAX_CORR], *sliceQ[SDR_MAX_CORR];
    const int32_t *csums[SDR_MAX_CORR], *csumsQ[SDR_MAX_CORR];
    const int8_t *slice2[SDR_MAX_CORR], *slice2Q[SDR_MAX_CORR];
    const int32_t *csums2[SDR_MAX_CORR], *csums2Q[SDR_MAX_CORR];
    int js[SDR_MAX_CORR];
    float dot_EPL = 0.0, sign, cscale = SDR_CSCALE / scale;
    double lscale = (double)(1 << 24) * NTBL;
    uint32_t p0 = (uint32_t)((phi - floor(phi)) * lscale);
    uint32_t sp = (uint32_t)(int)(fc / fs * lscale);

    for (int i = 0; i < n; i++) {
        double p = coff + pos[i];
        int j = (int)floor(p), k = (int)((p - j) * SDR_N_CODES);
        if (j < 0) j += N; else if (j >= N) j -= N;
        js[i] = j;
        slice[i] = code + k * N;
        csums[i] = code_sum + k * (N + 1);
        slice2[i] = (code2 ? code2 : code) + k * N;
        csums2[i] = (code2_sum ? code2_sum : code_sum) + k * (N + 1);
        if (code_Q) {
            sliceQ[i] = code_Q + k * N;
            csumsQ[i] = code_sum_Q + k * (N + 1);
            slice2Q[i] = (code2_Q ? code2_Q : code_Q) + k * N;
            csums2Q[i] = (code2_sum_Q ? code2_sum_Q : code_sum_Q) + k * (N + 1);
        }
    }
    for (int c0 = 0; c0 < N; c0 += MIX_CORR_CHUNK) {
        int len = MIN(MIX_CORR_CHUNK, N - c0);
        int ix2 = (ix + c0) % buff->N;
        uint32_t p = p0 + sp * (uint32_t)c0;

        // mix carrier of the chunk (+128-biased for dot_IQ_code_acc())
        if (ix2 + len <= buff->N) {
            mix_carr_p(buff, ix2, len, p, sp, mix_tbl_b, IQ);
        } else { // across IF buffer boundary
            int m = buff->N - ix2;
            mix_carr_p(buff, ix2, m, p, sp, mix_tbl_b, IQ);
            mix_carr_p(buff, 0, len - m, p + sp * (uint32_t)m, sp, mix_tbl_b,
                IQ + m);
        }
        // accumulate correlations before and after the code wrap-around
        for (int i = 0; i < n; i++) {
            int j = js[i], a1 = MIN(c0 + len, j), b0 = MAX(c0, j);
            if (a1 > c0) {
                if (code_Q) dot_IQ_code2_acc(IQ, slice[i] + N - j + c0,
                    sliceQ[i] + N - j + c0, csums[i] + N - j + c0,
                    csumsQ[i] + N - j + c0, a1 - c0, s1I + i, s1Q + i,
                    q1I + i, q1Q + i);
                else dot_IQ_code_acc(IQ, slice[i] + N - j + c0,
                    csums[i] + N - j + c0, a1 - c0, s1I + i, s1Q + i);
            }
            if (b0 < c0 + len) {
                if (code_Q) dot_IQ_code2_acc(IQ + b0 - c0, slice2[i] + b0 - j,
                    slice2Q[i] + b0 - j, csums2[i] + b0 - j,
                    csums2Q[i] + b0 - j, c0 + len - b0, s2I + i, s2Q + i,
                    q2I + i, q2Q + i);
                else dot_IQ_code_acc(IQ + b0 - c0, slice2[i] + b0 - j,
                    csums2[i] + b0 - j, c0 + len - b0, s2I + i, s2Q + i);
            }
        }
    }
    if (code_Q) { // conj(code + j code_Q): Re = sI + qQ, Im = sQ - qI
        for (int i = 0; i < n; i++) {
            corr1[i][0] = (s1I[i] + q1Q[i]) * cscale;
            corr1[i][1] = (s1Q[i] - q1I[i]) * cscale;
            corr2[i][0] = (s2I[i] + q2Q[i]) * cscale;
            corr2[i][1] = (s2Q[i] - q2I[i]) * cscale;
        }
    } else {
        for (int i = 0; i < n; i++) {
            corr1[i][0] = s1I[i] * cscale;
            corr1[i][1] = s1Q[i] * cscale;
            corr2[i][0] = s2I[i] * cscale;
            corr2[i][1] = s2Q[i] * cscale;
        }
    }
    // detect polarity flip over code wrap-around (unless fixed by pol)
    for (int i = 0; i < 3; i++) {
        dot_EPL += corr1[i][0] * corr2[i][0] + corr1[i][1] * corr2[i][1];
    }
    sign = pol ? (pol > 0 ? 1.0f : -1.0f) : (dot_EPL < 0.0f ? -1.0f : 1.0f);

    for (int i = 0; i < n; i++) {
        corr[i][0] = (corr1[i][0] + sign * corr2[i][0]) / N;
        corr[i][1] = (corr1[i][1] + sign * corr2[i][1]) / N;
    }
    for (int i = 0; i < 2; i++) {
        C[0][i] = corr1[0][i];
        C[1][i] = corr2[0][i];
    }
}

// mix carrier and standard correlator (same code bank over the wrap-around) ----
void sdr_corr_std(const sdr_buff_t *buff, int ix, int N, double fs,
    double fc, double phi, const int8_t *code, const int32_t *code_sum,
    const int8_t *code_Q, const int32_t *code_sum_Q, int scale, double coff,
    const double *pos, int n, int pol, sdr_cpx_t *corr, sdr_cpx_t *C)
{
    sdr_corr_std2(buff, ix, N, fs, fc, phi, code, code_sum, code_Q, code_sum_Q,
        NULL, NULL, NULL, NULL, scale, coff, pos, n, pol, corr, C);
}

// mix carrier and standard correlator for complex buffer (for python) ---------
void sdr_corr_std_cpx(const sdr_cpx_t *buff, int len_buff, int ix, int N,
    double fs, double fc, double phi, const float *code, const double *pos,
    int n, sdr_cpx_t *corr)
{
    sdr_cpx_t C[2];
    sdr_buff_t *buff_cpx8 = buff_new_cpx(buff, len_buff, ix, N);
    int8_t *code_res = (int8_t *)sdr_malloc(sizeof(int8_t) * N);
    int32_t *code_sum = (int32_t *)sdr_malloc(sizeof(int32_t) * (N + 1));
    for (int i = 0; i < N; i++) {
        code_res[i] = (int8_t)floorf(code[i] * SDR_CBOC_SCALE + 0.5f);
        code_sum[i+1] = code_sum[i] + code_res[i];
    }
    sdr_corr_std(buff_cpx8, 0, N, fs, fc, phi, code_res, code_sum, NULL, NULL,
        SDR_CBOC_SCALE, 0.0, pos, n, 0, corr, C);
    sdr_free(code_res);
    sdr_free(code_sum);
    sdr_buff_free(buff_cpx8);
}

// persistent scratch buffer for FFT correlator ---------------------------------
static sdr_cpx_t *corr_fft_buff(int N)
{
    static __thread sdr_cpx_t *cpx = NULL;
    static __thread int N_buff = 0;
    
    if (N > N_buff) {
        sdr_cpx_free(cpx);
        cpx = sdr_cpx_malloc(N * 2);
        N_buff = N;
    }
    return cpx;
}

//------------------------------------------------------------------------------
//  FFT correlator. Mix IF carrier to IF data and make parallel correlations
//  with resampled spreading codes FFT in a single fused pass.
//
//  args:
//      buff     (I) IF data buffer
//      ix       (I) start index of IF data in IF data buffer
//      N        (I) length of IF data and size of code_fft
//      fs       (I) IF data sampling frequency (Hz)
//      fc       (I) IF carrier frequency (Hz)
//      phi      (I) IF carrier phase offset (cyc)
//      code_fft (I) resampled spreading codes FFT with conjugate as
//                   sdr_cpx_t array (N x 1)
//      corr     (O) correlations as sdr_cpx_t array (N x 1)
//
//  return:
//      none
//
void sdr_corr_fft(const sdr_buff_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx_t *code_fft, sdr_cpx_t *corr)
{
    sdr_cpx_t *cpx = corr_fft_buff(N);
    double step = fc / fs;
    
    // mix carrier as complex float
    if (ix + N <= buff->N) {
        mix_carr_f32(buff, ix, N, phi, step, cpx);
    } else { // across IF buffer boundary
        int n = buff->N - ix;
        mix_carr_f32(buff, ix, n, phi, step, cpx);
        mix_carr_f32(buff, 0, N - n, phi + step * n, step, cpx + n);
    }
    // ifft(fft(data) * code_fft) / N^2
    if (sdr_cpx_fft(cpx, N, SDR_FFT_FORWARD, cpx + N)) {
        sdr_cpx_mul(cpx + N, code_fft, N, 1.0f / N / N, cpx);
        (void)sdr_cpx_fft(cpx, N, SDR_FFT_BACKWARD, corr);
    }
}

// mix carrier and FFT correlator for complex buffer (for python) --------------
void sdr_corr_fft_cpx(const sdr_cpx_t *buff, int len_buff, int ix, int N,
    double fs, double fc, double phi, const sdr_cpx_t *code_fft,
    sdr_cpx_t *corr)
{
    sdr_buff_t *buff_cpx8 = buff_new_cpx(buff, len_buff, ix, N);
    sdr_corr_fft(buff_cpx8, 0, N, fs, fc, phi, code_fft, corr);
    sdr_buff_free(buff_cpx8);
}

// Hann window function --------------------------------------------------------
static float hann_window(int N, float *w)
{
    float w_sum_sqr = 0.0;
    
    for (int i = 0; i < N; i++) {
        w[i] = (float)(0.5 + 0.5 * cos(2.0 * PI * (i - 0.5 * (N - 1)) / (N - 1)));
        w_sum_sqr += SQR(w[i]);
    }
    return w_sum_sqr;
}

//------------------------------------------------------------------------------
//  Power spectral density of digitalized IF (inter-frequency) data.
//
//  args:
//      buff     (I) IF data buffer as sdr_cpx_t array
//      len_buff (I) length of buff
//      N        (I) FFT size
//      fs       (I) IF data sampling frequency (Hz)
//      IQ       (I) sampling type (1: I-sampling, 2: IQ-sampling)
//      psd      (O) PSD (dB/Hz) size: N/2 (IQ=1), N (IQ=2)
//
//  return:
//      none
//
void sdr_psd_cpx(const sdr_cpx_t *buff, int len_buff, int N, double fs, int IQ,
    float *psd)
{
    float *p = (float *)sdr_malloc(sizeof(float) * N);
    float *w = (float *)sdr_malloc(sizeof(float) * N);
    sdr_cpx_t *cpx1 = sdr_cpx_malloc(N);
    sdr_cpx_t *cpx2 = sdr_cpx_malloc(N);
    
    float w_sum_sqr = hann_window(N, w);
    
    // Welch's method without overlap
    int M = len_buff / N;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            cpx1[j][0] = buff[N*i+j][0] * w[j];
            cpx1[j][1] = buff[N*i+j][1] * w[j];
        }
        if (!sdr_cpx_fft(cpx1, N, SDR_FFT_FORWARD, cpx2)) break;
        for (int j = 0; j < N; j++) {
            p[j] += SQR(cpx2[j][0]) + SQR(cpx2[j][1]);
        }
    }
    // scale complies with matplotlib.psd()
    float scale = (float)(1.0 / M / w_sum_sqr / fs * (IQ == 1 ? 2.0 : 1.0));
    
    if (IQ == 1) { // I
        for (int i = 0; i < N / 2; i++) {
            psd[i] = 10.0f * log10f(p[i] * scale);
        }
    } else { // IQ
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
    } else if (p && sscanf(p, ":%d", &port) == 1) { // TCP client (path = addr:port)
        stat = stropen(str, STR_TCPCLI, STR_MODE_W, path);
    } else { // file (path = file[::opt...])
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
    sdr_free(str); // allocated by sdr_str_open()
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
    if (!path || !*path || log_str_own) return 0;
    
    if (!(log_str_own = sdr_str_open(path))) {
        fprintf(stderr, "log stream open error %s\n", path);
        return 0;
    }
    sdr_log_add_str(log_str_own);
    return 1;
}

// close log -------------------------------------------------------------------
void sdr_log_close(void)
{
    sdr_log_rm_str(log_str_own);
    sdr_str_close(log_str_own);
    log_str_own = NULL;
}

// register log stream (non-owned) ---------------------------------------------
void sdr_log_add_str(stream_t *str)
{
    if (!str) return;
    for (int i = 0; i < SDR_MAX_STR; i++) {
        if (log_strs[i] == str) return;
    }
    for (int i = 0; i < SDR_MAX_STR; i++) {
        if (!log_strs[i]) {
            log_strs[i] = str;
            return;
        }
    }
}

// unregister log stream --------------------------------------------------------
void sdr_log_rm_str(stream_t *str)
{
    if (!str) return;
    for (int i = 0; i < SDR_MAX_STR; i++) {
        if (log_strs[i] == str) log_strs[i] = NULL;
    }
}

// set log level ---------------------------------------------------------------
void sdr_log_level(int level)
{
    log_lvl = level;
}

// set log mask ----------------------------------------------------------------
void sdr_log_mask(const int *mask, int n)
{
    for (int i = 0; i < n && log_types[i]; i++) {
        log_mask[i] = mask[i];
    }
}

// output log ------------------------------------------------------------------
void sdr_log(int level, const char *msg, ...)
{
    va_list ap;
    int i;
    
    for (i = 0; log_types[i]; i++) {
        if (log_mask[i] && !strncmp(msg, log_types[i], strlen(log_types[i]))) {
            break;
        }
    }
    if (!log_types[i]) return;
    
    va_start(ap, msg);
    
    if (log_lvl == 0) {
        vprintf(msg, ap);
    } else if (level <= log_lvl) {
        char buff[1024];
        int len = vsnprintf(buff, sizeof(buff) - 2, msg, ap);
        if (len < 0) {
            va_end(ap);
            return;
        }
        len = MIN(len, (int)sizeof(buff) - 3);
        for (i = 0; i < SDR_MAX_STR; i++) {
            if (!log_strs[i]) continue;
            strwrite(log_strs[i], (uint8_t *)buff, len);
            strwrite(log_strs[i], (uint8_t *)"\r\n", 2);
        }
        sdr_mutex_lock(&log_buff_mtx);
        if (log_buff_p + len + 2 < MAX_LOG_BUFF) {
            log_buff_p += sprintf(log_buff + log_buff_p, "%s\n", buff);
        }
        sdr_mutex_unlock(&log_buff_mtx);
    }
    va_end(ap);
}

// get log buffer --------------------------------------------------------------
int sdr_get_log(char *buff, int size)
{
    sdr_mutex_lock(&log_buff_mtx);
    int out_size = snprintf(buff, size, "%s", log_buff);
    log_buff[0] = '\0';
    log_buff_p = 0;
    sdr_mutex_unlock(&log_buff_mtx);
    return out_size <= size ? out_size : size;
}

// get log status --------------------------------------------------------------
int sdr_log_stat(void)
{
    char msg[1024];
    for (int i = 0; i < SDR_MAX_STR; i++) {
        if (log_strs[i]) return strstat(log_strs[i], msg);
    }
    return 0;
}

// parse numbers list and range ------------------------------------------------
int sdr_parse_nums(const char *str, int *prns)
{
    int n = 0, prn, prn1, prn2;
    char buff[1024], *p, *q;
    
    snprintf(buff, sizeof(buff), "%s", str);
    
    for (p = buff; ; p = q + 1) {
        if ((q = strchr(p, ','))) {
            *q = '\0';
        }
        if (sscanf(p, "%d-%d", &prn1, &prn2) == 2) {
            for (prn = prn1; prn <= prn2 && n < SDR_MAX_NPRN; prn++) {
                prns[n++] = prn;
            }
        } else if (sscanf(p, "%d", &prn) == 1 && n < SDR_MAX_NPRN) {
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
    for (int i = 0; i < nbit; i++) {
        buff[i] = (data[i / 8] >> (7 - i % 8)) & 1;
    }
}

// unpack uint32_t data to bit array -------------------------------------------
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


