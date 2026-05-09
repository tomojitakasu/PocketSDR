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
#define GAP_THRES     10000        // silent-drop detection threshold (ns)
#define GAP_MAX_SEC   0.1          // max zero-fill duration on a single gap (s)
#define TIMER_RES     1            // timer resolution for Windows (ms)
#define MIN(x, y)     ((x) < (y) ? (x) : (y))

// elapsed time in seconds -----------------------------------------------------
static double ts_elapsed(uint32_t tick)
{
    return (sdr_get_tick() - tick) * 1e-3;
}

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
        fprintf(stderr, "sdev: format error (only supports CS8 or CS16)\n");
        return NULL;
    }
    for (int retry = 0; retry < OPEN_RETRY; retry++) {
        if (strchr(driver, '=') || strchr(driver, ',')) {
            args = SoapySDRKwargs_fromString(driver);
        } else {
            args = SoapySDRKwargs_fromString("");
            SoapySDRKwargs_set(&args, "driver", driver);
        }
        size_t nfound = 0;
        SoapySDRKwargs *found = SoapySDRDevice_enumerate(&args, &nfound);
        if (nfound > 0) {
            dev = SoapySDRDevice_make(&found[0]);
        } else {
            dev = SoapySDRDevice_make(&args);
        }
        SoapySDRKwargsList_clear(found, nfound);
        SoapySDRKwargs_clear(&args);
        if (!dev) {
            fprintf(stderr, "sdev: SoapySDRDevice_make error: %s\n",
                SoapySDRDevice_lastError());
            return NULL;
        }
        if (bw > 0.0) {
            (void)SoapySDRDevice_setBandwidth(dev, SOAPY_SDR_RX, ch, bw);
        }
        SoapySDRDevice_setSampleRate(dev, SOAPY_SDR_RX, ch, rate);
        SoapySDRDevice_setFrequency(dev, SOAPY_SDR_RX, ch, freq, NULL);
        
        if (gain > 0.0) {
            (void)SoapySDRDevice_setGainMode(dev, SOAPY_SDR_RX, ch, false);
            (void)SoapySDRDevice_setGain(dev, SOAPY_SDR_RX, ch, gain);
        } else {
            (void)SoapySDRDevice_setGainMode(dev, SOAPY_SDR_RX, ch, true);
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
    
    // Driver-specific stream args:
    const char *str_args = "";
    if (!strcmp(driver, "lime")) { // LimeSDR
        str_args = "linkFormat=CS12,latency=1";
    } else if (!strcmp(driver, "uhd")) { // USRP
        str_args = "num_recv_frames=2048,recv_frame_size=16384,wirefmt=sc8";
    }
    SoapySDRKwargs stream_args = SoapySDRKwargs_fromString(str_args);
    str = SoapySDRDevice_setupStream(dev, SOAPY_SDR_RX, str_fmt, chs, 1,
        &stream_args);
    SoapySDRKwargs_clear(&stream_args);
    if (!str) {
        fprintf(stderr, "sdev: SoapySDRDevice_setupStream error: %s\n",
            SoapySDRDevice_lastError());
        SoapySDRDevice_unmake(dev);
        return NULL;
    }
    sdr_sdev_t *sdev = (sdr_sdev_t *)sdr_malloc(sizeof(sdr_sdev_t));
    sdev->dev = (void *)dev;
    sdev->str = (void *)str;
    snprintf(sdev->driver, sizeof(sdev->driver), "%s", driver);
    sdev->ssize = fmt == SDR_FMT_CS8 ? 2 : 4;
    sdev->rate = fs_act;
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

// raise process/thread priority -----------------------------------------------
#ifdef SOAPYSDR
static void raise_pri(void)
{
#ifdef WIN32
    if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        fprintf(stderr, "sdev: SetPriorityClass error (%d)\n", (int)GetLastError());
    }
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
        fprintf(stderr, "sdev: SetThreadPriority error (%d)\n", (int)GetLastError());
    }
    DWORD task = 0;
    HANDLE h = AvSetMmThreadCharacteristicsA("DisplayPostProcessing", &task);
    
    if (h == 0) {
        fprintf(stderr, "sdev: AvSetMmThreadCharacteristicsA error (%d)\n",
            (int)GetLastError());
    }
    else if (!AvSetMmThreadPriority(h, AVRT_PRIORITY_CRITICAL)) {
        fprintf(stderr, "sdev: AvSetMmThreadPriority error (%d)\n",
            (int)GetLastError());
    }
#endif
}
#endif

