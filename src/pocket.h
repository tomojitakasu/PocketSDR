/*----------------------------------------------------------------------------*/
/**
 *  Pocket SDR C Library - Header file for GNSS SDR Functions.
 *
 *  Author:
 *  T.TAKASU
 *
 *  History:
 *  2021-10-20  0.1  new
 *  2022-01-04  1.0  support CyUSB on Windows
 *  2022-01-10  1.1  SDR_SIZE_BUFF: (1<<12) -> (1<<14)
 *  2022-01-20  1.2  add API mix_carr(), corr_std(), corr_fft()
 *
 */
#ifndef POCKET_H
#define POCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef CYUSB
#include <windows.h>
#include <CyAPI.h>
#else
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#endif /* CYUSB */

#ifdef __cplusplus
extern "C" {
#endif

/* constants and macro -------------------------------------------------------*/
#define SDR_DEV_NAME    "Pocket SDR" /* SDR device name */
#define SDR_DEV_VID     0x04B4  /* SDR device vendor ID */
#define SDR_DEV_PID     0x1004  /* SDR device product ID */
#define SDR_DEV_IF      0       /* SDR device interface number */
#define SDR_DEV_EP      0x86    /* SDR device end point for bulk transter */

#define SDR_VR_STAT     0x40    /* SDR vendor request: Get status */
#define SDR_VR_REG_READ 0x41    /* SDR vendor request: Read register */
#define SDR_VR_REG_WRITE 0x42   /* SDR vendor request: Write register */
#define SDR_VR_SAVE     0x47    /* SDR vendor request: Save settings */

#define SDR_FREQ_TCXO   24.000  /* SDR frequency of TCXO (MHz) */

#define SDR_MAX_CH      2       /* number of channels in a SDR device */
#define SDR_MAX_REG     11      /* number of registers in a SDR device */

#ifdef CYUSB
#define SDR_MAX_BUFF    1024    /* number of digital IF data buffer */
#define SDR_SIZE_BUFF   (1<<14) /* size of digital IF data buffer (bytes) */
#else
#define SDR_MAX_BUFF    4       /* number of digital IF data buffer */
#define SDR_SIZE_BUFF   (1<<22) /* size of digital IF data buffer (bytes) */
#endif /* CYUSB */

#define PI  3.1415926535897932  /* pi */

/* type definitions ----------------------------------------------------------*/

#ifdef CYUSB

typedef CCyUSBDevice sdr_usb_t;  /* USB device type */
typedef CCyBulkEndPoint sdr_ep_t;  /* USB bulk endpoint type */

typedef struct {                /* SDR device type */
    sdr_usb_t *usb;             /* USB device */
    sdr_ep_t *ep;               /* bulk endpoint */
    uint8_t *buff[SDR_MAX_BUFF]; /* data buffers */
    int IQ[SDR_MAX_CH];         /* sampling types */
    int rp, wp;                 /* read/write pointer of data buffers */
    int state;                  /* state of event handler */
    HANDLE thread;              /* event handler thread */
} sdr_dev_t;

#else

typedef libusb_device_handle sdr_usb_t; /* USB device type */
typedef struct libusb_transfer sdr_transfer_t; /* USB transfer type */

typedef struct {                /* SDR device type */
    sdr_usb_t *usb;             /* USB device */
    sdr_transfer_t *transfer[SDR_MAX_BUFF]; /* USB transfers */
    uint8_t *data[SDR_MAX_BUFF]; /* USB transfer data */
    int IQ[SDR_MAX_CH];         /* sampling types */
    pthread_t thread;           /* USB event handler thread */
    int state;                  /* state of USB event handler */
    int rp, wp;                 /* read/write pointer of ring-buffer */
    uint8_t *buff[SDR_MAX_BUFF]; /* ring-buffer */
} sdr_dev_t;

#endif /* CYUSB */

typedef struct {                /* PRN code type */
    int len, len_s;             /* primary/secondary code length (chips) */
    double rate, rate_s;        /* primary/secondary code rate (chips/s) */
    int8_t *code, *code_s;      /* primary/secondary code {-1|0|1} */
} sdr_code_t;


/* function prototypes -------------------------------------------------------*/
void *sdr_malloc(size_t size);
void sdr_free(void *p);
uint32_t sdr_get_tick(void);
void sdr_sleep_msec(int msec);

void init_lib(const char *file);
void mix_carr(const float *buff, int ix, int N, double fs, double fc,
    double phi, float *data);
void corr_std(const float *buff, int ix, int N, double fs, double fc,
    double phi, const float *code, const int *pos, int n, float *corr);
void corr_fft(const float *buff, int ix, int N, double fs, double fc,
    double phi, const float *code_fft, float *corr);
int gen_fftw_wisdom(const char *file, int N);

sdr_usb_t *sdr_usb_open(int bus, int port, uint16_t vid, uint16_t pid);
void sdr_usb_close(sdr_usb_t *usb);
int sdr_usb_req(sdr_usb_t *usb, int mode, uint8_t req, uint16_t val,
        uint8_t *data, int size);

int sdr_read_settings(const char *file, int bus, int port, int opt);
int sdr_write_settings(const char *file, int bus, int port, int opt);

sdr_dev_t *sdr_dev_open(int bus, int port);
void sdr_dev_close(sdr_dev_t *dev);
int sdr_dev_data(sdr_dev_t *dev, int8_t **buff, int *n);

#ifdef __cplusplus
}
#endif
#endif /* POCKET_H */
