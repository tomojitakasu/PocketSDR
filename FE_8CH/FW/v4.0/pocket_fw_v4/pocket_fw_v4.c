//
//  Pocket SDR - Pocket SDR FE 8CH firmware for EZ-USB FX3
//
//  References:
//
//  [1] Cypress, EZ-USB FX3 Technical Reference Manual, Rev.F, May 9, 2019
//  [2] Cypress, FX3 Programmers Manual, Rev.K, 2018
//  [3] Cypress, EZ-USB FX3 SDK Firmware API Guide Version 1.3.5, 2023
//  [4] maxim integrated, MAX2771 Multiband Universal GNSS Receiver, Rev 0
//
//  Options:
//
//  Author:
//  T.TAKASU
//
//  History:
//  2024-11-15  1.0  new
//
#include <stdio.h>
#include <stdint.h>
#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3i2c.h"
#include "cyu3gpio.h"
#include "cyu3pib.h"
#include "cyu3utils.h"
#include "gpif_conf.h"

// constants and macros --------------------------------------------------------
#define VER_FW       0x40       // Firmware version (Pocket SDR FE 8CH)
#ifndef F_TCXO
#define F_TCXO       24000      // TCXO frequency (kHz)
#endif
#define LOCK_A       50         // FX3 GPIO(50) <-- MAX2771 CH1 LD  (0x32)
#define LOCK_B       51         // FX3 GPIO(51) <-- MAX2771 CH2 LD  (0x33)
#define LOCK_C       52         // FX3 GPIO(52) <-- MAX2771 CH3 LD  (0x34)
#define LOCK_D       53         // FX3 GPIO(53) <-- MAX2771 CH4 LD  (0x35)
#define LOCK_E       54         // FX3 GPIO(54) <-- MAX2771 CH5 LD  (0x36)
#define LOCK_F       55         // FX3 GPIO(55) <-- MAX2771 CH6 LD  (0x37)
#define LOCK_G       56         // FX3 GPIO(56) <-- MAX2771 CH7 LD  (0x38)
#define LOCK_H       57         // FX3 GPIO(57) <-- MAX2771 CH8 LD  (0x39)
#define CSN_A        17         // FX3 GPIO(17) --> MAX2771 CH1 CSN (0x11)
#define CSN_B        18         // FX3 GPIO(18) --> MAX2771 CH2 CSN (0x12)
#define CSN_C        19         // FX3 GPIO(19) --> MAX2771 CH3 CSN (0x13)
#define CSN_D        20         // FX3 GPIO(20) --> MAX2771 CH4 CSN (0x14)
#define CSN_E        21         // FX3 GPIO(21) --> MAX2771 CH5 CSN (0x15)
#define CSN_F        22         // FX3 GPIO(22) --> MAX2771 CH6 CSN (0x16)
#define CSN_G        23         // FX3 GPIO(23) --> MAX2771 CH7 CSN (0x17)
#define CSN_H        24         // FX3 GPIO(24) --> MAX2771 CH8 CSN (0x18)
#define LED1         27         // FX3 GPIO(27) --> LED1            (0x1B)
#define LED2         28         // FX3 GPIO(28) --> LED2            (0x1C)
#define LED3         29         // FX3 GPIO(29) --> LED3            (0x1D)
#define USB3_PORT_SEL 45        // FX3 GPIO(45) --> USB3_PORT_SEL   (0x2D)
#define SCLK         25         // FX3 GPIO(25) --> MAX2771 SCLK    (0x19)
#define SDATA        26         // FX3 GPIO(26) <-> MAX2771 SDATA   (0x1A)
#define SCLK_CYC     10         // SPI SCLK delay

#define VR_STAT      0x40       // USB vendor request: Get device info and status
#define VR_REG_READ  0x41       // USB vendor request: Read MAX2771 register
#define VR_REG_WRITE 0x42       // USB vendor request: Write MAX2771 register
#define VR_START     0x44       // USB vendor request: Start bulk transfer
#define VR_STOP      0x45       // USB vendor request: Stop bulk transfer
#define VR_RESET     0x46       // USB vendor request: Reset device
#define VR_SAVE      0x47       // USB vendor request: Save settings to EEPROM
#define VR_EE_READ   0x48       // USB vendor request: Read EEPROM
#define VR_EE_WRITE  0x49       // USB vendor request: Write EEPROM
#define VR_IO_READ   0x4A       // USB vendor request: Read IO port
#define VR_IO_WRITE  0x4B       // USB vendor request: Write IO port

