//
//  Pocket SDR C Library - Header file for GNSS SDR device Functions.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2021-10-20  0.1  new
//  2022-01-04  1.0  support CyUSB on Windows
//  2022-01-10  1.1  SDR_SIZE_BUFF: (1<<12) -> (1<<14)
//  2022-01-20  1.2  add API mix_carr(), corr_std(), corr_fft()
//  2023-12-24  1.3  SDR_MAX_CH: 2 -> 8
//                   support USB context
//  2024-04-04  1.4  update constants and types
//  2024-04-28  1.5  update constants, types and APIs
//  2024-05-28  1.6  update constants, types and APIs
//
#ifndef POCKET_DEV_H
#define POCKET_DEV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#ifdef WIN32
#include <windows.h>
#include <CyAPI.h>
#else
#include <libusb-1.0/libusb.h>
#endif // WIN32

#ifdef __cplusplus
extern "C" {
#endif

// constants and macro -------------------------------------------------------
#define SDR_DEV_NAME    "Pocket SDR" // SDR device name 
#define SDR_DEV_VID     0x04B4  // SDR device vendor ID 
#define SDR_DEV_PID1    0x1004  // SDR device product ID (EZ-USB FX2LP)
#define SDR_DEV_PID2    0x00F1  // SDR device product ID (EZ-USB FX3)
#define SDR_DEV_IF      0       // SDR device interface number 
#define SDR_DEV_EP      0x86    // SDR device end point for bulk transter 

#define SDR_VR_STAT     0x40    // SDR vendor request: Get status
#define SDR_VR_REG_READ 0x41    // SDR vendor request: Read register
#define SDR_VR_REG_WRITE 0x42   // SDR vendor request: Write register
#define SDR_VR_START    0x44    // SDR vendor request: Start bulk transfer
#define SDR_VR_STOP     0x45    // SDR vendor request: Stop bulk transfer
#define SDR_VR_RESET    0x46    // SDR vendor request: Reset device
#define SDR_VR_SAVE     0x47    // SDR vendor request: Save settings

#define SDR_MAX_RFCH    8       // max number of RF channels in a SDR device
#define SDR_MAX_REG     11      // max number of registers in a SDR device

#define SDR_MAX_BUFF    96      // number of digital IF data buffer
#define SDR_SIZE_BUFF   (1<<16) // size of digital IF data buffer (bytes)

// type definitions ----------------------------------------------------------

#ifdef WIN32
typedef CCyUSBDevice sdr_usb_t; // USB device type 
#else
typedef struct {                // USB device type
    libusb_context *ctx;        // USB context
    libusb_device_handle *h;    // USB device handle
} sdr_usb_t;
#endif

typedef struct {                // SDR device type
    sdr_usb_t *usb;             // USB device
    int state;                  // state of USB event handler
    int64_t rp, wp;             // read/write pointer of data buffer
    uint8_t *buff;              // data buffer
#ifndef WIN32
    struct libusb_transfer *transfer[SDR_MAX_BUFF]; // USB transfers
#endif
    pthread_t thread;           // USB event handler thread
    pthread_mutex_t mtx;        // lock flag
} sdr_dev_t;

// function prototypes -------------------------------------------------------

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
int sdr_dev_get_info(sdr_dev_t *dev, int *fmt, double *fs, double *fo, int *IQ);
int sdr_dev_get_gain(sdr_dev_t *dev, int ch);
int sdr_dev_set_gain(sdr_dev_t *dev, int ch, int gain);

// sdr_conf.c
int sdr_conf_read(sdr_dev_t *dev, const char *file, int opt);
int sdr_conf_write(sdr_dev_t *dev, const char *file, int opt);

#ifdef __cplusplus
}
#endif
#endif // POCKET_DEV_H 
