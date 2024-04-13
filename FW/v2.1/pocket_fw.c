//-----------------------------------------------------------------------------
//
//  Pocket SDR - SDR device firmware for EZ-USB FX2LP
//
//  References:
//
//  [1] Cypress, EZ-USB Technical Refrence Manual, Rev.G , January 31, 2019
//  [2] maxim integrated, MAX2771 Multiband Universal GNSS Receiver, Rev 0
//  [3] https://www.cypress.com/documentation/development-kitsboards/...
//      cy3684-ez-usb-fx2lp-development-kit
//
//  Author:
//  T.TAKASU
//
//  History:
//  2021-10-18  1.0  new
//
#pragma NOIV

#include "Fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"

// constants and macros -------------------------------------------------------
#define VER_FW       0x10       // Firmware version
#ifndef F_TCXO
#define F_TCXO       24000      // TCXO frequency (kHz)
#endif
#define CSN1         0          // EZ-USB FX2 PD0  -> MAX2771 CH1 CSN
#define CSN2         1          // EZ-USB FX2 PD1  -> MAX2771 CH2 CSN
#define SCLK         2          // EZ-USB FX2 PD2  -> MAX2771 SCLK
#define SDATA        3          // EZ-USB FX2 PD3 <-> MAX2771 SDATA
#define STAT1        4          // EZ-USB FX2 PD4 <-  MAX2771 CH1 LD
#define STAT2        5          // EZ-USB FX2 PD5 <-  MAX2771 CH2 LD
#define LED1         6          // EZ-USB FX2 PD6  -> LED1
#define LED2         7          // EZ-USB FX2 PD7  -> LED2
#define SCLK_CYC     5          // SPI SCLK delay

#define VR_STAT      0x40       // USB vendor request: Get device info and status
#define VR_REG_READ  0x41       // USB vendor request: Read MAX2771 register
#define VR_REG_WRITE 0x42       // USB vendor request: Write MAX2771 register
#define VR_START     0x44       // USB vendor request: Start bulk transfer
#define VR_STOP      0x45       // USB vendor request: Stop bulk transfer
#define VR_RESET     0x46       // USB vendor request: Reset device
#define VR_SAVE      0x47       // USB vendor request: Save settings to EEPROM
#define VR_EE_READ   0x48       // USB vendor request: Read EEPROM
#define VR_EE_WRITE  0x49       // USB vendor request: Write EEPROM

#define MAX_CH       2          // number of MAX2771 channels
#define MAX_ADDR     11         // number of MAX2771 registers

#define I2C_ADDR     0x51       // EEPROM I2C address (16 KB EEPROM)
#define EE_ADDR_0    0x2000     // EEPROM Writable address start
#define EE_ADDR_1    0x3FFF     // EEPROM Writable address end
#define EE_ADDR_H    0x3F00     // EEPROM MAX2771 settings header address
#define EE_ADDR_S    0x3F04     // EEPROM MAX2771 settings address
#define HEAD_REG     0xABC00CBA // MAX2771 settings header

#define BIT(port)    ((uint8_t)1<<(port))
#define WORD_(bytes) (((uint16_t)(bytes)[1]<<8) + (bytes)[0])

// type defintions -------------------------------------------------------------
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef signed char int8_t;
typedef short int16_t;
typedef long int32_t;

// default MAX2771 register settings -------------------------------------------
static uint32_t xdata reg_default[][MAX_ADDR] = {
  {0xA2241797, 0x20550288, 0x0E9F21DC, 0x69888000, 0x00082008, 0x0647AE70,
   0x08000000, 0x00000000, 0x01E0F281, 0x00000002, 0x00000004},
  {0xA224A019, 0x28550288, 0x0E9F31DC, 0x78888000, 0x00062008, 0x004CCD70,
   0x08000000, 0x10000000, 0x01E0F281, 0x00000002, 0x00000004}
};

// delay SPI SCLK --------------------------------------------------------------
static void delay(uint8_t cyc) {
  uint8_t i;
  for (i = 0; i < cyc; i++) SYNCDELAY;
}

