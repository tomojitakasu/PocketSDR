// 
//  Pocket SDR C Library - GNSS SDR Device Configuration Functions.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2021-10-03  0.1  new
//  2022-01-04  1.0  support C++
//  2022-05-23  1.1  change coding style
//  2022-08-08  1.2  support Spider SDR
//  2024-04-20  1.3  support Pocket SDR FE 4CH
//  2024-06-29  1.4  delete API sdr_read_settings(), sdr_write_settings()
//                   add API sdr_conf_read(), sdr_conf_write()
//  2024-11-25  1.5  support Pocket SDR FE 8CH
//
#include "pocket_sdr.h"

// constants -------------------------------------------------------------------
#define POCKET_DEV_NAME SDR_DEV_NAME
#define SPIDER_DEV_NAME "Spider SDR"
#define TYPE_POCKET_2CH 0      // device type: Pocket SDR FE 2CH
#define TYPE_POCKET_4CH 1      // device type: Pocket SDR FE 4CH
#define TYPE_SPIDER     2      // device type: Spider SDR
#define TYPE_POCKET_8CH 3      // device type: Pocket SDR FE 8CH
#define MAX_REG_MAX2771 11     // number of registers of MAX2771
#define MAX_REG_MAX2769 10     // number of registers of MAX2769

// type definitions ------------------------------------------------------------
typedef struct {       // register field definition type
    const char *field; // field name
    uint8_t addr;      // register address
    uint8_t nbit;      // number of bits
    uint8_t pos;       // bit position (0:LSB,31:MSB)
    uint8_t fix[SDR_MAX_RFCH];  // fixed setting (0:free,1:fixed)
    uint32_t val[SDR_MAX_RFCH]; // value for fixed setting
    const char *desc;  // description
} reg_t;

