//
//  Unit tests for the mocked sdr_dev.c API.
//
#include "test_sdr.h"

void test_sdr_dev_stub_push(sdr_dev_t *dev, const uint8_t *data, int size);

// open mock Pocket SDR device -------------------------------------------------
static sdr_dev_t *open_mock_dev(void)
{
    sdr_dev_t *dev = sdr_dev_open(-1, -1);
    
    TEST_ASSERT_TRUE(dev != NULL);
    TEST_ASSERT_TRUE(dev->buff != NULL);
    TEST_ASSERT_EQ_INT(0, dev->state);
    return dev;
}

// sdr_dev_open(), sdr_dev_close() --------------------------------------------
static void test_sdr_dev_open_close(void)
{
    sdr_dev_t *dev = open_mock_dev();
    
    sdr_dev_close(dev);
    
    // Edge case supported by the mock and useful for cleanup paths.
    sdr_dev_close(NULL);
}

// sdr_dev_start(), sdr_dev_stop() --------------------------------------------
static void test_sdr_dev_start_stop(void)
{
    sdr_dev_t *dev = open_mock_dev();
    
    TEST_ASSERT_EQ_INT(1, sdr_dev_start(dev));
    TEST_ASSERT_EQ_INT(1, dev->state);
    TEST_ASSERT_EQ_INT(0, dev->rp);
    TEST_ASSERT_EQ_INT(0, dev->wp);
    TEST_ASSERT_EQ_INT(0, sdr_dev_start(dev)); // already started
    
    TEST_ASSERT_EQ_INT(1, sdr_dev_stop(dev));
    TEST_ASSERT_EQ_INT(0, dev->state);
    TEST_ASSERT_EQ_INT(0, sdr_dev_stop(dev)); // already stopped
    
    TEST_ASSERT_EQ_INT(0, sdr_dev_start(NULL));
    TEST_ASSERT_EQ_INT(0, sdr_dev_stop(NULL));
    sdr_dev_close(dev);
}

// sdr_dev_read() --------------------------------------------------------------
static void test_sdr_dev_read(void)
{
    sdr_dev_t *dev = open_mock_dev();
    const uint8_t src[] = {1, 2, 3, 4};
    uint8_t dst[4] = {0};
    
    TEST_ASSERT_EQ_INT(-1, sdr_dev_read(NULL, dst, 1));
    TEST_ASSERT_EQ_INT(0, sdr_dev_read(dev, dst, 1)); // stopped device
    
    TEST_ASSERT_EQ_INT(1, sdr_dev_start(dev));
    TEST_ASSERT_EQ_INT(0, sdr_dev_read(dev, NULL, 1));
    TEST_ASSERT_EQ_INT(0, sdr_dev_read(dev, dst, 0));
    TEST_ASSERT_EQ_INT(0, sdr_dev_read(dev, dst, -1));
    TEST_ASSERT_EQ_INT(0, sdr_dev_read(dev, dst, 1)); // no data
    
    test_sdr_dev_stub_push(dev, src, sizeof(src));
    TEST_ASSERT_EQ_INT(2, sdr_dev_read(dev, dst, 2));
    TEST_ASSERT_EQ_INT(1, dst[0]);
    TEST_ASSERT_EQ_INT(2, dst[1]);
    TEST_ASSERT_EQ_INT(0, sdr_dev_read(dev, dst, 3)); // insufficient remainder
    TEST_ASSERT_EQ_INT(2, sdr_dev_read(dev, dst, 2));
    TEST_ASSERT_EQ_INT(3, dst[0]);
    TEST_ASSERT_EQ_INT(4, dst[1]);
    
    sdr_dev_stop(dev);
    sdr_dev_close(dev);
}

// sdr_dev_get_info() ----------------------------------------------------------
static void test_sdr_dev_get_info(void)
{
    sdr_dev_t *dev = open_mock_dev();
    int fmt = 0, IQ[SDR_MAX_RFCH] = {0}, bits[SDR_MAX_RFCH] = {0};
    double fs = 0.0, fo[SDR_MAX_RFCH] = {1.0};
    
    TEST_ASSERT_EQ_INT(2, sdr_dev_get_info(dev, &fmt, &fs, fo, IQ, bits));
    TEST_ASSERT_EQ_INT(SDR_FMT_RAW8, fmt);
    TEST_ASSERT_NEAR(24e6, fs, 1e-6);
    TEST_ASSERT_NEAR(0.0, fo[0], 1e-12);
    TEST_ASSERT_NEAR(0.0, fo[1], 1e-12);
    TEST_ASSERT_EQ_INT(2, IQ[0]);
    TEST_ASSERT_EQ_INT(2, IQ[1]);
    TEST_ASSERT_EQ_INT(4, bits[0]);
    TEST_ASSERT_EQ_INT(4, bits[1]);
    
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_info(NULL, &fmt, &fs, fo, IQ, bits));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_info(dev, NULL, &fs, fo, IQ, bits));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_info(dev, &fmt, NULL, fo, IQ, bits));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_info(dev, &fmt, &fs, NULL, IQ, bits));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_info(dev, &fmt, &fs, fo, NULL, bits));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_info(dev, &fmt, &fs, fo, IQ, NULL));
    
    sdr_dev_close(dev);
}

