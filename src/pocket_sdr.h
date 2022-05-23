//
//  Pocket SDR C Library - Header file for GNSS SDR Functions.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-05-23  1.0  new
//
#ifndef POCKET_SDR_H
#define POCKET_SDR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fftw3.h>

#ifdef __cplusplus
extern "C" {
#endif

// constants and macro -------------------------------------------------------
#define PI  3.1415926535897932  // pi 
#define SDR_MAX_NPRN   256      // max number of PRNs

// type definitions ----------------------------------------------------------
typedef fftwf_complex sdr_cpx_t; // single precision complex type 

typedef struct {                // signal acquisition type 
    float *code_fft;            // code FFT 
    float *fds;                 // Doppler bins 
    int n_fds;                  // number of Doppler bins 
    float *P_sum;               // sum of correlation powers 
    int n_sum;                  // number of sum 
} sdr_acq_t;

typedef struct {                // signal tracking type 
    int *pos;                   // correlator positions 
    int npos;                   // number of correlator positions 
    sdr_cpx_t *C;               // correlations 
    sdr_cpx_t *P;               // history of P correlations 
    int sec_sync;               // secondary code sync status 
    int sec_pol;                // secondary code polarity 
    float err_phas;             // phase error (cyc) 
    sdr_cpx_t sumP, sumE, sumL, sumN; // sum of correlations 
    float *code;                // resampled code 
} sdr_trk_t;

typedef struct {                // SDR receiver navigation data type
    int ssync;                  // symbol sync time as lock count (0: no-sync)
    int fsync;                  // nav frame sync time as lock count (0: no-sync)
    int rev;                    // code polarity (0: normal, 1: reversed)
    int seq;                    // sequence number (TOW, TOI, ...)
    int nerr;                   // number of error corrected
    uint8_t *syms;              // nav symbols buffer
    double *tsyms;              // nav symbols time (for debug)
    uint8_t *data;              // navigation data buffer
    int count[2];               // navigation data count (OK, error)
} sdr_nav_t;

typedef struct {                // SDR receiver channel type 
    const char *state;          // channel state 
    double time;                // receiver time 
    char sig[16];               // signal ID 
    int prn;                    // PRN number 
    const int8_t *code;         // primary code 
    const int8_t *sec_code;     // secondary code 
    double fc;                  // carrier frequency (Hz) 
    double fs;                  // sampling rate (sps) 
    double fi;                  // IF freqency (Hz) 
    double T;                   // code cycle (s) 
    int N;                      // code cycle (samples) 
    double fd;                  // Doppler frequency (Hz) 
    double coff;                // code offset (s) 
    double adr;                 // accumurated Doppler range (cyc) 
    double cn0;                 // C/N0 (dB-Hz) 
    int lock, lost;             // lock and lost counts 
    int costas;                 // Costas PLL flag 
    sdr_acq_t *acq;             // signal acquisition 
    sdr_trk_t *trk;             // signal tracking 
    sdr_nav_t *nav;             // navigation decoder 
} sdr_ch_t;

// function prototypes -------------------------------------------------------

// sdr_cmn.c
void *sdr_malloc(size_t size);
void sdr_free(void *p);
sdr_cpx_t *sdr_cpx_malloc(int N);
void sdr_cpx_free(sdr_cpx_t *cpx);
uint32_t sdr_get_tick(void);
void sdr_sleep_msec(int msec);

// sdr_func.c
void sdr_init_lib(const char *file);
void sdr_mix_carr(const sdr_cpx_t *buff, int ix, int N, double fs, double fc,
    double phi, sdr_cpx_t *data);
void sdr_corr_std(const sdr_cpx_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx_t *code, const int *pos, int n, sdr_cpx_t *corr);
void sdr_corr_fft(const sdr_cpx_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx_t *code_fft, sdr_cpx_t *corr);
int sdr_gen_fftw_wisdom(const char *file, int N);
sdr_cpx_t *sdr_read_data(const char *file, double fs, int IQ, double T,
    double toff, int *len_data);
int sdr_parse_nums(const char *str, int *prns);
void sdr_search_code(const sdr_cpx_t *code_fft, double T, const sdr_cpx_t *buff,
    int ix, int N, double fs, double fi, const float *fds, int len_fds,
    float *P);
float sdr_corr_max(const float *P, int N, int M, int Nmax, double T, int *ix);
float sdr_fine_dop(const float *P, int N, const float *fds, int len_fds,
    const int *ix);
double sdr_shift_freq(const char *sig, int fcn, double fi);
float *sdr_dop_bins(double T, float dop, float max_dop, int *len_fds);
void sdr_add_buff(void *buff, int len_buff, void *item, size_t size_item);
uint8_t sdr_xor_bits(uint32_t X);

// sdr_code.c
int8_t *sdr_gen_code(const char *sig, int prn, int *N);
int8_t *sdr_sec_code(const char *sig, int prn, int *N);
double sdr_code_cyc(const char *sig);
int sdr_code_len(const char *sig);
double sdr_sig_freq(const char *sig);
void sdr_res_code(const int8_t *code, int len_code, double T, double coff,
    double fs, int N, int Nz, sdr_cpx_t *code_res);
void sdr_gen_code_fft(const int8_t *code, int len_code, double T, double coff,
    double fs, int N, int Nz, sdr_cpx_t *code_fft);

#ifdef __cplusplus
}
#endif
#endif // POCKET_SDR_H 