// device register definitions -------------------------------------------------
static const reg_t MAX2771_field[] = { // MAX2771 register field definitions
    // field          ,addr,nbit,pos, fix[],  val[], desc
    {"CHIPEN"         , 0x0,  1, 31, {1, 1}, {1, 1}, "Chip enable (0:disable,1:enable)"},
    {"IDLE"           , 0x0,  1, 30, {1, 1}, {0, 0}, "Idle enable (0:operating-mode,1:idle-mode)"},
    {"MIXPOLE"        , 0x0,  1, 17, {1, 1}, {0, 0}, "Mixer pole selection (0:13MHz,1:36MHz)"},
    {"LNAMODE"        , 0x0,  2, 15, {0, 0}, {0, 1}, "LNA mode selection (0:high-band,1:low-band,2:disable)"},
    {"MIXERMODE"      , 0x0,  2, 13, {0, 0}, {0, 1}, "Mixer mode selection (0:high-band,1:low-band,2:disable)"},
    {"FCEN"           , 0x0,  7,  6, {0, 0}, {0, 0}, "IF filter center frequency: (128-FCEN)/2*{0.195|0.66|0.355} MHz"},
    {"FBW"            , 0x0,  3,  3, {0, 0}, {0, 0}, "IF filter BW (0:2.5MHz,1:8.7MHz,2:4.2MHz,3:23.4MHz,4:36MHz,7:16.4MHz)"},
    {"F3OR5"          , 0x0,  1,  2, {0, 0}, {0, 0}, "Filter order selection (0:5th,1:3rd)"},
    {"FCENX"          , 0x0,  1,  1, {0, 0}, {0, 0}, "Polyphase filter selection (0:lowpass,1:bandpass)"},
    {"FGAIN"          , 0x0,  1,  0, {0, 0}, {0, 0}, "IF filter gain setting (0:-6dB,1:normal)"},
    {"ANAIMON"        , 0x1,  1, 28, {1, 1}, {0, 0}, "Enable continuous spectrum monitoring (0:disable,1:enable)"},
    {"IQEN"           , 0x1,  1, 27, {0, 0}, {0, 0}, "I and Q channel enable (0:I-CH-only,1:I/Q-CH)"},
    {"GAINREF"        , 0x1, 12, 15, {0, 0}, {0, 0}, "AGC gain reference value (0-4095)"},
    {"SPI_SDIO_CONFIG", 0x1,  2, 13, {1, 1}, {0, 0}, "SPI SDIO pin config (0:none,1:pull-down,2:pull-up,3:bus-hold)"},
    {"AGCMODE"        , 0x1,  2, 11, {0, 0}, {0, 0}, "AGC mode control (0:independent-I/Q,2:gain-set-by-GAININ)"},
    {"FORMAT"         , 0x1,  2,  9, {1, 1}, {1, 1}, "Output data format (0:unsigned,1:sign-magnitude,2:2's-complement)"},
    {"BITS"           , 0x1,  3,  6, {1, 1}, {2, 2}, "Number of bits in ADC (0:1bit,2:2bit,4:3bit)"},
    {"DRVCFG"         , 0x1,  2,  4, {1, 1}, {0, 0}, "Output driver config (0:CMOS-logic,2:analog)"},
    {"DIEID"          , 0x1,  2,  0, {1, 1}, {0, 0}, "Identifiers version of IC"},
    {"GAININ"         , 0x2,  6, 22, {0, 0}, {0, 0}, "PGA gain value programming in steps of approx 1dB per LSB (0-63)"},
    {"HILODEN"        , 0x2,  1, 20, {1, 1}, {0, 0}, "Enable output driver to drive high loads (0:disable,1:enable)"},
    {"FHIPEN"         , 0x2,  1, 15, {0, 0}, {1, 1}, "Enable highpass coupling between filter and PGA (0:disable,1:enable)"},
    {"PGAIEN"         , 0x2,  1, 13, {0, 0}, {0, 0}, "I-CH PGA enable (0:disable,1:enable)"},
    {"PGAQEN"         , 0x2,  1, 12, {0, 0}, {0, 0}, "Q-CH PGA enable (0:disable,1:enable)"},
    {"STRMEN"         , 0x2,  1, 11, {1, 1}, {0, 0}, "Enable DSP interface (0:disable,1:enable)"},
    {"STRMSTART"      , 0x2,  1, 10, {1, 1}, {0, 0}, "Enable data streaming (rising edge)"},
    {"STRMSTOP"       , 0x2,  1,  9, {1, 1}, {0, 0}, "Disable data streaming (rising edge)"},
    {"STRMBITS"       , 0x2,  2,  4, {1, 1}, {1, 1}, "Number of bits streamed (1:IMSB/ILSB,3:IMSB/ILSB/QMSB/QLSB)"},
    {"STAMPEN"        , 0x2,  1,  3, {1, 1}, {0, 0}, "Enable insertion of frame numbers (0:disable,1:enable)"},
    {"TIMESYNCEN"     , 0x2,  1,  2, {1, 1}, {0, 0}, "Enable output of time sync pulse when streaming enabled by STRMEN"},
    {"DATASYNCEN"     , 0x2,  1,  1, {1, 1}, {0, 0}, "Enable sync pulse at DATASYNC"},
    {"STRMRST"        , 0x2,  1,  0, {1, 1}, {0, 0}, "Reset all counters"},
    {"LOBAND"         , 0x3,  1, 28, {0, 0}, {0, 1}, "Local oscillator band selection (0:L1,1:L2/L5)"},
    {"REFOUTEN"       , 0x3,  1, 24, {1, 1}, {1, 1}, "Output clock buffer enable (0:disable,1:enable)"},
    {"IXTAL"          , 0x3,  2, 19, {1, 1}, {1, 1}, "Current programming for XTAL (1:normal,3:high-current)"},
    {"ICP"            , 0x3,  1,  9, {1, 1}, {0, 0}, "Charge pump current selection (0:0.5mA,1:1mA)"},
    {"INT_PLL"        , 0x3,  1,  3, {0, 0}, {0, 0}, "PLL mode control (0:fractional-N,1:integer-N)"},
    {"PWRSAV"         , 0x3,  1,  2, {1, 1}, {0, 0}, "Enable PLL power-save mode (0:disable,1:enable)"},
    {"NDIV"           , 0x4, 15, 13, {0, 0}, {0, 0}, "PLL integer division ratio (36-32767): F_LO=F_XTAL/RDIV*(NDIV+FDIV/2^20)"},
    {"RDIV"           , 0x4, 10,  3, {0, 0}, {0, 0}, "PLL reference division ratio (1-1023)"},
    {"FDIV"           , 0x5, 20,  8, {0, 0}, {0, 0}, "PLL fractional division ratio (0-1048575)"},
    {"EXTADCCLK"      , 0x7,  1, 28, {1, 1}, {1, 1}, "External ADC clock selection (0:internal,1:ADC_CLKIN)"},
    {"PREFRACDIV_SEL" , 0xA,  1,  3, {0, 1}, {0, 0}, "Clock pre-divider selection (0:bypass,1:enable)"},
    {"REFCLK_L_CNT"   , 0x7, 12, 16, {0, 1}, {0, 0}, "Clock pre-divider L counter value (0-4095): L_CNT/(4096-M_CNT+L_CNT)"},
    {"REFCLK_M_CNT"   , 0x7, 12,  4, {0, 1}, {0, 0}, "Clock pre-divider M counter value (0-4095)"},
    {"ADCCLK"         , 0x7,  1,  2, {0, 1}, {0, 0}, "Integer clock div/mul selection (0:enable,1:bypass)"},
    {"REFDIV"         , 0x3,  3, 29, {0, 1}, {0, 0}, "Integer clock div/mul ratio (0:x2,1:1/4,2:1/2,3:x1,4:x4)"},
    {"FCLKIN"         , 0x7,  1,  3, {0, 1}, {0, 0}, "ADC clock divider selection (0:bypass,1:enable)"},
    {"ADCCLK_L_CNT"   , 0xA, 12, 16, {0, 1}, {0, 0}, "ADC clock divider L counter value (0-4095): L_CNT/(4096-M_CNT+L_CNT)"},
    {"ADCCLK_M_CNT"   , 0xA, 12,  4, {0, 1}, {0, 0}, "ADC clock divider M counter value (0-4095)"},
    {"CLKOUT_SEL"     , 0xA,  1,  2, {1, 1}, {1, 1}, "CLKOUT selection (0:integer-clock-div/mul,1:ADC-clock)"},
    {"MODE"           , 0x7,  1,  0, {1, 1}, {0, 0}, "DSP interface mode selection"},
    {"", 0, 0, 0, {0}, {0}, ""} // terminator
};

