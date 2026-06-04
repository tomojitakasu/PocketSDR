//
//  Performance measurements for important SDR APIs.
//
#include "test_sdr.h"

#define NUM_MAIN_N 7
#define NUM_PSD_N  3
#define TARGET_SEC 0.050

static const int MAIN_N[NUM_MAIN_N] = {
    6000, 12000, 16000, 24000, 32000, 48000, 64000
};
static const int PSD_N[NUM_PSD_N] = {1024, 2048, 4096};

static volatile double perf_sink = 0.0;

typedef void (*perf_func_t)(void *ctx);

typedef struct {
    sdr_cpx_t *a, *b, *c;
    int N;
} cpx_mul_ctx_t;

typedef struct {
    sdr_cpx_t *in, *out;
    int N;
} fft_ctx_t;

typedef struct {
    sdr_lpf_t *lpf;
    sdr_cpx8_t *data;
    int N;
} lpf_ctx_t;

typedef struct {
    sdr_buff_t buff;
    sdr_cpx16_t *IQ;
    int N;
} mix_ctx_t;

typedef struct {
    sdr_cpx16_t *IQ;
    sdr_cpx16_t *code;
    sdr_cpx_t corr[SDR_MAX_CORR], C[2];
    double pos[3];
    int N;
} corr_std_ctx_t;

typedef struct {
    sdr_cpx16_t *IQ;
    sdr_cpx_t *code_fft, *corr;
    int N;
} corr_fft_ctx_t;

typedef struct {
    sdr_cpx_t *buff;
    float *psd;
    int len_buff, N;
} psd_ctx_t;

typedef struct {
    int8_t *code_I;
    int len_code, N;
    sdr_cpx16_t *code_res;
} res_code_ctx_t;

typedef struct {
    int8_t *code_I;
    int len_code, N;
    sdr_cpx_t *code_fft;
} gen_code_fft_ctx_t;

typedef struct {
    sdr_array_t *array;
    sdr_rcv_t rcv;
    int N;
} arch_ctx_t;

// get high-resolution time ----------------------------------------------------
static double now_sec(void)
{
#ifdef WIN32
    LARGE_INTEGER t, f;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)f.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
#endif
}

// fill complex float test data ------------------------------------------------
static void fill_cpx(sdr_cpx_t *cpx, int N)
{
    for (int i = 0; i < N; i++) {
        cpx[i][0] = (float)((i % 17) - 8) * 0.125f;
        cpx[i][1] = (float)((i % 13) - 6) * 0.125f;
    }
}

// fill packed complex 4-bit test data -----------------------------------------
static void fill_cpx8(sdr_cpx8_t *data, int N)
{
    for (int i = 0; i < N; i++) {
        data[i] = SDR_CPX8((int8_t)(i % 7 - 3), (int8_t)(3 - i % 7));
    }
}

// fill complex 8-bit test data ------------------------------------------------
static void fill_cpx16(sdr_cpx16_t *data, int N)
{
    for (int i = 0; i < N; i++) {
        data[i].I = (int8_t)(i % 7 - 3);
        data[i].Q = (int8_t)(3 - i % 5);
    }
}

// fill resampled code bank ----------------------------------------------------
static void fill_code_bank(sdr_cpx16_t *code, int N, int cpx)
{
    for (int k = 0; k < SDR_N_CODES; k++) {
        for (int i = 0; i < N; i++) {
            int8_t I = (int8_t)(((i + k) & 1) ? -1 : 1);
            int8_t Q = cpx ? (int8_t)(((i + 2 * k) & 1) ? -1 : 1) : I;
            code[k * N + i].I = I;
            code[k * N + i].Q = Q;
        }
    }
}

// allocate IF data buffer for performance test --------------------------------
static sdr_buff_t *test_buff_new(int N)
{
    sdr_buff_t *buff = (sdr_buff_t *)sdr_malloc(sizeof(sdr_buff_t));
    
    buff->data = (sdr_cpx8_t *)sdr_malloc(sizeof(sdr_cpx8_t) * N);
    buff->IQ = 2;
    buff->N = N;
    fill_cpx8(buff->data, N);
    return buff;
}

// free IF data buffer for performance test ------------------------------------
static void test_buff_free(sdr_buff_t *buff)
{
    if (!buff) return;
    sdr_free(buff->data);
    sdr_free(buff);
}

