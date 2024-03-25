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
//  2024-01-16  1.5  update constants, types and API
//  2024-02-08  1.6  update constants, types and API
//  2024-03-20  1.7  update macros, types and APIs
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

// constants and macros ------------------------------------------------------
#define PI  3.1415926535897932  // pi 
#define SDR_MAX_NPRN   256      // max number of PRNs
#define SDR_MAX_NCH    999      // max number of receiver channels
#define SDR_MAX_BUFF   4        // max number of IF data buffer
#define SDR_MAX_NSYM   2000     // max number of symbols
#define SDR_MAX_DATA   4096     // max length of navigation data
#define SDR_N_HIST     1800     // number of P correlator history 

#define SDR_DEV_FILE   1        // SDR device: file
#define SDR_DEV_USB    2        // SDR device: USB device

#define SDR_FMT_INT8   0        // IF data format: int8
#define SDR_FMT_RAW8   1        // IF data format: packed 8 bit raw
#define SDR_FMT_RAW16  2        // IF data format: packed 16 bit raw

#define STATE_IDLE     1        // channel state idle
#define STATE_SRCH     2        // channel state search
#define STATE_LOCK     3        // channel state lock

#define SDR_CPX8(re, im) (sdr_cpx8_t)(((int8_t)(im)<<4)|(((int8_t)((re)<<4)>>4)&0xF))
#define SDR_CPX8_I(x)  ((int8_t)((x)<<4)>>4)
#define SDR_CPX8_Q(x)  ((int8_t)((x)<<0)>>4)

// type definitions ----------------------------------------------------------
typedef uint8_t sdr_cpx8_t;      // 8(4+4) bits complex type 
typedef struct {int8_t I, Q;} sdr_cpx16_t; // 16(8+8) bits complex type 
typedef fftwf_complex sdr_cpx_t; // single precision complex type 

typedef struct {                // signal acquisition type 
    sdr_cpx_t *code_fft;        // code FFT 
    float *fds;                 // Doppler bins 
    int len_fds;                // length of Doppler bins 
    float fd_ext;               // Doppler external assist
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
    double err_code;            // code error (chip) 
    double sumP, sumE, sumL, sumN; // sum of correlations 
    sdr_cpx16_t *code;          // resampled code 
    sdr_cpx_t *code_fft;        // code FFT
} sdr_trk_t;

typedef struct {                // SDR receiver navigation data type
    int ssync;                  // symbol sync time as lock count (0: no-sync)
    int fsync;                  // nav frame sync time as lock count (0: no-sync)
    int rev;                    // code polarity (0: normal, 1: reversed)
    int seq, mt;                // sequence number and subframe ID/message type
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
    char sat[16];               // satellite ID 
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
    double tow;                 // time of week (s) (<0: not resolved)
    int lock, lost;             // lock and lost counts 
    int costas;                 // Costas PLL flag 
    sdr_acq_t *acq;             // signal acquisition 
    sdr_trk_t *trk;             // signal tracking 
    sdr_nav_t *nav;             // navigation decoder
} sdr_ch_t;

typedef struct {                // IF data buffer type
    sdr_cpx8_t *data;           // IF data
    int IQ, N;                  // sampling types (1:I,2:IQ) and buffer size
} sdr_buff_t;

struct sdr_rcv_tag;

typedef struct {                // SDR receiver channel thread type
    int state;                  // state (0:stop,1:run)
    sdr_ch_t *ch;               // SDR receiver channel
    int if_ch;                  // IF data buffer channel
    int64_t ix;                 // IF data buffer read pointer (cyc)
    struct sdr_rcv_tag *rcv;    // pointer to SDR receiver
    pthread_t thread;           // SDR receiver channel thread
} sdr_ch_th_t;

typedef struct sdr_rcv_tag {    // SDR receiver type
    int state;                  // state (0:stop,1:run)
    int dev;                    // SDR device type (SDR_DEV_???)
    void *dp;                   // SDR device pointer
    int nch;                    // number of receiver channels
    int ich;                    // signal search channel index
    sdr_ch_th_t *th[SDR_MAX_NCH]; // SDR receiver channel threads
    int64_t ix;                 // IF data buffer write pointer (cyc)
    sdr_buff_t *buff[SDR_MAX_BUFF]; // IF data buffers
    int N;                      // IF data write cycle (samples)
    int fmt;                    // IF data format (SDR_FMT_???)
    double tint[3];             // time intervals
    uint8_t *raw;               // raw data buffer
    pthread_t thread;           // SDR receiver thread
} sdr_rcv_t;

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
void sdr_cpx_mul(const sdr_cpx_t *a, const sdr_cpx_t *b, int N, float s,
    sdr_cpx_t *c);
sdr_buff_t *sdr_buff_new(int N, int IQ);
void sdr_buff_free(sdr_buff_t *buff);
sdr_buff_t *sdr_read_data(const char *file, double fs, int IQ, double T,
    double toff);
void sdr_search_code(const sdr_cpx_t *code_fft, double T,
    const sdr_buff_t *buff, int ix, int N, double fs, double fi,
    const float *fds, int len_fds, float *P);
float sdr_corr_max(const float *P, int N, int M, int Nmax, double T, int *ix);
double sdr_fine_dop(const float *P, int N, const float *fds, int len_fds,
    const int *ix);
double sdr_shift_freq(const char *sig, int fcn, double fi);
float *sdr_dop_bins(double T, float dop, float max_dop, int *len_fds);
void sdr_corr_std(const sdr_buff_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx16_t *code, const int *pos, int n,
    sdr_cpx_t *corr);
void sdr_corr_fft(const sdr_buff_t *buff, int ix, int N, double fs, double fc,
    double phi, const sdr_cpx_t *code_fft, sdr_cpx_t *corr);
void sdr_mix_carr(const sdr_buff_t *buff, int ix, int N, double fs, double fc,
    double phi, sdr_cpx16_t *IQ);
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
void sdr_sat_id(const char *sig, int prn, char *sat);
void sdr_res_code(const int8_t *code, int len_code, double T, double coff,
    double fs, int N, int Nz, sdr_cpx16_t *code_res);
void sdr_gen_code_fft(const int8_t *code, int len_code, double T, double coff,
    double fs, int N, int Nz, sdr_cpx_t *code_fft);

// sdr_ch.c
sdr_ch_t *sdr_ch_new(const char *sig, int prn, double fs, double fi,
    double sp_corr, int add_corr, double ref_dop, double max_dop,
    const char *nav_opt);
void sdr_ch_free(sdr_ch_t *ch);
void sdr_ch_update(sdr_ch_t *ch, double time, const sdr_buff_t *buff, int ix);

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

// sdr_nb_ldpc.c
int sdr_decode_NB_LDPC(const uint8_t H_idx[][4], const uint8_t H_ele[][4],
    int m, int n, const uint8_t *syms, uint8_t *syms_dec);

// sdr_rcv.c
sdr_rcv_t *sdr_rcv_new(int dev, void *dp, char **sigs, const int *prns,
    const double *fi, int n, double fs, const double *dop, int fmt,
    const int *IQ);
void sdr_rcv_free(sdr_rcv_t *rcv);
int sdr_rcv_start(sdr_rcv_t *rcv, const double *tint);
void sdr_rcv_stop(sdr_rcv_t *rcv);

#ifdef __cplusplus
}
#endif
#endif // POCKET_SDR_H 