static const reg_t MAX2769B_field[] = { // MAX2769B register field definitions
    // field          ,addr,nbit,pos, fix[],  val[], desc
    {"CHIPEN"         , 0x0,  1, 27, {1, 1}, {1, 1}, "Chip enable (0:disable,1:enable)"},
    {"IDLE"           , 0x0,  1, 26, {1, 1}, {0, 0}, "Idle enable (0:operating-mode,1:idle-mode)"},
    {"MIXPOLE"        , 0x0,  1, 15, {1, 1}, {0, 0}, "Mixer pole selection (0:13MHz,1:36MHz)"},
    {"LNAMODE"        , 0x0,  2, 13, {1, 1}, {1, 1}, "LNA mode selection (0:by-ant-bias,1:LNA2,2:LNA1,3:off)"},
    {"MIXEN"          , 0x0,  1, 12, {1, 1}, {1, 1}, "Mixer enable (0:disable,1:enable)"},
    {"ANTEN"          , 0x0,  1, 11, {1, 1}, {0, 0}, "Antenna bias enable (0:disable,1:enable)"},
    {"FCEN"           , 0x0,  6,  5, {0, 0}, {0, 0}, "IF center freq. LSB 6bits (((128-flip(FCENMSB|FCEN))/2*{0.195|0.66|0.355}MHz))"},
    {"FBW"            , 0x0,  2,  3, {0, 0}, {0, 0}, "IF filter center bandwidth (0:2.5MHz,1:9.66MHz,2:4.2MHz)"},
    {"F3OR5"          , 0x0,  1,  2, {0, 0}, {0, 0}, "Filter order selection (0:5th,1:3rd)"},
    {"FCENX"          , 0x0,  1,  1, {1, 1}, {1, 1}, "Polyphase filter selection (0:lowpass,1:bandpass)"},
    {"FGAIN"          , 0x0,  1,  0, {1, 1}, {1, 1}, "IF filter gain (0:-6dB,1:0dB)"},
    {"IQEN"           , 0x1,  1, 27, {1, 1}, {0, 0}, "I and Q channels enable (0:I-CH-only,1:I/Q-CH)"},
    {"GAINREF"        , 0x1, 12, 15, {0, 0}, {0, 0}, "AGC gain ref value (0-4095)"},
    {"AGCMODE"        , 0x1,  2, 11, {0, 0}, {0, 0}, "AGC mode control (0:independent-I/Q,2:set-from-GAININ)"},
    {"FORMAT"         , 0x1,  2,  9, {1, 1}, {1, 1}, "Output data format (0:unsigned,1:sign-magnitude,2:2's-complement)"},
    {"BITS"           , 0x1,  3,  6, {1, 1}, {2, 2}, "Number of bits in ADC (0:1bit,2:2bit,4:3bit)"},
    {"DRVCFG"         , 0x1,  2,  4, {1, 1}, {0, 0}, "Output driver config (0:CMOS-logic,2:analog)"},
    {"DIEID"          , 0x1,  2,  0, {1, 1}, {0, 0}, "Identifiers version of IC"},
    {"GAININ"         , 0x2,  6, 22, {0, 0}, {0, 0}, "PGA gain value programming ((GAININ-1)dB) (0-63)"},
    {"HILOADEN"       , 0x2,  1, 20, {1, 1}, {0, 0}, "Enable output driver to drive high loads (0:disable,1:enable)"},
    {"FHIPEN"         , 0x2,  1, 15, {0, 0}, {1, 1}, "Enable highpass coupling between filter and PGA (0:disable,1:enable)"},
    {"STRMEN"         , 0x2,  1, 11, {1, 1}, {0, 0}, "Enable DSP interface (0:disable,1:enable)"},
    {"STRMSTART"      , 0x2,  1, 10, {1, 1}, {0, 0}, "Enable data streaming"},
    {"STRMSTOP"       , 0x2,  1,  9, {1, 1}, {0, 0}, "Disable data streaming"},
    {"STRMBITS"       , 0x2,  2,  4, {1, 1}, {0, 0}, "Number of bits streamed"},
    {"STRMPEN"        , 0x2,  1,  3, {1, 1}, {0, 0}, "Enable insertion of frame numbers (0:disble,1:enable)"},
    {"TIMESYNCEN"     , 0x2,  1,  2, {1, 1}, {0, 0}, "Enable output of time sync pulses (0:disable,1:enable)"},
    {"DATSYNCEN"      , 0x2,  1,  1, {1, 1}, {0, 0}, "Enable sync pulses at DATASYNC (0:disable,1:enable)"},
    {"STRMRST"        , 0x2,  1,  0, {1, 1}, {0, 0}, "Reset all counters"},
    {"REFOUTEN"       , 0x3,  1, 24, {1, 1}, {0, 0}, "Clock buffer enable (0:disable,1:enable)"},
    {"REFDIV"         , 0x3,  2, 21, {1, 1}, {3, 3}, "Clock output divider ratio (0:x2,1:1/4,2:1/2,3:x1)"},
    {"IXTAL"          , 0x3,  2, 19, {1, 1}, {0, 0}, "Current programing for XTAL (1:normal,3:high-current)"},
    {"LDMUX"          , 0x3,  4, 10, {1, 1}, {0, 0}, "Enable PLL lock-detect (0:disable,1:enable)"},
    {"ICP"            , 0x3,  1,  9, {1, 1}, {0, 0}, "Charge pump current selection (0:0.5mA,1:1mA)"},
    {"PFDEN"          , 0x3,  1,  8, {1, 1}, {0, 0}, "PLL phase freq. detector (0:normal,1:disable)"},
    {"INT_PLL"        , 0x3,  1,  3, {0, 0}, {0, 0}, "PLL mode control (0:fractional-N,1:integer-N)"},
    {"PWRSAV"         , 0x3,  1,  2, {1, 1}, {0, 0}, "Enable PLL power-save mode (0:disable,1:enable)"},
    {"NDIV"           , 0x4, 15, 13, {0, 0}, {0, 0}, "PLL integer division ratio (36-32767): F_LO=F_XTAL/RDIV*(NDIV+FDIV/2^20)"},
    {"RDIV"           , 0x4, 10,  3, {0, 0}, {0, 0}, "PLL reference division ratio (1-1024)"},
    {"FDIV"           , 0x5, 20,  8, {0, 0}, {0, 0}, "PLL fractional divider ratio (0-1048575)"},
    {"L_CNT"          , 0x7, 12, 16, {1, 1}, {0, 0}, "ADC clock divider L counter value (0-4095): L_CNT/(4096-M_CNT+L_CNT)"},
    {"M_CNT"          , 0x7, 12,  4, {1, 1}, {0, 0}, "ADC clock divider M counter value (0-4095)"},
    {"FCLKIN"         , 0x7,  1,  3, {1, 1}, {0, 0}, "ADC clock divider selection (0:bypass,1:enable)"},
    {"ADCCLK"         , 0x7,  1,  2, {1, 1}, {0, 0}, "Integer clock dev/mul selection (0:enable,1:bypass)"},
    {"MODE"           , 0x7,  1,  0, {1, 1}, {0, 0}, "DSP interface mode selection"},
    {"FCENMSB"        , 0x9,  1,  0, {0, 0}, {0, 0}, "IF center freq. MSB 1bit"},
    {"", 0, 0, 0, {0}, {0}, ""} // terminator
};