// run one benchmark case ------------------------------------------------------
static void run_bench(const char *name, int N, int work_N, perf_func_t func,
    void *ctx)
{
    int reps = 1, max_reps = 100000;
    double t0, dt = 0.0;
    
    func(ctx); // warm-up, including lazy FFT plan creation.
    
    while (reps < max_reps) {
        t0 = now_sec();
        for (int i = 0; i < reps; i++) {
            func(ctx);
        }
        dt = now_sec() - t0;
        if (dt >= TARGET_SEC) break;
        reps *= 2;
    }
    double avg_us = dt * 1e6 / reps;
    double msps = work_N / (avg_us * 1e-6) * 1e-6;
    
    printf("%-24s %7d %7d %10.3f %10.3f %10.3f\n", name, N, reps,
        dt * 1e3, avg_us, msps);
}

// benchmark sdr_cpx_mul() -----------------------------------------------------
static void bench_cpx_mul(void *ctx)
{
    cpx_mul_ctx_t *p = (cpx_mul_ctx_t *)ctx;
    sdr_cpx_mul(p->a, p->b, p->N, 0.25f, p->c);
    perf_sink += p->c[0][0];
}

// benchmark sdr_cpx_fft() -----------------------------------------------------
static void bench_cpx_fft(void *ctx)
{
    fft_ctx_t *p = (fft_ctx_t *)ctx;
    (void)sdr_cpx_fft(p->in, p->N, SDR_FFT_FORWARD, p->out);
    perf_sink += p->out[0][0];
}

// benchmark sdr_lpf_apply() ---------------------------------------------------
static void bench_lpf_apply(void *ctx)
{
    lpf_ctx_t *p = (lpf_ctx_t *)ctx;
    sdr_lpf_apply(p->lpf, p->data, p->N);
    perf_sink += SDR_CPX8_I(p->data[0]);
}

// benchmark sdr_mix_carr() ----------------------------------------------------
static void bench_mix_carr(void *ctx)
{
    mix_ctx_t *p = (mix_ctx_t *)ctx;
    sdr_mix_carr(&p->buff, 17, p->N, 24e6, -4200.0, 0.125, p->IQ);
    perf_sink += p->IQ[0].I;
}

// benchmark sdr_corr_std() ----------------------------------------------------
static void bench_corr_std(void *ctx)
{
    corr_std_ctx_t *p = (corr_std_ctx_t *)ctx;
    sdr_corr_std(p->IQ, p->code, p->N, 0.35, p->pos, 3, p->corr, p->C);
    perf_sink += p->corr[0][0];
}

// benchmark sdr_corr_std_cpx_code() -------------------------------------------
static void bench_corr_std_cpx_code(void *ctx)
{
    corr_std_ctx_t *p = (corr_std_ctx_t *)ctx;
    sdr_corr_std_cpx_code(p->IQ, p->code, p->N, 0.35, p->pos, 3, p->corr,
        p->C);
    perf_sink += p->corr[0][0];
}

// benchmark sdr_corr_fft() ----------------------------------------------------
static void bench_corr_fft(void *ctx)
{
    corr_fft_ctx_t *p = (corr_fft_ctx_t *)ctx;
    sdr_corr_fft(p->IQ, p->code_fft, p->N, p->corr);
    perf_sink += p->corr[0][0];
}

// benchmark sdr_psd_cpx() -----------------------------------------------------
static void bench_psd_cpx(void *ctx)
{
    psd_ctx_t *p = (psd_ctx_t *)ctx;
    sdr_psd_cpx(p->buff, p->len_buff, p->N, 24e6, 2, p->psd);
    perf_sink += p->psd[0];
}

// benchmark sdr_res_code() ----------------------------------------------------
static void bench_res_code(void *ctx)
{
    res_code_ctx_t *p = (res_code_ctx_t *)ctx;
    sdr_res_code(p->code_I, NULL, p->len_code, 1e-3, 0.1e-3,
        (double)p->N / 1e-3, p->N, 0, p->code_res);
    perf_sink += p->code_res[0].I;
}

// benchmark sdr_gen_code_fft() ------------------------------------------------
static void bench_gen_code_fft(void *ctx)
{
    gen_code_fft_ctx_t *p = (gen_code_fft_ctx_t *)ctx;
    sdr_gen_code_fft(p->code_I, NULL, p->len_code, 1e-3, 0.1e-3,
        (double)p->N / 1e-3, p->N, 0, p->code_fft);
    perf_sink += p->code_fft[0][0];
}

