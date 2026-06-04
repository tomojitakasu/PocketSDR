//
//  Unit tests for the mocked sdr_usb.c API.
//
#include "test_sdr.h"

void test_sdr_usb_stub_reset(void);
void test_sdr_usb_stub_set_req_status(int status);
void test_sdr_usb_stub_set_stat(const uint8_t stat[6]);
void test_sdr_usb_stub_set_reg(int ch, int addr, const uint8_t data[4]);

// open mock USB device --------------------------------------------------------
static sdr_usb_t *open_mock_usb(void)
{
    const uint16_t vid[2] = {SDR_DEV_VID, SDR_DEV_VID};
    const uint16_t pid[2] = {SDR_DEV_PID1, SDR_DEV_PID2};
    sdr_usb_t *usb = sdr_usb_open(-1, -1, vid, pid, 2);
    
    TEST_ASSERT_TRUE(usb != NULL);
    return usb;
}

// sdr_usb_open(), sdr_usb_close() --------------------------------------------
static void test_sdr_usb_open_close(void)
{
    sdr_usb_t *usb;
    
    test_sdr_usb_stub_reset();
    usb = open_mock_usb();
    sdr_usb_close(usb);
    
    // Edge case supported by the mock and useful for cleanup paths.
    sdr_usb_close(NULL);
}

// sdr_usb_req(): status request ----------------------------------------------
static void test_sdr_usb_req_status(void)
{
    const uint8_t stat_set[6] = {0x30, 0x00, 0x18, 0x02, 0x03, 0x04};
    uint8_t stat_get[6] = {0};
    sdr_usb_t *usb;
    
    test_sdr_usb_stub_reset();
    usb = open_mock_usb();
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_STAT, 0, stat_get, 6));
    
    test_sdr_usb_stub_set_stat(stat_set);
    test_sdr_usb_stub_set_req_status(1);
    TEST_ASSERT_EQ_INT(1, sdr_usb_req(usb, 0, SDR_VR_STAT, 0, stat_get, 6));
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQ_INT(stat_set[i], stat_get[i]);
    }
    
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(NULL, 0, SDR_VR_STAT, 0, stat_get, 6));
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_STAT, 0, stat_get, 65));
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_STAT, 0, stat_get, -1));
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_STAT, 0, NULL, 6));
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_STAT, 0, stat_get, 5));
    
    sdr_usb_close(usb);
}

// sdr_usb_req(): register read/write -----------------------------------------
static void test_sdr_usb_req_register(void)
{
    const uint8_t reg_set[4] = {0x12, 0x34, 0x56, 0x78};
    const uint8_t reg_preload[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t reg_get[4] = {0};
    sdr_usb_t *usb;
    
    test_sdr_usb_stub_reset();
    test_sdr_usb_stub_set_req_status(1);
    usb = open_mock_usb();
    
    TEST_ASSERT_EQ_INT(1, sdr_usb_req(usb, 1, SDR_VR_REG_WRITE, (2 << 8) + 3,
        (uint8_t *)reg_set, 4));
    TEST_ASSERT_EQ_INT(1, sdr_usb_req(usb, 0, SDR_VR_REG_READ, (2 << 8) + 3,
        reg_get, 4));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQ_INT(reg_set[i], reg_get[i]);
    }
    
    test_sdr_usb_stub_set_reg(1, 2, reg_preload);
    memset(reg_get, 0, sizeof(reg_get));
    TEST_ASSERT_EQ_INT(1, sdr_usb_req(usb, 0, SDR_VR_REG_READ, (1 << 8) + 2,
        reg_get, 4));
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQ_INT(reg_preload[i], reg_get[i]);
    }
    
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_REG_READ,
        (SDR_MAX_RFCH << 8), reg_get, 4));
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_REG_READ, SDR_MAX_REG,
        reg_get, 4));
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_REG_READ, 0, NULL, 4));
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 1, SDR_VR_REG_WRITE, 0,
        (uint8_t *)reg_set, 3));
    
    sdr_usb_close(usb);
}

// sdr_usb_req(): control requests --------------------------------------------
static void test_sdr_usb_req_control(void)
{
    uint8_t dummy[1] = {0};
    sdr_usb_t *usb;
    
    test_sdr_usb_stub_reset();
    test_sdr_usb_stub_set_req_status(1);
    usb = open_mock_usb();
    
    TEST_ASSERT_EQ_INT(1, sdr_usb_req(usb, 0, SDR_VR_START, 0, NULL, 0));
    TEST_ASSERT_EQ_INT(1, sdr_usb_req(usb, 0, SDR_VR_STOP, 0, NULL, 0));
    TEST_ASSERT_EQ_INT(1, sdr_usb_req(usb, 0, SDR_VR_RESET, 0, NULL, 0));
    TEST_ASSERT_EQ_INT(1, sdr_usb_req(usb, 1, SDR_VR_SAVE, 0, NULL, 0));
    
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_START, 0, dummy, 1));
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, 0xFF, 0, NULL, 0));
    
    test_sdr_usb_stub_set_req_status(0);
    TEST_ASSERT_EQ_INT(0, sdr_usb_req(usb, 0, SDR_VR_START, 0, NULL, 0));
    
    sdr_usb_close(usb);
}

// main ------------------------------------------------------------------------
int main(void)
{
    TEST_RUN(test_sdr_usb_open_close);
    TEST_RUN(test_sdr_usb_req_status);
    TEST_RUN(test_sdr_usb_req_register);
    TEST_RUN(test_sdr_usb_req_control);
    return 0;
}
