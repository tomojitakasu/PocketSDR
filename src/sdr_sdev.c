//
//  Pocket SDR C Library - SoapySDR Device Functions.
//
//  Options:
//      -DSOAPYSDR: enable SoapySDR device functions
//
//  Author:
//  T.TAKASU
//
//  History:
//  2025-11-17  0.1  new
//
#ifdef SOAPYSDR
#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#endif
#include "pocket_sdr.h"
#ifdef WIN32
#include <avrt.h>
#endif

#define BUFF_SIZE     (1<<20)      // buffer size (bytes)
#define BUFF_CNT      128          // buffer count
#define READ_TO       100000       // read timeout (us)
#define OPEN_RETRY    3            // device open retry count
#define RATE_TOL      100.0        // rate tolerance (sps)
#define FREQ_TOL      10e3         // freq tolerance (Hz)
#define MIN(x, y)     ((x) < (y) ? (x) : (y))

// list SoapySDR devices -------------------------------------------------------
int sdr_sdev_list(void)
{
    size_t len = 0;
#ifdef SOAPYSDR
    SoapySDRKwargs *devs = NULL;
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
        printf("#%d: %s | driver=%8s | serial=%8s\n", i,
            label ? label : "(no label)", driver ? driver : "?",
            serial ? serial : "");
    }
    SoapySDRKwargsList_clear(devs, len);
#endif
    return (int)len;
}

//------------------------------------------------------------------------------
//  Open SoapySDR device.
//
//  args:
//      driver      (I)   SoapySDR driver
//                          "uhd"     : USRP
//                          "lime"    : LimeSDR
//                          "bladrf"  : bladeRF
//                          "rtlsdr"  : RTL-SDR
//                          "plutosdr": PlutoSDR
//      fmt         (I)   sampling data format (SDR_FMT_CS8 or _CS16)
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
#ifdef SOAPYSDR
    SoapySDRDevice *dev = NULL;
    SoapySDRStream *str;
    SoapySDRKwargs args;
    const char *str_fmt;
    const size_t ch = 0;
    size_t chs[1] = {ch};
    double fs_act = 0.0, fo_act = 0.0;
    int ok = 0;
    
    if (fmt != SDR_FMT_CS8 && fmt != SDR_FMT_CS16) {
        return NULL;
    }
    for (int retry = 0; retry < OPEN_RETRY; retry++) {
        args = SoapySDRKwargs_fromString("");
        SoapySDRKwargs_set(&args, "driver", driver);
        dev = SoapySDRDevice_make(&args);
        SoapySDRKwargs_clear(&args);
        if (!dev) {
            fprintf(stderr, "SoapySDRDevice_make error\n");
            return NULL;
        }
        SoapySDRDevice_setSampleRate(dev, SOAPY_SDR_RX, ch, rate);
        SoapySDRDevice_setFrequency(dev, SOAPY_SDR_RX, ch, freq, NULL);
        
        if (gain > 0.0) {
            (void)SoapySDRDevice_setGainMode(dev, SOAPY_SDR_RX, ch, false);
            (void)SoapySDRDevice_setGain(dev, SOAPY_SDR_RX, ch, gain);
        } else {
            (void)SoapySDRDevice_setGainMode(dev, SOAPY_SDR_RX, ch, true);
        }
        if (bw > 0.0) {
            (void)SoapySDRDevice_setBandwidth(dev, SOAPY_SDR_RX, ch, bw);
        }
        (void)SoapySDRDevice_setDCOffsetMode(dev, SOAPY_SDR_RX, ch, true);
        (void)SoapySDRDevice_setIQBalanceMode(dev, SOAPY_SDR_RX, ch, true);
        
        fs_act = SoapySDRDevice_getSampleRate(dev, SOAPY_SDR_RX, ch);
        fo_act = SoapySDRDevice_getFrequency(dev, SOAPY_SDR_RX, ch);
        if (fabs(fs_act - rate) <= RATE_TOL && fabs(fo_act - freq) <= FREQ_TOL) {
            ok = 1;
            break;
        }
        fprintf(stderr, "sdev: setup mismatch attempt %d/%d - retrying\n",
            retry + 1, OPEN_RETRY);
        SoapySDRDevice_unmake(dev);
        dev = NULL;
    }
    if (!ok) {
        fprintf(stderr, "sdev: setup failed after %d retries\n", OPEN_RETRY);
        if (dev) SoapySDRDevice_unmake(dev);
        return NULL;
    }
    fprintf(stderr, "sdev: rate=%.6fMsps (req %.3f), LO=%.6fMHz (req %.3f)\n",
        fs_act * 1e-6, rate * 1e-6, fo_act * 1e-6, freq * 1e-6);
    str_fmt = fmt == SDR_FMT_CS8 ? SOAPY_SDR_CS8 : SOAPY_SDR_CS16;
    
    SoapySDRKwargs stream_args = SoapySDRKwargs_fromString("");
    str = SoapySDRDevice_setupStream(dev, SOAPY_SDR_RX, str_fmt, chs, 1,
        &stream_args);
    SoapySDRKwargs_clear(&stream_args);
    if (!str) {
        fprintf(stderr, "SoapySDRDevice_setupStream error\n");
        SoapySDRDevice_unmake(dev);
        return NULL;
    }
    sdr_sdev_t *sdev = (sdr_sdev_t *)sdr_malloc(sizeof(sdr_sdev_t));
    sdev->dev = (void *)dev;
    sdev->str = (void *)str;
    snprintf(sdev->driver, sizeof(sdev->driver), "%s", driver);
    sdev->ssize = fmt == SDR_FMT_CS8 ? 2 : 4;
    sdev->buff = (uint8_t *)sdr_malloc(BUFF_SIZE * BUFF_CNT);
    sdev->rp = sdev->wp = 0;
    sdr_mutex_init(&sdev->mtx);
    return sdev;