#define EP_BULK_IN   0x86       // Bulk transfer IN end point
#define APP_STACK    0x0800     // App thread stack size
#define APP_PRI      8          // App thread priority
#define BUFF_COUNT_HS 32        // DMA buffer count (high speed)
#define BUFF_COUNT_SS 2         // DMA buffer count (super speed)
#define BURST_LEN    16         // DMA burst length (super speed)
#define I2C_BITRATE  100000     // I2C bitrate (Hz)
#define I2C_ADDR     0x51       // I2C EEPROM address
#define EE_ADDR_0    0xF000     // EEPROM Writable address start
#define EE_ADDR_1    0xFFFF     // EEPROM Writable address end
#define EE_ADDR_H    0xFE00     // EEPROM MAX2771 settings header address
#define EE_ADDR_S    0xFE04     // EEPROM MAX2771 settings address (508 bytes)
#define HEAD_REG     0xABC00CBA // MAX2771 settings header
#define MAX_CH       8          // Number of MAX2771 channels
#define MAX_ADDR     11         // Number of MAX2771 registers

// external variables (pocket_usb_dscr.c) --------------------------------------
extern const uint8_t CyFxUSB20DeviceDscr[];
extern const uint8_t CyFxUSB30DeviceDscr[];
extern const uint8_t CyFxUSBDeviceQualDscr[];
extern const uint8_t CyFxUSBFSConfigDscr[];
extern const uint8_t CyFxUSBHSConfigDscr[];
extern const uint8_t CyFxUSBBOSDscr[];
extern const uint8_t CyFxUSBSSConfigDscr[];
extern const uint8_t CyFxUSBStringLangIDDscr[];
extern const uint8_t CyFxUSBManufactureDscr[];
extern const uint8_t CyFxUSBProductDscr[];

// global variables ------------------------------------------------------------
static CyU3PThread app_thread;      // application thread
static CyU3PDmaMultiChannel dma_ch; // DMA channel for bulk transfer
static uint8_t usb_event = 0;       // USB event state
static uint8_t app_act = 0;         // application active
static uint8_t bulk_act = 0;        // bulk transfer active
static uint8_t EP0BUFF[128] __attribute__ ((aligned (32))); // EP0 data buffer

// IO ports definitions
static const uint8_t port_ena[] = {USB3_PORT_SEL, SCLK, CSN_E, CSN_F, CSN_G,
    CSN_H, LED1, LED2, LED3, LOCK_A, LOCK_B, LOCK_C, LOCK_D, LOCK_E, LOCK_F,
    LOCK_G, LOCK_H, SDATA, 0};
static const uint8_t port_out[] = {USB3_PORT_SEL, SCLK, CSN_A, CSN_B, CSN_C,
    CSN_D, CSN_E, CSN_F, CSN_G, CSN_H, LED1, LED2, LED3, 0};
static const uint8_t port_inp[] = {LOCK_A, LOCK_B, LOCK_C, LOCK_D, LOCK_E,
    LOCK_F, LOCK_G, LOCK_H, SDATA, 0};
static const uint8_t port_sdata[MAX_CH] = {SDATA, SDATA, SDATA, SDATA, SDATA,
    SDATA, SDATA, SDATA};
static const uint8_t port_csn[MAX_CH] = {CSN_A, CSN_B, CSN_C, CSN_D, CSN_E,
    CSN_F, CSN_G, CSN_H};

