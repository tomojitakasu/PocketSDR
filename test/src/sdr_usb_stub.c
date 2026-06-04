//
//  Mock USB device API for unit tests.
//
#include "test_sdr.h"

static int stub_usb_req_status = 0;
static int stub_usb_save_count = 0;
static uint8_t stub_usb_stat[6] = {0};
static uint8_t stub_usb_regs[SDR_MAX_RFCH][SDR_MAX_REG][4] = {{{0}}};

void test_sdr_usb_stub_reset(void)
{
    stub_usb_req_status = 0;
    stub_usb_save_count = 0;
    memset(stub_usb_stat, 0, sizeof(stub_usb_stat));
    memset(stub_usb_regs, 0, sizeof(stub_usb_regs));
}

void test_sdr_usb_stub_set_req_status(int status)
{
    stub_usb_req_status = status;
}

void test_sdr_usb_stub_set_stat(const uint8_t stat[6])
{
    memcpy(stub_usb_stat, stat, sizeof(stub_usb_stat));
}

void test_sdr_usb_stub_set_reg(int ch, int addr, const uint8_t data[4])
{
    if (ch < 0 || ch >= SDR_MAX_RFCH || addr < 0 || addr >= SDR_MAX_REG) return;
    memcpy(stub_usb_regs[ch][addr], data, 4);
}

void test_sdr_usb_stub_get_reg(int ch, int addr, uint8_t data[4])
{
    if (ch < 0 || ch >= SDR_MAX_RFCH || addr < 0 || addr >= SDR_MAX_REG) {
        memset(data, 0, 4);
        return;
    }
    memcpy(data, stub_usb_regs[ch][addr], 4);
}

int test_sdr_usb_stub_save_count(void)
{
    return stub_usb_save_count;
}

sdr_usb_t *sdr_usb_open(int bus, int port, const uint16_t *vid,
    const uint16_t *pid, int n)
{
    (void)bus;
    (void)port;
    (void)vid;
    (void)pid;
    (void)n;
    sdr_usb_t *usb = (sdr_usb_t *)sdr_malloc(sizeof(sdr_usb_t));
    memset(usb, 0, sizeof(*usb));
    return usb;
}

void sdr_usb_close(sdr_usb_t *usb)
{
    sdr_free(usb);
}

int sdr_usb_req(sdr_usb_t *usb, int mode, uint8_t req, uint16_t val,
    uint8_t *data, int size)
{
    int ch = (val >> 8) & 0xFF;
    int addr = val & 0xFF;
    
    if (!usb || size < 0 || size > 64 || !stub_usb_req_status) return 0;
    
    if (!mode && req == SDR_VR_STAT && data && size >= 6) {
        memcpy(data, stub_usb_stat, 6);
        return 1;
    }
    else if (req == SDR_VR_REG_READ && data && size >= 4 &&
        ch >= 0 && ch < SDR_MAX_RFCH && addr >= 0 && addr < SDR_MAX_REG) {
        memcpy(data, stub_usb_regs[ch][addr], 4);
        return 1;
    }
    else if (req == SDR_VR_REG_WRITE && data && size >= 4 &&
        ch >= 0 && ch < SDR_MAX_RFCH && addr >= 0 && addr < SDR_MAX_REG) {
        memcpy(stub_usb_regs[ch][addr], data, 4);
        return 1;
    }
    else if (req == SDR_VR_START || req == SDR_VR_STOP || req == SDR_VR_RESET ||
        req == SDR_VR_SAVE) {
        if (req == SDR_VR_SAVE && size == 0) stub_usb_save_count++;
        return size == 0;
    }
    return 0;
}