// max number of channels ------------------------------------------------------
static int max_ch(int type)
{
    switch (type) {
        case TYPE_POCKET_2CH: return 2;
        case TYPE_POCKET_4CH: return 4;
        case TYPE_SPIDER    : return 8;
        case TYPE_POCKET_8CH: return 8;
    }
    return 0;
}

// max number of registers -----------------------------------------------------
static int max_reg(int type)
{
    switch (type) {
        case TYPE_POCKET_2CH:
        case TYPE_POCKET_4CH:
        case TYPE_POCKET_8CH: return MAX_REG_MAX2771;
        case TYPE_SPIDER    : return MAX_REG_MAX2769;
    }
    return 0;
}

// bit mask --------------------------------------------------------------------
static uint32_t bit_mask(const reg_t *reg)
{
    uint32_t mask = 0;
    int pos1 = reg->pos, pos2 = pos1 + reg->nbit;
    
    for (int i = 31; i >= 0; i--) {
        mask = (mask << 1) | (i >= pos1 && i < pos2 ? 1 : 0);
    }
    return mask;
}

// read device type and TCXO frequency -----------------------------------------
static int read_dev_type(sdr_usb_t *usb, double *fx)
{
    uint8_t data[6] = {0};
    
    // read device info and status
    if (!sdr_usb_req(usb, 0, SDR_VR_STAT, 0, data, 6)) {
       return -1;
    }
    *fx = (((uint16_t)data[1] << 8) + data[2]) * 1e3;
    if ((data[3] >> 4) &  1) return TYPE_SPIDER;
    if ((data[0] >> 4) <= 2) return TYPE_POCKET_2CH; // F/W ver.1 (FE 2CH)
    if ((data[0] >> 4) == 3) return TYPE_POCKET_4CH; // F/W ver.3 (FE 4CH)
    if ((data[0] >> 4) == 4) return TYPE_POCKET_8CH; // F/W ver.4 (FE 8CH)
    return -1;
}

