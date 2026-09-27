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
//  2026-09-03  0.2  support multiple devices and RF channels
//                   modify API sdr_sdev_open(), sdr_sdev_read()
//
#ifdef SOAPYSDR
#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <SoapySDR/Logger.h>
#endif
#include "pocket_sdr.h"
#ifdef __linux__
#include <sys/prctl.h>
#endif
#ifdef WIN32
#include <avrt.h>
#endif

#define BUFF_SIZE     (1<<20)      // buffer size (bytes)
#define BUFF_CNT      128          // buffer count of a RF channel
#define BUFF_CNT_ALL  512          // max buffer count of all RF channels
#define READ_TO       100000       // read timeout (us)
#define OPEN_RETRY    3            // device open retry count
#define RATE_TOL      100.0        // rate tolerance (sps)
#define FREQ_TOL      10e3         // freq tolerance (Hz)
#define GAP_THRES     10000        // silent-drop detection threshold (ns)
#define GAP_MAX_SEC   0.1          // max zero-fill duration on a single gap (s)
#define TIMER_RES     1            // timer resolution for Windows (ms)
#define PPS_WAIT      2500         // PPS edge wait timeout (ms)
#define PPS_LATCH     1200         // wait for the time latched at a PPS (ms)
#define PPS_TOL       1000000      // PPS time match tolerance (ns)
#define START_SEC     2            // stream start delay in whole seconds (s)
#define START_DELAY   500000000LL  // stream start delay of a device (ns)
#define MIN(x, y)     ((x) < (y) ? (x) : (y))

#ifdef SOAPYSDR
static void soapy_log_handler(const SoapySDRLogLevel level, const char *msg)
{
    if (level == SOAPY_SDR_SSI) {
        return;
    }
    if (level <= SOAPY_SDR_WARNING) {
        fprintf(stderr, "%s\n", msg);
    }
}
#endif

// elapsed time in seconds -----------------------------------------------------
#ifdef SOAPYSDR
static double ts_elapsed(uint32_t tick)
{
    return (sdr_get_tick() - tick) * 1e-3;
}
#endif

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

#ifdef SOAPYSDR

// split device specs separated by ';' -----------------------------------------
static int split_specs(const char *driver, char spec[][256], int max)
{
    const char *p = driver, *q;
    int n = 0;
    
    while (*p && n < max) {
        while (*p == ' ') p++;
        if (!(q = strchr(p, ';'))) q = p + strlen(p);
        int len = (int)(q - p);
        while (len > 0 && p[len-1] == ' ') len--;
        if (len > 0) {
            snprintf(spec[n], 256, "%.*s", MIN(len, 255), p);
            n++;
        }
        p = *q ? q + 1 : q;
    }
    return n;
}

// make device args from a spec ------------------------------------------------
static SoapySDRKwargs make_args(const char *spec, char *drv_name, int size,
    int *nch)
{
    SoapySDRKwargs args;
    const char *p;
    
    if (strchr(spec, '=') || strchr(spec, ',')) {
        args = SoapySDRKwargs_fromString(spec);
    } else {
        args = SoapySDRKwargs_fromString("");
        SoapySDRKwargs_set(&args, "driver", spec);
    }
    // nchan is a Pocket SDR extension - do not pass it to SoapySDR
    *nch = 1;
    if ((p = SoapySDRKwargs_get(&args, "nchan"))) {
        *nch = atoi(p);
        SoapySDRKwargs_set(&args, "nchan", "");
    }
    if (*nch < 1) *nch = 1;
    p = SoapySDRKwargs_get(&args, "driver");
    snprintf(drv_name, size, "%.*s", size - 1, p ? p : spec);
    
    if (!strcmp(drv_name, "uhd")) {
        // B2xx USB transport buffers are sized by device args at make time
        // (explicit recv_frame_size breaks sc8/sc12 wire formats - do not set)
        // (Windows: bind B2xx to libusb0 driver - libusbK corrupts streaming)
        if (!SoapySDRKwargs_get(&args, "num_recv_frames")) {
            SoapySDRKwargs_set(&args, "num_recv_frames", "512");
        }
    }
    return args;
}

