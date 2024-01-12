//
//  Pocket SDR C Library - Header file for GNSS SDR Functions.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-05-23  1.0  new
//  2022-07-08  1.1  modify types, add APIs
//  2022-07-16  1.2  modify API
//  2023-12-28  1.3  modify types and APIs
//  2024-01-12  1.4  modify constants, types and API
//
#ifndef POCKET_SDR_H
#define POCKET_SDR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fftw3.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// constants and macro -------------------------------------------------------
#define PI  3.1415926535897932  // pi 
#define SDR_MAX_NPRN   256      // max number of PRNs
#define SDR_MAX_NCH    999      // max number of receiver channels
#define SDR_MAX_NSYM   18000    // max number of symbols
#define SDR_MAX_DATA   4096     // max length of navigation data
#define SDR_N_HIST     10000    // number of P correlator history 

#define STATE_IDLE     1        // channel state idle
#define STATE_SRCH     2        // channel state search
#define STATE_LOCK     3        // channel state lock

// type definitions ----------------------------------------------------------
typedef fftwf_complex sdr_cpx_t; // single precision complex type 

typedef struct {                // signal acquisition type 
    sdr_cpx_t *code_fft;        // code FFT 
    float *fds;                 // Doppler bins 
    int len_fds;                // length of Doppler bins 
    float *P_sum;               // sum of correlation powers 
    int n_sum;                  // number of sum 
} sdr_acq_t;

typedef struct {                // signal tracking type 
    int *pos;                   // correlator positions 
    int npos;                   // number of correlator positions 
    sdr_cpx_t *C;               // correlations 
    sdr_cpx_t P[SDR_N_HIST];    // history of P correlations 
    int sec_sync;               // secondary code sync status 
    int sec_pol;                // secondary code polarity 
    double err_phas;            // phase error (cyc) 
    double sumP, sumE, sumL, sumN; // sum of correlations 
    float *code;                // resampled code 
    sdr_cpx_t *code_fft;        // code FFT
} sdr_trk_t;

typedef struct {                // SDR receiver navigation data type
    int ssync;                  // symbol sync time as lock count (0: no-sync)
    int fsync;                  // nav frame sync time as lock count (0: no-sync)
    int rev;                    // code polarity (0: normal, 1: reversed)
    int seq;                    // sequence number (TOW, TOI, ...)
    int nerr;                   // number of error corrected
    uint8_t syms[SDR_MAX_NSYM]; // nav symbols buffer
    double tsyms[SDR_MAX_NSYM]; // nav symbols time (for debug)
    uint8_t data[SDR_MAX_DATA]; // navigation data buffer
    double time_data;           // navigation data time
    int count[2];               // navigation data count (OK, error)
    char opt[256];              // navigation option string
} sdr_nav_t;

typedef struct {                // SDR receiver channel type 
    int no;                     // channel number
    int state;                  // channel state 
    double time;                // receiver time 
    char sig[16];               // signal ID 
    int prn;                    // PRN number 
    const int8_t *code;         // primary code 
    const int8_t *sec_code;     // secondary code
    int len_code, len_sec_code;
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
void sdr_get_time(double *t);
uint32_t sdr_get_tick(void);
void sdr_sleep_msec(int msec);

// sdr_func.c
void sdr_func_init(const char *file);
sdr_cpx_t *sdr_cpx_malloc(int N);
void sdr_cpx_free(sdr_cpx_t *cpx);
float sdr_cpx_abs(sdr_cpx_t cpx);
sdr_cpx_t *sdr_read_data(const char *file, double fs, int IQ, double T,
    double toff, int *len_data);
void sdr_search_code(const sdr_cpx_t *code_fft, double T, const sdr_cpx_t *buff,
    int len_buff, int ix, int N, double fs, double fi, const float *fds,
    int len_fds, float *P);
float sdr_corr_max(const float *P, int N, int M, int Nmax, double T, int *ix);
double sdr_fine_dop(const float *P, int N, const float *fds, int len_fds,
    const int *ix);
double sdr_shift_freq(const char *sig, int fcn, double fi);
float *sdr_dop_bins(double T, float dop, float max_dop, int *len_fds);
void sdr_corr_std(const sdr_cpx_t *buff, int len_buff, int ix, int N, double fs,
    double fc, double phi, const float *code, const int *pos, int n,
    sdr_cpx_t *corr);
void sdr_corr_fft(const sdr_cpx_t *buff, int len_buff, int ix, int N, double fs,
    double fc, double phi, const sdr_cpx_t *code_fft, sdr_cpx_t *corr);
void sdr_mix_carr(const sdr_cpx_t *buff, int len_buff, int ix, int N, double fs,
    double fc, double phi, sdr_cpx_t *data);
void sdr_corr_std_(const sdr_cpx_t *data, const float *code, int N,
    const int *pos, int n, sdr_cpx_t *corr);
void sdr_corr_fft_(const sdr_cpx_t *data, const sdr_cpx_t *code_fft, int N,
    sdr_cpx_t *corr);
int sdr_log_open(const char *path);
void sdr_log_close(void);
void sdr_log_level(int level);
void sdr_log(int level, const char *msg, ...);
int sdr_parse_nums(const char *str, int *prns);
void sdr_add_buff(void *buff, int len_buff, void *item, size_t size_item);
void sdr_pack_bits(const uint8_t *data, int nbit, int nz, uint8_t *buff);
void sdr_unpack_bits(const uint8_t *data, int nbit, uint8_t *buff);
void sdr_unpack_data(uint32_t data, int nbit, uint8_t *buff);
uint8_t sdr_xor_bits(uint32_t X);
int sdr_gen_fftw_wisdom(const char *file, int N);

// sdr_code.c
int8_t *sdr_gen_code(const char *sig, int prn, int *N);
int8_t *sdr_sec_code(const char *sig, int prn, int *N);
double sdr_code_cyc(const char *sig);
int sdr_code_len(const char *sig);
double sdr_sig_freq(const char *sig);
void sdr_res_code(const int8_t *code, int len_code, double T, double coff,
    double fs, int N, int Nz, float *code_res);
void sdr_gen_code_fft(const int8_t *code, int len_code, double T, double coff,
    double fs, int N, int Nz, sdr_cpx_t *code_fft);

// sdr_ch.c
sdr_ch_t *sdr_ch_new(const char *sig, int prn, double fs, double fi,
    double sp_corr, int add_corr, double ref_dop, double max_dop,
    const char *nav_opt);
void sdr_ch_free(sdr_ch_t *ch);
void sdr_ch_update(sdr_ch_t *ch, double time, const sdr_cpx_t *buff,
    int len_buff, int ix);

// sdr_nav.c
sdr_nav_t *sdr_nav_new(const char *nav_opt);
void sdr_nav_free(sdr_nav_t *nav);
void sdr_nav_init(sdr_nav_t *nav);
void sdr_nav_decode(sdr_ch_t *ch);

// sdr_fec.c
void sdr_decode_conv(const uint8_t *data, int N, uint8_t *dec_data);
int sdr_decode_rs(uint8_t *syms);

// sdr_ldpc.c
int sdr_decode_LDPC(const char *type, const uint8_t *syms, int N,
    uint8_t *syms_dec);

#ifdef __cplusplus
}
#endif
#endif // POCKET_SDR_H 