// read settings from configuration file in hexadecimal format -----------------
static void read_config_hex(FILE *fp, int type, uint32_t regs[][SDR_MAX_REG])
{
    char buff[128];
    
    while (fgets(buff, sizeof(buff), fp)) {
        uint32_t addr, val;
        int ch;
        char *p;
        
        if ((p = strchr(buff, '#'))) *p = '\0';
        if (sscanf(buff, "%d 0x%X 0x%X", &ch, &addr, &val) < 3) continue;
        if (ch < 1 || ch > max_ch(type)) {
            fprintf(stderr, "Invalid channel: CH=%d\n", ch);
            continue;
        }
        if ((int)addr > max_reg(type)) {
            fprintf(stderr, "Invalid address: ADDR=0x%X\n", addr);
            continue;
        }
        regs[ch-1][addr] = val;
    }
}

// read settings from configuration file in keyword = value format -------------
static void read_config_key(FILE *fp, int type, uint32_t regs[][SDR_MAX_REG])
{
    const reg_t *reg_field =
        (type == TYPE_SPIDER) ? MAX2769B_field : MAX2771_field;
    char buff[128];
    int ch = 1;
    
    while (fgets(buff, sizeof(buff), fp)) {
        uint32_t val, mask;
        char key[32], *p;
        
        if ((p = strchr(buff, '#'))) *p = '\0';
        if (sscanf(buff, "[CH%d]", &ch) == 1) continue;
        if (ch < 1 || ch > max_ch(type)) continue;
        if (!(p = strchr(buff, '='))) continue;
        *p++ = '\0';
        if (sscanf(buff, "%31s", key) < 1) continue;
        int i;
        for (i = 0; *reg_field[i].field; i++) {
            if (!strcmp(key, reg_field[i].field)) break;
        }
        if (!*reg_field[i].field) {
            fprintf(stderr, "Invalid field: [CH%d] %s\n", ch, key);
            continue;
        }
        if (sscanf(p, "%d", (int *)&val) < 1 && sscanf(p, "0x%X", &val) < 1) {
            fprintf(stderr, "Invalid value: [CH%d] %s = %s\n", ch, key, p);
            continue;
        }
        if (val >= ((uint32_t)1 << reg_field[i].nbit)) {
            fprintf(stderr, "Invalid value: [CH%d] %s = %d\n", ch, key, val);
            continue;
        }
        mask = bit_mask(reg_field + i);
        regs[ch-1][reg_field[i].addr] &= ~mask;
        regs[ch-1][reg_field[i].addr] |= (val << reg_field[i].pos) & mask;
    }
}

// read settings from configuration file ---------------------------------------
static int read_config(const char *file, int type, uint32_t regs[][SDR_MAX_REG],
    int opt)
{
    FILE *fp;
    
    if (!(fp = fopen(file, "r"))) {
        fprintf(stderr, "File open error. %s\n", file);
        return 0;
    }
    if (opt & 4) {
        read_config_hex(fp, type, regs);
    }
    else {
        read_config_key(fp, type, regs);
    }
    fclose(fp);
    return 1;
}