// make a SoapySDR device ------------------------------------------------------
static SoapySDRDevice *make_dev(SoapySDRKwargs *args)
{
    size_t nfound = 0;
    SoapySDRKwargs *found = SoapySDRDevice_enumerate(args, &nfound);
    SoapySDRDevice *dev;
    
    if (nfound > 0) {
        // enumerate results drop non-identification args
        for (size_t i = 0; i < args->size; i++) {
            SoapySDRKwargs_set(found, args->keys[i], args->vals[i]);
        }
        dev = SoapySDRDevice_make(found);
    } else {
        dev = SoapySDRDevice_make(args);
    }
    SoapySDRKwargsList_clear(found, nfound);
    return dev;
}

// setup RX channels of a device -----------------------------------------------
static int setup_chs(SoapySDRDevice *dev, int nch, double rate,
    const double *freq, double bw, double gain)
{
    for (int i = 0; i < nch; i++) {
        size_t ch = (size_t)i;
        SoapySDRDevice_setSampleRate(dev, SOAPY_SDR_RX, ch, rate);
        SoapySDRDevice_setFrequency(dev, SOAPY_SDR_RX, ch, freq[i], NULL);
        if (bw > 0.0) { // after rate set - rate change resets the BB filter
            (void)SoapySDRDevice_setBandwidth(dev, SOAPY_SDR_RX, ch, bw * 1e6);
        }
        if (gain > 0.0) {
            (void)SoapySDRDevice_setGainMode(dev, SOAPY_SDR_RX, ch, false);
            (void)SoapySDRDevice_setGain(dev, SOAPY_SDR_RX, ch, gain);
        } else {
            (void)SoapySDRDevice_setGainMode(dev, SOAPY_SDR_RX, ch, true);
        }
        (void)SoapySDRDevice_setDCOffsetMode(dev, SOAPY_SDR_RX, ch, true);
        (void)SoapySDRDevice_setIQBalanceMode(dev, SOAPY_SDR_RX, ch, true);
        
        double fs = SoapySDRDevice_getSampleRate(dev, SOAPY_SDR_RX, ch);
        double fo = SoapySDRDevice_getFrequency(dev, SOAPY_SDR_RX, ch);
        if (fabs(fs - rate) > RATE_TOL || fabs(fo - freq[i]) > FREQ_TOL) {
            return 0;
        }
    }
    return 1;
}

// output a value to the GPIO pins of a device ---------------------------------
static void set_gpio(SoapySDRDevice *dev, int idx, const char *drv_name,
    unsigned value)
{
    char bank[64] = "FP0", attr[80];
    int stat = 0;
    
    if (!strcmp(drv_name, "uhd")) { // USRP front-panel GPIO: ATR to GPIO mode
        snprintf(attr, sizeof(attr), "%s:CTRL", bank);
        stat |= SoapySDRDevice_writeGPIOMasked(dev, attr, 0, 0xFF);
    } else { // other drivers: the first GPIO bank
        size_t len = 0;
        char **banks = SoapySDRDevice_listGPIOBanks(dev, &len);
        if (len > 0) snprintf(bank, sizeof(bank), "%s", banks[0]);
        SoapySDRStrings_clear(&banks, len);
        if (len == 0) {
            fprintf(stderr, "sdev: dev%d no GPIO bank\n", idx);
            return;
        }
    }
    stat |= SoapySDRDevice_writeGPIODirMasked(dev, bank, 0xFF, 0xFF);
    stat |= SoapySDRDevice_writeGPIOMasked(dev, bank, value, 0xFF);
    if (stat) {
        fprintf(stderr, "sdev: dev%d GPIO write error: %s\n", idx,
            SoapySDRDevice_lastError());
        return;
    }
    fprintf(stderr, "sdev: dev%d GPIO=0x%02X (readback 0x%02X)\n", idx, value,
        SoapySDRDevice_readGPIO(dev, bank) & 0xFF);
}

