/*----------------------------------------------------------------------------*/
/**
 *  Pocket SDR - SDR Device Functions.
 *
 *  Author:
 *  T.TAKASU
 *
 *  History:
 *  2021-10-20  0.1  new
 *
 */
#include <sys/time.h>
#include "pocket.h"

/* constants and macros ------------------------------------------------------*/
#define TO_TRANSFER     3000    /* USB transfer timeout (ms) */

/* quantization lookup table -------------------------------------------------*/
static int8_t LUT[2][2][256];

/* generate quantization lookup table ----------------------------------------*/
static void gen_LUT(void)
{
    static const int8_t val[] = {+1, +3, -1, -3}; /* 2bit, sign + magnitude */
    int i;
    
    if (LUT[0][0][0]) return;
    
    for (i = 0; i < 256; i++) {
        LUT[0][0][i] = val[(i>>0) & 0x3]; /* CH1 I */
        LUT[0][1][i] = val[(i>>2) & 0x3]; /* CH1 Q */
        LUT[1][0][i] = val[(i>>4) & 0x3]; /* CH2 I */
        LUT[1][1][i] = val[(i>>6) & 0x3]; /* CH2 Q */
    }
}

/* read sampling type --------------------------------------------------------*/
static int read_sample_type(sdr_dev_t *dev)
{
    uint8_t data[4];
    int i;
    
    for (i = 0; i < SDR_MAX_CH; i++) {
        /* read MAX2771 ENIQ field */
        if (!sdr_usb_req(dev->usb, 0, SDR_VR_REG_READ, (uint16_t)((i << 8) + 1),
                 data, 4)) {
           return 0;
        }
        dev->IQ[i] = ((data[0] >> 3) & 1) ? 2 : 1; /* I:1,IQ:2 */
    }
    return 1;
}

/* USB bulk transfer callback ------------------------------------------------*/
static void transfer_cb(struct libusb_transfer *transfer)
{
    sdr_dev_t *dev = (sdr_dev_t *)transfer->user_data;
    
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "USB bulk transfer error (%d)\n", transfer->status);
    }
    else if (dev->n < SDR_MAX_BUFF) {
        dev->data[dev->n++] = transfer->buffer;
    }
    else {
        fprintf(stderr, "USB bulk transfer buffer overflow\n");
    }
    libusb_submit_transfer(transfer);
}

/*----------------------------------------------------------------------------*/
/**
 *  Open a SDR device.
 *
 *  args:
 *      bus         (I)   USB bus number of SDR device  (-1:any)
 *      port        (I)   USB port number of SDR device (-1:any)
 *
 *  return
 *      SDR device pointer (NULL: error)
 */
sdr_dev_t *sdr_dev_open(int bus, int port)
{
    sdr_dev_t *dev;
    int i;
    
    dev = (sdr_dev_t *)sdr_malloc(sizeof(sdr_dev_t));
    
    if (!(dev->usb = sdr_usb_open(bus, port, SDR_DEV_VID, SDR_DEV_PID))) {
        sdr_free(dev);
        return NULL;
    }
    if (!read_sample_type(dev)) {
        sdr_usb_close(dev->usb);
        sdr_free(dev);
        fprintf(stderr, "Read sampling type error\n");
        return NULL;
    }
    for (i = 0; i < SDR_MAX_BUFF; i++) {
        dev->buff[i] = (uint8_t *)sdr_malloc(SDR_SIZE_BUFF);
        if (!(dev->transfer[i] = libusb_alloc_transfer(0))) {
            sdr_usb_close(dev->usb);
            sdr_free(dev);
            return NULL;
        }
        libusb_fill_bulk_transfer(dev->transfer[i], dev->usb, SDR_DEV_EP,
            dev->buff[i], SDR_SIZE_BUFF, transfer_cb, dev, TO_TRANSFER);
    }
    for (i = 0; i < SDR_MAX_BUFF; i++) {
        libusb_submit_transfer(dev->transfer[i]);
    }
    gen_LUT();
    
    return dev;
}

/*----------------------------------------------------------------------------*/
/**
 *  Close SDR device.
 *
 *  args:
 *      dev         (I)   SDR device
 *
 *  return
 *      none
 */
void sdr_dev_close(sdr_dev_t *dev)
{
    int i;
    
    for (i = 0; i < SDR_MAX_BUFF; i++) {
        libusb_cancel_transfer(dev->transfer[i]);
    }
    sdr_sleep_msec(100);
    sdr_usb_close(dev->usb);
    
    for (i = 0; i < SDR_MAX_BUFF; i++) {
        libusb_free_transfer(dev->transfer[i]);
        sdr_free(dev->buff[i]);
    }
    sdr_free(dev);
}

/* copy digital IF data ------------------------------------------------------*/
static int copy_data(const uint8_t *data, int ch, int IQ, int8_t *buff)
{
    int i, j, size = SDR_SIZE_BUFF;
    
    if (IQ == 0) { /* raw */
        if (ch != 0) return 0;
        memcpy(buff, data, size);
    }
    else if (IQ == 1) { /* I sampling */
        for (i = 0; i < size; i += 2) {
            buff[i  ] = LUT[ch][0][data[i  ]];
            buff[i+1] = LUT[ch][0][data[i+1]];
        }
    }
    else if (IQ == 2) { /* I/Q sampling */
        size *= 2;
        for (i = j = 0; i < size; i += 4, j += 2) {
            buff[i  ] = LUT[ch][0][data[j  ]];
            buff[i+1] = LUT[ch][1][data[j  ]];
            buff[i+2] = LUT[ch][0][data[j+1]];
            buff[i+3] = LUT[ch][1][data[j+1]];
        }
    }
    return size;
}

/* get digital IF data -------------------------------------------------------*/
int sdr_dev_data(sdr_dev_t *dev, int8_t **buff, int *n)
{
    struct timeval to = {0, TO_TRANSFER * 1000};
    int i, size = 0;
    
    n[0] = n[1] = 0;
    
    if (libusb_handle_events_timeout(NULL, &to)) {
        return 0;
    }
    for (i = 0; i < dev->n; i++) {
        n[0] += copy_data(dev->data[i], 0, dev->IQ[0], buff[0] + n[0]);
        n[1] += copy_data(dev->data[i], 1, dev->IQ[1], buff[1] + n[1]);
        size += SDR_SIZE_BUFF;
    }
    dev->n = 0;
    return size;
}