// default MAX2771 settings
static const uint32_t reg_default[MAX_CH][MAX_ADDR] = {
    {0xA2240015, 0x28550288, 0x0EAF31D0, 0x698C0008, 0x0CD22C80, 0x00000070,
     0x08000000, 0x10000002, 0x01E0F401, 0x00000002, 0x00000004},
    {0xA2240015, 0x28550288, 0x0EAF31D0, 0x698C0008, 0x0CD22C80, 0x00000070,
     0x08000000, 0x10000002, 0x01E0F401, 0x00000002, 0x00000004},
    {0xA2240015, 0x28550288, 0x0EAF31D0, 0x698C0008, 0x0CD22C80, 0x00000070,
     0x08000000, 0x10000002, 0x01E0F401, 0x00000002, 0x00000004},
    {0xA2240015, 0x28550288, 0x0EAF31D0, 0x698C0008, 0x0CD22C80, 0x00000070,
     0x08000000, 0x10000002, 0x01E0F401, 0x00000002, 0x00000004},
    {0xA2240015, 0x28550288, 0x0EAF31D0, 0x698C0008, 0x0CD22C80, 0x00000070,
     0x08000000, 0x10000002, 0x01E0F401, 0x00000002, 0x00000004},
    {0xA2240015, 0x28550288, 0x0EAF31D0, 0x698C0008, 0x0CD22C80, 0x00000070,
     0x08000000, 0x10000002, 0x01E0F401, 0x00000002, 0x00000004},
    {0xA2240015, 0x28550288, 0x0EAF31D0, 0x698C0008, 0x0CD22C80, 0x00000070,
     0x08000000, 0x10000002, 0x01E0F401, 0x00000002, 0x00000004},
    {0xA2240015, 0x28550288, 0x0EAF31D0, 0x698C0008, 0x0CD22C80, 0x00000070,
     0x08000000, 0x10000002, 0x01E0F401, 0x00000002, 0x00000004}
};

// configure IO port (mode = 0:in, 1:out, 2:hi-z) ------------------------------
static void conf_iop(uint8_t port, uint8_t mode)
{
    CyU3PGpioSimpleConfig_t gcfg;
    gcfg.outValue = 1; // outport initial high
    gcfg.driveLowEn = mode == 1;
    gcfg.driveHighEn = mode == 1;
    gcfg.inputEn = mode <= 1;
    gcfg.intrMode = CY_U3P_GPIO_NO_INTR;
    CyU3PGpioSetSimpleConfig(port, &gcfg);
}

// read IO port ----------------------------------------------------------------
static uint8_t read_iop(uint8_t port)
{
    CyBool_t on = 0;
    CyU3PGpioGetValue(port, &on);
    return on ? 1 : 0;
}

// write IO port ---------------------------------------------------------------
static void write_iop(uint8_t port, uint8_t dat)
{
    CyU3PGpioSetValue(port, dat != 0);
}

// handle fatal error ----------------------------------------------------------
static void fatal_error(void)
{
    while (1) ; // halt
}

// handle application error ----------------------------------------------------
static void app_error(void)
{
    for (int i = 0; ; i++) {
        write_iop(LED1, i % 2 == 1); // blink LED1 and LED2
        write_iop(LED2, i % 2 == 0);
        CyU3PThreadSleep(100);
    }
}

// read I2C EEPROM (n <= 64) ---------------------------------------------------
static int read_eeprom(uint16_t addr, uint8_t n, uint8_t *buff)
{
    if (n > 64) return 0;
    CyU3PI2cPreamble_t pre;
    pre.length = 4;
    pre.buffer[0] = I2C_ADDR << 1;
    pre.buffer[1] = (uint8_t)(addr >> 8);
    pre.buffer[2] = (uint8_t)(addr & 0xFF);
    pre.buffer[3] = (I2C_ADDR << 1) | 0x01;
    pre.ctrlMask = 0x0004;
    return !CyU3PI2cReceiveBytes(&pre, buff, n, 0);
}

// write I2C EEPROM (n <= 64) --------------------------------------------------
static int write_eeprom(uint16_t addr, uint8_t n, uint8_t *buff)
{
    if (n > 64) return 0;
    CyU3PI2cPreamble_t pre;
    pre.length = 3;
    pre.buffer[0] = I2C_ADDR << 1;
    pre.buffer[1] = (uint8_t)(addr >> 8);
    pre.buffer[2] = (uint8_t)(addr & 0xFF);
    pre.ctrlMask = 0x0000;
    if (CyU3PI2cTransmitBytes(&pre, buff, n, 0)) return 0;
    pre.length = 1;
    return !CyU3PI2cWaitForAck(&pre, 200);
}

// SPI delay -------------------------------------------------------------------
static void spi_delay(void)
{
    CyU3PBusyWait(SCLK_CYC);
}

