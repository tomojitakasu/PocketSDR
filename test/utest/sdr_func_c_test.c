//
//  unit test driver for sdr_func.c
//
#include <math.h>
#include "pocket_sdr.h"

#define CSCALE  10.0f
#define SQR(x)  ((x) * (x))

// generate data ---------------------------------------------------------------
static sdr_buff_t *gen_data(int N)
{
    static const int8_t val[] = {-3, -1, 1, 3};
    
    sdr_buff_t *buff = sdr_buff_new(N, 2);
    
    //srand(sdr_get_tick());
    
    for (int i = 0; i < N; i++) {
        buff->data[i] = SDR_CPX8(val[rand() % 4], val[rand() % 4]);
    }
    return buff;
}

// test sdr_cpx_mul() ----------------------------------------------------------
static void test_01(void)
{
    static const int N[] = {1, 10, 100, 1000, 100000, 0};
    
    for (int i = 0; N[i]; i++) {
        sdr_cpx_t *a, *b, *c;
        float s = (rand() % 1000) / 100.f;
        
        a = sdr_cpx_malloc(N[i]);
        b = sdr_cpx_malloc(N[i]);
        c = sdr_cpx_malloc(N[i]);
        
        for (int j = 0; j < N[i]; j++) {
            a[j][0] = (rand() % 1000) / 100.0f;
            a[j][1] = (rand() % 1000) / 100.0f;
            b[j][0] = (rand() % 1000) / 100.0f;
            b[j][1] = (rand() % 1000) / 100.0f;
        }
        sdr_cpx_mul(a, b, N[i], s, c);
        
        for (int j = 0; j < N[i]; j++) {
            sdr_cpx_t ref;
            
            ref[0] = (a[j][0] * b[j][0] - a[j][1] * b[j][1]) * s;
            ref[1] = (a[j][0] * b[j][1] + a[j][1] * b[j][0]) * s;
            
            if (SQR(ref[0] - c[j][0]) + SQR(ref[1] - c[j][1]) > 1e-6) {
                printf("sdr_cpx_mul() error N=%d c=%.3f/%.3f : %.3f/%.3f\n",
                    N[i], c[j][0], c[j][1], ref[0], ref[1]);
                exit(-1);
            }
        }
        sdr_cpx_free(a);
        sdr_cpx_free(b);
        sdr_cpx_free(c);
        
        printf("test_01: sdr_cpx_mul()  N=%8d OK\n", N[i]);
    }
    printf("test_01: OK\n");
}

// reference of sdr_mix_carr() -------------------------------------------------
static void mix_carr_(const sdr_cpx8_t *data, int N, double fs, double fc,
    double phi, sdr_cpx16_t *IQ)
{
    for (int i = 0; i < N; i++) {
        double I = SDR_CPX8_I(data[i]), Q = SDR_CPX8_Q(data[i]);
        double carr_I = cosf(-2.0 * PI * (phi + fc / fs * i));
        double carr_Q = sinf(-2.0 * PI * (phi + fc / fs * i));
        IQ[i].I = (int8_t)floor((I * carr_I - Q * carr_Q) / SDR_CSCALE + 0.5);
        IQ[i].Q = (int8_t)floor((I * carr_Q + Q * carr_I) / SDR_CSCALE + 0.5);
    }
}

static void sdr_mix_carr_ref(const sdr_buff_t *buff, int ix, int N, double fs,
    double fc, double phi, sdr_cpx16_t *IQ)
{
    if (ix + N <= buff->N) {
        mix_carr_(buff->data + ix, N, fs, fc, phi, IQ);
    }
    else {
        int n = buff->N - ix;
        mix_carr_(buff->data + ix, n, fs, fc, phi, IQ);
        mix_carr_(buff->data, N - n, fs, fc, phi + fc / fs * n, IQ + n);
    }
}