// read port D bit -------------------------------------------------------------
static uint8_t digitalRead(uint8_t port) {
  OED &= ~BIT(port);
  return (IOD & BIT(port)) ? 1 : 0;
}

// write port D bit ------------------------------------------------------------
static void digitalWrite(uint8_t port, uint8_t dat) {
  OED |= BIT(port);
  if (dat) IOD |= BIT(port); else IOD &= ~BIT(port);
}

// write SPI SCLK --------------------------------------------------------------
static void write_sclk(void) {
  digitalWrite(SCLK, 1);
  delay(SCLK_CYC);
  digitalWrite(SCLK, 0);
  delay(SCLK_CYC);
}

// write SPI SDATA -------------------------------------------------------------
static void write_sdata(uint8_t dat) {
  digitalWrite(SDATA, dat);
  write_sclk();
}

// read SPI SDATA --------------------------------------------------------------
static uint8_t read_sdata(void) {
  uint8_t dat = digitalRead(SDATA);
  write_sclk();
  return dat;
}

// write MAX2771 SPI frame header ----------------------------------------------
static void write_head(uint16_t addr, uint8_t mode) {
  int8_t i;
  
  for (i = 11; i >= 0; i--) {
    write_sdata((uint8_t)(addr >> i) & 1);
  }
  write_sdata(mode); // 0:write,1:read
  
  for (i = 0; i < 3; i++) {
    write_sdata(0);
  }
  delay(SCLK_CYC);
}

// write MAX2771 register ------------------------------------------------------
static void write_reg(uint8_t cs, uint8_t addr, uint32_t val) {
  int8_t i;
  
  digitalWrite(cs, 0);
  write_head(addr, 0);
  
  for (i = 31; i >= 0; i--) {
    write_sdata((uint8_t)(val >> i) & 1);
  }
  digitalWrite(cs, 1);
}

// read MAX2771 register -------------------------------------------------------
static uint32_t read_reg(uint8_t cs, uint8_t addr) {
  uint32_t val = 0;
  int8_t i;
  
  digitalWrite(cs, 0);
  write_head(addr, 1);
  
  for (i = 31; i >= 0; i--) {
    val <<= 1;
    val |= read_sdata();
  }
  digitalWrite(cs, 1);
  return val;
}

// start bulk trasfer ----------------------------------------------------------
static void start_bulk(void) {
  FIFORESET  = 0x80; SYNCDELAY; // NAK-ALL
  EP6FIFOCFG = 0x00; SYNCDELAY; // manual mode
  FIFORESET  = 0x06; SYNCDELAY; // reset EP6 FIFO
  EP6FIFOCFG = 0x0C; SYNCDELAY; // EP6FIFO: AUTOIN=ON, ZEROLENIN=1, WORDWIDE=BYTE
  FIFORESET  = 0x00; SYNCDELAY; // release NAK-ALL
  digitalWrite(LED2, 1);
}

// stop bulk trasfer -----------------------------------------------------------
static void stop_bulk(void) {
  EP6FIFOCFG = 0x04; SYNCDELAY; // EP6FIFO: AUTOIN=OFF, ZEROLENIN=1, WORDWIDE=BYTE
  digitalWrite(LED2, 0);
}

// read EEPROM -----------------------------------------------------------------
static void read_eeprom(uint16_t addr, uint8_t len, uint8_t xdata *buff) {
  uint8_t xdata str[2];
  
  str[0] = MSB(addr);
  str[1] = LSB(addr);
  EZUSB_WriteI2C(I2C_ADDR, 2, str);
  EZUSB_ReadI2C(I2C_ADDR, len, buff);
}

// write EEPROM ----------------------------------------------------------------
static void write_eeprom(uint16_t addr, uint8_t len, const uint8_t xdata *buff) {
  uint8_t xdata str[3];
  uint8_t i;
  
  for (i = 0; i < len; i++, addr++) {
    str[0] = MSB(addr);
    str[1] = LSB(addr);
    str[2] = buff[i];
    EZUSB_WriteI2C(I2C_ADDR, 3, str);
    EZUSB_WaitForEEPROMWrite(I2C_ADDR);
  }
}