#else
    return NULL;
#endif
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
#ifdef SOAPYSDR
    if (!sdev) return;
    if (sdev->state) sdr_sdev_stop(sdev);
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev;
    SoapySDRStream *str = (SoapySDRStream *)sdev->str;
    SoapySDRDevice_closeStream(dev, str);
    SoapySDRDevice_unmake(dev);
    sdr_free(sdev->buff);
    sdr_free(sdev);
#endif
}

// rise process/thread priority ------------------------------------------------
#ifdef SOAPYSDR
static void rise_pri(void)
{
#ifdef WIN32
    if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        fprintf(stderr, "SetPriorityClass error (%d)\n", (int)GetLastError());
    }
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
        fprintf(stderr, "SetThreadPriority error (%d)\n", (int)GetLastError());
    }
    DWORD task = 0;
    HANDLE h = AvSetMmThreadCharacteristicsA("DisplayPostProcessing", &task);

    if (h == 0) {
        fprintf(stderr, "AvSetMmThreadCharacteristicsA error (%d)\n",
            (int)GetLastError());
    }
    else if (!AvSetMmThreadPriority(h, AVRT_PRIORITY_CRITICAL)) {
        fprintf(stderr, "AvSetMmThreadPriority error (%d)\n",
            (int)GetLastError());
    }
#endif
}
#endif

// Soapy device reader thread --------------------------------------------------
#ifdef SOAPYSDR
static void *reader_thread(void *arg)
{
    sdr_sdev_t *sdev = (sdr_sdev_t *)arg;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev;
    SoapySDRStream *str = (SoapySDRStream *)sdev->str;
    void *buffs[1];
    int ret, flags, ovcnt = 0;
    long long tns;
    
    // rise process/thread priority
    rise_pri();
    
    while (sdev->state) {
        int pos = (int)(sdev->wp % (BUFF_SIZE * BUFF_CNT));
        int size = MIN(BUFF_SIZE, (BUFF_SIZE * BUFF_CNT - pos)) / sdev->ssize;
        buffs[0] = (void *)(sdev->buff + pos);
        flags = 0;
        ret = SoapySDRDevice_readStream(dev, str, buffs, size, &flags, &tns,
            READ_TO);
        if (ret == SOAPY_SDR_TIMEOUT) continue;
        if (ret == SOAPY_SDR_OVERFLOW) {
            ovcnt++;
            continue;
        }
        if (ret < 0) {
            fprintf(stderr, "SoapySDRDevice_readStream error (%s)\n",
                SoapySDR_errToStr(ret));
            sdev->state = -1;
            break;
        }
        sdr_mutex_lock(&sdev->mtx);
        sdev->wp += ret * sdev->ssize;
        sdr_mutex_unlock(&sdev->mtx);
    }
    if (ovcnt > 0) {
        fprintf(stderr, "SoapySDRDevice_readStream: overflow=%d\n", ovcnt);
    }
    return NULL;
}
#endif

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
#ifdef SOAPYSDR
    if (!sdev || sdev->state) return 0;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev;
    SoapySDRStream *str = (SoapySDRStream *)sdev->str;
    if (SoapySDRDevice_activateStream(dev, str, 0, 0, 0)) return 0;
    sdev->rp = sdev->wp = 0;
    sdev->state = 1;
    if (!sdr_thread_create(&sdev->thread, reader_thread, sdev)) {
        sdev->state = 0;
        SoapySDRDevice_deactivateStream(dev, str, 0, 0);
        return 0;
    }
    return 1;
#else
    return 0;
#endif
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
#ifdef SOAPYSDR
    if (!sdev || !sdev->state) return 0;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev;
    SoapySDRStream *str = (SoapySDRStream *)sdev->str;
    sdev->state = 0;
    SoapySDRDevice_deactivateStream(dev, str, 0, 0);
    sdr_thread_join(sdev->thread);
    return 1;
#else
    return 0;
#endif
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
    int64_t wp, bsize;
    int psize, pos;
    
    if (!sdev || sdev->state < 0) return -1;
    if (!sdev->state) return 0;
    
    sdr_mutex_lock(&sdev->mtx);
    wp = sdev->wp;
    sdr_mutex_unlock(&sdev->mtx);
    
    bsize = wp - sdev->rp;
    
    if (bsize > BUFF_SIZE * BUFF_CNT) {
        fprintf(stderr, "buffer overflown\n");
        sdev->rp = wp;
        return 0;
    } else if (bsize < size) {
        return 0;
    }
    pos = (int)(sdev->rp % (BUFF_SIZE * BUFF_CNT));
    
    if (pos + size <= BUFF_SIZE * BUFF_CNT) {
        memcpy(buff, sdev->buff + pos, size);
    } else {
        psize = BUFF_SIZE * BUFF_CNT - pos;
        memcpy(buff, sdev->buff + pos, psize);
        memcpy(buff + psize, sdev->buff, size - psize);
    }
    sdev->rp += size;
    return size;
}