// benchmark sdr_arch_combine() ------------------------------------------------
static void bench_arch_combine(void *ctx)
{
    arch_ctx_t *p = (arch_ctx_t *)ctx;
    sdr_arch_combine(p->rcv.arch, &p->rcv, 0);
    perf_sink += SDR_CPX8_I(p->rcv.buff[p->rcv.nrfch]->data[0]);
}

// setup array combiner context ------------------------------------------------
static void setup_arch_ctx(arch_ctx_t *ctx, int N)
{
    ctx->array = sdr_array_new(2, 0);
    memset(&ctx->rcv, 0, sizeof(ctx->rcv));
    ctx->N = N;
    ctx->rcv.fs = 4e6;
    ctx->rcv.N = N;
    ctx->rcv.nrfch = 2;
    ctx->rcv.narch = 1;
    ctx->rcv.array = ctx->array;
    ctx->rcv.rfch[0].fo = 1575.42e6;
    ctx->rcv.rfch[1].fo = 1575.42e6;
    ctx->rcv.rfch[0].IQ = ctx->rcv.rfch[1].IQ = 2;
    ctx->rcv.rfch[0].bits = ctx->rcv.rfch[1].bits = 4;
    ctx->rcv.buff[0] = test_buff_new(N);
    ctx->rcv.buff[1] = test_buff_new(N);
    ctx->rcv.buff[2] = test_buff_new(N);
    sdr_arch_set_beam(ctx->rcv.arch, &ctx->rcv, 0.0, PI / 2, 1.0);
}

// free array combiner context -------------------------------------------------
static void free_arch_ctx(arch_ctx_t *ctx)
{
    for (int i = 0; i < 3; i++) {
        test_buff_free(ctx->rcv.buff[i]);
    }
    sdr_array_free(ctx->array);
}

// run one main benchmark case -------------------------------------------------
static void run_main_size_case(const char *api, int N, int8_t *code_I,
    int len_code)
{
    if (!strcmp(api, "sdr_cpx_mul")) {
        cpx_mul_ctx_t ctx;
        
        ctx.N = N;
        ctx.a = sdr_cpx_malloc(N);
        ctx.b = sdr_cpx_malloc(N);
        ctx.c = sdr_cpx_malloc(N);
        fill_cpx(ctx.a, N);
        fill_cpx(ctx.b, N);
        run_bench(api, N, N, bench_cpx_mul, &ctx);
        sdr_cpx_free(ctx.c);
        sdr_cpx_free(ctx.b);
        sdr_cpx_free(ctx.a);
    }
    else if (!strcmp(api, "sdr_cpx_fft")) {
        fft_ctx_t ctx;
        
        ctx.N = N;
        ctx.in = sdr_cpx_malloc(N);
        ctx.out = sdr_cpx_malloc(N);
        fill_cpx(ctx.in, N);
        run_bench(api, N, N, bench_cpx_fft, &ctx);
        sdr_cpx_free(ctx.out);
        sdr_cpx_free(ctx.in);
    }
    else if (!strcmp(api, "sdr_lpf_apply")) {
        lpf_ctx_t ctx;
        
        ctx.N = N;
        ctx.lpf = sdr_lpf_new(1.0e6, 24.0e6);
        ctx.data = (sdr_cpx8_t *)sdr_malloc(sizeof(sdr_cpx8_t) * N);
        fill_cpx8(ctx.data, N);
        run_bench(api, N, N, bench_lpf_apply, &ctx);
        sdr_free(ctx.data);
        sdr_lpf_free(ctx.lpf);
    }
    else if (!strcmp(api, "sdr_mix_carr")) {
        mix_ctx_t ctx;
        
        ctx.N = N;
        ctx.buff.data = (sdr_cpx8_t *)sdr_malloc(sizeof(sdr_cpx8_t) *
            (N + 32));
        ctx.buff.IQ = 2;
        ctx.buff.N = N + 32;
        fill_cpx8(ctx.buff.data, ctx.buff.N);
        ctx.IQ = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N);
        run_bench(api, N, N, bench_mix_carr, &ctx);
        sdr_free(ctx.IQ);
        sdr_free(ctx.buff.data);
    }
    else if (!strcmp(api, "sdr_corr_std") ||
        !strcmp(api, "sdr_corr_std_cpx_code")) {
        corr_std_ctx_t ctx;
        int cpx = !strcmp(api, "sdr_corr_std_cpx_code");
        
        ctx.N = N;
        ctx.IQ = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N);
        ctx.code = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N *
            SDR_N_CODES);
        fill_cpx16(ctx.IQ, N);
        fill_code_bank(ctx.code, N, cpx);
        ctx.pos[0] = -0.5;
        ctx.pos[1] = 0.0;
        ctx.pos[2] = 0.5;
        run_bench(api, N, N, cpx ? bench_corr_std_cpx_code : bench_corr_std,
            &ctx);
        sdr_free(ctx.code);
        sdr_free(ctx.IQ);
    }
    else if (!strcmp(api, "sdr_corr_fft")) {
        corr_fft_ctx_t ctx;
        
        ctx.N = N;
        ctx.IQ = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N);
        ctx.code_fft = sdr_cpx_malloc(N);
        ctx.corr = sdr_cpx_malloc(N);
        fill_cpx16(ctx.IQ, N);
        sdr_gen_code_fft(code_I, NULL, len_code, 1e-3, 0.0,
            (double)N / 1e-3, N, 0, ctx.code_fft);
        run_bench(api, N, N, bench_corr_fft, &ctx);
        sdr_cpx_free(ctx.corr);
        sdr_cpx_free(ctx.code_fft);
        sdr_free(ctx.IQ);
    }
    else if (!strcmp(api, "sdr_res_code")) {
        res_code_ctx_t ctx;
        
        ctx.code_I = code_I;
        ctx.len_code = len_code;
        ctx.N = N;
        ctx.code_res = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N);
        run_bench(api, N, N, bench_res_code, &ctx);
        sdr_free(ctx.code_res);
    }
    else if (!strcmp(api, "sdr_gen_code_fft")) {
        gen_code_fft_ctx_t ctx;
        
        ctx.code_I = code_I;
        ctx.len_code = len_code;
        ctx.N = N;
        ctx.code_fft = sdr_cpx_malloc(N);
        run_bench(api, N, N, bench_gen_code_fft, &ctx);
        sdr_cpx_free(ctx.code_fft);
    }
    else if (!strcmp(api, "sdr_arch_combine")) {
        arch_ctx_t ctx;
        
        setup_arch_ctx(&ctx, N);
        run_bench(api, N, N, bench_arch_combine, &ctx);
        free_arch_ctx(&ctx);
    }
}

