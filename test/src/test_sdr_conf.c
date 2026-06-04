//
//  Unit tests for sdr_conf.c.
//
#include "test_sdr.h"

void test_sdr_usb_stub_reset(void);
void test_sdr_usb_stub_set_req_status(int status);
void test_sdr_usb_stub_set_stat(const uint8_t stat[6]);
void test_sdr_usb_stub_set_reg(int ch, int addr, const uint8_t data[4]);
void test_sdr_usb_stub_get_reg(int ch, int addr, uint8_t data[4]);
int test_sdr_usb_stub_save_count(void);

#define TMP_CONF_KEY "test_sdr_conf_key.tmp"
#define TMP_CONF_HEX "test_sdr_conf_hex.tmp"
#define TMP_CONF_IN  "test_sdr_conf_in.tmp"

// set mock register as uint32 -------------------------------------------------
static void set_reg_u32(int ch, int addr, uint32_t val)
{
    uint8_t data[4];
    
    data[0] = (uint8_t)(val >> 24);
    data[1] = (uint8_t)(val >> 16);
    data[2] = (uint8_t)(val >> 8);
    data[3] = (uint8_t)val;
    test_sdr_usb_stub_set_reg(ch, addr, data);
}

// get mock register as uint32 -------------------------------------------------
static uint32_t get_reg_u32(int ch, int addr)
{
    uint8_t data[4];
    
    test_sdr_usb_stub_get_reg(ch, addr, data);
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

// write text file -------------------------------------------------------------
static void write_text_file(const char *file, const char *text)
{
    FILE *fp = fopen(file, "w");
    
    TEST_ASSERT_TRUE(fp != NULL);
    TEST_ASSERT_TRUE(fputs(text, fp) >= 0);
    TEST_ASSERT_EQ_INT(0, fclose(fp));
}

// assert file contains text ---------------------------------------------------
static void assert_file_contains(const char *file, const char *text)
{
    FILE *fp = fopen(file, "r");
    char buff[512];
    int found = 0;
    
    TEST_ASSERT_TRUE(fp != NULL);
    while (fgets(buff, sizeof(buff), fp)) {
        if (strstr(buff, text)) {
            found = 1;
            break;
        }
    }
    fclose(fp);
    TEST_ASSERT_TRUE(found);
}

// setup mocked Pocket SDR 4CH registers ---------------------------------------
static void setup_pocket_4ch(sdr_dev_t *dev)
{
    const uint8_t stat[6] = {0x30, 0x5D, 0xC0, 0x00, 0x00, 0x00};
    
    memset(dev, 0, sizeof(*dev));
    dev->usb = sdr_usb_open(-1, -1, NULL, NULL, 0);
    test_sdr_usb_stub_reset();
    test_sdr_usb_stub_set_stat(stat);
    test_sdr_usb_stub_set_req_status(1);
    
    for (int ch = 0; ch < 4; ch++) {
        set_reg_u32(ch, 4, (100u << 13) | (1u << 3));
    }
}

// close mocked device and reset registers -------------------------------------
static void teardown_dev(sdr_dev_t *dev)
{
    sdr_usb_close(dev->usb);
    dev->usb = NULL;
}

// test sdr_conf_read() keyword mode -------------------------------------------
static void test_sdr_conf_read_key_api(void)
{
    sdr_dev_t dev;
    
    remove(TMP_CONF_KEY);
    setup_pocket_4ch(&dev);
    
    TEST_ASSERT_EQ_INT(1, sdr_conf_read(&dev, TMP_CONF_KEY, 0));
    assert_file_contains(TMP_CONF_KEY, "Pocket SDR device settings");
    assert_file_contains(TMP_CONF_KEY, "[CH1]");
    assert_file_contains(TMP_CONF_KEY, "GAININ");
    
    teardown_dev(&dev);
    remove(TMP_CONF_KEY);
}

// test sdr_conf_read() hexadecimal mode ---------------------------------------
static void test_sdr_conf_read_hex_api(void)
{
    sdr_dev_t dev;
    
    remove(TMP_CONF_HEX);
    setup_pocket_4ch(&dev);
    set_reg_u32(0, 1, 0x12345678u);
    
    TEST_ASSERT_EQ_INT(1, sdr_conf_read(&dev, TMP_CONF_HEX, 4));
    assert_file_contains(TMP_CONF_HEX, "#CH");
    assert_file_contains(TMP_CONF_HEX, "  1  0x01  0x12345678");
    
    teardown_dev(&dev);
    remove(TMP_CONF_HEX);
}

// test sdr_conf_read() error cases --------------------------------------------
static void test_sdr_conf_read_error_api(void)
{
    sdr_dev_t dev;
    
    memset(&dev, 0, sizeof(dev));
    dev.usb = sdr_usb_open(-1, -1, NULL, NULL, 0);
    test_sdr_usb_stub_reset();
    TEST_ASSERT_EQ_INT(0, sdr_conf_read(&dev, TMP_CONF_KEY, 0));
    
    teardown_dev(&dev);
    remove(TMP_CONF_KEY);
}

// test sdr_conf_write() keyword mode ------------------------------------------
static void test_sdr_conf_write_key_api(void)
{
    sdr_dev_t dev;
    uint32_t reg0, reg2;
    
    write_text_file(TMP_CONF_IN,
        "# key format input\n"
        "[CH1]\n"
        "GAININ = 7\n"
        "[CH2]\n"
        "GAININ = 9\n"
        "[CHALL]\n"
        "FCEN = 5\n");
    
    setup_pocket_4ch(&dev);
    
    TEST_ASSERT_EQ_INT(1, sdr_conf_write(&dev, TMP_CONF_IN, 1));
    TEST_ASSERT_EQ_INT(1, test_sdr_usb_stub_save_count());
    
    reg2 = get_reg_u32(0, 2);
    TEST_ASSERT_EQ_INT(7, (reg2 >> 22) & 0x3F);
    
    reg2 = get_reg_u32(1, 2);
    TEST_ASSERT_EQ_INT(9, (reg2 >> 22) & 0x3F);
    
    for (int ch = 0; ch < 4; ch++) {
        reg0 = get_reg_u32(ch, 0);
        TEST_ASSERT_EQ_INT(5, (reg0 >> 6) & 0x7F);
    }
    
    teardown_dev(&dev);
    remove(TMP_CONF_IN);
}

// test sdr_conf_write() hexadecimal mode --------------------------------------
static void test_sdr_conf_write_hex_api(void)
{
    sdr_dev_t dev;
    
    write_text_file(TMP_CONF_IN,
        "# hex format input\n"
        "1 0x02 0x01400000\n"
        "2 0x02 0x02400000\n"
        "99 0x02 0xFFFFFFFF\n");
    
    setup_pocket_4ch(&dev);
    
    TEST_ASSERT_EQ_INT(1, sdr_conf_write(&dev, TMP_CONF_IN, 4));
    TEST_ASSERT_TRUE(get_reg_u32(0, 2) == 0x01400000u);
    TEST_ASSERT_TRUE(get_reg_u32(1, 2) == 0x02400000u);
    
    teardown_dev(&dev);
    remove(TMP_CONF_IN);
}

// test sdr_conf_write() error cases -------------------------------------------
static void test_sdr_conf_write_error_api(void)
{
    sdr_dev_t dev;
    
    setup_pocket_4ch(&dev);
    TEST_ASSERT_EQ_INT(0, sdr_conf_write(&dev, "no_such_sdr_conf_file.tmp", 0));
    teardown_dev(&dev);
    
    memset(&dev, 0, sizeof(dev));
    dev.usb = sdr_usb_open(-1, -1, NULL, NULL, 0);
    test_sdr_usb_stub_reset();
    write_text_file(TMP_CONF_IN, "[CH1]\nGAININ = 1\n");
    TEST_ASSERT_EQ_INT(0, sdr_conf_write(&dev, TMP_CONF_IN, 0));
    
    teardown_dev(&dev);
    remove(TMP_CONF_IN);
}

// main ------------------------------------------------------------------------
int main(void)
{
    TEST_RUN(test_sdr_conf_read_key_api);
    TEST_RUN(test_sdr_conf_read_hex_api);
    TEST_RUN(test_sdr_conf_read_error_api);
    TEST_RUN(test_sdr_conf_write_key_api);
    TEST_RUN(test_sdr_conf_write_hex_api);
    TEST_RUN(test_sdr_conf_write_error_api);
    
    return 0;
}