// write MAX2771 channel status ------------------------------------------------
static void write_MAX2771_stat(FILE *fp, double fx, int ch, uint32_t *reg)
{
    static double f_bw[8] = {2.5, 8.7, 4.2, 23.4, 36.0, 0.0, 0.0, 16.4};
    static double f_step[8] = {0.195, 0.66, 0.355};
    static double ratio[8] = {2.0, 0.25, 0.5, 1.0, 4.0};
    
    uint32_t FCEN       = (reg[0x0] >>  6) & 0x7F;
    uint32_t FBW        = (reg[0x0] >>  3) & 0x7;
    uint32_t FCENX      = (reg[0x0] >>  1) & 0x1;
    uint32_t ENIQ       = (reg[0x1] >> 27) & 0x1;
    uint32_t INT_PLL    = (reg[0x3] >>  3) & 0x1;
    uint32_t NDIV       = (reg[0x4] >> 13) & 0x7FFF;
    uint32_t RDIV       = (reg[0x4] >>  3) & 0x3FF;
    uint32_t FDIV       = (reg[0x5] >>  8) & 0xFFFFF;
    uint32_t REFDIV     = (reg[0x3] >> 29) & 0x7;
    uint32_t EXTADCCLK  = (reg[0x7] >> 28) & 0x1;
    uint32_t FCLKIN     = (reg[0x7] >>  3) & 0x1;
    uint32_t ADCCLK     = (reg[0x7] >>  2) & 0x1;
    uint32_t REFCLK_L   = (reg[0x7] >> 16) & 0xFFF;
    uint32_t REFCLK_M   = (reg[0x7] >>  4) & 0xFFF;
    uint32_t ADCCLK_L   = (reg[0xA] >> 16) & 0xFFF;
    uint32_t ADCCLK_M   = (reg[0xA] >>  4) & 0xFFF;
    uint32_t PREFRACDIV = (reg[0xA] >>  3) & 0x1;
    
    double f_lo = fx / RDIV;
    f_lo *= INT_PLL ? NDIV : NDIV + FDIV / 1048576.0;
    double f_adc = (ch != 0 && EXTADCCLK) ? 0.0 : fx;
    f_adc *= !PREFRACDIV ? 1.0 : REFCLK_L / (4096.0 - REFCLK_M + REFCLK_L);
    f_adc *= ADCCLK      ? 1.0 : ratio[REFDIV];
    f_adc *= !FCLKIN     ? 1.0 : ADCCLK_L / (4096.0 - ADCCLK_M + ADCCLK_L);
    double f_cen = FCENX ? (128 - FCEN) / 2.0 * f_step[FBW] : 0.0;
    fprintf(fp, "#  [CH%d] F_LO =%9.3f MHz, F_ADC =%7.3f MHz (%-2s), "
        "F_FILT =%5.1f MHz, BW_FILT =%5.1f MHz\n", ch + 1, f_lo, f_adc,
        ENIQ ? "IQ" : "I", f_cen, f_bw[FBW]);
}

// flip bits -------------------------------------------------------------------
static uint32_t flip_bits(uint32_t reg, int nbit)
{
    uint32_t reg_flip = 0;
    for (int i = 0; i < nbit; i++) {
        reg_flip = (reg_flip << 1) | ((reg >> i) & 1);
    }
    return reg_flip;
}

// write MAX2769B channel status -----------------------------------------------
static void write_MAX2769B_stat(FILE *fp, double fx, int ch, uint32_t *reg)
{
    static double f_bw[8] = {2.5, 9.66, 4.2, 0.0};
    static double f_step[8] = {0.195, 0.66, 0.355, 0.0};
    static double ratio[8] = {2.0, 0.25, 0.5, 1.0};
    
    uint32_t FCEN       = ((reg[0x0] >>  5) & 0x3F) + ((reg[0x9] & 0x1) << 6);
    uint32_t FCENX      = (reg[0x0] >>  1) & 0x1;
    uint32_t FBW        = (reg[0x0] >>  3) & 0x3;
    uint32_t IQEN       = (reg[0x1] >> 27) & 0x1;
    uint32_t INT_PLL    = (reg[0x3] >>  3) & 0x1;
    uint32_t NDIV       = (reg[0x4] >> 13) & 0x7FFF;
    uint32_t RDIV       = (reg[0x4] >>  3) & 0x3FF;
    uint32_t FDIV       = (reg[0x5] >>  8) & 0xFFFFF;
    uint32_t REFDIV     = (reg[0x3] >> 21) & 0x3;
    uint32_t L_CNT      = (reg[0x7] >> 16) & 0xFFF;
    uint32_t M_CNT      = (reg[0x7] >>  4) & 0xFFF;
    uint32_t FCLKIN     = (reg[0x7] >>  3) & 0x1;
    uint32_t ADCCLK     = (reg[0x7] >>  2) & 0x1;
    
    double f_lo = fx / RDIV;
    f_lo *= INT_PLL ? NDIV : NDIV + FDIV / 1048576.0;
    double f_adc = fx * (ADCCLK ? 1.0 : ratio[REFDIV]);
    f_adc *= !FCLKIN ? 1.0 : L_CNT / (4096.0 - M_CNT + L_CNT);
    double f_cen = FCENX ? (128 - flip_bits(FCEN, 7)) / 2.0 * f_step[FBW] : 0.0;
    fprintf(fp, "#  [CH%d] F_LO =%9.3f MHz, F_ADC =%7.3f MHz (%-2s), "
        "F_FILT =%5.1f MHz, BW_FILT =%5.1f MHz\n", ch + 1, f_lo, f_adc,
        IQEN ? "IQ" : "I", f_cen, f_bw[FBW]);
}

