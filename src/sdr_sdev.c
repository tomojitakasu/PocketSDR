//
//  Pocket SDR C Library - SoapySDR Device Functions.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2025-11-17  0.1  new
//
#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <signal.h>
#include "pocket_sdr.h"

#define BUFF_SIZE   (1<<14)      // buffer size (bytes)
#define BUFF_CNT    256          // buffer count
#define READ_TO     100000       // read timeout (us)

// list SoapySDR devices -------------------------------------------------------
int sdr_sdev_list(void)
{
    SoapySDRKwargs *devs = NULL;
    size_t len = 0;
    const char *label, *driver, *serial;
    
    devs = SoapySDRDevice_enumerate(NULL, &len);
    
    if (!devs || len == 0) {
        printf("No devices found.\n");
        SoapySDRKwargsList_clear(devs, len);
        return 0;
    }
    for (int i = 0; i < (int)len; i++) {
        label = SoapySDRKwargs_get(devs + i, "label");
        driver = SoapySDRKwargs_get(devs + i, "driver");
        serial = SoapySDRKwargs_get(devs + i, "serial");
        printf("#%d: %s | driver=%8s | serial=%8s\n", i, label ? label : "(no label)",
            driver ? driver : "?", serial ? serial : "");
    }
    SoapySDRKwargsList_clear(devs, len);
    return len;
}

//------------------------------------------------------------------------------
//  Open SoapySDR device.
//
//  args:
//      driver      (I)   SoapySDR driver
//                          "uhd"     : USRP
//                          "lime"    : LimeSDR
//                          "rtlsdr"  : RTL-SDR
//                          "plutosdr": Pluto-SDR
//      fmt         (I)   sampling data format (CS8 or CS16)
//      rate        (I)   sampling rate (sps)
//      freq        (I)   carrier center frequency (Hz)
//      bw          (I)   filter bandwidth (Hz) (<= 0: not set)
//      gain        (I)   LNA gain control (dB) (<= 0: not set)
//
//  return
//      SoapySDR device pointer (NULL: error)
//
sdr_sdev_t *sdr_sdev_open(const char *driver, int fmt, double rate,
    double freq, double bw, double gain)
{
    SoapySDRDevice *dev;
    SoapySDRStream *str;
    SoapySDRKwargs args = SoapySDRKwargs_fromString("");
    const char *str_fmt;
    const size_t ch = 0;
    size_t chs[1] = {ch};
    
    if (fmt != SDR_FMT_CS8 && fmt != SDR_FMT_CS16) {
        return NULL;
    }
    SoapySDRKwargs_set(&args, "driver", driver);
    if (!(dev = SoapySDRDevice_make(&args))) {
        SoapySDRKwargs_clear(&args);
        return NULL;
    }
    SoapySDRKwargs_clear(&args);
    SoapySDRDevice_setSampleRate(dev, SOAPY_SDR_RX, ch, rate);
    SoapySDRDevice_setFrequency(dev, SOAPY_SDR_RX, ch, freq, NULL);
    
    if (gain > 0.0) {
        (void)SoapySDRDevice_setGainMode(dev, SOAPY_SDR_RX, ch, false);
        (void)SoapySDRDevice_setGain(dev, SOAPY_SDR_RX, ch, gain);
    }
    else {
        (void)SoapySDRDevice_setGainMode(dev, SOAPY_SDR_RX, ch, true);
    }
    if (bw > 0.0) {
        (void)SoapySDRDevice_setBandwidth(dev, SOAPY_SDR_RX, ch, bw);
    }
    (void)SoapySDRDevice_setDCOffsetMode(dev, SOAPY_SDR_RX, ch, true);
    (void)SoapySDRDevice_setIQBalanceMode(dev, SOAPY_SDR_RX, ch, true);

    str_fmt = fmt == SDR_FMT_CS8 ? SOAPY_SDR_CS8 : SOAPY_SDR_CS16;

    if (!(str = SoapySDRDevice_setupStream(dev, SOAPY_SDR_RX, str_fmt, chs, 1,
        NULL))) {
        fprintf(stderr, "SoapySDRDevice_setupStream error\n");
        SoapySDRDevice_unmake(dev);
        return NULL;
    }
    sdr_sdev_t *sdev = (sdr_sdev_t *)sdr_malloc(sizeof(sdr_sdev_t));
    sdev->dev = (void *)dev;
    sdev->str = (void *)str;
    snprintf(sdev->driver, sizeof(sdev->driver), "%s", driver);
    sdev->ssize = fmt == SDR_FMT_CS8 ? 2 : 4;
    sdev->buff = (uint8_t *)sdr_malloc(BUFF_SIZE * BUFF_CNT * sdev->ssize);
    sdev->rp = sdev->wp = 0;
    pthread_mutex_init(&sdev->mtx, NULL);
    return sdev;
}