// stream args for a driver ----------------------------------------------------
static const char *stream_args(const char *drv_name)
{
    if (!strcmp(drv_name, "lime")) { // LimeSDR
        return "linkFormat=CS12,latency=1";
    } else if (!strcmp(drv_name, "uhd")) { // USRP: 12-bit over-the-wire format
        return "WIRE=sc12";
    } else if (!strcmp(drv_name, "bladerf")) { // bladeRF
        return "buffers=128,transfers=32,buflen=32768,meta=normal";
    }
    return "";
}

// close all devices and free buffers ------------------------------------------
static void close_devs(sdr_sdev_t *sdev)
{
    for (int i = 0; i < sdev->ndev; i++) {
        SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev[i];
        if (!dev) continue;
        if (sdev->str[i]) {
            SoapySDRDevice_closeStream(dev, (SoapySDRStream *)sdev->str[i]);
        }
        SoapySDRDevice_unmake(dev);
    }
    for (int i = 0; i < sdev->nch; i++) {
        sdr_free(sdev->buff[i]);
    }
}

#endif // SOAPYSDR

//------------------------------------------------------------------------------
//  Open SoapySDR device.
//
//  args:
//      driver      (I)   SoapySDR driver or device specs separated by ';'
//                          "uhd"     : USRP
//                          "lime"    : LimeSDR
//                          "bladrf"  : bladeRF
//                          "rtlsdr"  : RTL-SDR
//                          "plutosdr": PlutoSDR
//                        A spec may be a SoapySDR key-value string. The key
//                        nchan=<n> selects the number of RX channels of the
//                        device (default: 1). Total number of RF channels of
//                        all devices has to be SDR_MAX_RFCH or less.
//                          "driver=uhd,serial=1;driver=uhd,serial=2"
//                          "driver=uhd,serial=1,nchan=2"
//      fmt         (I)   sampling data format (SDR_FMT_CS8 or _CS16)
//      rate        (I)   sampling rate (sps)
//      freq        (I)   carrier center frequency for each RF CH (Hz)
//                        (freq[i] <= 0: same as freq[0])
//      opt         (I)   device options separated by spaces ("": all defaults)
//                          -BW=bw      : filter bandwidth (MHz) (omit: not set)
//                          -GAIN=gain  : LNA gain (dB) (omit: AGC)
//                          -EXT_CLOCK  : enable external clock source
//                          -NO_SYNC    : no device time sync by 1PPS
//                          -GPIO=val   : output val (0-255) to GPIO pins
//
//  return
//      SoapySDR device pointer (NULL: error)
//
//  notes:
//      With multiple devices, the external clock and time sources are
//      selected and the streams are started at a common time latched by a
//      1PPS, so that the RF channels are aligned in samples. With -NO_SYNC,
//      the devices run on their own clocks and the RF channels of different
//      devices are NOT aligned in samples (for non-coherent uses).
//      -GPIO=val sets the front-panel GPIO pins (bank FP0) of a USRP, or the
//      first GPIO bank of other drivers, to outputs and drives val on them at
//      the device open, since UHD resets the pins at every open.
//
sdr_sdev_t *sdr_sdev_open(const char *driver, int fmt, double rate,
    const double *freq, const char *opt)
{
#ifdef SOAPYSDR
    char spec[SDR_MAX_SDEV][256], drv_name[32] = "";
    const char *str_fmt, *p;
    double fo[SDR_MAX_RFCH], bw = 0.0, gain = 0.0;
    int ndev, nch = 0, ext_clock = 0, no_sync = 0, gpio = -1;
    
    SoapySDR_registerLogHandler(soapy_log_handler);
    
    if (!opt) opt = "";
    if ((p = strstr(opt, "-BW="))) sscanf(p, "-BW=%lf", &bw);
    if ((p = strstr(opt, "-GAIN="))) sscanf(p, "-GAIN=%lf", &gain);
    ext_clock = strstr(opt, "-EXT_CLOCK") != NULL;
    no_sync = strstr(opt, "-NO_SYNC") != NULL;
    if ((p = strstr(opt, "-GPIO=")) && (sscanf(p, "-GPIO=%d", &gpio) < 1 ||
        gpio < 0 || gpio > 255)) {
        fprintf(stderr, "sdev: GPIO value out of range (0-255)\n");
        return NULL;
    }
    
    if (fmt != SDR_FMT_CS8 && fmt != SDR_FMT_CS16) {
        fprintf(stderr, "sdev: format error (only supports CS8 or CS16)\n");
        return NULL;
    }
    if (!(ndev = split_specs(driver, spec, SDR_MAX_SDEV))) {
        fprintf(stderr, "sdev: no device specified\n");
        return NULL;
    }
    sdr_sdev_t *sdev = (sdr_sdev_t *)sdr_malloc(sizeof(sdr_sdev_t));
    sdev->ndev = ndev;
    sdev->ssize = fmt == SDR_FMT_CS8 ? 2 : 4;
    sdev->rate = rate;
    sdr_mutex_init(&sdev->mtx);
    
    // multiple devices are only coherent with common clock and time sources
    sdev->sync = ndev >= 2 && !no_sync;
    if (sdev->sync) ext_clock = 1;
    
    for (int i = 0; i < ndev; i++) {
        SoapySDRDevice *dev = NULL;
        int dev_nch = 1, ok = 0;
        SoapySDRKwargs args = make_args(spec[i], drv_name, sizeof(drv_name),
            &dev_nch);
        
        if (nch + dev_nch > SDR_MAX_RFCH) {
            fprintf(stderr, "sdev: too many RF CHs (max %d)\n", SDR_MAX_RFCH);
            SoapySDRKwargs_clear(&args);
            close_devs(sdev);
            sdr_free(sdev);
            return NULL;
        }
        for (int j = 0; j < dev_nch; j++) {
            int k = nch + j;
            fo[k] = (freq[k] > 0.0) ? freq[k] : freq[0];
        }
        for (int retry = 0; retry < OPEN_RETRY; retry++) {
            if (!(dev = make_dev(&args))) {
                fprintf(stderr, "sdev: SoapySDRDevice_make error: %s\n",
                    SoapySDRDevice_lastError());
                break;
            }
            if (ext_clock) {
                if (SoapySDRDevice_setClockSource(dev, "external")) {
                    fprintf(stderr, "sdev: setClockSource error: %s\n",
                        SoapySDRDevice_lastError());
                }
                if (sdev->sync &&
                    SoapySDRDevice_setTimeSource(dev, "external")) {
                    fprintf(stderr, "sdev: setTimeSource error: %s\n",
                        SoapySDRDevice_lastError());
                }
            }
            if (setup_chs(dev, dev_nch, rate, fo + nch, bw, gain)) {
                ok = 1;
                break;
            }
            fprintf(stderr, "sdev: setup mismatch attempt %d/%d - retrying\n",
                retry + 1, OPEN_RETRY);
            SoapySDRDevice_unmake(dev);
            dev = NULL;
        }
        SoapySDRKwargs_clear(&args);
        if (!ok) {
            fprintf(stderr, "sdev: dev%d setup failed (%s)\n", i, spec[i]);
            if (dev) SoapySDRDevice_unmake(dev);
            close_devs(sdev);
            sdr_free(sdev);
            return NULL;
        }
        if (gpio >= 0) set_gpio(dev, i, drv_name, (unsigned)gpio);
        sdev->dev[i] = (void *)dev;
        sdev->dev_nch[i] = dev_nch;
        sdev->dev_ch0[i] = nch;
        nch += dev_nch;
        sdev->nch = nch;
        
        fprintf(stderr, "sdev: dev%d nch=%d rate=%.6fMsps (req %.3f)\n", i,
            dev_nch, SoapySDRDevice_getSampleRate(dev, SOAPY_SDR_RX, 0) * 1e-6,
            rate * 1e-6);
        for (int j = 0; j < dev_nch; j++) {
            fprintf(stderr, "sdev: dev%d ch%d LO=%.6fMHz (req %.3f)\n", i, j,
                SoapySDRDevice_getFrequency(dev, SOAPY_SDR_RX, (size_t)j) * 1e-6,
                fo[sdev->dev_ch0[i]+j] * 1e-6);
        }
        if (bw > 0.0) {
            fprintf(stderr, "sdev: dev%d BW=%.3fMHz (req %.3f)\n", i,
                SoapySDRDevice_getBandwidth(dev, SOAPY_SDR_RX, 0) * 1e-6, bw);
        }
    }
    snprintf(sdev->driver, sizeof(sdev->driver), "%s", drv_name);
    str_fmt = fmt == SDR_FMT_CS8 ? SOAPY_SDR_CS8 : SOAPY_SDR_CS16;
    
    for (int i = 0; i < ndev; i++) {
        size_t chs[SDR_MAX_RFCH];
        for (int j = 0; j < sdev->dev_nch[i]; j++) chs[j] = (size_t)j;
        SoapySDRKwargs args = SoapySDRKwargs_fromString(stream_args(drv_name));
        sdev->str[i] = (void *)SoapySDRDevice_setupStream(
            (SoapySDRDevice *)sdev->dev[i], SOAPY_SDR_RX, str_fmt, chs,
            (size_t)sdev->dev_nch[i], &args);
        SoapySDRKwargs_clear(&args);
        if (!sdev->str[i]) {
            fprintf(stderr, "sdev: SoapySDRDevice_setupStream error: %s\n",
                SoapySDRDevice_lastError());
            close_devs(sdev);
            sdr_free(sdev);
            return NULL;
        }
    }
    // share the total buffer size among the RF channels
    int cnt = BUFF_CNT_ALL / nch;
    if (cnt > BUFF_CNT) cnt = BUFF_CNT;
    sdev->bsize = (int64_t)BUFF_SIZE * cnt;
    
    for (int i = 0; i < nch; i++) {
        sdev->buff[i] = (uint8_t *)sdr_malloc(sdev->bsize);
        sdev->wp[i] = 0;
    }
    sdev->rp = 0;
    sdev->state = 0;
    fprintf(stderr, "sdev: ndev=%d nch=%d sync=%d buff=%dMB/CH\n", ndev, nch,
        sdev->sync, (int)(sdev->bsize >> 20));
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
    close_devs(sdev);
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
#else
    struct sched_param param = {0};
    param.sched_priority = 50;
    int err = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    if (err) {
        fprintf(stderr, "sdev: pthread_setschedparam error (%s) - grant rtprio "
            "to enable real-time RX thread\n", strerror(err));
    }
#endif
}
#endif