// sdr_dev_set_gain(), sdr_dev_get_gain() -------------------------------------
static void test_sdr_dev_gain(void)
{
    sdr_dev_t *dev = open_mock_dev();
    
    TEST_ASSERT_EQ_INT(1, sdr_dev_set_gain(dev, 0, 17));
    TEST_ASSERT_EQ_INT(17, sdr_dev_get_gain(dev, 0));
    TEST_ASSERT_EQ_INT(1, sdr_dev_set_gain(dev, 0, 0)); // AGC
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_gain(dev, 0));
    TEST_ASSERT_EQ_INT(1, sdr_dev_set_gain(dev, SDR_MAX_RFCH - 1, 5));
    TEST_ASSERT_EQ_INT(5, sdr_dev_get_gain(dev, SDR_MAX_RFCH - 1));
    
    TEST_ASSERT_EQ_INT(0, sdr_dev_set_gain(NULL, 0, 1));
    TEST_ASSERT_EQ_INT(0, sdr_dev_set_gain(dev, -1, 1));
    TEST_ASSERT_EQ_INT(0, sdr_dev_set_gain(dev, SDR_MAX_RFCH, 1));
    TEST_ASSERT_EQ_INT(-1, sdr_dev_get_gain(NULL, 0));
    TEST_ASSERT_EQ_INT(-1, sdr_dev_get_gain(dev, -1));
    TEST_ASSERT_EQ_INT(-1, sdr_dev_get_gain(dev, SDR_MAX_RFCH));
    
    sdr_dev_close(dev);
}

// sdr_dev_set_filt(), sdr_dev_get_filt() -------------------------------------
static void test_sdr_dev_filter(void)
{
    sdr_dev_t *dev = open_mock_dev();
    double bw = 0.0, freq = 0.0;
    int order = -1;
    
    TEST_ASSERT_EQ_INT(1, sdr_dev_set_filt(dev, 0, 2.5, 0.195, 1));
    TEST_ASSERT_EQ_INT(1, sdr_dev_get_filt(dev, 0, &bw, &freq, &order));
    TEST_ASSERT_NEAR(2.5, bw, 1e-12);
    TEST_ASSERT_NEAR(0.195, freq, 1e-12);
    TEST_ASSERT_EQ_INT(1, order);
    
    TEST_ASSERT_EQ_INT(1, sdr_dev_set_filt(dev, SDR_MAX_RFCH - 1, 8.7, 0.0, 0));
    TEST_ASSERT_EQ_INT(1, sdr_dev_get_filt(dev, SDR_MAX_RFCH - 1, &bw, &freq,
        &order));
    TEST_ASSERT_NEAR(8.7, bw, 1e-12);
    TEST_ASSERT_NEAR(0.0, freq, 1e-12);
    TEST_ASSERT_EQ_INT(0, order);
    
    TEST_ASSERT_EQ_INT(0, sdr_dev_set_filt(NULL, 0, 2.5, 0.0, 0));
    TEST_ASSERT_EQ_INT(0, sdr_dev_set_filt(dev, -1, 2.5, 0.0, 0));
    TEST_ASSERT_EQ_INT(0, sdr_dev_set_filt(dev, SDR_MAX_RFCH, 2.5, 0.0, 0));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_filt(NULL, 0, &bw, &freq, &order));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_filt(dev, -1, &bw, &freq, &order));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_filt(dev, SDR_MAX_RFCH, &bw, &freq,
        &order));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_filt(dev, 0, NULL, &freq, &order));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_filt(dev, 0, &bw, NULL, &order));
    TEST_ASSERT_EQ_INT(0, sdr_dev_get_filt(dev, 0, &bw, &freq, NULL));
    
    sdr_dev_close(dev);
}

// main ------------------------------------------------------------------------
int main(void)
{
    TEST_RUN(test_sdr_dev_open_close);
    TEST_RUN(test_sdr_dev_start_stop);
    TEST_RUN(test_sdr_dev_read);
    TEST_RUN(test_sdr_dev_get_info);
    TEST_RUN(test_sdr_dev_gain);
    TEST_RUN(test_sdr_dev_filter);
    return 0;
}