// write device channel status -------------------------------------------------
static void write_stat(FILE *fp, int type, double fx, int ch, uint32_t *reg)
{
    if (type == TYPE_SPIDER) {
        write_MAX2769B_stat(fp, fx, ch, reg);
    }
    else {
        write_MAX2771_stat(fp, fx, ch, reg);
    }
}

// write settings to configuration file in hexadecimal format ------------------
static void write_config_hex(FILE *fp, int type, uint32_t regs[][SDR_MAX_REG])
{
    fprintf(fp, "#%2s  %4s  %10s\n", "CH", "ADDR", "VALUE");
    
    for (int i = 0; i < max_ch(type); i++) {
        for (int j = 0; j < max_reg(type); j++) {
            fprintf(fp, "%3d  0x%02X  0x%08X\n", i + 1, j, regs[i][j]);
        }
    }
}

// write settings to configuration file in keyword = value format --------------
static void write_config_key(FILE *fp, int type, double fx,
    uint32_t regs[][SDR_MAX_REG], int opt)
{
    const reg_t *reg_field =
        (type == TYPE_SPIDER) ? MAX2769B_field : MAX2771_field;
    
    fprintf(fp, "#\n#  %s device settings (%s)\n#\n",
        (type == TYPE_SPIDER) ? SPIDER_DEV_NAME : POCKET_DEV_NAME,
        (type == TYPE_SPIDER) ? "MAX2769B" : "MAX2771");
    
    for (int i = 0; i < max_ch(type); i++) {
        write_stat(fp, type, fx, i, regs[i]);
    }
    for (int i = 0; i < max_ch(type); i++) {
        fprintf(fp, "\n[CH%d]\n", i + 1);
        
        for (int j = 0; *reg_field[j].field; j++) {
            uint32_t val, mask;
            
            if (!(opt & 1)) {
                if (type == TYPE_SPIDER) {
                    if (reg_field[j].fix[0]) continue;
                }
                else {
                    if (reg_field[j].fix[i >= 1 ? 1 : 0]) continue;
                }
            }
            mask = bit_mask(reg_field + j);
            val = (regs[i][reg_field[j].addr] & mask) >> reg_field[j].pos;
            fprintf(fp, "%-15s = %7u  # %s\n", reg_field[j].field, val,
                    reg_field[j].desc);
        }
    }
}

// write settings to configuration file ----------------------------------------
static int write_config(const char *file, int type, double fx,
    uint32_t regs[][SDR_MAX_REG], int opt)
{
    FILE *fp = stdout;
    
    if (*file && !(fp = fopen(file, "w"))) {
        fprintf(stderr, "File open error. %s\n", file);
        return 0;
    }
    if (opt & 4) {
        write_config_hex(fp, type, regs);
    }
    else {
        write_config_key(fp, type, fx, regs, opt);
    }
    fclose(fp);
    return 1;
}

// read device register --------------------------------------------------------
static uint32_t read_reg(sdr_usb_t *usb, int ch, int addr)
{
    uint8_t data[4] = {0};
    
    if (!sdr_usb_req(usb, 0, SDR_VR_REG_READ, (uint16_t)((ch << 8) + addr),
             data, 4)) {
        fprintf(stderr, "register read error. [CH%d] 0x%X\n", ch + 1, addr);
        return 0;
    }
    uint32_t val = 0;
    
    for (int i = 0; i < 4; i++ ) {
        val = (val << 8) | data[i];
    }
    return val;
}

// write device register -------------------------------------------------------
static void write_reg(sdr_usb_t *usb, int ch, int addr, uint32_t val)
{
    uint8_t data[4];
    
    for (int i = 0; i < 4; i++ ) {
        data[i] = (uint8_t)(val >> (3 - i) * 8);
    }
    if (!sdr_usb_req(usb, 1, SDR_VR_REG_WRITE, (uint16_t)((ch << 8) + addr),
             data, 4)) {
        fprintf(stderr, "register write error. [CH%d] 0x%X\n", ch + 1, addr);
    }
}

