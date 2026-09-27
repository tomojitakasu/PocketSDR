//
//  Mock SoapySDR device API for unit tests.
//
#include "test_sdr.h"

#define SDEV_STUB_BUFF_SIZE 4096

void test_sdr_sdev_stub_push(sdr_sdev_t *sdev, const uint8_t *data, int size)
{
    if (!sdev || !data || size <= 0 || size > SDEV_STUB_BUFF_SIZE) return;
    for (int i = 0; i < sdev->nch; i++) {
        memcpy(sdev->buff[i], data, size);
        sdev->wp[i] = size;
    }
    sdev->rp = 0;
}

int sdr_sdev_list(void)
{
    return 0;
}

sdr_sdev_t *sdr_sdev_open(const char *driver, int fmt, double rate,
    const double *freq, const char *opt)
{
    const char *p;
    int nch = 1;
    
    (void)freq;
    if (!driver || (fmt != SDR_FMT_CS8 && fmt != SDR_FMT_CS16)) return NULL;
    if (opt && (p = strstr(opt, "-NCH="))) sscanf(p, "-NCH=%d", &nch);
    if (nch < 1 || nch > SDR_MAX_RFCH) return NULL;
    
    sdr_sdev_t *sdev = (sdr_sdev_t *)sdr_malloc(sizeof(sdr_sdev_t));
    memset(sdev, 0, sizeof(*sdev));
    snprintf(sdev->driver, sizeof(sdev->driver), "%s", driver);
    sdev->ndev = 1;
    sdev->nch = nch;
    sdev->dev_nch[0] = nch;
    sdev->dev_ch0[0] = 0;
    sdev->rate = rate;
    sdev->ssize = (fmt == SDR_FMT_CS16) ? 4 : 2;
    sdev->bsize = SDEV_STUB_BUFF_SIZE;
    for (int i = 0; i < nch; i++) {
        sdev->buff[i] = (uint8_t *)sdr_malloc(SDEV_STUB_BUFF_SIZE);
    }
    sdr_mutex_init(&sdev->mtx);
    return sdev;
}

void sdr_sdev_close(sdr_sdev_t *sdev)
{
    if (!sdev) return;
    for (int i = 0; i < sdev->nch; i++) {
        sdr_free(sdev->buff[i]);
    }
    sdr_free(sdev);
}

int sdr_sdev_start(sdr_sdev_t *sdev)
{
    if (!sdev || sdev->state) return 0;
    sdev->state = 1;
    sdev->rp = 0;
    for (int i = 0; i < sdev->nch; i++) {
        sdev->wp[i] = 0;
    }
    return 1;
}

int sdr_sdev_stop(sdr_sdev_t *sdev)
{
    if (!sdev || !sdev->state) return 0;
    sdev->state = 0;
    return 1;
}

int sdr_sdev_read(sdr_sdev_t *sdev, uint8_t **buff, int size)
{
    if (!sdev) return -1;
    if (sdev->state < 0) return -1;
    if (!sdev->state || !buff || size <= 0) return 0;
    
    int64_t wp = sdev->wp[0];
    for (int i = 1; i < sdev->nch; i++) {
        if (sdev->wp[i] < wp) wp = sdev->wp[i];
    }
    if (wp < sdev->rp + size) return 0;
    for (int i = 0; i < sdev->nch; i++) {
        if (!buff[i]) return 0;
        memcpy(buff[i], sdev->buff[i] + sdev->rp, size);
    }
    sdev->rp += size;
    return size;
}

int sdr_sdev_sensor(sdr_sdev_t *sdev, int ch, const char *name, double *value)
{
    if (!sdev || ch < 0 || ch >= sdev->nch || !name || !value) return 0;
    if (!strcmp(name, "temp")) {
        *value = 40.0 + ch; // mock temperature (C)
        return 1;
    }
    if (!strcmp(name, "lo_locked")) {
        *value = 1.0;
        return 1;
    }
    if (!strcmp(name, "ref_locked")) { // mock device (mboard) sensor
        *value = 1.0;
        return 1;
    }
    return 0;
}