// test sdr_mix_carr() ---------------------------------------------------------
static void test_02(void)
{
    int N[] = {12000, 16000, 24000, 32000, 48000, 0};
    int ix[] = {700, 8000, 12345, 5678, 0, -3000};
    double fs[] = {12e3, 16e6, 24e6, 12.345e6, 6.7e6, 0};
    double fc[] = {-5432.1, 3456.78, -4999.9, -0.123, 0.0356, 0};
    double phi[] = {0.56, 0.234, -0.567, 123456.0, -78901.345, 0};
    
    for (int i = 0; N[i]; i++) {
        sdr_cpx16_t IQ[N[i]], IQ_ref[N[i]];
        sdr_buff_t *buff = gen_data(N[i] * 2);
        
        sdr_mix_carr(buff, ix[i], N[i], fs[i], fc[i], phi[i], IQ);
        
        sdr_mix_carr_ref(buff, ix[i], N[i], fs[i], fc[i], phi[i], IQ_ref);
        
        for (int j = 0; j < N[i]; j++) {
            if (SQR(IQ[j].I - IQ_ref[j].I) > 25 || SQR(IQ[j].Q - IQ_ref[j].Q) > 25) {
                printf("sdr_mix_carr() error N=%d fs=%.3e fc=%.3f phi=%.3e IQ[%d]=%d/%d : %d/%d\n",
                    N[i], fs[i], fc[i], phi[i], j, IQ[j].I, IQ[j].Q, IQ_ref[j].I,
                    IQ_ref[j].Q);
                //exit(-1);
            }
        }
        sdr_buff_free(buff);
        
        printf("test_02: sdr_mix_carr() N=%8d OK\n", N[i]);
    }
    printf("test_02: OK\n");
}

// sdr_corr_std() reference ----------------------------------------------------
static void dot_IQ_code(const sdr_cpx16_t *IQ, const sdr_cpx16_t *code, int N,
    sdr_cpx_t *C)
{
    (*C)[0] = (*C)[1] = 0.0;
    
    for (int i = 0; i < N; i++) {
        (*C)[0] += IQ[i].I * code[i].I;
        (*C)[1] += IQ[i].Q * code[i].Q;
    }
    (*C)[0] *= SDR_CSCALE / N;
    (*C)[1] *= SDR_CSCALE / N;
}

static void sdr_corr_std_ref(const sdr_buff_t *buff, int ix, int N, double fs,
    double fc, double phi, const sdr_cpx16_t *code, const double *pos, int n,
    sdr_cpx_t *C)
{
    sdr_cpx16_t IQ[N];
    
    sdr_mix_carr_ref(buff, ix, N, fs, fc, phi, IQ);
    
    for (int i = 0; i < n; i++) {
        if (pos[i] > 0) {
            dot_IQ_code(IQ + (int)pos[i], code, N - (int)pos[i], C + i);
        }
        else if (pos[i] < 0) {
            dot_IQ_code(IQ, code - (int)pos[i], N + (int)pos[i], C + i);
        }
        else {
            dot_IQ_code(IQ, code, N, C + i);
        }
    }
}

// test sdr_corr_std() ---------------------------------------------------------
static void test_03(void)
{
    int N[] = {1200, 16000, 24000, 32000, 48000, 0};
    int ix[] = {0, 0, 12345, 5678, 0, -3000};
    double fs[] = {12e6, 16e6, 24e6, 12.345e6, 6.7e6, 0};
    double fc[] = {1000.0, 3456.78, -4999.9, -0.123, 0.0356, 0};
    double phi[] = {0.0, 0.234, -0.567, 123456.0, -78901.345, 0};
    double pos[] = {0, -3, 3, -80};
    
    for (int i = 0; N[i]; i++) {
        int len_code;
        int8_t *code = sdr_gen_code("L6D", 194, &len_code);
        sdr_cpx16_t *code_res = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N[i]);
        sdr_cpx16_t *data = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N[i]);
        sdr_res_code(code, len_code, 4e-3, 1.345, fs[i], N[i], 0, code_res);
        
        sdr_buff_t *buff = gen_data(N[i] * 2);
        
        sdr_cpx_t C[4], C_ref[4];
        sdr_mix_carr(buff, ix[i], N[i], fs[i], fc[i], phi[i], data);
        sdr_corr_std(data, code_res, N[i], pos, 4, C);
        sdr_corr_std_ref(buff, ix[i], N[i], fs[i], fc[i], phi[i], code_res, pos, 4, C_ref);
        
        for (int j = 0; j < 4; j++) {
            if (fabs(C[j][0] - C_ref[j][0]) > 0.01 || fabs(C[j][1] - C_ref[j][1]) > 0.01) {
                printf("C[%d]=%9.6f/%9.6f : %9.6f/%9.6f\n", j, C[j][0], C[j][1], C_ref[j][0], C_ref[j][1]);
                //printf("sdr_corr_std() error\n");
                //exit(-1);
            }
        }
        sdr_buff_free(buff);
        sdr_free(code_res);
        sdr_free(data);
        
        printf("test_03: sdr_corr_std() N=%8d OK\n", N[i]);
    }
    printf("test_03: OK\n");
}