// write SPI SCLK --------------------------------------------------------------
static void write_sclk(void)
{
    write_iop(SCLK, 1);
    spi_delay();
    write_iop(SCLK, 0);
    spi_delay();
}

// write SPI SDATA -------------------------------------------------------------
static void write_sdata(uint8_t ch, uint8_t dat)
{
    write_iop(port_sdata[ch], dat);
    write_sclk();
}

// read SPI SDATA --------------------------------------------------------------
static uint8_t read_sdata(uint8_t ch)
{
    uint8_t dat = read_iop(port_sdata[ch]);
    write_sclk();
    return dat;
}

// write MAX2771 SPI frame header (mode = 0:write, 1:read) ---------------------
static void write_head(uint8_t ch, uint16_t addr, uint8_t mode)
{
    for (int i = 11; i >= 0; i--) {
        write_sdata(ch, (uint8_t)(addr >> i) & 1);
    }
    write_sdata(ch, mode);
    
    for (int8_t i = 0; i < 3; i++) {
        write_sdata(ch, 0);
    }
    spi_delay();
}

// write MAX2771 register ------------------------------------------------------
static int write_reg(uint8_t ch, uint8_t addr, uint32_t val)
{
    if (ch >= MAX_CH) return 0;
    
    if (addr == 0) { // force LNAMODE = High-band, MIXERMODE = High-band
        val &= 0xFFFE1FFF;
    }
    write_iop(port_csn[ch], 0);
    spi_delay();
    conf_iop(port_sdata[ch], 1);
    write_head(ch, addr, 0);
    
    for (int i = 31; i >= 0; i--) {
        write_sdata(ch, (uint8_t)(val >> i) & 1);
    }
    conf_iop(port_sdata[ch], 0);
    write_iop(port_csn[ch], 1);
    spi_delay();
    
    return 1;
}

// read MAX2771 register -------------------------------------------------------
static uint32_t read_reg(uint8_t ch, uint8_t addr)
{
    if (ch >= MAX_CH) return 0;
    write_iop(port_csn[ch], 0);
    spi_delay();
    conf_iop(port_sdata[ch], 1);
    write_head(ch, addr, 1);
    conf_iop(port_sdata[ch], 0);
    
    uint32_t val = 0;
    for (int i = 31; i >= 0; i--) {
        val <<= 1;
        val |= read_sdata(ch);
    }
    write_iop(port_csn[ch], 1);
    spi_delay();
    return val;
}

// load default MAX2771 register settings ----------------------------------------
static void load_default(void)
{
    for (uint8_t ch = 0; ch < MAX_CH; ch++) {
        for (uint8_t addr = 0; addr < MAX_ADDR; addr++) {
            write_reg(ch, addr, reg_default[ch][addr]);
        }
    }
}

// load MAX2771 register settings from EEPROM ----------------------------------
static int load_settings(void)
{
    uint16_t ee_addr = EE_ADDR_S;
    uint32_t reg;
    
    if (!read_eeprom(EE_ADDR_H, 4, (uint8_t *)&reg) || reg != HEAD_REG) {
        return 0;
    }
    for (uint8_t ch = 0; ch < MAX_CH; ch++) {
        for (uint8_t addr = 0; addr < MAX_ADDR; addr++, ee_addr += 4) {
            if (!read_eeprom(ee_addr, 4, (uint8_t *)&reg)) return 0;
            write_reg(ch, addr, reg);
        }
    }
    return 1;
}

// save MAX2771 register settings to EEPROM ------------------------------------
static int save_settings(void)
{
    uint16_t ee_addr = EE_ADDR_S;
    uint32_t reg = HEAD_REG;
    
    if (!write_eeprom(EE_ADDR_H, 4, (uint8_t *)&reg)) return 0;
    
    for (uint8_t ch = 0; ch < MAX_CH; ch++) {
        for (uint8_t addr = 0; addr < MAX_ADDR; addr++, ee_addr += 4) {
            reg = read_reg(ch, addr);
            if (!write_eeprom(ee_addr, 4, (uint8_t *)&reg)) return 0;
        }
    }
    return 1;
}

