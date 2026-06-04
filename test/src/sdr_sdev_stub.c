//
//  Mock SoapySDR device API for unit tests.
//
#include "test_sdr.h"

#define SDEV_STUB_BUFF_SIZE 4096

void test_sdr_sdev_stub_push(sdr_sdev_t *sdev, const uint8_t *data, int size)
{
    if (!sdev || !data || size <= 0 || size > SDEV_STUB_BUFF_SIZE) return;
    memcpy(sdev->buff, data, size);
    sdev->rp = 0;
    sdev->wp = size;
}

int sdr_sdev_list(void)
{
    return 0;
}

sdr_sdev_t *sdr_sdev_open(const char *driver, int fmt, double rate,
    double freq, double bw, double gain)
{
    (void)freq;
    (void)bw;
    (void)gain;
    if (!driver || (fmt != SDR_FMT_CS8 && fmt != SDR_FMT_CS16)) return NULL;
    
    sdr_sdev_t *sdev = (sdr_sdev_t *)sdr_malloc(sizeof(sdr_sdev_t));
    memset(sdev, 0, sizeof(*sdev));
    snprintf(sdev->driver, sizeof(sdev->driver), "%s", driver);
    sdev->rate = rate;
    sdev->ssize = (fmt == SDR_FMT_CS16) ? 4 : 2;
    sdev->buff = (uint8_t *)sdr_malloc(SDEV_STUB_BUFF_SIZE);
    sdr_mutex_init(&sdev->mtx);
    return sdev;
}

void sdr_sdev_close(sdr_sdev_t *sdev)
{
    if (!sdev) return;
    sdr_free(sdev->buff);
    sdr_free(sdev);
}

int sdr_sdev_start(sdr_sdev_t *sdev)
{
    if (!sdev || sdev->state) return 0;
    sdev->state = 1;
    sdev->rp = sdev->wp = 0;
    return 1;
}

int sdr_sdev_stop(sdr_sdev_t *sdev)
{
    if (!sdev || !sdev->state) return 0;
    sdev->state = 0;
    return 1;
}

int sdr_sdev_read(sdr_sdev_t *sdev, uint8_t *buff, int size)
{
    if (!sdev) return -1;
    if (sdev->state < 0) return -1;
    if (!sdev->state || !buff || size <= 0) return 0;
    if (sdev->wp < sdev->rp + size) return 0;
    memcpy(buff, sdev->buff + sdev->rp, size);
    sdev->rp += size;
    return size;
}