// write data (NULL: zeros) to a RF CH buffer ----------------------------------
#ifdef SOAPYSDR
static void write_buff(sdr_sdev_t *sdev, int ch, const uint8_t *data,
    int64_t size)
{
    int64_t pos = sdev->wp[ch] % sdev->bsize, off = 0, rem = size;
    
    while (rem > 0) {
        int64_t chunk = MIN(rem, sdev->bsize - pos);
        if (data) {
            memcpy(sdev->buff[ch] + pos, data + off, chunk);
        } else {
            memset(sdev->buff[ch] + pos, 0, chunk);
        }
        rem -= chunk;
        off += chunk;
        pos = (pos + chunk) % sdev->bsize;
    }
    sdr_mutex_lock(&sdev->mtx);
    sdev->wp[ch] += size;
    sdr_mutex_unlock(&sdev->mtx);
}
#endif

// Soapy device reader thread --------------------------------------------------
#ifdef SOAPYSDR
static void *reader_thread(void *arg)
{
    sdr_sdev_th_t *param = (sdr_sdev_th_t *)arg;
    sdr_sdev_t *sdev = param->sdev;
    int idev = param->idev, nch = sdev->dev_nch[idev];
    int ch0 = sdev->dev_ch0[idev], ssize = sdev->ssize;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev[idev];
    SoapySDRStream *str = (SoapySDRStream *)sdev->str[idev];
    uint8_t *data[SDR_MAX_RFCH];
    void *buffs[SDR_MAX_RFCH];
    int tns_ena = !strcmp(sdev->driver,"lime") || !strcmp(sdev->driver,"uhd");
    int ret, flags, ovcnt = 0, dropcnt = 0, gapcnt = 0, backcnt = 0;
    int64_t max_gap = (int64_t)(sdev->rate * GAP_MAX_SEC) * ssize;
    long long tns;
    uint32_t tick = sdr_get_tick();
#ifdef __linux__
    char name[16];
    snprintf(name, sizeof(name), "sdev_rd%d", idev); // thread name for top -H
    prctl(PR_SET_NAME, name);
#endif
    
    for (int i = 0; i < nch; i++) {
        data[i] = (uint8_t *)sdr_malloc(BUFF_SIZE);
        buffs[i] = (void *)data[i];
    }
    // raise process/thread priority
    raise_pri();

#ifdef WIN32 // raise timer resolution
    if (timeBeginPeriod(TIMER_RES)) {
        fprintf(stderr, "sdev: timeBeginPeriod (%d)\n", (int)GetLastError());
    }
#endif
    while (sdev->state > 0) {
        flags = 0;
        ret = SoapySDRDevice_readStream(dev, str, buffs, BUFF_SIZE / ssize,
            &flags, &tns, READ_TO);
        if (ret == SOAPY_SDR_TIMEOUT) continue;
        if (ret == SOAPY_SDR_OVERFLOW) {
            ovcnt++;
            continue;
        }
        if (ret == 0) {
            sdr_sleep_msec(1);
            continue;
        }
        if (ret < 0) {
            fprintf(stderr, "sdev: dev%d readStream error (%s) ts=%.3f\n", idev,
                SoapySDR_errToStr(ret), ts_elapsed(tick));
            sdev->state = -1;
            break;
        }
        if (flags & SOAPY_SDR_END_ABRUPT) {
            dropcnt++;
        }
        // locate the samples by the HW timestamp and zero-fill the gap
        int64_t gap = 0;
        if (tns_ena && (flags & SOAPY_SDR_HAS_TIME)) {
            if (sdev->tns0[idev] == 0) sdev->tns0[idev] = tns; // 1st block
            int64_t pos = (int64_t)((tns - sdev->tns0[idev]) * 1e-9 * sdev->rate +
                0.5) * ssize;
            gap = pos - sdev->wp[ch0];
            if (gap > 0) {
                if (gap > max_gap) gap = max_gap;
                if (sdev->wp[ch0] == 0) { // offset of the first samples
                    fprintf(stderr, "sdev: dev%d first samples at +%.3fus of "
                        "the start time (zero-fill)\n", idev,
                        gap / ssize / sdev->rate * 1e6);
                } else {
                    fprintf(stderr, "sdev: dev%d samples dropped ts=%.3fs "
                        "gap=%.3fms (zero-fill)\n", idev, ts_elapsed(tick),
                        gap / ssize / sdev->rate * 1e3);
                    gapcnt++;
                }
            } else if
 (gap < -(int64_t)ssize) { // timestamp went backwards
                if (!backcnt++) {
                    fprintf(stderr, "sdev: dev%d timestamp backward ts=%.3fs "
                        "(%.3fms)\n", idev, ts_elapsed(tick),
                        -gap / ssize / sdev->rate * 1e3);
                }
                gap = 0;
            } else {
                gap = 0;
            }
        }
        for (int i = 0; i < nch; i++) {
            if (gap > 0) write_buff(sdev, ch0 + i, NULL, gap);
            write_buff(sdev, ch0 + i, data[i], (int64_t)ret * ssize);
        }
    }
    if (ovcnt > 0 || dropcnt > 0 || gapcnt > 0 || backcnt > 0) {
        fprintf(stderr, "sdev: dev%d total ts=%.3fs overflow=%d dev-drop=%d "
            "drop=%d backward=%d\n", idev, ts_elapsed(tick), ovcnt, dropcnt,
            gapcnt, backcnt);
    }
    for (int i = 0; i < nch; i++) {
        sdr_free(data[i]);
    }
#ifdef WIN32
    timeEndPeriod(TIMER_RES);
#endif
    return NULL;
}
#endif