// stop bulk transfer ----------------------------------------------------------
static int stop_bulk(void)
{
    if (!bulk_act) return 0;
    
    // stop DMA channel
    if (CyU3PDmaMultiChannelSetSuspend(&dma_ch, CyFalse, CyTrue)) return 0;
    bulk_act = 0;
    return 1;
}

// start bulk transfer ---------------------------------------------------------
static int start_bulk(void)
{
    if (bulk_act) {
        stop_bulk();
        CyU3PThreadSleep(100);
    }
    // start DMA channel
    if (CyU3PDmaMultiChannelResume(&dma_ch, CyFalse, CyTrue)) return 0;
    bulk_act = 1;
    return 1;
}

// start application -----------------------------------------------------------
static int app_start(void)
{
    if (app_act) return 1;
    
    CyU3PUSBSpeed_t speed = CyU3PUsbGetSpeed();
    if (speed != CY_U3P_HIGH_SPEED && speed != CY_U3P_SUPER_SPEED) return 0;
    uint16_t pckt_size = (speed == CY_U3P_HIGH_SPEED) ? 512 : 1024;
    uint8_t burst_len  = (speed == CY_U3P_HIGH_SPEED) ? 1 : BURST_LEN;
    
    // enable bulk IN endpoint
    CyU3PEpConfig_t ecfg = {0};
    ecfg.enable = CyTrue;
    ecfg.epType = CY_U3P_USB_EP_BULK;
    ecfg.pcktSize = pckt_size;
    ecfg.burstLen = burst_len;
    if (CyU3PSetEpConfig(EP_BULK_IN, &ecfg)) return 0;
    
    // generate DMA channel
    CyU3PDmaMultiChannelConfig_t dcfg = {0};
    dcfg.size = burst_len * pckt_size;
    dcfg.count = (speed == CY_U3P_HIGH_SPEED) ? BUFF_COUNT_HS : BUFF_COUNT_SS;
    dcfg.validSckCount = 2;
    dcfg.dmaMode = CY_U3P_DMA_MODE_BYTE;
    dcfg.prodSckId[0] = CY_U3P_PIB_SOCKET_0;
    dcfg.prodSckId[1] = CY_U3P_PIB_SOCKET_1;
    dcfg.consSckId[0] = CY_U3P_UIB_SOCKET_CONS_6; // EP 0x86
    dcfg.notification = CY_U3P_DMA_CB_PROD_EVENT;
    if (CyU3PDmaMultiChannelCreate(&dma_ch, CY_U3P_DMA_TYPE_AUTO_MANY_TO_ONE,
        &dcfg)) {
        return 0;
    }
    // set data counter for socket switch
    CyU3PGpifInitDataCounter(0, dcfg.size / 4 - 2, CyFalse, CyTrue, 1);
    
    // prepare and suspend DMA channel
    if (CyU3PDmaMultiChannelSetXfer(&dma_ch, 0, 0) ||
        CyU3PDmaMultiChannelSetSuspend(&dma_ch, CyFalse, CyTrue)) {
        return 0;
    }
    app_act = 1;
    return 1;
}

// stop application ------------------------------------------------------------
static int app_stop(void)
{
    if (!app_act) return 1;
    
    stop_bulk();
    
    // flush buffer
    CyU3PUsbFlushEp(EP_BULK_IN);
    
    // disable bulk IN endpoint
    CyU3PEpConfig_t ecfg = {0};
    if (CyU3PSetEpConfig(EP_BULK_IN, &ecfg)) return 0;
    
    // destroy DMA channel
    CyU3PDmaMultiChannelDestroy(&dma_ch);
    
    app_act = 0;
    return 1;
}

// set device descriptors ------------------------------------------------------
static int set_dev_desc(void)
{
    CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB30DeviceDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB20DeviceDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR   , 0, (uint8_t *)CyFxUSBBOSDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR  , 0, (uint8_t *)CyFxUSBDeviceQualDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBSSConfigDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBHSConfigDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBFSConfigDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR   , 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR   , 1, (uint8_t *)CyFxUSBManufactureDscr);
    CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR   , 2, (uint8_t *)CyFxUSBProductDscr);
    return 1;
}

