//
//  Unit tests for the mocked sdr_sdev.c API.
//
#include "test_sdr.h"

void test_sdr_sdev_stub_push(sdr_sdev_t *sdev, const uint8_t *data, int size);

// open mock SoapySDR device ---------------------------------------------------
static sdr_sdev_t *open_mock_sdev(int fmt)
{
    sdr_sdev_t *sdev = sdr_sdev_open("mock", fmt, 1.0e6, 1.57542e9, 0.0, 0.0);
    
    TEST_ASSERT_TRUE(sdev != NULL);
    TEST_ASSERT_TRUE(sdev->buff != NULL);
    TEST_ASSERT_TRUE(!strcmp("mock", sdev->driver));
    TEST_ASSERT_NEAR(1.0e6, sdev->rate, 1e-12);
    TEST_ASSERT_EQ_INT(0, sdev->state);
    return sdev;
}

// sdr_sdev_list() -------------------------------------------------------------
static void test_sdr_sdev_list_api(void)
{
    TEST_ASSERT_EQ_INT(0, sdr_sdev_list());
}

// sdr_sdev_open(), sdr_sdev_close() ------------------------------------------
static void test_sdr_sdev_open_close(void)
{
    sdr_sdev_t *sdev;
    
    TEST_ASSERT_TRUE(sdr_sdev_open(NULL, SDR_FMT_CS8, 1.0, 0.0, 0.0, 0.0) == NULL);
    TEST_ASSERT_TRUE(sdr_sdev_open("mock", SDR_FMT_RAW8, 1.0, 0.0, 0.0, 0.0) == NULL);
    
    sdev = open_mock_sdev(SDR_FMT_CS8);
    TEST_ASSERT_EQ_INT(2, sdev->ssize);
    sdr_sdev_close(sdev);
    
    sdev = open_mock_sdev(SDR_FMT_CS16);
    TEST_ASSERT_EQ_INT(4, sdev->ssize);
    sdr_sdev_close(sdev);
    
    // Edge case supported by the mock and useful for cleanup paths.
    sdr_sdev_close(NULL);
}

// sdr_sdev_start(), sdr_sdev_stop() ------------------------------------------
static void test_sdr_sdev_start_stop(void)
{
    sdr_sdev_t *sdev = open_mock_sdev(SDR_FMT_CS8);
    
    TEST_ASSERT_EQ_INT(1, sdr_sdev_start(sdev));
    TEST_ASSERT_EQ_INT(1, sdev->state);
    TEST_ASSERT_EQ_INT(0, sdev->rp);
    TEST_ASSERT_EQ_INT(0, sdev->wp);
    TEST_ASSERT_EQ_INT(0, sdr_sdev_start(sdev)); // already started
    
    TEST_ASSERT_EQ_INT(1, sdr_sdev_stop(sdev));
    TEST_ASSERT_EQ_INT(0, sdev->state);
    TEST_ASSERT_EQ_INT(0, sdr_sdev_stop(sdev)); // already stopped
    
    TEST_ASSERT_EQ_INT(0, sdr_sdev_start(NULL));
    TEST_ASSERT_EQ_INT(0, sdr_sdev_stop(NULL));
    sdr_sdev_close(sdev);
}

// sdr_sdev_read() -------------------------------------------------------------
static void test_sdr_sdev_read(void)
{
    sdr_sdev_t *sdev = open_mock_sdev(SDR_FMT_CS8);
    const uint8_t src[] = {9, 8, 7, 6, 5};
    uint8_t dst[5] = {0};
    
    TEST_ASSERT_EQ_INT(-1, sdr_sdev_read(NULL, dst, 1));
    TEST_ASSERT_EQ_INT(0, sdr_sdev_read(sdev, dst, 1)); // stopped device
    
    TEST_ASSERT_EQ_INT(1, sdr_sdev_start(sdev));
    TEST_ASSERT_EQ_INT(0, sdr_sdev_read(sdev, NULL, 1));
    TEST_ASSERT_EQ_INT(0, sdr_sdev_read(sdev, dst, 0));
    TEST_ASSERT_EQ_INT(0, sdr_sdev_read(sdev, dst, -1));
    TEST_ASSERT_EQ_INT(0, sdr_sdev_read(sdev, dst, 1)); // no data
    
    test_sdr_sdev_stub_push(sdev, src, sizeof(src));
    TEST_ASSERT_EQ_INT(3, sdr_sdev_read(sdev, dst, 3));
    TEST_ASSERT_EQ_INT(9, dst[0]);
    TEST_ASSERT_EQ_INT(8, dst[1]);
    TEST_ASSERT_EQ_INT(7, dst[2]);
    TEST_ASSERT_EQ_INT(0, sdr_sdev_read(sdev, dst, 3)); // insufficient remainder
    TEST_ASSERT_EQ_INT(2, sdr_sdev_read(sdev, dst, 2));
    TEST_ASSERT_EQ_INT(6, dst[0]);
    TEST_ASSERT_EQ_INT(5, dst[1]);
    
    sdev->state = -1; // mock device error state
    TEST_ASSERT_EQ_INT(-1, sdr_sdev_read(sdev, dst, 1));
    sdr_sdev_close(sdev);
}

// main ------------------------------------------------------------------------
int main(void)
{
    TEST_RUN(test_sdr_sdev_list_api);
    TEST_RUN(test_sdr_sdev_open_close);
    TEST_RUN(test_sdr_sdev_start_stop);
    TEST_RUN(test_sdr_sdev_read);
    return 0;
}