// load default MAX2771 register settings --------------------------------------
static void load_default(void) {
  uint8_t cs, addr;
  
  for (cs = 0; cs < MAX_CH; cs++) {
    for (addr = 0; addr < MAX_ADDR; addr++) {
      write_reg(cs, addr, reg_default[cs][addr]);
    }
  }
}

// load MAX2771 register settings from EEPROM ----------------------------------
static void load_settings(void) {
  uint16_t ee_addr = EE_ADDR_S;
  uint32_t xdata reg;
  uint8_t cs, addr;
  
  read_eeprom(EE_ADDR_H, 4, (uint8_t xdata *)&reg);
  if (reg != HEAD_REG) return;
  
  for (cs = 0; cs < MAX_CH; cs++) {
    for (addr = 0; addr < MAX_ADDR; addr++) {
      read_eeprom(ee_addr, 4, (uint8_t xdata *)&reg);
      write_reg(cs, addr, reg);
      ee_addr += 4;
    }
  }
}

// save MAX2771 register settings to EEPROM ------------------------------------
static void save_settings(void) {
  uint16_t ee_addr = EE_ADDR_S;
  uint32_t xdata reg = HEAD_REG;
  uint8_t cs, addr;
  
  write_eeprom(EE_ADDR_H, 4, (uint8_t xdata *)&reg);
  
  for (cs = 0; cs < MAX_CH; cs++) {
    for (addr = 0; addr < MAX_ADDR; addr++) {
      reg = read_reg(cs, addr);
      write_eeprom(ee_addr, 4, (uint8_t xdata *)&reg);
      ee_addr += 4;
    }
  }
}

// SETUP routine ---------------------------------------------------------------
void setup(void) {
  CPUCS         = 0x12; SYNCDELAY; // CPU: CLKSPD=48MHz, CLKOE=FLOAT
  EP2FIFOCFG    = 0x00; SYNCDELAY; // EPxFIFO: WORDWIDE=BYTE (PD enabled)
  EP4FIFOCFG    = 0x00; SYNCDELAY;
  EP6FIFOCFG    = 0x00; SYNCDELAY;
  EP8FIFOCFG    = 0x00; SYNCDELAY;
  IFCONFIG      = 0x53; SYNCDELAY; // IFCLK=EXT, OUT_DIS, POL=INV, SLAVE_FIFO
  REVCTL        = 0x03; SYNCDELAY; // SLAVE-FIFO enabled
  EP2CFG        = 0x20; SYNCDELAY; // EP2: OFF, DIR=OUT, TYPE=BULK
  EP4CFG        = 0x20; SYNCDELAY; // EP4: OFF, DIR=OUT, TYPE=BULK
  EP6CFG        = 0xE0; SYNCDELAY; // EP6: ON, DIR=IN, TYPE=BULK, SIZE=512, BUF=4X
  EP8CFG        = 0x60; SYNCDELAY; // EP8: OFF, DIR=IN, TYPE=BULK
  FIFOPINPOLAR  = 0x00; SYNCDELAY; // SLAVE_FIFO_IF: PKTEND=ACT_H, SLWR=ACT_H
  EP6AUTOINLENH = 0x02; SYNCDELAY; // EP6AUTOIN: PACKETLEN=512
  EP6AUTOINLENL = 0x00; SYNCDELAY;
  FIFORESET     = 0x86; SYNCDELAY; // EP6FIFO: RESET
  FIFORESET     = 0x00; SYNCDELAY;
  
  digitalWrite(CSN1, 1);
  digitalWrite(CSN2, 1);
  digitalWrite(SCLK, 0);
  
  EZUSB_InitI2C();
  load_default();
  load_settings();
  
  delay(255);
  start_bulk();
}

// MAIN loop -------------------------------------------------------------------
void loop(void) {
  // update LEDs
  digitalWrite(LED1, digitalRead(STAT1) && digitalRead(STAT2));
}