// handle USB vendor request ---------------------------------------------------
// 
//  USB vendor request      code dir wValue     bytes data
//
//  Get device Info         0x40  I  -             6  Device info and status
//  Read MAX2771 register   0x41  I  CH + addr*    4  Register value
//  Write MAX2771 register  0x42  O  CH + addr*    4  Register value
//  Start bulk transfer     0x44  O  -             0  -
//  Stop bulk transfer      0x45  O  -             0  -
//  Reset device            0x46  O  -             0  -
//  Save settings to EEPROM 0x47  O  -             0  -
//  Read EEPROM             0x48  I  address       n  data (n <= 64)
//  Write EEPROM            0x49  O  address       n  data (n <= 64)
//  Read IO port            0x4A  I  IO port       1  0:off, 1:on
//  Write IO port           0x4B  O  IO port       1  0:off, 1:on
//
//  * bit15-8= MAX2771 CH (0:CH1,1:CH2,...), bit7-0= MAX2771 register address
//
static int handle_req(uint8_t req, uint16_t val, uint16_t len)
{
    uint8_t ch = (uint8_t)(val >> 8), addr = (uint8_t)(val & 0xFF);
    
    if (req == VR_STAT) {
        uint8_t stat1 = (app_act << 5) + (bulk_act << 4);
        uint8_t stat2 = (read_iop(LOCK_A) << 7) + (read_iop(LOCK_B) << 6) +
            (read_iop(LOCK_C) << 5) + (read_iop(LOCK_D) << 4) +
            (read_iop(LOCK_E) << 3) + (read_iop(LOCK_F) << 2) +
            (read_iop(LOCK_G) << 1) + read_iop(LOCK_H);
        EP0BUFF[0] = VER_FW;
        EP0BUFF[1] = (uint8_t)((F_TCXO >> 8) & 0xFF);
        EP0BUFF[2] = (uint8_t)(F_TCXO & 0xFF);
        EP0BUFF[3] = stat1;
        EP0BUFF[4] = stat2;
        EP0BUFF[5] = 0;
        return !CyU3PUsbSendEP0Data(6, EP0BUFF);
    }
    else if (req == VR_REG_READ) {
        uint32_t reg = read_reg(ch, addr);
        EP0BUFF[0] = (reg >> 24) & 0xFF;
        EP0BUFF[1] = (reg >> 16) & 0xFF;
        EP0BUFF[2] = (reg >>  8) & 0xFF;
        EP0BUFF[3] = (reg >>  0) & 0xFF;
        return !CyU3PUsbSendEP0Data(4, EP0BUFF);
    }
    else if (req == VR_REG_WRITE) {
        if (len < 4) return 0;
        if (CyU3PUsbGetEP0Data(len, EP0BUFF, NULL)) return 0;
        uint32_t reg =
            ((uint32_t)EP0BUFF[0] << 24) + ((uint32_t)EP0BUFF[1] << 16) +
            ((uint32_t)EP0BUFF[2] <<  8) + ((uint32_t)EP0BUFF[3] <<  0);
        if (!write_reg(ch, addr, reg)) return 0;
    }
    else if (req == VR_START) {
        start_bulk();
    }
    else if (req == VR_STOP) {
        stop_bulk();
    }
    else if (req == VR_RESET) {
        if (!app_stop() || !app_start()) return 0;
    }
    else if (req == VR_SAVE) {
        if (!save_settings()) return 0;
    }
    else if (req == VR_EE_READ) {
        if (!read_eeprom(val, (uint8_t)len, EP0BUFF)) return 0;
        if (CyU3PUsbSendEP0Data(len, EP0BUFF)) return 0;
    }
    else if (req == VR_EE_WRITE) {
        if (CyU3PUsbGetEP0Data(len, EP0BUFF, NULL)) return 0;
        if (!write_eeprom(val, (uint8_t)len, EP0BUFF)) return 0;
    }
    else if (req == VR_IO_READ) {
        if (len < 1) return 0;
        EP0BUFF[0] = read_iop(val);
        if (CyU3PUsbSendEP0Data(1, EP0BUFF)) return 0;
    }
    else if (req == VR_IO_WRITE) {
        if (len < 1) return 0;
        if (CyU3PUsbGetEP0Data(1, EP0BUFF, NULL)) return 0;
        write_iop(val, EP0BUFF[0]);
    }
    else { // unknown request
        return 0;
    }
    CyU3PUsbAckSetup();
    return 1;
}