// zero-fill dropped samples ---------------------------------------------------
static void fill_gap(sdr_sdev_t *sdev, int pos, int data_samples,
    int64_t gap_samples, uint8_t *scratch)
{
    int64_t gap_bytes = gap_samples * sdev->ssize;
    int64_t data_bytes = (int64_t)data_samples * sdev->ssize;
    int64_t buf_size = (int64_t)BUFF_SIZE * BUFF_CNT;
    
    if (data_bytes > 0) {
        memcpy(scratch, sdev->buff + pos, data_bytes);
    }
    int64_t fill = pos, rem = gap_bytes;
    while (rem > 0) {
        int64_t chunk = MIN(rem, buf_size - fill);
        memset(sdev->buff + fill, 0, chunk);
        rem -= chunk;
        fill = (fill + chunk) % buf_size;
    }
    int64_t dst = (pos + gap_bytes) % buf_size, off = 0;
    rem = data_bytes;
    while (rem > 0) {
        int64_t chunk = MIN(rem, buf_size - dst);
        memcpy(sdev->buff + dst, scratch + off, chunk);
        rem -= chunk;
        off += chunk;
        dst = (dst + chunk) % buf_size;
    }
}

// Soapy device reader thread --------------------------------------------------
#ifdef SOAPYSDR
static void *reader_thread(void *arg)
{
    sdr_sdev_t *sdev = (sdr_sdev_t *)arg;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev;
    SoapySDRStream *str = (SoapySDRStream *)sdev->str;
    uint8_t *gap_buf = (uint8_t *)sdr_malloc(BUFF_SIZE);
    void *buffs[1];
    int tns_ena = !strcmp(sdev->driver,"lime") || !strcmp(sdev->driver,"uhd");
    int ret, flags, ovcnt = 0, dropcnt = 0, gapcnt = 0, prev_ret = 0;
    long long tns, tns_prev = 0;
    uint32_t tick = sdr_get_tick();
    
    // raise process/thread priority
    raise_pri();
    
#ifdef WIN32 // raise timer resolution
    if (timeBeginPeriod(TIMER_RES)) {
        fprintf(stderr, "sdev: timeBeginPeriod (%d)\n", (int)GetLastError());
    }
#endif
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
            fprintf(stderr, "sdev: SoapySDRDevice_readStream error (%s) ts=%.3f\n",
                SoapySDR_errToStr(ret), ts_elapsed(tick));
            sdev->state = -1;
            break;
        }
        if (flags & SOAPY_SDR_END_ABRUPT) {
            dropcnt++;
        }
        // detect drop via HW timestamp gap and zero-fill
        int64_t gap_samples = 0;
        if (tns_ena && tns_prev != 0 && prev_ret > 0) {
            int64_t gap_ns = (tns - tns_prev) - (int64_t)(prev_ret * 1e9 / sdev->rate);
            if (gap_ns > GAP_THRES) {
                gap_samples = (int64_t)(sdev->rate * gap_ns * 1e-9 + 0.5);
                int64_t max_gap = (int64_t)(sdev->rate * GAP_MAX_SEC);
                if (gap_samples > max_gap) gap_samples = max_gap;
                fprintf(stderr, "sdev: samples dropped ts=%.3fs gap=%.3fms (zero-fill)\n",
                    ts_elapsed(tick), gap_ns * 1e-6);
                gapcnt++;
            }
        }
        prev_ret = ret;
        tns_prev = tns;
        
        if (gap_samples > 0) {
            fill_gap(sdev, pos, ret, gap_samples, gap_buf);
        }
        sdr_mutex_lock(&sdev->mtx);
        sdev->wp += gap_samples * sdev->ssize + ret * sdev->ssize;
        sdr_mutex_unlock(&sdev->mtx);
    }
    if (ovcnt > 0 || dropcnt > 0 || gapcnt > 0) {
        fprintf(stderr, "sdev: total ts=%.3fs overflow=%d dev-drop=%d drop=%d\n",
            ts_elapsed(tick), ovcnt, dropcnt, gapcnt);
    }
    sdr_free(gap_buf);
#ifdef WIN32
    timeEndPeriod(TIMER_RES);
#endif
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
        fprintf(stderr, "sdev: buffer overflown\n");
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

