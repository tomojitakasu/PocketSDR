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
//  2024-04-04  1.8  update constants, types and API
//  2024-04-28  1.9  update constants, types and API
//  2024-05-28  1.10 update constants, types and APIs
//  2024-07-01  1.11 import pocket_dev.h
//  2024-08-26  1.12 update types and APIs
//  2026-05-10  1.13 ver.0.15
//
#ifndef POCKET_SDR_H
#define POCKET_SDR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "rtklib.h"
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#endif // WIN32

#ifdef __cplusplus
extern "C" {
#endif

// constants and macros ------------------------------------------------------
#define SDR_LIB_NAME   "Pocket SDR" // library name
#define SDR_LIB_VER    "0.15"   // library version
#define SDR_MAX_RFCH   8        // max number of RF channels
#define SDR_MAX_ARCH   8        // max number of array channels
#define SDR_MAX_BUFF   (SDR_MAX_RFCH+SDR_MAX_ARCH) // max number of IF buffer
#define SDR_MAX_REG    11       // max number of registers in a SDR device
#define SDR_MAX_UBUFF  6        // number of USB buffer
#define SDR_SIZE_UBUFF (1<<20)  // size of USB buffer (bytes)
#define SDR_MAX_NPRN   256      // max number of PRNs
#define SDR_MAX_NCH    1500     // max number of receiver channels
#define SDR_MAX_NSYM   2000     // max number of symbols
#define SDR_MAX_DATA   4096     // max length of navigation data
#define SDR_N_CORRX    101      // number of additional correlators
#define SDR_MAX_CORR   (6+SDR_N_CORRX) // max number of correlators
#define SDR_N_HIST     5000     // number of P correlator history
#define SDR_N_CODES    10       // number of resampled code bank
#define SDR_CSCALE    (1/11.2f) // carrier scale (max(IQ)*sqrt(2)/scale<=127)
#define SDR_CYC        1e-3     // IF data processing cycle (s)
#define PI 3.1415926535897932   // pi

#define SDR_DEV_FILE   1        // SDR device: file
#define SDR_DEV_USB    2        // SDR device: Pocket SDR FE
#define SDR_DEV_STR    3        // SDR device: stream
#define SDR_DEV_SOAPY  4        // SDR device: SoapySDR

#define SDR_DEV_NAME   "Pocket SDR" // SDR device name 
#define SDR_DEV_VID    0x04B4   // SDR USB device vendor ID 
#define SDR_DEV_PID1   0x1004   // SDR USB device product ID (EZ-USB FX2LP)
#define SDR_DEV_PID2   0x00F1   // SDR USB device product ID (EZ-USB FX3)
#define SDR_DEV_IF     0        // SDR USB device interface number 
#define SDR_DEV_EP     0x86     // SDR USB device end point for bulk transter 

#define SDR_VR_STAT    0x40     // SDR USB vendor request: Get status
#define SDR_VR_REG_READ 0x41    // SDR USB vendor request: Read register
#define SDR_VR_REG_WRITE 0x42   // SDR USB vendor request: Write register
#define SDR_VR_START   0x44     // SDR USB vendor request: Start bulk transfer
#define SDR_VR_STOP    0x45     // SDR USB vendor request: Stop bulk transfer
#define SDR_VR_RESET   0x46     // SDR USB vendor request: Reset device
#define SDR_VR_SAVE    0x47     // SDR USB vendor request: Save settings

#define SDR_FMT_INT8   1        // SDR IF data format: int8 (I)
#define SDR_FMT_INT8X2 2        // SDR IF data format: int8 x 2 complex (IQ)
#define SDR_FMT_RAW8   3        // SDR IF data format: packed 8 bits raw  (2CH)
#define SDR_FMT_RAW16  4        // SDR IF data format: packed 16 bits raw (4CH)
#define SDR_FMT_RAW16I 5        // SDR IF data format: packed 16 bits raw (8CH)
#define SDR_FMT_RAW32  6        // SDR IF data format: packed 32 bits raw (8CH)
#define SDR_FMT_CS8    7        // SDR IF data format: int8 x 2 complex (IQ)
#define SDR_FMT_CS16   8        // SDR IF data format: int16 x 2 complex (IQ)

#define SDR_STATE_IDLE 1        // SDR channel state: idle
#define SDR_STATE_SRCH 2        // SDR channel state: search
#define SDR_STATE_LOCK 3        // SDR channel state: lock

#define SDR_FFT_FORWARD 0       // SDR FFT direction forward
#define SDR_FFT_BACKWARD 1      // SDR FFT direction backward

#define SDR_CALIB_BOTH 0        // calib mode: estimate rpy + bias
#define SDR_CALIB_BIAS 1        // calib mode: estimate bias only (rpy=0 fixed)
#define SDR_CALIB_RPY  2        // calib mode: estimate rpy only (bias fixed)

#define SDR_CPX8(re, im) (sdr_cpx8_t)(((int8_t)(im)<<4)|(((int8_t)((re)<<4)>>4)&0xF))
#define SDR_CPX8_I(x)  ((int8_t)((x)<<4)>>4)
#define SDR_CPX8_Q(x)  ((int8_t)((x)<<0)>>4)

// type definitions ----------------------------------------------------------
typedef uint8_t sdr_cpx8_t;      // 8(4+4) bits complex type
typedef struct {int8_t  I, Q;} sdr_cpx16_t; // 16(8+8)   bits complex type
typedef struct {int32_t I, Q;} sdr_cpx64_t; // 64(32+32) bits complex type
typedef float sdr_cpx_t[2];      // single precision complex type
typedef struct sdr_lpf_tag sdr_lpf_t; // LPF type

#ifdef WIN32
typedef HANDLE sdr_thread_t;     // thread type
typedef SRWLOCK sdr_mutex_t;     // mutex type
#define SDR_MUTEX_INIT SRWLOCK_INIT // mutex initializer
#else
typedef pthread_t sdr_thread_t;
typedef pthread_mutex_t sdr_mutex_t;
#define SDR_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#endif

typedef struct {                // USB device type
#ifdef WIN32
    void *h;                    // USB device handle
#else
    libusb_context *ctx;        // USB context
    libusb_device_handle *h;    // USB device handle
    struct libusb_transfer *transfer[SDR_MAX_UBUFF]; // USB transfers
#endif
} sdr_usb_t;

typedef struct {                // Pocket SDR FE device type
    sdr_usb_t *usb;             // USB device
    int state;                  // state of USB event handler
    int64_t rp, wp;             // read/write pointer of raw data buffer
    uint8_t *buff;              // raw data buffer
    sdr_thread_t thread;        // USB event handler thread
    sdr_mutex_t mtx;            // lock flag
} sdr_dev_t;

typedef struct {                // SoapySDR device type
    void *dev;                  // SoapySDR device handle
    void *str;                  // SoapySDR device stream
    char driver[32];            // SoapySDR driver
    uint8_t *buff;              // sample buffer
    int state;                  // state of sampling thread
    int64_t rp, wp;             // read/write pointer of sampling data buffer
    int ssize;                  // sample size (bytes/sample)
    double rate;                // sample rate (sps)
    sdr_thread_t thread;        // reading sampling thread
    sdr_mutex_t mtx;            // lock flag
} sdr_sdev_t;

typedef struct {                // signal acquisition type 
    sdr_cpx_t *code_fft;        // code FFT 
    float *fds;                 // Doppler bins 
    int len_fds;                // length of Doppler bins 
    float fd_ext;               // Doppler external assist
    float *P_sum;               // sum of correlation powers 
    int n_sum;                  // number of sum 
} sdr_acq_t;

typedef struct {                // signal tracking type 
    int npos, nposx;            // number of correlator positions
    double pos[SDR_MAX_CORR];   // correlator positions (samples)
    sdr_cpx_t C[SDR_MAX_CORR];  // correlations 
    sdr_cpx_t C0, C1;           // correlation buffers
    sdr_cpx_t P[SDR_N_HIST];    // history of P correlations 
    int sec_sync;               // secondary code sync status 
    int sec_pol;                // secondary code polarity 
    double err_phas;            // phase error (cyc)
    double err_code;            // code error (chip)
    double phas_acc;            // 3rd-order PLL acceleration accumulator (Hz/s)
    double code_int;            // 2nd-order DLL integrator (s/s)
    double sumP, sumN, sumVE, sumVL; // sum of correlations
    double sumC[SDR_MAX_CORR];  // sum of correlations DLL
    double sumI[SDR_MAX_CORR];  // sum of I*sign(IP) DLL (data-wipe-off)
    double aveP[SDR_MAX_CORR];  // average of correlation powers
    double aveI[SDR_MAX_CORR];  // average of I*sign(IP) DLL
    sdr_cpx16_t *code;          // resampled code 
    sdr_cpx_t *code_fft;        // code FFT
} sdr_trk_t;

typedef struct {                // SDR receiver navigation data type
    int ssync;                  // symbol sync time as lock count (0: no-sync)
    int fsync;                  // nav frame sync time as lock count (0: no-sync)
    int rev;                    // code polarity (0: normal, 1: reversed)
    int nerr;                   // number of error corrected
    int seq, type, stat;        // sequence number, type, update status
    double coff;                // code offset for L6D/E CSK
    uint8_t syms[SDR_MAX_NSYM]; // nav symbols buffer
    uint8_t data[SDR_MAX_DATA]; // navigation data buffer
    int lock_sf[16];            // lock time of subframes
    int count[2];               // navigation data count (OK, error)
} sdr_nav_t;

typedef struct {                // SDR receiver channel type 
    int no;                     // channel number
    int rf_ch;                  // RF channel
    int sig_srch;               // signal search flag
    int state;                  // channel state 
    double time;                // receiver time 
    char sat[16];               // satellite ID 
    char sig[16];               // signal ID 
    int prn;                    // PRN number 
    const int8_t *code;         // primary code 
    const int8_t *sec_code;     // secondary code
    const int8_t *sec_code2;    // secondary code 2
    int len_code, len_sec_code, len_sec_code2;
    double fc;                  // carrier frequency (Hz) 
    double fs;                  // sampling rate (sps) 
    double fi;                  // IF frequency (Hz) 
    double T;                   // code cycle (s) 
    int N;                      // code cycle (samples) 
    double fd;                  // Doppler frequency (Hz) 
    double coff;                // code offset (s) 
    double adr;                 // accumulated Doppler range (cyc) 
    double cn0;                 // C/N0 (dB-Hz) 
    int week, tow;              // week number (week), TOW (ms)
    int tow_v;                  // TOW flag (0:invalid,1:valid,2:amb-unresolved)
    int lock, lost;             // lock and lost counts 
    int costas;                 // Costas PLL flag 
    int obs_idx;                // observation data index
    sdr_acq_t *acq;             // signal acquisition 
    sdr_trk_t *trk;             // signal tracking 
    sdr_nav_t *nav;             // navigation decoder
    sdr_cpx16_t *data;          // data buffer
    sdr_cpx_t *corr;            // correlation buffer
    sdr_mutex_t mtx;            // lock flag
} sdr_ch_t;

typedef struct {                // IF data buffer type
    sdr_cpx8_t *data;           // IF data
    int IQ, N;                  // sampling types (1:I,2:IQ) and buffer size
} sdr_buff_t;

typedef struct {                // SDR IF data statistics
    double std;                 // IF data std-dev
    double rate, sum;           // IF data rate (MB/s) and total size (MB)
    double buff_use;            // IF data buffer usage (%)
    double sum_sq[2];           // accumulators {sum, sum-of-squares}
    double sum_iq[2];           // CS16 accumulators {sum I, sum Q}
    double sumsq_iq[2];         // CS16 accumulators {sum-of-squares I, Q}
    int cnt;                    // sample count for stats
} sdr_stats_t;

struct sdr_rcv_tag;

typedef struct {                // SDR receiver channel thread type
    int state;                  // state (0:stop,1:run)
    sdr_ch_t *ch;               // SDR receiver channel
    int64_t ix;                 // IF data buffer read pointer (cyc)
    struct sdr_rcv_tag *rcv;    // pointer to SDR receiver
    sdr_thread_t thread;        // SDR receiver channel thread
} sdr_ch_th_t;

typedef struct {                // SDR PVT type
    gtime_t time;               // epoch time
    int64_t ix;                 // epoch cycle (cyc)
    int nsat, nch;              // number of satellites and updated channels
    obs_t *obs;                 // observation data
    nav_t *nav;                 // navigation data
    sol_t *sol;                 // PVT solution
    ssat_t *ssat;               // satellite status
    rtcm_t *rtcm;               // RTCM control
    double latency;             // solution latency (s)
    int count[3];               // solution, OBS and NAV count
    struct sdr_rcv_tag *rcv;    // pointer to SDR receiver
    sdr_mutex_t mtx;            // lock flag
} sdr_pvt_t;

typedef struct {                // SDR RF channel type
    double fo;                  // LO frequencies (Hz)
    int IQ, bits;               // IF sampling types (I:1,I/Q:2), bits
    sdr_lpf_t *lpf;             // LPF state (NULL = disabled)
    void *LUT;                  // raw to IF data LUT (256 or 4*256 entries)
} sdr_rfch_t;

typedef struct {                // SDR array channel type
    double az, el;              // beam azimuth and elevation (rad)
    double scale;               // beam-forming scale (<=0: disabled)
    int16_t w[SDR_MAX_RFCH*2];  // beam-forming weights in Q8 [re_0,im_0,...]
    sdr_cpx64_t LUT[SDR_MAX_RFCH][256]; // LUT: cpx8 -> cpx64
} sdr_arch_t;

typedef struct {                // SDR antenna array type
    int calib_run;              // calibration state
    int calib_mode;             // calibration mode (SDR_CALIB_???)
    int freq;                   // frequency index (0:L1,1:L2,...)
    int nrfch;                  // number of RF channels
    int ant_ena[SDR_MAX_RFCH];  // element enable flag
    double ant_pos[SDR_MAX_RFCH][3]; // element positions in body-frame (m)
    double x[3+SDR_MAX_RFCH];   // EKF state {roll,pitch,yaw,bias_1..bias_n-1}
    double P[(3+SDR_MAX_RFCH)*(3+SDR_MAX_RFCH)]; // EKF covariance
    int nep;                    // calibration epoch count
    double rms;                 // calibration RMS (m)
} sdr_array_t;

typedef struct sdr_rcv_tag {    // SDR receiver type
    int state;                  // state (0:stop,1:run)
    int dev;                    // SDR device type (SDR_DEV_???)
    void *dp;                   // SDR device pointer
    int fmt;                    // IF data format (SDR_FMT_???)
    double fs;                  // IF data sampling rate (sps)
    int N;                      // IF data cycle (sample)
    int nch, nrfch;             // number of BB and RF channels
    int narch;                  // number of array channels
    int ich;                    // signal search channel index
    int64_t ix;                 // IF data cycle count (cyc)
    sdr_rfch_t rfch[SDR_MAX_RFCH]; // RF channels (array CHs derive from rfch[0])
    sdr_arch_t arch[SDR_MAX_ARCH]; // per-array-CH beam state (relative index)
    sdr_buff_t *buff[SDR_MAX_BUFF]; // IF data buffers (RF + array)
    sdr_ch_th_t *th[SDR_MAX_NCH]; // SDR receiver channel threads
    sdr_array_t *array;         // antenna array state (NULL if narch == 0)
    sdr_pvt_t *pvt;             // SDR PVT
    sdr_stats_t stats;          // IF data statistics
    stream_t *strs[4];          // NMEA, RTCM3 and IF data log streams
    gtime_t start_time;         // receiver start time (UTC)
    double tscale;              // time scale to replay IF data file
    char opt[1024];             // receiver options
    sdr_thread_t thread;        // SDR receiver thread
    sdr_mutex_t mtx;            // lock flag
} sdr_rcv_t;

// function prototypes -------------------------------------------------------

// sdr_cmn.c
void *sdr_malloc(size_t size);
void sdr_free(void *p);
void sdr_get_time(double *t);
uint32_t sdr_get_tick(void);
void sdr_sleep_msec(int msec);
const char *sdr_get_name(void);
const char *sdr_get_ver(void);
int sdr_thread_create(sdr_thread_t *thread, void *(func)(void *), void *arg);
void sdr_thread_join(sdr_thread_t thread);
void sdr_mutex_init(sdr_mutex_t *mtx);
void sdr_mutex_lock(sdr_mutex_t *mtx);
void sdr_mutex_unlock(sdr_mutex_t *mtx);

// sdr_usb.c
sdr_usb_t *sdr_usb_open(int bus, int port, const uint16_t *vid,
    const uint16_t *pid, int n);
void sdr_usb_close(sdr_usb_t *usb);
int sdr_usb_req(sdr_usb_t *usb, int mode, uint8_t req, uint16_t val,
    uint8_t *data, int size);

// sdr_dev.c
sdr_dev_t *sdr_dev_open(int bus, int port);
void sdr_dev_close(sdr_dev_t *dev);
int sdr_dev_start(sdr_dev_t *dev);
int sdr_dev_stop(sdr_dev_t *dev);
int sdr_dev_read(sdr_dev_t *dev, uint8_t *buff, int size);
int sdr_dev_get_info(sdr_dev_t *dev, int *fmt, double *fs, double *fo, int *IQ,
    int *bits);
int sdr_dev_get_gain(sdr_dev_t *dev, int ch);
int sdr_dev_set_gain(sdr_dev_t *dev, int ch, int gain);
int sdr_dev_get_filt(sdr_dev_t *dev, int ch, double *bw, double *freq,
    int *order);
int sdr_dev_set_filt(sdr_dev_t *dev, int ch, double bw, double freq, int order);

// sdr_sdev.c
int sdr_sdev_list(void);
sdr_sdev_t *sdr_sdev_open(const char *driver, int fmt, double rate, double freq,
    double bw, double gain);
void sdr_sdev_close(sdr_sdev_t *sdev);
int sdr_sdev_start(sdr_sdev_t *sdev);
int sdr_sdev_stop(sdr_sdev_t *sdev);
int sdr_sdev_read(sdr_sdev_t *sdev, uint8_t *buff, int size);

// sdr_conf.c
int sdr_conf_read(sdr_dev_t *dev, const char *file, int opt);
int sdr_conf_write(sdr_dev_t *dev, const char *file, int opt);

// sdr_func.c
void sdr_func_init(const char *file);
sdr_cpx_t *sdr_cpx_malloc(int N);
void sdr_cpx_free(sdr_cpx_t *cpx);
float sdr_cpx_abs(sdr_cpx_t cpx);
void sdr_cpx_mul(const sdr_cpx_t *a, const sdr_cpx_t *b, int N, float s,
    sdr_cpx_t *c);
int sdr_cpx_fft(sdr_cpx_t *cpx1, int N, int dir, sdr_cpx_t *cpx2);
sdr_buff_t *sdr_buff_new(int N, int IQ);
void sdr_buff_free(sdr_buff_t *buff);
sdr_buff_t *sdr_read_data(const char *file, double fs, int IQ, double T,
    double toff);
int sdr_tag_write(const char *file, const char *prog, gtime_t time, int fmt,
    double fs, const double *fo, const int *IQ, const int *bits);
int sdr_tag_read(const char *file, char *prog, gtime_t *time, int *fmt,
    double *fs, double *fo, int *IQ, int *bits);
void sdr_search_code(const sdr_cpx_t *code_fft, double T,
    const sdr_buff_t *buff, int ix, int N, double fs, double fi,
    const float *fds, int len_fds, float *P);
float sdr_corr_max(const float *P, int N, int Nmax, int M, double T, int *ix);
double sdr_fine_dop(const float *P, int N, const float *fds, int len_fds,
    const int *ix);
double sdr_shift_freq(const char *sig, int fcn, double fi);
float *sdr_dop_bins(double T, float dop, float max_dop, int *len_fds);
void sdr_corr_std(const sdr_cpx16_t *IQ, const sdr_cpx16_t *code, int N,
    double coff, const double *pos, int n, sdr_cpx_t *corr, sdr_cpx_t *C);
void sdr_corr_std_cpx_code(const sdr_cpx16_t *IQ, const sdr_cpx16_t *code,
    int N, double coff, const double *pos, int n, sdr_cpx_t *corr,
    sdr_cpx_t *C);
void sdr_corr_std_cpx(const sdr_cpx_t *buff, int len_buff, int ix, int N,
    double fs, double fc, double phi, const float *code, const double *pos,
    int n, sdr_cpx_t *corr);
void sdr_corr_fft(const sdr_cpx16_t *IQ, const sdr_cpx_t *code_fft, int N,
    sdr_cpx_t *corr);
void sdr_corr_fft_cpx(const sdr_cpx_t *buff, int len_buff, int ix, int N,
    double fs, double fc, double phi, const sdr_cpx_t *code_fft,
    sdr_cpx_t *corr);
void sdr_mix_carr(const sdr_buff_t *buff, int ix, int N, double fs, double fc,
    double phi, sdr_cpx16_t *IQ);
void sdr_psd_cpx(const sdr_cpx_t *buff, int len_buff, int N, double fs, int IQ,
    float *psd);
stream_t *sdr_str_open(const char *path);
void sdr_str_close(stream_t *str);
int sdr_str_write(stream_t *str, uint8_t *data, int size);
int sdr_log_open(const char *path);
void sdr_log_close(void);
void sdr_log_level(int level);
void sdr_log_mask(const int *mask, int n);
void sdr_log(int level, const char *msg, ...);
int sdr_log_stat(void);
int sdr_get_log(char *buff, int size);
int sdr_parse_nums(const char *str, int *prns);
void sdr_add_buff(void *buff, int len_buff, void *item, size_t size_item);
void sdr_pack_bits(const uint8_t *data, int nbit, int nz, uint8_t *buff);
void sdr_unpack_bits(const uint8_t *data, int nbit, uint8_t *buff);
void sdr_unpack_data(uint32_t data, int nbit, uint8_t *buff);
uint8_t sdr_xor_bits(uint32_t X);
int sdr_gen_fftw_wisdom(const char *file, int N);
sdr_lpf_t *sdr_lpf_new(double fc, double fs);
void sdr_lpf_free(sdr_lpf_t *lpf);
void sdr_lpf_apply(sdr_lpf_t *lpf, sdr_cpx8_t *data, int N);

// sdr_code.c
int8_t *sdr_gen_code(const char *sig, int prn, int *N);
int8_t *sdr_sec_code(const char *sig, int prn, int *N);
double sdr_code_cyc(const char *sig);
int sdr_code_len(const char *sig);
double sdr_sig_freq(const char *sig);
void sdr_sat_id(const char *sig, int prn, char *sat);
int sdr_sig_boc(const char *sig);
// Pass code_Q = NULL for real (BPSK) codes; non-NULL for complex codes.
void sdr_res_code(const int8_t *code_I, const int8_t *code_Q, int len_code,
    double T, double coff, double fs, int N, int Nz, sdr_cpx16_t *code_res);
void sdr_gen_code_fft(const int8_t *code_I, const int8_t *code_Q, int len_code,
    double T, double coff, double fs, int N, int Nz, sdr_cpx_t *code_fft);

// sdr_ch.c
sdr_ch_t *sdr_ch_new(const char *sig, int prn, double fs, double fi);
void sdr_ch_free(sdr_ch_t *ch);
void sdr_ch_update(sdr_ch_t *ch, double time, const sdr_buff_t *buff, int ix);
void sdr_ch_set_corr(sdr_ch_t *ch, int nposx, double width);
int sdr_ch_corr_stat(sdr_ch_t *ch, double *stat, double *pos, sdr_cpx_t *C,
    double *P, double *I);
int sdr_ch_corr_hist(sdr_ch_t *ch, double tspan, double *stat, sdr_cpx_t *P);

// sdr_nav.c
sdr_nav_t *sdr_nav_new(void);
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

// sdr_pvt.c
sdr_pvt_t *sdr_pvt_new(sdr_rcv_t *rcv);
void sdr_pvt_free(sdr_pvt_t *pvt);
void sdr_pvt_udobs(sdr_pvt_t *pvt, int64_t ix, sdr_ch_t *ch);
void sdr_pvt_udnav(sdr_pvt_t *pvt, sdr_ch_t *ch);
void sdr_pvt_udsol(sdr_pvt_t *pvt, int64_t ix);
void sdr_pvt_solstr(sdr_pvt_t *pvt, char *buff, int size);

// sdr_array.c
sdr_array_t *sdr_array_new(int nrfch, int freq);
void sdr_array_free(sdr_array_t *array);
int sdr_array_ant_pos(sdr_array_t *array, const double *ant_pos,
    const int *ant_ena);
int sdr_array_run(sdr_array_t *array, int run);
int sdr_array_set_mode(sdr_array_t *array, int mode);
int sdr_array_set(sdr_array_t *array, const double *rpy, const double *bias);
int sdr_array_stat(sdr_array_t *array, double *rpy, double *bias, double *rms,
    int *nep);
void sdr_array_calib(sdr_array_t *array, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr);
int sdr_array_save(sdr_array_t *array, const char *file);
int sdr_array_load(sdr_array_t *array, const char *file);
int sdr_array_geom_save(const char *file, const double *ant_pos, int n);
int sdr_array_geom_load(const char *file, double *ant_pos, int max_ant);
void sdr_arch_free(sdr_arch_t *arch);
void sdr_arch_set_beam(sdr_arch_t *arch, const sdr_rcv_t *rcv, double az,
    double el, double scale);
void sdr_arch_get_beam(const sdr_arch_t *arch, double *az, double *el);
void sdr_arch_combine(const sdr_arch_t *arch, const sdr_rcv_t *rcv, int base);

// receiver array wrappers
int sdr_rcv_array_ant_pos(sdr_rcv_t *rcv, const double *ant_pos,
    const int *ant_ena);
int sdr_rcv_array_run(sdr_rcv_t *rcv, int run);
int sdr_rcv_array_set_mode(sdr_rcv_t *rcv, int mode);
int sdr_rcv_array_stat(sdr_rcv_t *rcv, double *rpy, double *bias, double *rms,
    int *nep);
int sdr_rcv_array_set_beam(sdr_rcv_t *rcv, int ach, double az, double el);
int sdr_rcv_array_get_beam(sdr_rcv_t *rcv, int ach, double *az, double *el);
int sdr_rcv_array_save(sdr_rcv_t *rcv, const char *file);
int sdr_rcv_array_load(sdr_rcv_t *rcv, const char *file);
void sdr_rcv_array_calib(sdr_rcv_t *rcv, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr);

// sdr_rcv.c
sdr_rcv_t *sdr_rcv_new(const char **sigs, const int *prns, int n, int fmt,
    double fs, const double *fo, const int *IQ, const int *bits,
    const char *opt);
void sdr_rcv_free(sdr_rcv_t *rcv);
int sdr_rcv_start(sdr_rcv_t *rcv, int dev, void *dp, const char **paths);
void sdr_rcv_stop(sdr_rcv_t *rcv);
sdr_rcv_t *sdr_rcv_open_dev(const char **sigs, int *prns, int n, int bus,
    int port, const char *conf_file, const char **paths, const char *opt);
sdr_rcv_t *sdr_rcv_open_sdev(const char **sigs, int *prns, int n,
    const char *driver, int fmt, double rate, double freq, const char **paths,
    const char *opt);
sdr_rcv_t *sdr_rcv_open_file(const char **sigs, int *prns, int n, int fmt,
    double fs, const double *fo, const int *IQ, const int *bits, double toff,
    double tscale, const char *file, const char **paths, const char *opt);
void sdr_rcv_close(sdr_rcv_t *rcv);
void sdr_rcv_setopt(const char *opt, double value);
int sdr_rcv_rcv_stat(sdr_rcv_t *rcv, char *buff, int size);
void sdr_rcv_str_stat(sdr_rcv_t *rcv, int *stat);
int sdr_rcv_sat_stat(sdr_rcv_t *rcv, const char *sat, char *buff, int size);
int sdr_rcv_ch_stat(sdr_rcv_t *rcv, const char *sys, int all,
    double min_lock, int rfch, int opt, char *buff, int size);
void sdr_rcv_sel_ch(sdr_rcv_t *rcv, int ch, double width);
int sdr_rcv_corr_stat(sdr_rcv_t *rcv, int ch, double *stat, double *pos,
    sdr_cpx_t *C, double *P, double *I);
int sdr_rcv_corr_hist(sdr_rcv_t *rcv, int ch, double tspan, double *stat,
    sdr_cpx_t *P);
int sdr_rcv_rfch_stat(sdr_rcv_t *rcv, int ch, double *stat);
int sdr_rcv_rfch_psd(sdr_rcv_t *rcv, int ch, double tave, int N, float *psd);
int sdr_rcv_rfch_hist(sdr_rcv_t *rcv, int ch, double tave, int *val,
    double *hist1, double *hist2);
int sdr_rcv_pvt_sol(sdr_rcv_t *rcv, char *buff, int size);
int sdr_rcv_get_gain(sdr_rcv_t *rcv, int ch);
int sdr_rcv_set_gain(sdr_rcv_t *rcv, int ch, int gain);
int sdr_rcv_get_filt(sdr_rcv_t *rcv, int ch, double *bw, double *freq,
    int *order);
int sdr_rcv_set_filt(sdr_rcv_t *rcv, int ch, double bw, double freq, int order);

#ifdef __cplusplus
}
#endif
#endif // POCKET_SDR_H 