// USB setup request callback -------------------------------------------------
static CyBool_t usb_setup_cb(uint32_t data0, uint32_t data1)
{
    uint8_t type, target, req;
    uint16_t val, len;
    
    type   = data0 & CY_U3P_USB_REQUEST_TYPE_MASK & CY_U3P_USB_TYPE_MASK;
    target = data0 & CY_U3P_USB_REQUEST_TYPE_MASK & CY_U3P_USB_TARGET_MASK;
    req = (data0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS;
    val = (data0 & CY_U3P_USB_VALUE_MASK  ) >> CY_U3P_USB_VALUE_POS;
    len = (data1 & CY_U3P_USB_LENGTH_MASK ) >> CY_U3P_USB_LENGTH_POS;
    
    if (type == CY_U3P_USB_VENDOR_RQT) {
        return handle_req(req, val, len);
    }
    else if (type == CY_U3P_USB_STANDARD_RQT &&
        target == CY_U3P_USB_TARGET_INTF && (req == CY_U3P_USB_SC_SET_FEATURE ||
        req == CY_U3P_USB_SC_CLEAR_FEATURE) && val == 0) {
        if (app_act) {
            CyU3PUsbAckSetup();
        }
        else {
            CyU3PUsbStall(0, CyTrue, CyFalse);
        }
    }
    return CyTrue;
}

// USB event callback ----------------------------------------------------------
static void usb_event_cb(CyU3PUsbEventType_t event, uint16_t data)
{
    if (event == CY_U3P_USB_EVENT_SETCONF) {
        CyU3PUsbLPMDisable();
        if (!app_stop() || !app_start()) {
            app_error();
        }
    }
    else if (event == CY_U3P_USB_EVENT_RESET ||
             event == CY_U3P_USB_EVENT_DISCONNECT) {
        if (!app_stop()) {
            app_error();
        }
    }
    else if (event == CY_U3P_USB_EVENT_SS_COMP_ENTRY ||
             event == CY_U3P_USB_EVENT_USB3_LNKFAIL) {
        usb_event = 1;
    }
}

// USB 3.0 LPM request callback ------------------------------------------------
static CyBool_t lpm_req_cb(CyU3PUsbLinkPowerMode link_mode)
{
    return CyTrue;
}

// initialize application ------------------------------------------------------
static int app_init(void)
{
    // initialize P-port I/F block
    CyU3PPibClock_t pclk = {0};
    pclk.clkDiv = 2;
    pclk.clkSrc = CY_U3P_SYS_CLK;
    if (CyU3PPibInit(CyTrue, &pclk)) return 0;
    
    // load GPIF configuration
    if (CyU3PGpifLoad(&CyFxGpifConfig)) return 0;
    
    // start GPIF state machine
    if (CyU3PGpifSMStart(RESET, ALPHA_RESET)) return 0;
    
    // initialize GPIO module
    CyU3PGpioClock_t gclk = {0};
    gclk.fastClkDiv = 2;
    gclk.simpleDiv = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gclk.clkSrc = CY_U3P_SYS_CLK;
    if (CyU3PGpioInit(&gclk, NULL)) return 0;
    
    // configure IO ports
    for (int i = 0; port_inp[i]; i++) conf_iop(port_inp[i], 0);
    for (int i = 0; port_out[i]; i++) conf_iop(port_out[i], 1);
    write_iop(LED1, 0);
    write_iop(LED2, 0);
    write_iop(LED3, 0);
    write_iop(SCLK, 0);
    
    // initialize and configure I2C
    if (CyU3PI2cInit()) return 0;
    CyU3PI2cConfig_t i2ccfg = {0};
    i2ccfg.bitRate = I2C_BITRATE;
    i2ccfg.busTimeout = 0xFFFFFFFF; // no timeout
    i2ccfg.dmaTimeout = 0xFFFF;     // no timeout
    if (CyU3PI2cSetConfig(&i2ccfg, NULL)) return 0;
    
    // start USB function
    if (CyU3PUsbStart()) return 0;
    
    // set callbacks
    CyU3PUsbRegisterSetupCallback(usb_setup_cb, CyTrue);
    CyU3PUsbRegisterEventCallback(usb_event_cb);
    CyU3PUsbRegisterLPMRequestCallback(lpm_req_cb);
    
    // set device descriptors
    return set_dev_desc();
}

// select USB 3.0 port and connect USB -----------------------------------------
static int usb_connect(void)
{
    // disable USB 2.0
    if (CyU3PUsbControlUsb2Support(CyFalse)) return 0;
    CyU3PThreadSleep(20);
    
    // connect USB
    usb_event = 0;
    if (CyU3PConnectState(CyTrue, CyTrue)) return 0;
    CyU3PThreadSleep(50);
    for (int i = 0 ; i < 100 && !usb_event; i++) {
        CyU3PThreadSleep(5);
    }
    if (!usb_event) { // USB 3.0 connected
        return 1;
    }
    // disconnect USB
    if (CyU3PConnectState(CyFalse, CyFalse)) return 0;
    
    // flip USB 3.0 port selection for type-C receptacle
    write_iop(USB3_PORT_SEL, 0);
    
    // enable USB 2.0
    if (CyU3PUsbControlUsb2Support(CyTrue)) return 0;
    CyU3PThreadSleep(20);
    
    // connect USB
    return !CyU3PConnectState(CyTrue, CyTrue);
}

// application thread function -------------------------------------------------
static void app_func(uint32_t input)
{
    // initialize application
    if (!app_init()) {
        app_error();
    }
    // load MAX2771 register settings
    if (!load_settings()) {
        load_default();
    }
    // connect USB
    if (!usb_connect()) {
        app_error();
    }
    // application loop
    for (int i = 0; ; i++) {
        uint8_t stat = read_iop(LOCK_A) && read_iop(LOCK_B) &&
                       read_iop(LOCK_C) && read_iop(LOCK_D) &&
                       read_iop(LOCK_E) && read_iop(LOCK_F) &&
                       read_iop(LOCK_G) && read_iop(LOCK_H);
        write_iop(LED1, stat);
        write_iop(LED3, bulk_act);
        CyU3PThreadSleep(100);
    }
}

// application function --------------------------------------------------------
void CyFxApplicationDefine(void)
{
    void *p = CyU3PMemAlloc(APP_STACK);
    
    // generate application thread
    if (CyU3PThreadCreate(&app_thread, "app_func", app_func, 0, p, APP_STACK,
        APP_PRI, APP_PRI, CYU3P_NO_TIME_SLICE, CYU3P_AUTO_START)) {
        fatal_error();
    }
}

// main ------------------------------------------------------------------------
int main(void)
{
    // initialize device
    CyU3PSysClockConfig_t ccfg = {0};
    ccfg.setSysClk400 = CyTrue;
    ccfg.cpuClkDiv = 2;
    ccfg.dmaClkDiv = 2;
    ccfg.mmioClkDiv = 2;
    ccfg.clkSrc = CY_U3P_SYS_CLK;
    if (CyU3PDeviceInit(&ccfg)) {
        fatal_error();
    }
    // initialize caches
    if (CyU3PDeviceCacheControl(CyTrue, CyTrue, CyTrue)) {
        fatal_error();
    }
    // IO port mask
    uint64_t mask = 0;
    for (int i = 0; port_ena[i]; i++) mask |= (uint64_t)1 << port_ena[i];
    
    // initialize device IO
    CyU3PIoMatrixConfig_t icfg = {0};
    icfg.s0Mode = CY_U3P_SPORT_INACTIVE;
    icfg.s1Mode = CY_U3P_SPORT_INACTIVE;
    icfg.useI2C = CyTrue;
    icfg.lppMode = CY_U3P_IO_MATRIX_LPP_DEFAULT;
    icfg.gpioSimpleEn[0] = (uint32_t)(mask & 0xFFFFFFFF);
    icfg.gpioSimpleEn[1] = (uint32_t)(mask >> 32);
    icfg.isDQ32Bit = CyTrue; // 32 bit bus
    if (CyU3PDeviceConfigureIOMatrix(&icfg)) {
        fatal_error();
    }
    // override GPIO 17-20 (CTL0-3)
    for (int port = 17; port <= 20; port++) {
        CyU3PDeviceGpioOverride(port, CyTrue);
    }
    // start RTOS kernel
    CyU3PKernelEntry();
    
    return 0;
}