// handle USB vendor request ---------------------------------------------------
// 
//  USB vendor request      code dir wValue   bytes data
//
//  Get device Info         0x40  I  -           6  Device info and status
//  Read MAX2771 register   0x41  I  CH + addr*  4  Register value
//  Write MAX2771 register  0x42  O  CH + addr*  4  Register value
//  Start bulk transfer     0x43  O  -           0  -
//  Stop bulk transfer      0x44  O  -           0  -
//  Reset device            0x45  O  -           0  -
//  Save settings to EEPROM 0x46  O  -           0  -
//  Read EEPROM             0x47  I  address     n  data (n <= 64)
//  Write EEPROM            0x48  O  address     n  data (n <= 64)
//
//  * bit15-8= MAX2771 CH (0:CH1,1:CH2), bit7-0= MAX2771 register address
//
BOOL handle_req(void) {
  uint16_t len = WORD_(SETUPDAT + 6);
  uint16_t val = WORD_(SETUPDAT + 2);
  
  if (SETUPDAT[1] == VR_STAT) {
    EP0BUF[0] = VER_FW;             // F/W version
    EP0BUF[1] = MSB(F_TCXO);        // TCXO Frequency (kHz)
    EP0BUF[2] = LSB(F_TCXO);
    EP0BUF[3] = digitalRead(STAT1); // MAX2771 CH1 PLL status (0:unlock,1:lock)
    EP0BUF[4] = digitalRead(STAT2); // MAX2771 CH2 PLL status (0:unlock,1:lock)
    EP0BUF[5] = digitalRead(LED2);  // Bulk transfer status (0:stop,1:start)
    EP0BCH = 0;
    EP0BCL = 6;
  }
  else if (SETUPDAT[1] == VR_REG_READ) {
    *(uint32_t *)EP0BUF = read_reg(SETUPDAT[3], SETUPDAT[2]);
    EP0BCH = 0;
    EP0BCL = 4;
  }
  else if (SETUPDAT[1] == VR_REG_WRITE) {
    if (len < 4) return TRUE;
    EP0BCH = EP0BCL = 0;
    while (EP0CS & bmEPBUSY) ;
    write_reg(SETUPDAT[3], SETUPDAT[2], *(uint32_t *)EP0BUF);
  }
  else if (SETUPDAT[1] == VR_START) {
    EP0BCH = EP0BCL = 0;
    start_bulk();
  }
  else if (SETUPDAT[1] == VR_STOP) {
    EP0BCH = EP0BCL = 0;
    stop_bulk();
  }
  else if (SETUPDAT[1] == VR_RESET) {
    EP0BCH = EP0BCL = 0;
    stop_bulk();
    setup();
  }
  else if (SETUPDAT[1] == VR_SAVE) {
    EP0BCH = EP0BCL = 0;
    save_settings();
  }
#ifdef ENA_VR_EE // disabled to save code size
  else if (SETUPDAT[1] == VR_EE_READ) {
    if (len > 64) return TRUE;
    read_eeprom(val, (uint8_t)len, EP0BUF);
    EP0BCH = 0;
    EP0BCL = (uint8_t)len;
  }
  else if (SETUPDAT[1] == VR_EE_WRITE) {
    if (len > 64 || val < EE_ADDR_0 || val + len > EE_ADDR_1) {
        return TRUE;
    }
    EP0BCH = EP0BCL = 0;
    while (EP0CS & bmEPBUSY) ;
    write_eeprom(val, (uint8_t)len, EP0BUF);
  }
#endif
  else {
    return TRUE; // undefined vendor request
  }
  return FALSE;
}

// Cypress firmware framework functions ----------------------------------------
extern BOOL GotSUD, Sleep, Rwuen;
BYTE config, altset;

// task dispatcher hooks
void TD_Init            (void) {Rwuen = TRUE; setup();}
void TD_Poll            (void) {loop();}
BOOL TD_Suspend         (void) {return TRUE;}
BOOL TD_Resume          (void) {return TRUE;}
BOOL DR_GetDescriptor   (void) {return TRUE;}
BOOL DR_GetStatus       (void) {return TRUE;}
BOOL DR_ClearFeature    (void) {return TRUE;}
BOOL DR_SetFeature      (void) {return TRUE;}
BOOL DR_VendorCmnd      (void) {return handle_req();}
BOOL DR_SetConfiguration(void) {config = SETUPDAT[2]; return TRUE;}
BOOL DR_SetInterface    (void) {altset = SETUPDAT[2]; return TRUE;}
BOOL DR_GetConfiguration(void) {EP0BUF[0] = config; EP0BCH = 0; EP0BCL = 1; return TRUE;}
BOOL DR_GetInterface    (void) {EP0BUF[0] = altset; EP0BCH = 0; EP0BCL = 1; return TRUE;}