// read settings from device registers -----------------------------------------
static void read_regs(sdr_usb_t *usb, int type, uint32_t regs[][SDR_MAX_REG])
{
    for (int i = 0; i < max_ch(type); i++) {
        for (int j = 0; j < max_reg(type); j++) {
            regs[i][j] = read_reg(usb, i, j);
        }
    }
}

// set fixed value of settings -------------------------------------------------
static void set_fixed(int type, uint32_t regs[][SDR_MAX_REG])
{
    const reg_t *reg_field =
        (type == TYPE_SPIDER) ? MAX2769B_field : MAX2771_field;
    
    for (int i = 0; *reg_field[i].field; i++) {
        for (int j = 0; j < max_ch(type); j++) {
            uint32_t val;
            if (type == TYPE_SPIDER) {
                if (!reg_field[i].fix[0]) continue;
                val = reg_field[i].val[0];
            }
            else if (!strcmp(reg_field[i].field, "EXTADCCLK")) {
                val = (type == TYPE_POCKET_2CH && j == 0) ? 0 : 1; // 0:int,1:ext
            }
            else {
                if (!reg_field[i].fix[j >= 1 ? 1 : 0]) continue;
                val = reg_field[i].val[j >= 1 ? 1 : 0];
            }
            uint32_t mask = bit_mask(reg_field + i);
            regs[j][reg_field[i].addr] &= ~mask;
            regs[j][reg_field[i].addr] |= (val << reg_field[i].pos) & mask;
        }
    }
}

// write settings to device registers ------------------------------------------
static void write_regs(sdr_usb_t *usb, int type, uint32_t regs[][SDR_MAX_REG])
{
    for (int i = 0; i < max_ch(type); i++) {
        for (int j = 0; j < max_reg(type); j++) {
            // write register except reserved or test reg
            if (type == TYPE_SPIDER) {
                if (j == 6 || j == 8) continue;
            }
            else { // Pocket SDR
                if (j == 6 || j == 8 || j == 9) continue;
            }
            write_reg(usb, i, j, regs[i][j]);
        }
    }
}

// save device registers to EEPROM ---------------------------------------------
static void save_regs(sdr_usb_t *usb)
{
    if (!sdr_usb_req(usb, 1, SDR_VR_SAVE, 0, NULL, 0)) {
        fprintf(stderr, "Register save error.\n");
    }
}

//------------------------------------------------------------------------------
//  Read SDR device settings and output to a configuration file.
//  
//  args:
//      dev      (I)  SDR device
//      file     (I)  configuration file ("": stdout)
//      opt      (I)  options (OR of the followings)
//                      1: read all registers
//                      4: output in hexadecimal format
//
//  return:
//      status (0: error, 1: OK)
//
int sdr_conf_read(sdr_dev_t *dev, const char *file, int opt)
{
    double fx = 0.0;
    
    uint32_t regs[SDR_MAX_RFCH][SDR_MAX_REG] = {{0}};
    
    // read device type and TCXO frequency
    int type = read_dev_type(dev->usb, &fx);
    if (type < 0) {
        fprintf(stderr, "No proper device found.\n");
        return 0;
    }
    // read settings from device registers
    read_regs(dev->usb, type, regs);
    
    // write settings to configuration file
    if (!write_config(file, type, fx * 1e-6, regs, opt)) {
        return 0;
    }
    return 1;
}

//------------------------------------------------------------------------------
//  Write SDR device settings in a configuration file.
//  
//  args:
//      dev      (I)  SDR device
//      file     (I)  configuration file
//      opt      (I)  options (OR of the followings)
//                      1: save settings to EEPROM
//                      4: input in hexadecimal format
//
//  return:
//      status (0: error, 1: OK)
//
int sdr_conf_write(sdr_dev_t *dev, const char *file, int opt)
{
    double fx;
    
    uint32_t regs[SDR_MAX_RFCH][SDR_MAX_REG] = {{0}};
    
    // read device type and TCXO frequency
    int type = read_dev_type(dev->usb, &fx);
    if (type < 0) {
        fprintf(stderr, "No proper device found.\n");
        return 0;
    }
    // read settings from device registers
    read_regs(dev->usb, type, regs);
    
    // set fixed value of settings
    set_fixed(type, regs);
    
    // read settings from configuration file
    if (!read_config(file, type, regs, opt)) {
        return 0;
    }
    // write settings to device registers
    write_regs(dev->usb, type, regs);
    
    if (opt & 1) {
        // save device registers to EEPROM
        save_regs(dev->usb);
    }
    return 1;
}