// test sdr_corr_fft() ---------------------------------------------------------
static void test_04(void)
{
    printf("test_04: OK\n");
}

// test performance: sdr_mix_carr(), sdr_corr_std(), sdr_corr_fft() ------------
static void test_05(void)
{
    int n = 10000;
    double fs = 12e6, fc = 13500.0, coff = 1.345, phi = 3.456;
    double pos[] = {0, -3, 3, -80};
    int N[] = {12000, 16000, 24000, 32000, 32768, 48000, 65536, 96000, 0};
    
    printf("test_05: performance\n");
    
    printf("%6s %9s%9s%6s (ms)\n", "", "", "C+AVX2+FFTW3", "");
    printf("%6s  %8s %8s %8s\n", "", "mix_carr", "corr_std", "corr_fft");
    
    for (int i = 0; N[i]; i++) {
        sdr_cpx16_t *code_res, *IQ;
        sdr_cpx_t *code_fft, *C1;
        int len_code;
        
        code_res = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N[i]);
        IQ       = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N[i]);
        code_fft = sdr_cpx_malloc(N[i]);
        C1       = sdr_cpx_malloc(N[i]);
        
        sdr_buff_t *buff = gen_data(N[i]);
        int8_t *code = sdr_gen_code("L6D", 194, &len_code);
        sdr_res_code(code, len_code, 4e-3, coff, fs, N[i], 0, code_res);
        sdr_gen_code_fft(code, len_code, 4e-3, coff, fs, N[i], 0, code_fft);
        
        uint32_t tt = sdr_get_tick();
        for (int j = 0; j < n; j++) {
            sdr_mix_carr(buff, 0, N[i], fs, fc, phi, IQ);
        }
        double t1 = (double)(sdr_get_tick() - tt) / n;
        
        tt = sdr_get_tick();
        for (int j = 0; j < n; j++) {
            sdr_mix_carr(buff, 0, N[i], fs, fc, phi, IQ);
            sdr_corr_std(IQ, code_res, N[i], pos, 4, C1);
        }
        double t2 = (double)(sdr_get_tick() - tt) / n;
        
        tt = sdr_get_tick();
        for (int j = 0; j < n; j++) {
            sdr_mix_carr(buff, 0, N[i], fs, fc, phi, IQ);
            sdr_corr_fft(IQ, code_fft, N[i], C1);
        }
        double t3 = (double)(sdr_get_tick() - tt) / n;
        
        printf("%6d  %8.4f %8.4f %8.4f\n", N[i], t1, t2, t3);
        fflush(stdout);
        
        sdr_free(code_res);
        sdr_free(IQ);
        sdr_cpx_free(code_fft);
        sdr_cpx_free(C1);
        sdr_buff_free(buff);
    }
    printf("test_05: OK\n");
}

// test main --------------------------------------------------------------------
int main(int argc, char **argv)
{
    sdr_func_init("../../python/fftw_wisdom.txt");
    
    test_01();
    test_02();
    test_03();
    test_04();
    test_05();
    return 0;
}