//------------------------------------------------------------------------------
//  Close SoapySDR device.
//
//  args:
//      sdev        (I)   SoapySDR device
//
//  return
//      None
//
void sdr_sdev_close(sdr_sdev_t *sdev)
{
    if (!sdev) return;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev;
    SoapySDRStream *str = (SoapySDRStream *)sdev->str;
    SoapySDRDevice_closeStream(dev, str);
    SoapySDRDevice_unmake(dev);
    sdr_free(sdev->buff);
    sdr_free(sdev);
}

// Soapy device reader thread --------------------------------------------------
static void *reader_thread(void *arg)
{
    sdr_sdev_t *sdev = (sdr_sdev_t *)arg;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev;
    SoapySDRStream *str = (SoapySDRStream *)sdev->str;
    void *buffs[1];
    int ret, flags;
    long long tns;
    
    for (int i = 0; sdev->state; i = (i + 1) % BUFF_CNT) {
        buffs[0] = (void *)(sdev->buff + BUFF_SIZE * i);
        ret = SoapySDRDevice_readStream(dev, str, buffs, BUFF_SIZE / sdev->ssize,
            &flags, &tns, READ_TO);
        if (ret == SOAPY_SDR_TIMEOUT) continue;
        if (ret < 0) {
            fprintf(stderr, "device read error (%s)\n", SoapySDR_errToStr(ret));
            break;
        }
        pthread_mutex_lock(&sdev->mtx);
        sdev->wp += BUFF_SIZE;
        pthread_mutex_unlock(&sdev->mtx);
    }
    return NULL;
}

//------------------------------------------------------------------------------
//  Start SoapySDR device.
//
//  args:
//      sdev        (I)   SoapySDR device
//
//  return
//      status (1:OK, 0:error)
//
int sdr_sdev_start(sdr_sdev_t *sdev)
{
    if (!sdev || sdev->state) return 0;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev;
    SoapySDRStream *str = (SoapySDRStream *)sdev->str;
    sdev->state = 1;
    sdev->rp = sdev->wp = 0;
    pthread_create(&sdev->thread, NULL, reader_thread, sdev);
    return SoapySDRDevice_activateStream(dev, str, 0, 0, 0) == 0;
}

//------------------------------------------------------------------------------
//  Stop SoapySDR device.
//
//  args:
//      sdev        (I)   SoapySDR device
//
//  return
//      status (1:OK, 0:error)
//
int sdr_sdev_stop(sdr_sdev_t *sdev)
{
    if (!sdev || !sdev->state) return 0;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev;
    SoapySDRStream *str = (SoapySDRStream *)sdev->str;
    SoapySDRDevice_deactivateStream(dev, str, 0, 0);
    sdev->state = 0;
    pthread_join(sdev->thread, NULL);
    return 1;
}

//------------------------------------------------------------------------------
//  Read SoapySDR device
//
//  args:
//      sdev        (I)   SoapySDR device
//      buff        (IO)  Sampling data buffer
//      size        (I)   Sampling data buffer size (bytes)
//
//  return
//      read data size (bytes) (0: no data, -1: error)
//
int sdr_sdev_read(sdr_sdev_t *sdev, uint8_t *buff, int size)
{
    int64_t wp;
    int bsize, psize, pos;
    
    if (!sdev || !sdev->state) return 0;
    
    pthread_mutex_lock(&sdev->mtx);
    wp = sdev->wp;
    pthread_mutex_unlock(&sdev->mtx);
    
    bsize = (int)(wp - sdev->rp);
    
    if (bsize > BUFF_SIZE * BUFF_CNT) {
        fprintf(stderr, "buffer overflown");
        sdev->rp = wp;
        return 0;
    } else if (bsize < size) {
        return 0;
    }
    pos = (int)(sdev->rp % (BUFF_SIZE * BUFF_CNT));
    
    if (pos + size <= BUFF_SIZE * BUFF_CNT) {
        memcpy(buff, sdev->buff + pos, size);
    }
    else {
        psize = BUFF_SIZE * BUFF_CNT - pos;
        memcpy(buff, sdev->buff + pos, psize);
        memcpy(buff + psize, sdev->buff, size - psize);
    }
    sdev->rp += size;
    return size;
}