// run main benchmark cases grouped by API -------------------------------------
static void run_main_api_cases(const char *api, int8_t *code_I, int len_code)
{
    for (int i = 0; i < NUM_MAIN_N; i++) {
        run_main_size_case(api, MAIN_N[i], code_I, len_code);
    }
}

// run PSD benchmark cases -----------------------------------------------------
static void run_psd_cases(void)
{
    for (int i = 0; i < NUM_PSD_N; i++) {
        int N = PSD_N[i];
        psd_ctx_t psd;
        
        psd.N = N;
        psd.len_buff = N * 4;
        psd.buff = sdr_cpx_malloc(psd.len_buff);
        psd.psd = (float *)sdr_malloc(sizeof(float) * N);
        fill_cpx(psd.buff, psd.len_buff);
        run_bench("sdr_psd_cpx", N, psd.len_buff, bench_psd_cpx, &psd);
        sdr_free(psd.psd);
        sdr_cpx_free(psd.buff);
    }
}

// main ------------------------------------------------------------------------
int main(void)
{
    int len_code = 0;
    int8_t *code_I = sdr_gen_code("L1CA", 1, &len_code);
    
    TEST_ASSERT_TRUE(code_I != NULL);
    TEST_ASSERT_TRUE(len_code > 0);
    
    printf("Pocket SDR performance measurements\n");
    printf("%-24s %7s %7s %10s %10s %10s\n", "API", "N", "reps",
        "total_ms", "avg_us", "Msamp/s");
    
    run_main_api_cases("sdr_cpx_mul", code_I, len_code);
    run_main_api_cases("sdr_cpx_fft", code_I, len_code);
    run_main_api_cases("sdr_lpf_apply", code_I, len_code);
    run_main_api_cases("sdr_mix_carr", code_I, len_code);
    run_main_api_cases("sdr_corr_std", code_I, len_code);
    run_main_api_cases("sdr_corr_std_cpx_code", code_I, len_code);
    run_main_api_cases("sdr_corr_fft", code_I, len_code);
    run_psd_cases();
    run_main_api_cases("sdr_res_code", code_I, len_code);
    run_main_api_cases("sdr_gen_code_fft", code_I, len_code);
    run_main_api_cases("sdr_arch_combine", code_I, len_code);
    
    sdr_free(code_I);
    printf("perf_sink=%.3f\n", perf_sink);
    return 0;
}