// interrupt handlers
void ISR_Sudav(void) interrupt 0 {
  SUDPTRCTL |= bmSDPAUTO;
  GotSUD = TRUE;
  EZUSB_IRQ_CLEAR();
  USBIRQ = bmSUDAV;
}
void ISR_Sutok(void) interrupt 0 {
  EZUSB_IRQ_CLEAR();
  USBIRQ = bmSUTOK;
}
void ISR_Sof(void) interrupt 0 {
  EZUSB_IRQ_CLEAR();
  USBIRQ = bmSOF;
}
void ISR_Ures(void) interrupt 0 {
  if (EZUSB_HIGHSPEED()) {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
  }
  else {
    pConfigDscr = pFullSpeedConfigDscr;
    pOtherConfigDscr = pHighSpeedConfigDscr;
  }
  EZUSB_IRQ_CLEAR();
  USBIRQ = bmURES;
}
void ISR_Susp(void) interrupt 0 {
  Sleep = TRUE;
  EZUSB_IRQ_CLEAR();
  USBIRQ = bmSUSP;
}
void ISR_Highspeed(void) interrupt 0 {
  if (EZUSB_HIGHSPEED()) {
    pConfigDscr = pHighSpeedConfigDscr;
    pOtherConfigDscr = pFullSpeedConfigDscr;
  }
  else {
    pConfigDscr = pFullSpeedConfigDscr;
    pOtherConfigDscr = pHighSpeedConfigDscr;
  }
  EZUSB_IRQ_CLEAR();
  USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack      (void) interrupt 0 {}
void ISR_Stub        (void) interrupt 0 {}
void ISR_Ep0in       (void) interrupt 0 {}
void ISR_Ep0out      (void) interrupt 0 {}
void ISR_Ep1in       (void) interrupt 0 {}
void ISR_Ep1out      (void) interrupt 0 {}
void ISR_Ep2inout    (void) interrupt 0 {}
void ISR_Ep4inout    (void) interrupt 0 {}
void ISR_Ep6inout    (void) interrupt 0 {}
void ISR_Ep8inout    (void) interrupt 0 {}
void ISR_Ibn         (void) interrupt 0 {}
void ISR_Ep0pingnak  (void) interrupt 0 {}
void ISR_Ep1pingnak  (void) interrupt 0 {}
void ISR_Ep2pingnak  (void) interrupt 0 {}
void ISR_Ep4pingnak  (void) interrupt 0 {}
void ISR_Ep6pingnak  (void) interrupt 0 {}
void ISR_Ep8pingnak  (void) interrupt 0 {}
void ISR_Errorlimit  (void) interrupt 0 {}
void ISR_Ep2piderror (void) interrupt 0 {}
void ISR_Ep4piderror (void) interrupt 0 {}
void ISR_Ep6piderror (void) interrupt 0 {}
void ISR_Ep8piderror (void) interrupt 0 {}
void ISR_Ep2pflag    (void) interrupt 0 {}
void ISR_Ep4pflag    (void) interrupt 0 {}
void ISR_Ep6pflag    (void) interrupt 0 {}
void ISR_Ep8pflag    (void) interrupt 0 {}
void ISR_Ep2eflag    (void) interrupt 0 {}
void ISR_Ep4eflag    (void) interrupt 0 {}
void ISR_Ep6eflag    (void) interrupt 0 {}
void ISR_Ep8eflag    (void) interrupt 0 {}
void ISR_Ep2fflag    (void) interrupt 0 {}
void ISR_Ep4fflag    (void) interrupt 0 {}
void ISR_Ep6fflag    (void) interrupt 0 {}
void ISR_Ep8fflag    (void) interrupt 0 {}
void ISR_GpifComplete(void) interrupt 0 {}
void ISR_GpifWaveform(void) interrupt 0 {}

// end of Cypress firmware framework functions ---------------------------------