// align the device times to a 1PPS --------------------------------------------
#ifdef SOAPYSDR
static int sync_time(sdr_sdev_t *sdev)
{
    SoapySDRDevice *dev0 = (SoapySDRDevice *)sdev->dev[0];
    long long t0 = SoapySDRDevice_getHardwareTime(dev0, "PPS");
    uint32_t tick = sdr_get_tick();
    int ok = 1;
    
    // wait for a PPS edge to have a whole second to distribute the command
    while (SoapySDRDevice_getHardwareTime(dev0, "PPS") == t0) {
        if (sdr_get_tick() - tick > PPS_WAIT) {
            fprintf(stderr, "sdev: no PPS detected - check the 1PPS input\n");
            return 0;
        }
        sdr_sleep_msec(10);
    }
    for (int i = 0; i < sdev->ndev; i++) {
        if (SoapySDRDevice_setHardwareTime((SoapySDRDevice *)sdev->dev[i], 0,
                "PPS")) {
            fprintf(stderr, "sdev: dev%d setHardwareTime error: %s\n", i,
                SoapySDRDevice_lastError());
            return 0;
        }
    }
    sdr_sleep_msec(PPS_LATCH); // wait for the time latched at the next PPS
    
    long long tp0 = SoapySDRDevice_getHardwareTime(dev0, "PPS");
    for (int i = 1; i < sdev->ndev; i++) {
        long long tp = SoapySDRDevice_getHardwareTime(
            (SoapySDRDevice *)sdev->dev[i], "PPS");
        if (llabs(tp - tp0) > PPS_TOL) {
            fprintf(stderr, "sdev: dev%d PPS time unmatch %.6fs\n", i,
                (tp - tp0) * 1e-9);
            ok = 0;
        }
    }
    return ok;
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
    long long tstart = 0;
    
    if (!sdev || sdev->state) return 0;
    
    if (sdev->sync) { // align the streams of the devices by a 1PPS
        if (!sync_time(sdev)) {
            fprintf(stderr, "sdev: device time sync error\n");
            return 0;
        }
        long long now = SoapySDRDevice_getHardwareTime(
            (SoapySDRDevice *)sdev->dev[0], "");
        tstart = (now / 1000000000LL + START_SEC) * 1000000000LL;
        fprintf(stderr, "sdev: streams start at %.3fs of the device time\n",
            tstart * 1e-9);
    }
    for (int i = 0; i < sdev->nch; i++) {
        sdev->wp[i] = 0;
    }
    sdev->rp = 0;
    sdev->state = 1;
    
    for (int i = 0; i < sdev->ndev; i++) {
        SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev[i];
        long long t = 0;
        int flags = 0;
        
        if (sdev->sync) { // common start time latched by the 1PPS
            t = tstart;
            flags = SOAPY_SDR_HAS_TIME;
        } else if (sdev->dev_nch[i] >= 2) {
            // a multi-channel stream has to be started at a time to align the
            // channels (an immediate start is rejected by UHD)
            t = SoapySDRDevice_getHardwareTime(dev, "") + START_DELAY;
            flags = SOAPY_SDR_HAS_TIME;
        }
        sdev->tns0[i] = t; // 0: set by the first samples received
        if (SoapySDRDevice_activateStream(dev, (SoapySDRStream *)sdev->str[i],
                flags, t, 0)) {
            fprintf(stderr, "sdev: dev%d activateStream error: %s\n", i,
                SoapySDRDevice_lastError());
            sdev->state = 0;
            for (int j = 0; j < i; j++) {
                SoapySDRDevice_deactivateStream((SoapySDRDevice *)sdev->dev[j],
                    (SoapySDRStream *)sdev->str[j], 0, 0);
            }
            return 0;
        }
    }
    for (int i = 0; i < sdev->ndev; i++) {
        sdev->th[i].sdev = sdev;
        sdev->th[i].idev = i;
        if (!sdr_thread_create(sdev->thread + i, reader_thread, sdev->th + i)) {
            fprintf(stderr, "sdev: dev%d thread create error\n", i);
            sdev->state = 0;
            for (int j = 0; j < i; j++) sdr_thread_join(sdev->thread[j]);
            for (int j = 0; j < sdev->ndev; j++) {
                SoapySDRDevice_deactivateStream((SoapySDRDevice *)sdev->dev[j],
                    (SoapySDRStream *)sdev->str[j], 0, 0);
            }
            return 0;
        }
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
    sdev->state = 0;
    for (int i = 0; i < sdev->ndev; i++) {
        SoapySDRDevice_deactivateStream((SoapySDRDevice *)sdev->dev[i],
            (SoapySDRStream *)sdev->str[i], 0, 0);
    }
    for (int i = 0; i < sdev->ndev; i++) {
        sdr_thread_join(sdev->thread[i]);
    }
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
//      buff        (IO)  Sampling data buffer of each RF CH (buff[nch])
//      size        (I)   Sampling data buffer size of a RF CH (bytes)
//
//  return
//      read data size of a RF CH (bytes) (0: no data, -1: error)
//
//  notes:
//      The RF CHs are read at the same sample position, so the data of the
//      RF CHs are aligned in samples.
//
int sdr_sdev_read(sdr_sdev_t *sdev, uint8_t **buff, int size)
{
    int64_t wp, bsize;
    
    if (!sdev || sdev->state < 0) return -1;
    if (!sdev->state) return 0;
    
    sdr_mutex_lock(&sdev->mtx);
    wp = sdev->wp[0];
    for (int i = 1; i < sdev->nch; i++) {
        if (sdev->wp[i] < wp) wp = sdev->wp[i]; // align the RF CHs
    }
    sdr_mutex_unlock(&sdev->mtx);
    
    bsize = wp - sdev->rp;
    
    if (bsize > sdev->bsize) {
        fprintf(stderr, "sdev: buffer overflown\n");
        sdev->rp = wp;
        return 0;
    } else if (bsize < size) {
        return 0;
    }
    int pos = (int)(sdev->rp % sdev->bsize);
    
    for (int i = 0; i < sdev->nch; i++) {
        if (pos + size <= sdev->bsize) {
            memcpy(buff[i], sdev->buff[i] + pos, size);
        } else {
            int psize = (int)(sdev->bsize - pos);
            memcpy(buff[i], sdev->buff[i] + pos, psize);
            memcpy(buff[i] + psize, sdev->buff[i], size - psize);
        }
    }
    sdev->rp += size;
    return size;
}

//------------------------------------------------------------------------------
//  Read a sensor of a RF channel of SoapySDR device.
//
//  args:
//      sdev        (I)   SoapySDR device
//      ch          (I)   RF channel index (0 ~ nch-1)
//      name        (I)   sensor name as defined by the device ("temp", "rssi",
//                        "lo_locked", "ref_locked", ...)
//      value       (O)   sensor value (a boolean sensor as 1 or 0)
//
//  return
//      status (1:OK, 0:no such sensor or error)
//
int sdr_sdev_sensor(sdr_sdev_t *sdev, int ch, const char *name, double *value)
{
#ifdef SOAPYSDR
    if (!sdev || ch < 0 || ch >= sdev->nch || !name || !value) return 0;
    
    int idev = 0;
    while (idev + 1 < sdev->ndev && ch >= sdev->dev_ch0[idev+1]) idev++;
    SoapySDRDevice *dev = (SoapySDRDevice *)sdev->dev[idev];
    size_t dch = (size_t)(ch - sdev->dev_ch0[idev]), len = 0, i;
    
    // read only a listed sensor - an unknown one raises a driver exception
    // (RX channel sensors first, then device (mboard) sensors as a fallback)
    char *str = NULL;
    char **names = SoapySDRDevice_listChannelSensors(dev, SOAPY_SDR_RX, dch,
        &len);
    for (i = 0; i < len; i++) {
        if (!strcmp(names[i], name)) break;
    }
    SoapySDRStrings_clear(&names, len);
    if (i < len) {
        str = SoapySDRDevice_readChannelSensor(dev, SOAPY_SDR_RX, dch, name);
    }
    else {
        names = SoapySDRDevice_listSensors(dev, &len);
        for (i = 0; i < len; i++) {
            if (!strcmp(names[i], name)) break;
        }
        SoapySDRStrings_clear(&names, len);
        if (i >= len) return 0;
        str = SoapySDRDevice_readSensor(dev, name);
    }
    if (!str) return 0;
    if (!strcmp(str, "true")) {
        *value = 1.0;
    } else if (!strcmp(str, "false")) {
        *value = 0.0;
    } else {
        *value = atof(str);
    }
    SoapySDR_free(str);
    return 1;
#else
    return 0;
#endif
}
