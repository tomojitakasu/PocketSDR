//
//  Mock Pocket SDR front-end device API for unit tests.
//
#include "test_sdr.h"

static int stub_gain[SDR_MAX_RFCH] = {0};
static double stub_bw[SDR_MAX_RFCH] = {0};
static double stub_freq[SDR_MAX_RFCH] = {0};
static int stub_order[SDR_MAX_RFCH] = {0};

void test_sdr_dev_stub_reset(void)
{
    memset(stub_gain, 0, sizeof(stub_gain));
    memset(stub_bw, 0, sizeof(stub_bw));
    memset(stub_freq, 0, sizeof(stub_freq));
    memset(stub_order, 0, sizeof(stub_order));
}

void test_sdr_dev_stub_push(sdr_dev_t *dev, const uint8_t *data, int size)
{
    if (!dev || !data || size <= 0 || size > SDR_SIZE_UBUFF) return;
    memcpy(dev->buff, data, size);
    dev->rp = 0;
    dev->wp = size;
}

sdr_dev_t *sdr_dev_open(int bus, int port)
{
    (void)bus;
    (void)port;
    sdr_dev_t *dev = (sdr_dev_t *)sdr_malloc(sizeof(sdr_dev_t));
    memset(dev, 0, sizeof(*dev));
    dev->buff = (uint8_t *)sdr_malloc(SDR_SIZE_UBUFF);
    sdr_mutex_init(&dev->mtx);
    test_sdr_dev_stub_reset();
    return dev;
}

void sdr_dev_close(sdr_dev_t *dev)
{
    if (!dev) return;
    sdr_free(dev->buff);
    sdr_free(dev);
}

int sdr_dev_start(sdr_dev_t *dev)
{
    if (!dev || dev->state) return 0;
    dev->state = 1;
    dev->rp = dev->wp = 0;
    return 1;
}

int sdr_dev_stop(sdr_dev_t *dev)
{
    if (!dev || !dev->state) return 0;
    dev->state = 0;
    return 1;
}

int sdr_dev_read(sdr_dev_t *dev, uint8_t *buff, int size)
{
    if (!dev) return -1;
    if (!dev->state || !buff || size <= 0) return 0;
    if (dev->wp < dev->rp + size) return 0;
    memcpy(buff, dev->buff + dev->rp, size);
    dev->rp += size;
    return size;
}

int sdr_dev_get_info(sdr_dev_t *dev, int *fmt, double *fs, double *fo, int *IQ,
    int *bits)
{
    if (!dev || !fmt || !fs || !fo || !IQ || !bits) return 0;
    *fmt = SDR_FMT_RAW8;
    *fs = 24e6;
    for (int i = 0; i < 2; i++) {
        fo[i] = 0.0;
        IQ[i] = 2;
        bits[i] = 4;
    }
    return 2;
}

int sdr_dev_get_gain(sdr_dev_t *dev, int ch)
{
    return dev && ch >= 0 && ch < SDR_MAX_RFCH ? stub_gain[ch] : -1;
}

int sdr_dev_set_gain(sdr_dev_t *dev, int ch, int gain)
{
    if (!dev || ch < 0 || ch >= SDR_MAX_RFCH) return 0;
    stub_gain[ch] = gain > 0 ? gain : 0;
    return 1;
}

int sdr_dev_get_filt(sdr_dev_t *dev, int ch, double *bw, double *freq,
    int *order)
{
    if (!dev || ch < 0 || ch >= SDR_MAX_RFCH || !bw || !freq || !order) return 0;
    *bw = stub_bw[ch];
    *freq = stub_freq[ch];
    *order = stub_order[ch];
    return 1;
}

int sdr_dev_set_filt(sdr_dev_t *dev, int ch, double bw, double freq, int order)
{
    if (!dev || ch < 0 || ch >= SDR_MAX_RFCH) return 0;
    stub_bw[ch] = bw;
    stub_freq[ch] = freq;
    stub_order[ch] = order;
    return 1;
}
