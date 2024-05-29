//
//  Pocket SDR C Library - GNSS SDR Nav Data Functions
//
//  References:
//  [1] IS-GPS-200K, NAVSTAR GPS Space Segment/Navigation User Segment
//      Interfaces, May 19, 2019
//  [2] Galileo Open Service Signal In Space Interface Control Document -
//      Issue 1, February 2010
//  [3] Galileo E6-B/C Codes Technical Note - Issue 1, January 2019
//  [4] IS-QZSS-PNT-004, Quasi-Zenith Satellite System Interface Specification
//      Satellite Positioning, Navigation and Timing Service, November 5, 2018
//  [5] IS-QZSS-L6-003, Quasi-Zenith Satellite System Interface Specification
//      Centimeter Level Augmentation Service, August 20, 2020
//  [6] IS-QZSS-TV-004, Quasi-Zenith Satellite System Interface Specification
//      Positioning Technology Verification Service, September 27, 2023
//  [7] BeiDou Navigation Satellite System Signal In Space Interface Control
//      Document - Open Service Signal B1I (Version 3.0), February, 2019
//  [8] BeiDou Navigation Satellite System Signal In Space Interface Control
//      Document - Open Service Signal B1C (Version 1.0), December, 2017
//  [9] BeiDou Navigation Satellite System Signal In Space Interface Control
//      Document - Open Service Signal B2a (Version 1.0), December, 2017
//  [10] BeiDou Navigation Satellite System Signal In Space Interface Control
//      Document - Open Service Signal B2b (Version 1.0), July, 2020
//  [11] BeiDou Navigation Satellite System Signal In Space Interface Control
//      Document - Open Service Signal B3I (Version 1.0), February, 2018
//  [12] IS-GPS-800F, Navstar GPS Space Segment / User Segment L1C Interfaces,
//      March 4, 2019
//  [13] IS-GPS-705A, Navstar GPS Space Segment / User Segment L5 Interfaces,
//      June 8, 2010
//  [14] Global Navigation Satellite System GLONASS Interface Control Document
//      Navigational radiosignal In bands L1, L2 (Edition 5.1), 2008
//  [15] IRNSS SIS ICD for Standard Positioning Service version 1.1, August,
//      2017
//  [16] GLONASS Interface Control Document Code Devision Multiple Access Open
//      Service Navigation Signal in L3 frequency band Edition 1.0, 2016
//  [17] NavIC Signal in Space ICD for Standard Positioning Service in L1
//      Frequency version 1.0, August, 2023
//  [18] GLONASS Interface Control Document Code Devision Multiple Access Open
//      Service Navigation Signal in L1 frequency band Edition 1.0, 2016
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-07-08  1.0  port sdr_nav.py to C
//  2023-12-28  1.1  fix L1CA_SBAS, L5I, L5I_SBAS, L5SI, L5SIV, G1CA and G3OCD
//  2024-01-06  1.2  support I1SD
//  2024-01-12  1.3  support B1CD, B2AD, B2BI
//  2024-01-19  1.4  support G1OCD
//  2024-05-22  1.5  support tow update for pseudorange generation
//
#include "pocket_sdr.h"

// constants -------------------------------------------------------------------
#define THRES_SYNC  0.02      // threshold for symbol sync
#define THRES_LOST  0.002     // threshold for symbol lost
#define GPST_OFF_W  2048      // GPST offset (week) (2019-4-7 ~ 2038-11-20)
#define GPST_GST_W  1024      // GPST - GST (week)
#define GPST_BDT_W  1356      // GPST - BDT (week)
#define GPST_IRT_W  1024      // GPST - IRT (week)
#define GPST_BDT    14.0      // GPST - BDT (s)
#define GPST_UTC    18.0      // GPST - UTC (s) (2017-1-1 ~ )
#define TOFF_L1CA   0.160     // time offset (s) L1CA
#define TOFF_L1CA_S 1.084     // time offset (s) L1CA SBAS
#define TOFF_L1CD  18.511     // time offset (s) L1CD
#define TOFF_L1CP  17.991     // time offset (s) L1CP
#define TOFF_L2CM   0.861     // time offset (s) L2CM
#define TOFF_L5I    0.440     // time offset (s) L5I
#define TOFF_L5Q    0.440     // time offset (s) L5Q ?
#define TOFF_L5I_S  1.088     // time offset (s) L5I SBAS
#define TOFF_L6DE   1.0175    // time offset (s) L6D/E
#define TOFF_G1CA   2.000     // time offset (s) G1CA
#define TOFF_G1OCD  2.207     // time offset (s) G1OCD
#define TOFF_G3OCD  0.340     // time offset (s) G3OCD
#define TOFF_G3OCP  0.340     // time offset (s) G3OCP
#define TOFF_E1B    2.037     // time offset (s) E1B
#define TOFF_E1C    0.897     // time offset (s) E1C
#define TOFF_E5AI  10.240     // time offset (s) E5AI
#define TOFF_E5AQ   0.900     // time offset (s) E5AQ
#define TOFF_E5BI   2.040     // time offset (s) E5BI
#define TOFF_E5BQ   0.900     // time offset (s) E5BQ
#define TOFF_E6B    1.016     // time offset (s) E6B
#define TOFF_E6C    0.900     // time offset (s) E6C
#define TOFF_B1I_D1 6.220     // time offset (s) B1I D1
#define TOFF_B1I_D2 0.622     // time offset (s) B1I D2
#define TOFF_B1CD  18.711     // time offset (s) B1CD
#define TOFF_B1CP  13.991     // time offset (s) B1CP
#define TOFF_B2AD   3.120     // time offset (s) B2AD
#define TOFF_B2AP   0.900     // time offset (s) B2AP
#define TOFF_B2BI   1.016     // time offset (s) B2BI
#define TOFF_I1SD  18.511     // time offset (s) I1SD
#define TOFF_I5S    0.320     // time offset (s) I5S

// function prototypes in sdr_code.c -------------------------------------------
int32_t rev_reg(int32_t R, int N);
int8_t *LFSR(int N, int32_t R, int32_t tap, int n);

// BCH(15,11,1) error correction table ([7] Table 5-2) -------------------------
static uint32_t BCH_CORR_TBL[] = {
    0x0000, 0x0001, 0x0002, 0x0010, 0x0004, 0x0100, 0x0020, 0x0400,
    0x0008, 0x4000, 0x0200, 0x0080, 0x0040, 0x2000, 0x0800, 0x1000
};

// code caches -----------------------------------------------------------------
static uint8_t *CNV2_SF1  [400] = {NULL};
static uint8_t *BCNV1_SF1A[ 63] = {NULL};
static uint8_t *BCNV1_SF1B[200] = {NULL};
static uint8_t *IRNV1_SF1 [400] = {NULL};

// average of IP correlation ---------------------------------------------------
static float mean_IP(const sdr_ch_t *ch, int N)
{
    float P = 0.0;
    
    for (int i = 0; i < N; i++) {
        P += (ch->trk->P[SDR_N_HIST-N+i][0] - P) / (i + 1);
    }
    return P;
}

// sync nav symbols by bit transition ------------------------------------------
static int sync_symb(sdr_ch_t *ch, int N)
{
    if (ch->nav->ssync == 0) {
        float P = 0.0, R = 0.0;
        int n = (N <= 2) ? 1 : N - 1;
        for (int i = 0; i < 2 * n; i++) {
            int8_t code = (i < n) ? -1 : 1;
            P += ch->trk->P[SDR_N_HIST-2*n+i][0] * code / (2 * n);
            R += fabsf(ch->trk->P[SDR_N_HIST-2*n+i][0]) / (2 * n);
        }
        if (fabsf(P) >= R && R >= THRES_SYNC) {
            ch->nav->ssync = ch->lock - n;
            sdr_log(4, "$LOG,%.3f,%s,%d,SYMBOL SYNC (%.3f)", ch->time, ch->sig,
                ch->prn, P);
        }
    }
    else if ((ch->lock - ch->nav->ssync) % N == 0) {
        float P = mean_IP(ch, N);
        if (fabsf(P) >= THRES_LOST) {
            uint8_t sym = (P >= 0.0) ? 1 : 0;
            sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));
            return 1;
        }
        else {
            ch->nav->ssync = ch->nav->rev = 0;
            sdr_log(4, "$LOG,%.3f,%s,%d,SYMBOL LOST (%.3f)", ch->time, ch->sig,
                ch->prn, P);
        }
    }
    return 0;
}

// sync secondary code ---------------------------------------------------------
static int sync_sec_code(sdr_ch_t *ch)
{
    int N = ch->len_sec_code;
    
    if (N < 2 || ch->trk->sec_sync == 0 ||
       (ch->lock - ch->trk->sec_sync) % N != 0) {
        return 0;
    }
    uint8_t sym = (mean_IP(ch, N) >= 0.0) ? 1 : 0;
    sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));
    return 1;
}

// match bits normal -----------------------------------------------------------
static int bmatch_n(const uint8_t *bits0, const uint8_t *bits1, int n, int m)
{
    for (int i = 0, nerr = 0; i < n; i++) {
        if (bits0[i] != bits1[i] && ++nerr > m) return 0;
    }
    return 1;
}

// match bits reverse ----------------------------------------------------------
static int bmatch_r(const uint8_t *bits0, const uint8_t *bits1, int n, int m)
{
    for (int i = 0, nerr = 0; i < n; i++) {
        if (bits0[i] == bits1[i] && ++nerr > m) return 0;
    }
    return 1;
}

// sync nav frame by 2 preambles -----------------------------------------------
static int sync_frame(sdr_ch_t *ch, const uint8_t *preamb, int n, int m,
    const uint8_t *bits, int N)
{
    if (bmatch_n(preamb, bits, n, m) && bmatch_n(preamb, bits + N, n, m)) {
        sdr_log(4, "$LOG,%.3f,%s,%d,FRAME SYNC (N)", ch->time, ch->sig, ch->prn);
        return 0; // normal
    }
    if (bmatch_r(preamb, bits, n, m) && bmatch_r(preamb, bits + N, n, m)) {
        sdr_log(4, "$LOG,%.3f,%s,%d,FRAME SYNC (R)", ch->time, ch->sig, ch->prn);
        return 1; // reversed
    }
    return -1;
}

// test CRC24Q -----------------------------------------------------------------
int test_CRC(const uint8_t *bits, int len_bits)
{
    uint8_t buff[4096];
    int N = (len_bits - 24 + 7) / 8 * 8;
    sdr_pack_bits(bits, len_bits, N + 24 - len_bits, buff); // aligned right
    return rtk_crc24q(buff, N / 8) == getbitu(buff, N, 24);
}

// test CRC(250,234) ([18] 4.4) ------------------------------------------------
int test_CRC16_GLO(const uint8_t *bits, int len_bits)
{
    uint16_t R = 0;
    for (int i = 0; i < 250; i++) {
        R = ((R << 1) | bits[i]) ^ ((R & 0x8000) ? 0x6F63 : 0);
    }
    return R == 0;
}

// to hex string ---------------------------------------------------------------
static void hex_str(const uint8_t *data, int nbits, char *str)
{
    char *p = str;
    for (int i = 0; i < (nbits + 7) / 8; i++) {
        p += sprintf(p, "%02X", data[i]);
    }
}

// update tow ------------------------------------------------------------------
static void update_tow(sdr_ch_t *ch, double tow)
{
    if (ch->tow <= 0) {
        ch->tow = (int)(tow / 1e-3);
    }
    else if (ch->tow == (int)(tow / 1e-3)) {
        ch->tow_v = 1; // tow valid
    }
    else { // TOW mismatch
        trace(2, "tow mismatch: sat=%s sig=%s tow=%.3f -> %.3f\n", ch->sat,
            ch->sig, ch->tow * 1e-3, tow);
        ch->tow = -1;
        ch->tow_v = 0; // tow invalid
    }
}

// unsync navigation message ---------------------------------------------------
static void unsync_nav(sdr_ch_t *ch)
{
    ch->nav->fsync = ch->nav->ssync = ch->nav->rev = 0;
    ch->nav->coff = 0.0;
    ch->tow = -1;
    ch->tow_v = 0;
}

// new nav data ----------------------------------------------------------------
sdr_nav_t *sdr_nav_new(void)
{
    return (sdr_nav_t *)sdr_malloc(sizeof(sdr_nav_t));
}

// free nav data ---------------------------------------------------------------
void sdr_nav_free(sdr_nav_t *nav)
{
    if (!nav) return;
    sdr_free(nav);
}

// initialize nav data ---------------------------------------------------------
void sdr_nav_init(sdr_nav_t *nav)
{
    nav->ssync = nav->fsync = nav->rev = nav->seq = nav->type = nav->stat = 0;
    nav->nerr = 0;
    nav->coff = 0.0;
    memset(nav->syms, 0, SDR_MAX_NSYM);
    memset(nav->data, 0, SDR_MAX_DATA);
}

// sync SBAS message -----------------------------------------------------------
static int sync_SBAS_msgs(const uint8_t *bits, int N)
{
    static const uint8_t preamb[][8] = {
        {0, 1, 0, 1, 0, 0, 1, 1}, {1, 0, 0, 1, 1, 0, 1, 0},
        {1, 1, 0, 0, 0, 1, 1, 0}
    };
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        if (bmatch_n(bits, preamb[i], 8, 0) &&
            bmatch_n(bits + N, preamb[j], 8, 0)) {
            return 0;
        }
        if (bmatch_r(bits, preamb[i], 8, 0) &&
            bmatch_r(bits + N, preamb[j], 8, 0)) {
            return 1;
        }
    }
    return -1;
}

// decode SBAS message ---------------------------------------------------------
static void decode_SBAS_msgs(sdr_ch_t *ch, const uint8_t *bits, int rev)
{
    double toff = !strcmp(ch->sig, "L1CA") ? TOFF_L1CA_S : TOFF_L5I_S;
    double time = ch->time - toff;
    uint8_t buff[250];
    
    for (int i = 0; i < 250; i++) {
        buff[i] = bits[i] ^ (uint8_t)rev;
    }
    if (test_CRC(buff, 250)) {
        ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        ch->tow = (int)(toff / 1e-3);
        ch->tow_v = 2;
        int off = !strcmp(ch->sig, "L1CA") ? 8 : 6;
        ch->nav->type = getbitu(ch->nav->data, off, 6); // SBAS message type
        sdr_pack_bits(buff, 250, 0, ch->nav->data); // SBAS message (250 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(ch->nav->data, 250, str);
        sdr_log(3, "$SBAS,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,SBAS FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// search SBAS message ---------------------------------------------------------
static void search_SBAS_msgs(sdr_ch_t *ch)
{
    uint8_t syms[544], bits[266];
    
    // decode 1/2 FEC (544 syms -> 258 + 8 bits)
    for (int i = 0; i < 544; i++) {
        syms[i] = ch->nav->syms[SDR_MAX_NSYM-544+i] * 255;
    }
    sdr_decode_conv(syms, 544, bits);
    
    // search and decode SBAS message
    int rev = sync_SBAS_msgs(bits, 250);
    if (rev >= 0) {
        decode_SBAS_msgs(ch, bits, rev);
    }
}

// decode SBAS nav data --------------------------------------------------------
static void decode_SBAS(sdr_ch_t *ch)
{
    if (!sync_symb(ch, 2)) { // sync symbol
        return;
    }
    if (ch->nav->fsync > 0) { // sync SBAS message
        if (ch->lock == ch->nav->fsync + 1000) {
            search_SBAS_msgs(ch);
        }
        else if (ch->lock > ch->nav->fsync + 1000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock > 1088 + 1000) {
        search_SBAS_msgs(ch);
    }
}

// test LNAV parity ([1]) ------------------------------------------------------
static int test_LNAV_parity(const uint8_t *syms, uint8_t *data)
{
    static const uint32_t mask[] = {
        0x2EC7CD2, 0x1763E69, 0x2BB1F34, 0x15D8F9A, 0x1AEC7CD, 0x22DEA27
    };
    uint32_t buff = 0;
    
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 30; j++) {
            buff = (buff << 1) | syms[i*30+j];
        }
        if (buff & (1 << 30)) {
            buff ^= 0x3FFFFFC0;
        }
        for (int j = 0; j < 6; j++) {
            if (sdr_xor_bits((buff >> 6) & mask[j]) != ((buff >> (5 - j)) & 1)) {
                return 0;
            }
        }
        setbitu(data, 24 * i, 24, (buff >> 6) & 0xFFFFFF);
    }
    return 1;
}

// decode LNAV ([1]) -----------------------------------------------------------
static void decode_LNAV(sdr_ch_t *ch, const uint8_t *syms, int rev)
{
    double time = ch->time - TOFF_L1CA;
    uint8_t buff[300], data[30];
    
    for (int i = 0; i < 300; i++) {
        buff[i] = syms[i] ^ (uint8_t)rev;
    }
    if (test_LNAV_parity(buff, data)) {
        ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        int sf = getbitu(data, 43, 3);
        if (sf == 1) {
            ch->week = getbitu(data, 48, 10) + GPST_OFF_W;
        }
        update_tow(ch, getbitu(data, 24, 17) * 6.0 + TOFF_L1CA);
        if (sf >= 1 && sf <= 5) {
            ch->nav->type = sf; // SF ID
            memcpy(ch->nav->data + 30 * (sf - 1), data, 30); // SF 24 x 10 bits
        }
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 240, str);
        sdr_log(3, "$LNAV,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,LNAV PARITY ERROR", time, ch->sig, ch->prn);
    }
}

// decode L1CA nav data ([1]) --------------------------------------------------
static void decode_L1CA(sdr_ch_t *ch)
{
    static const uint8_t preamb[] = {1, 0, 0, 0, 1, 0, 1, 1};
    
    if (ch->prn >= 120 && ch->prn <= 158) { // L1 SBAS
        decode_SBAS(ch);
        return;
    }
    if (!sync_symb(ch, 20)) { // sync symbol
        return;
    }
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 308;
    
    if (ch->nav->fsync > 0) { // sync LNAV subframe
        if (ch->lock == ch->nav->fsync + 6000) {
            int rev = sync_frame(ch, preamb, 8, 0, syms, 300);
            if (rev == ch->nav->rev) {
                decode_LNAV(ch, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 6000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 20 * 308 + 1000) {
        // sync and decode LNAV subframe
        int rev = sync_frame(ch, preamb, 8, 0, syms, 300);
        if (rev >= 0) {
            decode_LNAV(ch, syms, rev);
        }
    }
}

// decode L1S nav data ([4]) ---------------------------------------------------
static void decode_L1S(sdr_ch_t *ch)
{
    decode_SBAS(ch);
}

// decode L1CB nav data ([4]) --------------------------------------------------
static void decode_L1CB(sdr_ch_t *ch)
{
    decode_L1CA(ch);
}

// sync CNAV-2 frame by subframe 1 symbols ([12]) ------------------------------
static int sync_CNV2_frame(sdr_ch_t *ch, const uint8_t *syms, int toi)
{
    // generate CNAV-2 subframe 1 symbols
    if (!CNV2_SF1[0]) {
        for (int t = 0; t < 400; t++) {
            CNV2_SF1[t] = (uint8_t *)sdr_malloc(52);
            int8_t *code = LFSR(51, rev_reg(t & 0xFF, 8), 0x9F, 8);
            uint8_t bit9 = (uint8_t)((t >> 8) & 1);
            CNV2_SF1[t][0] = bit9;
            for (int i = 1; i < 52; i++) {
                CNV2_SF1[t][i] = (uint8_t)((code[i-1] + 1) / 2) ^ bit9;
            }
            sdr_free(code);
        }
    }
    uint8_t *SF1 = CNV2_SF1[toi];
    uint8_t *SFn = CNV2_SF1[(toi + 1) % 400];
    
    if (bmatch_n(syms, SF1, 52, 2) && bmatch_n(syms + 1800, SFn, 52, 2)) {
        sdr_log(4, "$LOG,%.3f,%s,%d,FRAME SYNC (N) TOI=%d", ch->time, ch->sig,
            ch->prn, toi);
        return 1; // normal
    }
    if (bmatch_r(syms, SF1, 52, 2) && bmatch_r(syms + 1800, SFn, 52, 2)) {
        sdr_log(4, "$LOG,%.3f,%s,%d,FRAME SYNC (R) TOI=%d", ch->time, ch->sig,
            ch->prn, toi);
        return 0; // reversed
    }
    return -1;
}

// decode CNAV-2 frame ([12]) --------------------------------------------------
static void decode_CNV2(sdr_ch_t *ch, const uint8_t *syms, int rev, int toi)
{
    double time = ch->time - TOFF_L1CD;
    uint8_t buff[1748], bits[883], data[111];
    
    // decode block-interleave (38 x 46 = 1748 syms)
    for (int i = 0, k = 0; i < 38; i++) {
        for (int j = 0; j < 46; j++) {
            buff[k++] = syms[52+j*38+i] ^ (uint8_t)rev;
        }
    }
    // decode LDPC (1200 + 548 syms -> 600 + 274 bits)
    int nerr1 = sdr_decode_LDPC("CNV2_SF2", buff, 1200, bits + 9);
    int nerr2 = sdr_decode_LDPC("CNV2_SF3", buff + 1200, 548, bits + 609);
    
    if (nerr1 >= 0 && nerr2 >= 0 && test_CRC(bits + 9, 600) &&
        test_CRC(bits + 609, 274)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        ch->nav->seq = toi;
        ch->nav->nerr = nerr1 + nerr2;
        sdr_unpack_data(toi, 9, bits);
        sdr_pack_bits(bits, 883, 0, data);
        ch->week = getbitu(data, 9, 13);
        update_tow(ch, getbitu(data, 22, 8) * 7200.0 + (toi - 1) * 18.0 +
            TOFF_L1CD);
        ch->nav->type = getbitu(data, 617, 6); // CNAV-2 SF3 page number
        memcpy(ch->nav->data, data, 111); // CNAV-2 SF1+SF2+SF3 (9+600+274 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 883, str);
        sdr_log(3, "$CNV2,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,CNV2 FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// decode L1CD nav data ([12]) -------------------------------------------------
static void decode_L1CD(sdr_ch_t *ch)
{
    // add symbol buffer
    uint8_t sym = ch->trk->P[SDR_N_HIST-1][0] >= 0.0 ? 1 : 0;
    sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 1852;
    
    if (ch->nav->fsync > 0) { // sync CNAV-2 frame
        if (ch->lock == ch->nav->fsync + 1800) {
            int toi = (ch->nav->seq + 1) % 400;
            int rev = sync_CNV2_frame(ch, syms, toi);
            uint8_t sym = syms[52]; // WN MSB in SF2
            if (rev == ch->nav->rev && (sym ^ rev)) {
                decode_CNV2(ch, syms, rev, toi);
            }
        }
        else if (ch->lock > ch->nav->fsync + 1800) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 1852 + 100) {
        // search and decode CNAV-2 frame
        for (int toi = 0; toi < 400; toi++) {
            int rev = sync_CNV2_frame(ch, syms, toi);
            uint8_t sym = syms[52];
            if (rev >= 0 && (sym ^ rev)) {
                decode_CNV2(ch, syms, rev, toi);
                break;
            }
        }
    }
}

// decode L1CP nav data ([12]) -------------------------------------------------
static void decode_L1CP(sdr_ch_t *ch)
{
    if (ch->trk->sec_sync == 0) {
        ch->tow = -1;
        ch->tow_v = 0;
    }
    else if ((ch->lock - ch->trk->sec_sync) % ch->len_sec_code == 0) {
        ch->tow = (int)(TOFF_L1CP / 1e-3);
        ch->tow_v = 2; // amb-unresolved
    }
}

// decode CNAV subframe ([13]) -------------------------------------------------
static void decode_CNAV(sdr_ch_t *ch, const uint8_t *bits, int rev)
{
    double toff = (!strcmp(ch->sig, "L2CM")) ? TOFF_L2CM : TOFF_L5I;
    double time = ch->time - toff;
    uint8_t buff[300], data[38];
    
    for (int i = 0; i < 300; i++) {
        buff[i] = bits[i] ^ (uint8_t)rev;
    }
    if (test_CRC(buff, 300)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        sdr_pack_bits(buff, 300, 0, data);
        int type = getbitu(data, 14, 6);
        if (type == 10) {
            ch->week = getbitu(data, 38, 13);
        }
        update_tow(ch, getbitu(data, 20, 17) * 6.0 + toff);
        ch->nav->type = type; // CNAV message type ID
        memcpy(ch->nav->data, data, 38); // CNAV message (300 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 300, str);
        sdr_log(3, "$CNAV,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,CNAV FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// search CNAV subframe ([13]) -------------------------------------------------
static void search_CNAV_frame(sdr_ch_t *ch)
{
    static const uint8_t preamb[] = {1, 0, 0, 0, 1, 0, 1, 1};
    uint8_t buff[644], bits[316];
    
    // decode 1/2 FEC (644 syms -> 308 + 8 bits)
    for (int i = 0; i < 644; i++) {
        buff[i] = ch->nav->syms[SDR_MAX_NSYM-644+i] * 255;
    }
    sdr_decode_conv(buff, 644, bits);
    
    // search and decode CNAV subframe
    int rev = sync_frame(ch, preamb, 8, 0, bits, 300);
    if (rev >= 0) {
        decode_CNAV(ch, bits, rev);
    }
}

// decode L2CM nav data --------------------------------------------------------
static void decode_L2CM(sdr_ch_t *ch)
{
    // add symbol buffer
    uint8_t sym = (ch->trk->P[SDR_N_HIST-1][0] >= 0.0) ? 1 : 0;
    sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));
    
    if (ch->nav->fsync > 0) { // sync CNAV subframe
        if (ch->lock == ch->nav->fsync + 600) {
            search_CNAV_frame(ch);
        }
        else if (ch->lock > ch->nav->fsync + 600) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock > 644 + 50) {
        search_CNAV_frame(ch);
    }
}

// sync L5 SBAS message --------------------------------------------------------
static int sync_L5_SBAS_msgs(const uint8_t *bits, int N)
{
    static const uint8_t preamb[][8] = {
        {0, 1, 0, 1}, {1, 1, 0, 0}, {0, 1, 1, 0}, {1, 0, 0, 1}, {0, 0, 1, 1},
        {1, 0, 1, 0}
    };
    for (int i = 0; i < 6; i++) {
        int j = (i + 1) % 6, k = (i + 2) % 6, m = (i + 3) % 6;
        if (bmatch_n(bits, preamb[i], 4, 0) &&
            bmatch_n(bits + N, preamb[j], 4, 0) &&
            bmatch_n(bits + 2 * N, preamb[k], 4, 0) &&
            bmatch_n(bits + 3 * N, preamb[m], 4, 0)) {
            return 0;
        }
        if (bmatch_r(bits, preamb[i], 4, 0) &&
            bmatch_r(bits + N, preamb[j], 4, 0) &&
            bmatch_r(bits + 2 * N, preamb[k], 4, 0) &&
            bmatch_r(bits + 3 * N, preamb[m], 4, 0)) {
            return 1;
        }
    }
    return -1;
}

// search L5 SBAS message ------------------------------------------------------
static void search_L5_SBAS_msgs(sdr_ch_t *ch)
{
    uint8_t syms[1546], bits[766];
    
    // decode 1/2 FEC (1546 syms -> 758 + 8 bits)
    for (int i = 0; i < 1546; i++) {
        syms[i] = ch->nav->syms[SDR_MAX_NSYM-1546+i] * 255;
    }
    sdr_decode_conv(syms, 1546, bits);
    
    // search and decode SBAS message
    int rev = sync_L5_SBAS_msgs(bits, 250);
    if (rev >= 0) {
        decode_SBAS_msgs(ch, bits + 500, rev);
    }
}

// decode L5 SBAS nav data ----------------------------------------------------
static void decode_L5_SBAS(sdr_ch_t *ch)
{
    if (!sync_sec_code(ch)) { // sync secondary code
        return;
    }
    if (ch->nav->fsync > 0) { // sync L5 SBAS message
        if (ch->lock == ch->nav->fsync + 1000) {
            search_L5_SBAS_msgs(ch);
        }
        else if (ch->lock > ch->nav->fsync + 1000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 3093 + 1000) {
        search_L5_SBAS_msgs(ch);
    }
}

// decode L5I nav data ([13]) --------------------------------------------------
static void decode_L5I(sdr_ch_t *ch)
{
    if (ch->prn >= 120 && ch->prn <= 158) { // L5 SBAS
        decode_L5_SBAS(ch);
        return;
    }
    if (!sync_sec_code(ch)) { // sync secondary code
        return;
    }
    if (ch->nav->fsync > 0) { // sync CNAV subframe
        if (ch->lock == ch->nav->fsync + 6000) {
            search_CNAV_frame(ch);
        }
        else if (ch->lock > ch->nav->fsync + 6000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock > 6440 + 1000) {
        search_CNAV_frame(ch);
    }
}

// decode L5Q nav data ([13]) --------------------------------------------------
static void decode_L5Q(sdr_ch_t *ch)
{
    if (ch->prn >= 120 && ch->prn <= 158) { // SBAS
        return;
    }
    if (ch->trk->sec_sync == 0) {
        ch->tow = -1;
        ch->tow_v = 0;
    }
    else if ((ch->lock - ch->trk->sec_sync) % ch->len_sec_code == 0) {
        ch->tow = (int)(TOFF_L5Q / 1e-3);
        ch->tow_v = 2;
    }
}

// decode L5SI nav data ([6]) --------------------------------------------------
static void decode_L5SI(sdr_ch_t *ch)
{
    decode_SBAS(ch);
}

// decode L5SQ nav data ([6]) --------------------------------------------------
static void decode_L5SQ(sdr_ch_t *ch)
{
    decode_L5Q(ch);
}

// decode L5SIV nav data ([6]) -------------------------------------------------
static void decode_L5SIV(sdr_ch_t *ch)
{
    decode_L5_SBAS(ch);
}

// decode L5SQV nav data ([6]) -------------------------------------------------
static void decode_L5SQV(sdr_ch_t *ch)
{
}

// sync and decode L6 frame ([5]) ----------------------------------------------
static void decode_L6_frame(sdr_ch_t *ch, const uint8_t *syms, int N)
{
    uint8_t preamb[5] = {0x1A, 0xCF, 0xFC, 0x1D};
    
    preamb[4] = ch->prn;
    
    // sync 2 premable differences
    int n1 = 0, n2 = 0;
    for (int i = 1; i < 5; i++) {
        if ((uint8_t)(syms[i] - syms[0]) == (uint8_t)(preamb[i] - preamb[0])) n1++;
    }
    for (int i = 0; i < 5; i++) {
        if ((uint8_t)(syms[i+N] - syms[0]) == (uint8_t)(preamb[i] - preamb[0])) n2++;
    }
    if (n1 + n2 < 9) { // test # of symbol matchs
        unsync_nav(ch);
        return;
    }
    // restore symbols
    double time = ch->time - TOFF_L6DE;
    uint8_t data[250], off = preamb[0] - syms[0];
    for (int i = 0; i < 250; i++) {
        data[i] = syms[i] + off;
    }
    // decode RS(255,223) and correct errors
    uint8_t buff[255] = {0};
    memcpy(buff + 9, data + 4, 246);
    ch->nav->nerr = sdr_decode_rs(buff);
    memcpy(data + 4, buff + 9, 246);
    
    if (ch->nav->nerr >= 0) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->tow = (int)(TOFF_L6DE / 1e-3);
        ch->tow_v = 2;
        ch->nav->coff = off * ch->T / 10230;
        ch->nav->type = getbitu(data, 40, 5); // L6 vender + facility ID
        memcpy(ch->nav->data, data, 250); // L6 frame (2000 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[512];
        hex_str(data, 2000, str);
        sdr_log(3, "$L6FRM,%.3f,%s,%d,%d,%s", time, ch->sig, ch->prn,
            ch->nav->nerr, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,L6FRM RS ERROR", time, ch->sig, ch->prn);
    }
}

// decode L6D nav data ([5]) ---------------------------------------------------
static void decode_L6D(sdr_ch_t *ch)
{
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 255;
    
    if (ch->nav->fsync > 0) { // sync L6 frame
        if (ch->lock == ch->nav->fsync + 250) {
            decode_L6_frame(ch, syms, 250);
        }
        else if (ch->lock > ch->nav->fsync + 250) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 255) { // sync and decode L6 frame
        decode_L6_frame(ch, syms, 250);
    }
}

// decode L6E nav data ---------------------------------------------------------
static void decode_L6E(sdr_ch_t *ch)
{
    decode_L6D(ch);
}

// decode GLONASS nav string ([14]) --------------------------------------------
static void decode_glo_str(sdr_ch_t *ch, const uint8_t *syms, int rev)
{
    double time = ch->time - TOFF_G1CA;
    uint8_t bits[85] = {0}, data[11];
    
    for (int i = 1; i < 85; i++) {
        // handle meander and relative code transformation ([14] fig.3.4)
        bits[i] = syms[(i-1)*2] ^ syms[i*2];
    }
    sdr_pack_bits(bits, 85, 0, data); // GLO string (85 bits, packed)
    
    if (test_glostr(data)) {
        ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        int sno = getbitu(data, 1, 4);
        if (sno == 4) {
            sprintf(ch->sat, "R%02d", getbitu(data, 70, 5)); // set sat ID
        }
        if (sno == 1) {
            double tod = getbitu(data, 9, 5) * 3600.0 +
                getbitu(data, 14, 6) * 60.0 + getbitu(data, 20, 1) * 30.0;
            update_tow(ch, tod + TOFF_G1CA + GPST_UTC);
            ch->tow_v = 2;
        }
        if (sno >= 1 && sno <= 5) { // GLO string w/o mark and hamming (77 bits)
            ch->nav->type = sno; // GLO string number
            sdr_pack_bits(bits, 77, 0, ch->nav->data + 10 * (sno - 1));
        }
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 85, str);
        sdr_log(3, "$GSTR,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,GSTR HAMMING ERROR", time, ch->sig, ch->prn);
    }
}

// decode G1CA nav data ([14]) -------------------------------------------------
static void decode_G1CA(sdr_ch_t *ch)
{
    static const uint8_t time_mark[] = {
        1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0,
        0, 1, 0, 0, 1, 0, 1, 1, 0
    };
    if (!sync_symb(ch, 10)) { // sync symbol
        return;
    }
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 230;
    
    if (ch->nav->fsync > 0) { // sync GLONASS nav string
        if (ch->lock == ch->nav->fsync + 2000) {
            int rev = sync_frame(ch, time_mark, 30, 2, syms, 200);
            if (rev == ch->nav->rev) {
                decode_glo_str(ch, syms + 30, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 2000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 2300 + 2000) {
        // sync and decode GLONASS nav string
        int rev = sync_frame(ch, time_mark, 30, 2, syms, 200);
        if (rev >= 0) {
            decode_glo_str(ch, syms + 30, rev);
        }
    }
}

// decode G2CA nav data --------------------------------------------------------
static void decode_G2CA(sdr_ch_t *ch)
{
    decode_G1CA(ch);
}

// decode GLONASS L1OCD nav string ---------------------------------------------
static void decode_glo_L1OCD_str(sdr_ch_t *ch, const uint8_t *bits, int rev)
{
    double time = ch->time - TOFF_G1OCD;
    uint8_t buff[250], data[32];
    
    for (int i = 0; i < 250; i++) {
        buff[i] = bits[i] ^ (uint8_t)rev;
    }
    if (test_CRC16_GLO(buff, 250)) {
        ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        sdr_pack_bits(buff, 250, 0, data);
        update_tow(ch, getbitu(data, 34, 16) * 2.0 + TOFF_G1OCD + GPST_UTC);
        ch->tow_v = 2;
        ch->nav->type = getbitu(data, 12, 6); // L1OCD nav string type
        memcpy(ch->nav->data, data, 32); // L1OCD nav string (250 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 250, str);
        sdr_log(3, "$G1OCD,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,G1OCD STRING ERROR", time, ch->sig, ch->prn);
    }
}

// search GLONASS L1OCD nav string ---------------------------------------------
static void search_glo_L1OCD_str(sdr_ch_t *ch)
{
    static uint8_t preamb[] = {0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1};
    uint8_t syms[668], bits[328];
    
    // swap convolutional code G1 and G2
    for (int i = 0; i < 552; i += 2) {
        syms[i  ] = ch->nav->syms[SDR_MAX_NSYM-552+i+1] * 255;
        syms[i+1] = ch->nav->syms[SDR_MAX_NSYM-552+i  ] * 255;
    }
    // decode 1/2 FEC (552 syms -> 262 + 8 bits)
    sdr_decode_conv(syms, 552, bits);
    
    // search and decode GLONASS L1OCD nav string
    int rev = sync_frame(ch, preamb, 12, 0, bits, 250);
    if (rev >= 0) {
        decode_glo_L1OCD_str(ch, bits, rev);
    }
}

// decode G1OCD nav data ([18]) ------------------------------------------------
static void decode_G1OCD(sdr_ch_t *ch)
{
    if (!sync_sec_code(ch)) { // sync secondary code
        return;
    }
    if (ch->nav->fsync > 0) { // sync GLONASS L1OCD nav string
        if (ch->lock == ch->nav->fsync + 1000) {
            search_glo_L1OCD_str(ch);
        }
        else if (ch->lock > ch->nav->fsync + 1000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock > ch->nav->ssync + 1104) {
        search_glo_L1OCD_str(ch);
    }
}

// decode G1OCP nav data ([18]) ------------------------------------------------
static void decode_G1OCP(sdr_ch_t *ch)
{
    // to be implemented
}

// decode GLONASS L3OCD nav string ---------------------------------------------
static void decode_glo_L3OCD_str(sdr_ch_t *ch, const uint8_t *bits, int rev)
{
    double time = ch->time - TOFF_G3OCD;
    uint8_t buff[300], data[38];
    
    for (int i = 0; i < 300; i++) {
        buff[i] = bits[i] ^ (uint8_t)rev;
    }
    if (test_CRC(buff, 300)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        sdr_pack_bits(buff, 300, 0, data);
        update_tow(ch, getbitu(data, 26, 15) * 3.0 + TOFF_G3OCD + GPST_UTC);
        ch->tow_v = 2;
        ch->nav->type = getbitu(data, 20, 6); // GLO L3OCD nav string type
        memcpy(ch->nav->data, data, 38); // GLO L3OCD nav string (300 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 300, str);
        sdr_log(3, "$G3OCD,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,G3OCD STRING ERROR", time, ch->sig, ch->prn);
    }
}

// search GLONASS L3OCD nav string ---------------------------------------------
static void search_glo_L3OCD_str(sdr_ch_t *ch)
{
    static uint8_t preamb[] = {
        0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0
    };
    uint8_t syms[668], bits[328];
    
    // swap convolutional code G1 and G2
    for (int i = 0; i < 668; i += 2) {
        syms[i  ] = ch->nav->syms[SDR_MAX_NSYM-668+i+1] * 255;
        syms[i+1] = ch->nav->syms[SDR_MAX_NSYM-668+i  ] * 255;
    }
    // decode 1/2 FEC (668 syms -> 320 + 8 bits)
    sdr_decode_conv(syms, 668, bits);
    
    // search and decode GLONASS L3OCD nav string
    int rev = sync_frame(ch, preamb, 20, 1, bits, 300);
    if (rev >= 0) {
        decode_glo_L3OCD_str(ch, bits, rev);
    }
}

// decode G3OCD nav data ([16]) ------------------------------------------------
static void decode_G3OCD(sdr_ch_t *ch)
{
    if (!sync_sec_code(ch)) { // sync secondary code
        return;
    }
    if (ch->nav->fsync > 0) { // sync GLONASS L3OCD nav string
        if (ch->lock == ch->nav->fsync + 3000) {
            search_glo_L3OCD_str(ch);
        }
        else if (ch->lock > ch->nav->fsync + 3000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock > ch->nav->ssync + 6680) {
        search_glo_L3OCD_str(ch);
    }
}

// decode G3OCP nav data ([16]) ------------------------------------------------
static void decode_G3OCP(sdr_ch_t *ch)
{
    if (ch->trk->sec_sync == 0) {
        ch->tow = -1;
        ch->tow_v = 0;
    }
    else if ((ch->lock - ch->trk->sec_sync) % ch->len_sec_code == 0) {
        ch->tow = (int)(TOFF_G3OCP / 1e-3);
        ch->tow_v = 2;
    }
}

// decode Galileo symbols ([2]) ------------------------------------------------
static void decode_gal_syms(const uint8_t *syms, int ncol, int nrow,
    uint8_t *bits)
{
    uint8_t *buff = (uint8_t *)sdr_malloc(ncol * nrow);
    
    // decode block-interleave and invert G2
    for (int i = 0, k = 0; i < ncol; i++) {
        for (int j = 0; j < nrow; j++) {
            buff[k++] = (syms[j*ncol+i] ^ ((j % 2) ? 1 : 0)) * 255;
        }
    }
    // decode 1/2 FEC
    sdr_decode_conv(buff, ncol * nrow, bits);
    
    sdr_free(buff);
}

// decode Galileo I/NAV pages ([2]) --------------------------------------------
static void decode_gal_INAV(sdr_ch_t *ch, const uint8_t *syms, int rev)
{
    double toff = (!strcmp(ch->sig, "E1B")) ? TOFF_E1B : TOFF_E5BI;
    double time = ch->time - toff;
    uint8_t buff[500], bits[114*2], data[16];
    
    for (int i = 0; i < 500; i++) {
        buff[i] = syms[i] ^ (uint8_t)rev;
    }
    // decode Galileo symbols (240 syms x 2 -> 114 bits x 2)
    decode_gal_syms(buff +  10, 30, 8, bits);
    decode_gal_syms(buff + 260, 30, 8, bits + 114);
    
    // test even and odd pages
    if (bits[0] != 0 || bits[114] != 1) {
        ch->nav->ssync = ch->nav->fsync = ch->nav->rev = 0;
        return;
    }
    if (test_CRC(bits, 220)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        sdr_pack_bits(bits +   2, 112, 0, data); // I/NAV word (112+16 bits)
        sdr_pack_bits(bits + 116,  16, 0, data + 14);
        int type = getbitu(data, 0, 6);
        if (type == 5) {
            ch->week = getbitu(data, 73, 12) + GPST_GST_W;
            update_tow(ch, getbitu(data, 85, 20) + toff);
        }
        ch->nav->type = type; // I/NAV word type
        if (type >= 0 && type <= 6) {
            memcpy(ch->nav->data + 16 * type, data, 16);
        }
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 128, str);
        sdr_log(3, "$INAV,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,INAV FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// decode E1B nav data ([2]) ---------------------------------------------------
static void decode_E1B(sdr_ch_t *ch)
{
    static const uint8_t preamb[] = {0, 1, 0, 1, 1, 0, 0, 0, 0, 0};
    
    // add symbol buffer
    uint8_t sym = (ch->trk->P[SDR_N_HIST-1][0] >= 0.0) ? 1 : 0;
    sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 510;
    
    if (ch->nav->fsync > 0) { // sync frame
        if (ch->lock == ch->nav->fsync + 500) {
            int rev = sync_frame(ch, preamb, 10, 0, syms, 500);
            if (rev == ch->nav->rev) {
                decode_gal_INAV(ch, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 500) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 510 + 250) {
        // sync and decode Galileo I/NAV pages
        int rev = sync_frame(ch, preamb, 10, 0, syms, 500);
        if (rev >= 0) {
            decode_gal_INAV(ch, syms, rev);
        }
    }
}

// decode E1C nav data ([2]) ---------------------------------------------------
static void decode_E1C(sdr_ch_t *ch)
{
    if (ch->trk->sec_sync == 0) {
        ch->tow = -1;
        ch->tow_v = 0;
    }
    else if ((ch->lock - ch->trk->sec_sync) % ch->len_sec_code == 0) {
        ch->tow = (int)(TOFF_E1C / 1e-3);
        ch->tow_v = 2;
    }
}

// decode Galileo F/NAV page ([2]) ---------------------------------------------
static void decode_gal_FNAV(sdr_ch_t *ch, const uint8_t *syms, int rev)
{
    double time = ch->time - TOFF_E5AI;
    uint8_t buff[500], bits[238], data[30];
    
    for (int i = 0; i < 500; i++) {
        buff[i] = syms[i] ^ (uint8_t)rev;
    }
    // decode Galileo symbols (488 syms -> 238 bits)
    decode_gal_syms(buff + 12, 61, 8, bits);
    
    if (test_CRC(bits, 238)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        sdr_pack_bits(bits, 238, 0, data); // F/NAV page (238 bits)
        int type = getbitu(data, 0, 6);
        if (type >= 1 && type <= 3) {
            int off[] = {155, 182, 174};
            ch->week = getbitu(data, off[type-1], 12) + GPST_GST_W;
        }
        if (type >= 1 && type <= 4) {
            int off[] = {167, 194, 186, 189};
            update_tow(ch, getbitu(data, off[type-1], 20) + TOFF_E5AI);
        }
        ch->nav->type = type; // F/NAV page type
        if (type >= 1 && type <= 6) {
            memcpy(ch->nav->data + 31 * (type - 1), data, 30);
        }
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 238, str);
        sdr_log(3, "$FNAV,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,FNAV FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// decode E5AI nav data ([2]) --------------------------------------------------
static void decode_E5AI(sdr_ch_t *ch)
{
    static const uint8_t preamb[] = {1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0};
    
    if (!sync_sec_code(ch)) { // sync secondary code
        return;
    }
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 512;
    
    if (ch->nav->fsync > 0) { // sync Galileo F/NAV page
        if (ch->lock == ch->nav->fsync + 10000) {
            int rev = sync_frame(ch, preamb, 12, 0, syms, 500);
            if (rev == ch->nav->rev) {
                decode_gal_FNAV(ch, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 10000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= ch->len_sec_code * 512 + 250) {
        // sync and decode Galileo F/NAV page
        int rev = sync_frame(ch, preamb, 12, 0, syms, 500);
        if (rev >= 0) {
            decode_gal_FNAV(ch, syms, rev);
        }
    }
}

// decode E5AQ nav data ([2]) --------------------------------------------------
static void decode_E5AQ(sdr_ch_t *ch)
{
    if (ch->trk->sec_sync == 0) {
        ch->tow = -1;
        ch->tow_v = 0;
    }
    else if ((ch->lock - ch->trk->sec_sync) % ch->len_sec_code == 0) {
        ch->tow = (int)(TOFF_E5AQ / 1e-3);
        ch->tow_v = 2;
    }
}

// decode E5BI nav data ([2]) --------------------------------------------------
static void decode_E5BI(sdr_ch_t *ch)
{
    static const uint8_t preamb[] = {0, 1, 0, 1, 1, 0, 0, 0, 0, 0};
    
    if (!sync_sec_code(ch)) { // sync secondary code
        return;
    }
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 510;
    
    if (ch->nav->fsync > 0) { // sync frame
        if (ch->lock == ch->nav->fsync + 2000) {
            int rev = sync_frame(ch, preamb, 10, 0, syms, 500);
            if (rev == ch->nav->rev) {
                decode_gal_INAV(ch, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 2000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= ch->len_sec_code * 510 + 250) {
        // sync and decode Galileo I/NAV pages
        int rev = sync_frame(ch, preamb, 10, 0, syms, 500);
        if (rev >= 0) {
            decode_gal_INAV(ch, syms, rev);
        }
    }
}

// decode E5BQ nav data ([2]) --------------------------------------------------
static void decode_E5BQ(sdr_ch_t *ch)
{
    if (ch->trk->sec_sync == 0) {
        ch->tow = -1;
        ch->tow_v = 0;
    }
    else if ((ch->lock - ch->trk->sec_sync) % ch->len_sec_code == 0) {
        ch->tow = (int)(TOFF_E5BQ / 1e-3);
        ch->tow_v = 2;
    }
}

// decode Galileo C/NAV page ([3]) ---------------------------------------------
static void decode_gal_CNAV(sdr_ch_t *ch, const uint8_t *syms, int rev)
{
    double time = ch->time - TOFF_E6B;
    uint8_t buff[1000], bits[486], data[61];
    
    for (int i = 0; i < 1000; i++) {
        buff[i] = syms[i] ^ (uint8_t)rev;
    }
    // decode Galileo symbols (984 syms -> 486 bits)
    decode_gal_syms(buff + 16, 123, 8, bits);
    
    if (test_CRC(bits, 486)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        sdr_pack_bits(bits, 486, 0, data);
        ch->tow = (int)(TOFF_E6B / 1e-3);
        ch->tow_v = 2;
        ch->nav->type = getbitu(data, 20, 5); // C/NAV HAS message ID
        memcpy(ch->nav->data, data, 61); // C/NAV frame (486 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 486, str);
        sdr_log(3, "$CNAV,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(4, "$LOG,%.3f,%s,%d,CNAV FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// decode E6B nav data ([3]) ---------------------------------------------------
static void decode_E6B(sdr_ch_t *ch)
{
    static const uint8_t preamb[] = {
        1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0
    };
    // add symbol buffer
    uint8_t sym = (ch->trk->P[SDR_N_HIST-1][0] >= 0.0) ? 1 : 0;
    sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 1016;
    
    if (ch->nav->fsync > 0) { // sync Galileo C/NAV page
        if (ch->lock == ch->nav->fsync + 1000) {
            int rev = sync_frame(ch, preamb, 16, 0, syms, 1000);
            if (rev == ch->nav->rev) {
                decode_gal_CNAV(ch, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 1000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 1016 + 1000) {
        // sync and decode Galileo C/NAV page
        int rev = sync_frame(ch, preamb, 16, 0, syms, 1000);
        if (rev >= 0) {
            decode_gal_CNAV(ch, syms, rev);
        }
    }
}

// decode E6C nav data ([3]) ---------------------------------------------------
static void decode_E6C(sdr_ch_t *ch)
{
    if (ch->trk->sec_sync == 0) {
        ch->tow = -1;
        ch->tow_v = 0;
    }
    else if ((ch->lock - ch->trk->sec_sync) % ch->len_sec_code == 0) {
        ch->tow = (int)(TOFF_E6C / 1e-3);
        ch->tow_v = 2;
    }
}

// decode symbols by BCH(15,11,1) ([7] Figure 5-4) -----------------------------
static int decode_D1D2_BCH(uint8_t *syms)
{
    uint8_t R = 0, corr[15];
    
    for (int i = 0; i < 15; i++) {
        R = (syms[i] << 3) ^ ((R & 1) * 0x0C) ^ (R >> 1);
    }
    if (R == 0) return 0;
    
    // correct error
    sdr_unpack_data(BCH_CORR_TBL[R], 15, corr);
    for (int i = 0; i < 15; i++) {
        syms[i] ^= corr[i];
    }
    return 1;
}

// decode BDS D1/D2 NAV subframe ([7]) -----------------------------------------
static void decode_D1D2NAV(sdr_ch_t *ch, int type, const uint8_t *syms, int rev)
{
    double toff = (type == 1) ? TOFF_B1I_D1 : TOFF_B1I_D2;
    double time = ch->time - toff;
    uint8_t bits[300], s1[15], s2[15], data[38];
    int nerr = 0;
    
    for (int i = 0; i < 300; i++) {
        bits[i] = syms[i] ^ (uint8_t)rev;
    }
    nerr += decode_D1D2_BCH(bits + 15);
    
    for (int i = 30; i < 300; i += 30) {
        for (int j = 0; j < 15; j++) { // de-interleave
            s1[j] = bits[i+2*j  ];
            s2[j] = bits[i+2*j+1];
        }
        nerr += decode_D1D2_BCH(s1);
        nerr += decode_D1D2_BCH(s2);
        memcpy(bits + i     , s1, 11);
        memcpy(bits + i + 11, s2, 11);
        memcpy(bits + i + 22, s1 + 11, 4);
        memcpy(bits + i + 26, s2 + 11, 4);
    }
    ch->nav->ssync = ch->nav->fsync = ch->lock;
    ch->nav->rev = rev;
    ch->nav->nerr = nerr;
    sdr_pack_bits(bits, 300, 0, data);
    int sf = getbitu(data, 15, 3), pg = getbitu(data, 42, 4);
    if (type == 1 && sf == 1) {
        ch->week = getbitu(data, 60, 13) + GPST_BDT_W;
    }
    else if (type == 2 && sf == 1 && pg == 1) {
        ch->week = getbitu(data, 64, 13) + GPST_BDT_W;
    }
    if (type == 1 || (type == 2 && sf == 1)) {
        update_tow(ch, getbitu(data, 18, 8) * 4096.0 + getbitu(data, 30, 12) +
            toff + GPST_BDT);
    }
    if (type == 1 && sf >= 1 && sf <= 5) {
        ch->nav->type = sf; // D1 SF ID
        memcpy(ch->nav->data + 38 * (sf - 1), data, 38); // D1 SF (300 bits)
    }
    else if (type == 2 && sf == 1 && pg >= 1 && pg <= 10) {
        ch->nav->type = pg; // D2 SF1 page
        memcpy(ch->nav->data + 38 * (pg - 1), data, 38); // D2 SF1 page (300 bits)
    }
    ch->nav->stat = 1;
    ch->nav->count[0]++;
    char str[256];
    hex_str(data, 300, str);
    sdr_log(3, "$D%dNAV,%.3f,%s,%d,%s", type, time, ch->sig, ch->prn, str);
}

// decode B1I D1 nav data ([7]) ------------------------------------------------
static void decode_B1I_D1(sdr_ch_t *ch)
{
    static const uint8_t preamb[] = {1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0};
    
    if (!sync_sec_code(ch)) { // sync secondary code
        return;
    }
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 311;
    
    if (ch->nav->fsync > 0) { // sync frame
        if (ch->lock == ch->nav->fsync + 6000) {
            int rev = sync_frame(ch, preamb, 11, 0, syms, 300);
            if (rev == ch->nav->rev) {
                decode_D1D2NAV(ch, 1, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 6000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= ch->len_sec_code * 311 + 1000) {
        // sync and decode BDS D1 NAV subframe
        int rev = sync_frame(ch, preamb, 11, 0, syms, 300);
        if (rev >= 0) {
            decode_D1D2NAV(ch, 1, syms, rev);
        }
    }
}

// decode B1I D2 nav data ([7]) ------------------------------------------------
static void decode_B1I_D2(sdr_ch_t *ch)
{
    static uint8_t preamb[] = {1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0};
    
    if (!sync_symb(ch, 2)) { // sync symbol
        return;
    }
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 311;
    
    if (ch->nav->fsync > 0) { // sync frame
        if (ch->lock == ch->nav->fsync + 600) {
            int rev = sync_frame(ch, preamb, 11, 0, syms, 300);
            if (rev == ch->nav->rev) {
                decode_D1D2NAV(ch, 2, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 600) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 2 * 311 + 1000) {
        // sync and decode BDS D2 NAV subframe
        int rev = sync_frame(ch, preamb, 11, 0, syms, 300);
        if (rev >= 0) {
            decode_D1D2NAV(ch, 2, syms, rev);
        }
    }
}

// decode B1I nav data ([7]) ---------------------------------------------------
static void decode_B1I(sdr_ch_t *ch)
{
    if (ch->prn >= 6 && ch->prn <= 58) {
       decode_B1I_D1(ch);
    }
    else {
       decode_B1I_D2(ch);
    }
}

// sync B1CD B-CNAV1 frame by subframe 1 symbols -------------------------------
static int sync_BCNV1_frame(sdr_ch_t *ch, const uint8_t *syms, int soh)
{
    // generate CNAV-1 subframe 1 symbols
    if (!BCNV1_SF1A[ch->prn-1]) {
        BCNV1_SF1A[ch->prn-1] = (uint8_t *)sdr_malloc(21);
        int8_t *code = LFSR(21, rev_reg(ch->prn, 6), 0x17, 6);
        for (int i = 0; i < 21; i++) {
            BCNV1_SF1A[ch->prn-1][i] = (code[i] + 1) / 2;
        }
        sdr_free(code);
    }
    if (!BCNV1_SF1B[0]) {
        for (int soh = 0; soh < 200; soh++) {
            BCNV1_SF1B[soh] = (uint8_t *)sdr_malloc(51);
            int8_t *code = LFSR(51, rev_reg(soh, 8), 0x9F, 8);
            for (int i = 0; i < 51; i++) {
                BCNV1_SF1B[soh][i] = (code[i] + 1) / 2;
            }
            sdr_free(code);
        }
    }
    uint8_t SF1[72], SFn[72];
    memcpy(SF1, BCNV1_SF1A[ch->prn-1], 21);
    memcpy(SFn, BCNV1_SF1A[ch->prn-1], 21);
    memcpy(SF1 + 21, BCNV1_SF1B[soh], 51);
    memcpy(SFn + 21, BCNV1_SF1B[(soh + 1) % 200], 51);
    
    if (bmatch_n(syms, SF1, 72, 3) && bmatch_n(syms + 1800, SFn, 72, 3)) {
        sdr_log(4, "$LOG,%.3f,%s,%d,FRAME SYNC (N) SOH=%d", ch->time, ch->sig,
            ch->prn, soh);
        return 1; // normal
    }
    if (bmatch_r(syms, SF1, 72, 3) && bmatch_r(syms + 1800, SFn, 72, 3)) {
        sdr_log(4, "$LOG,%.3f,%s,%d,FRAME SYNC (R) SOH=%d", ch->time, ch->sig,
            ch->prn, soh);
        return 0; // reversed
    }
    return -1;
}

// decode B1CD B-CNAV1 frame ([8]) ---------------------------------------------
static void decode_BCNV1(sdr_ch_t *ch, const uint8_t *syms, int rev, int soh)
{
    double time = ch->time - TOFF_B1CD;
    uint8_t symsr[1728], syms2[1200], syms3[528], bits[878], data[110];
    
    // decode block interleave of SF2,3 (36 x 48 = 1728 syms)
    for (int i = 0, k = 0; i < 36; i++) {
        for (int j = 0; j < 48; j++) {
            symsr[k++] = syms[72+j*36+i] ^ (uint8_t)rev;
        }
    }
    for (int i = 0; i < 11; i++) {
        memcpy(syms2 + i*96   , symsr + (i*3  )*48, 48);
        memcpy(syms2 + i*96+48, symsr + (i*3+1)*48, 48);
        memcpy(syms3 + i*48   , symsr + (i*3+2)*48, 48);
    }
    for (int i = 22; i < 25; i++) {
        memcpy(syms2 + i*48   , symsr + (i+11 )*48, 48);
    }
    // decode LDPC (1200 + 528 syms -> 600 + 264 bits)
    int nerr1 = sdr_decode_LDPC("BCNV1_SF2", syms2, 1200, bits + 14);
    int nerr2 = sdr_decode_LDPC("BCNV1_SF3", syms3, 528 , bits + 614);
    sdr_unpack_data(ch->prn, 6, bits);
    sdr_unpack_data(soh, 8, bits + 6);
    
    if (nerr1 >= 0 && nerr2 >= 0 && test_CRC(bits + 14, 600) &&
        test_CRC(bits + 614, 264)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        ch->nav->seq = soh;
        ch->nav->nerr = nerr1 + nerr2;
        sdr_pack_bits(bits, 878, 0, data);
        ch->week = getbitu(data, 14, 13) + GPST_BDT_W;
        update_tow(ch, getbitu(data, 27, 8) * 3600.0 + soh * 18.0 + TOFF_B1CD +
            GPST_BDT);
        ch->nav->type = getbitu(data, 614, 6); // CNAV-2 SF3 page ID
        memcpy(ch->nav->data, data, 110); // CNAV-2 SF1+SF2+SF3 (14+600+264 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 878, str);
        sdr_log(3, "$BCNV1,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(3, "$LOG,%.3f,%s,%d,BCNV1 FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// decode B1CD nav data ([8]) --------------------------------------------------
static void decode_B1CD(sdr_ch_t *ch)
{
    // add symbol buffer
    uint8_t sym = (ch->trk->P[SDR_N_HIST-1][0] >= 0.0) ? 1 : 0;
    sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 1872;
    
    if (ch->nav->fsync > 0) { // sync B-CNAV1 frame
        if (ch->lock == ch->nav->fsync + 1800) {
            int soh = (ch->nav->seq + 1) % 200;
            int rev = sync_BCNV1_frame(ch, syms, soh);
            if (rev == ch->nav->rev) {
                decode_BCNV1(ch, syms, rev, soh);
            }
        }
        else if (ch->lock > ch->nav->fsync + 1800) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 1872 + 100) {
        // search and decode B-CNAV1 frame
        for (int soh = 0; soh < 200; soh++) {
            int rev = sync_BCNV1_frame(ch, syms, soh);
            if (rev >= 0) {
                decode_BCNV1(ch, syms, rev, soh);
                break;
            }
        }
    }
}

// decode B1CP nav data ([8]) --------------------------------------------------
static void decode_B1CP(sdr_ch_t *ch)
{
    if (ch->trk->sec_sync == 0) {
        ch->tow = -1;
        ch->tow_v = 0;
    }
    else if ((ch->lock - ch->trk->sec_sync) % ch->len_sec_code == 0) {
        ch->tow = (int)(TOFF_B1CP / 1e-3);
        ch->tow_v = 2;
    }
}

// decode B2I nav data ([7]) ---------------------------------------------------
static void decode_B2I(sdr_ch_t *ch)
{
    decode_B1I(ch);
}

// decode B2AD B-CNAV2 frame ([9]) ---------------------------------------------
static void decode_BCNV2(sdr_ch_t *ch, const uint8_t *syms, int rev)
{
    double time = ch->time - TOFF_B2AD;
    uint8_t buff[600], bits[288], data[36];
    
    for (int i = 0; i < 600; i++) {
        buff[i] = syms[i] ^ (uint8_t)rev;
    }
    // decode LDPC (576 syms -> 288 bits)
    int nerr = sdr_decode_LDPC("BCNV2", buff + 24, 576, bits);
     
    if (nerr >= 0 && test_CRC(bits, 288)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        ch->nav->nerr = nerr;
        sdr_pack_bits(bits, 288, 0, data);
        int type = getbitu(data, 6, 6);
        if (type == 10) {
            ch->week = getbitu(data, 30, 13) + GPST_BDT_W;
        }
        update_tow(ch, getbitu(data, 12, 18) * 3.0 + TOFF_B2AD + GPST_BDT);
        ch->nav->type = type; // B-CNAV2 message type
        memcpy(ch->nav->data, data, 36); // B-CNAV2 message (288 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 288, str);
        sdr_log(3, "$BCNV2,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(3, "$LOG,%.3f,%s,%d,BCNV2 FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// decode B2AD nav data ([9]) --------------------------------------------------
static void decode_B2AD(sdr_ch_t *ch)
{
    static const uint8_t preamb[] = {
        1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0
    };
    if (!sync_sec_code(ch)) { // sync secondary code
        return;
    }
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 624;
    
    if (ch->nav->fsync > 0) { // sync frame
        if (ch->lock == ch->nav->fsync + 3000) {
            // sync and decode B-CNAV2 frame
            int rev = sync_frame(ch, preamb, 24, 1, syms, 600);
            if (rev == ch->nav->rev) {
                decode_BCNV2(ch, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 3000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= ch->len_sec_code * 624 + 1000) {
        // sync and decode B-CNAV2 frame
        int rev = sync_frame(ch, preamb, 24, 1, syms, 600);
        if (rev >= 0) {
            decode_BCNV2(ch, syms, rev);
        }
    }
}

// decode B2AP nav data ([9]) --------------------------------------------------
static void decode_B2AP(sdr_ch_t *ch)
{
    if (ch->trk->sec_sync == 0) {
        ch->tow = -1;
        ch->tow_v = 0;
    }
    else if ((ch->lock - ch->trk->sec_sync) % ch->len_sec_code == 0) {
        ch->tow = (int)(TOFF_B2AP / 1e-3);
        ch->tow_v = 2;
    }
}

// decode B2BI B-CNAV3 frame ([10]) --------------------------------------------
static void decode_BCNV3(sdr_ch_t *ch, const uint8_t *syms, int rev)
{
    double time = ch->time - TOFF_B2BI;
    uint8_t buff[1000], bits[486], data[61];
    
    for (int i = 0; i < 1000; i++) {
        buff[i] = syms[i] ^ (uint8_t)rev;
    }
    // decode LDPC (972 syms -> 486 bits)
    int nerr = sdr_decode_LDPC("BCNV3", buff + 28, 972, bits);
     
    if (nerr >= 0 && test_CRC(bits, 486)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        ch->nav->nerr = nerr;
        sdr_pack_bits(bits, 486, 0, data);
        int type = getbitu(data, 0, 6);
        if (ch->prn >= 6 && ch->prn <= 58) {
            if (type == 30) {
                ch->week = getbitu(data, 26, 13) + GPST_BDT_W;
            }
            update_tow(ch, getbitu(data, 6, 20) + TOFF_B2BI + GPST_BDT);
        }
        else { // PPP-B2b
            ch->tow = (int)((TOFF_B2BI + GPST_BDT) / 1e-3);
            ch->tow_v = 2;
        }
        ch->nav->type = type; // B-CNAV3 message type
        memcpy(ch->nav->data, data, 61); // B-CNAV3 message (486 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 486, str);
        sdr_log(3, "$BCNV3,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(3, "$LOG,%.3f,%s,%d,BCNV3 FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// decode B2BI nav data ([10]) -------------------------------------------------
static void decode_B2BI(sdr_ch_t *ch)
{
    uint8_t preamb[] = {1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0};
    
    // add symbol buffer
    uint8_t sym = (ch->trk->P[SDR_N_HIST-1][0] >= 0.0) ? 1 : 0;
    sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 1016;
    
    if (ch->nav->fsync > 0) { // sync frame
        if (ch->lock == ch->nav->fsync + 1000) {
            int rev = sync_frame(ch, preamb, 16, 0, syms, 1000);
            if (rev == ch->nav->rev) {
                decode_BCNV3(ch, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 1000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 1022 + 1000) {
        // sync and decode B-CNAV3 frame
        int rev = sync_frame(ch, preamb, 16, 0, syms, 1000);
        if (rev >= 0) {
            decode_BCNV3(ch, syms, rev);
        }
    }
}

// decode B3I nav data ([11]) -------------------------------------------------
static void decode_B3I(sdr_ch_t *ch)
{
    decode_B1I(ch);
}

// sync I1SD NavIC L1-SPS NAV frame by subframe 1 symbols ([17]) --------------
static int sync_IRNV1_frame(sdr_ch_t *ch, const uint8_t *syms, int toi)
{
    // generate NavIC L1-SPS subframe 1 symbols
    if (!IRNV1_SF1[0]) {
        for (int t = 0; t < 400; t++) {
            IRNV1_SF1[t] = (uint8_t *)sdr_malloc(52);
            int8_t *code = LFSR(52, rev_reg(t+1, 9), 0x1BF, 9);
            for (int i = 0; i < 52; i++) {
                IRNV1_SF1[t][i] = (uint8_t)((code[i] + 1) / 2);
            }
            sdr_free(code);
        }
    }
    uint8_t *SF1 = IRNV1_SF1[toi];
    uint8_t *SFn = IRNV1_SF1[(toi + 1) % 400];
    
    if (bmatch_n(syms, SF1, 52, 2) && bmatch_n(syms + 1800, SFn, 52, 2)) {
        sdr_log(4, "$LOG,%.3f,%s,%d,FRAME SYNC (N) TOI=%d", ch->time, ch->sig,
            ch->prn, toi+1);
        return 1; // normal
    }
    if (bmatch_r(syms, SF1, 52, 2) && bmatch_r(syms + 1800, SFn, 52, 2)) {
        sdr_log(4, "$LOG,%.3f,%s,%d,FRAME SYNC (R) TOI=%d", ch->time, ch->sig,
            ch->prn, toi+1);
        return 0; // reversed
    }
    return -1;
}

// decode I1SD NavIC L1-SPS NAV frame ([17]) -----------------------------------
static void decode_IRNV1(sdr_ch_t *ch, const uint8_t *syms, int rev, int toi)
{
    double time = ch->time - TOFF_I1SD;
    uint8_t buff[1748], bits[883], data[111];
    
    // decode block-interleave (38 x 46 = 1748 syms)
    for (int i = 0, k = 0; i < 38; i++) {
        for (int j = 0; j < 46; j++) {
            buff[k++] = syms[52+j*38+i] ^ (uint8_t)rev;
        }
    }
    // decode LDPC (1200 + 548 syms -> 600 + 274 bits)
    int nerr1 = sdr_decode_LDPC("IRNV1_SF2", buff, 1200, bits + 9);
    int nerr2 = sdr_decode_LDPC("IRNV1_SF3", buff + 1200, 548, bits + 609);
    sdr_unpack_data(toi, 9, bits);
    
    if (nerr1 >= 0 && nerr2 >= 0 && test_CRC(bits + 9, 600) &&
        test_CRC(bits + 609, 274)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        ch->nav->seq = toi;
        ch->nav->nerr = nerr1 + nerr2;
        sdr_pack_bits(bits, 883, 0, data);
        ch->week = getbitu(data, 9, 13) + GPST_IRT_W;
        update_tow(ch, getbitu(data, 22, 8) * 7200.0 + toi * 18.0 + TOFF_I1SD);
        ch->nav->type = getbitu(data, 609, 6); // L1-SPS SF3 ID
        memcpy(ch->nav->data, data, 111); // L1-SPS SF1+SF2+SF3 (9+600+274 bits)
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 883, str);
        sdr_log(3, "$IRNV1,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(3, "$LOG,%.3f,%s,%d,IRNV1 FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// decode I1SD nav data ([17]) -------------------------------------------------
static void decode_I1SD(sdr_ch_t *ch)
{
    // add symbol buffer
    uint8_t sym = ch->trk->P[SDR_N_HIST-1][0] >= 0.0 ? 1 : 0;
    sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 1852;
    
    if (ch->nav->fsync > 0) { // sync NavIC L1-SPS NAV frame
        if (ch->lock == ch->nav->fsync + 1800) {
            int toi = (ch->nav->seq + 1) % 400;
            int rev = sync_IRNV1_frame(ch, syms, toi);
            if (rev == ch->nav->rev) {
                decode_IRNV1(ch, syms, rev, toi);
            }
        }
        else if (ch->lock > ch->nav->fsync + 1800) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 1852 + 100) {
        // search and decode NavIC L1-SPS NAV frame
        for (int toi = 0; toi < 400; toi++) {
            int rev = sync_IRNV1_frame(ch, syms, toi);
            if (rev >= 0) {
                decode_IRNV1(ch, syms, rev, toi);
                break;
            }
        }
    }
}

// decode I1SP nav data ([17]) -------------------------------------------------
static void decode_I1SP(sdr_ch_t *ch)
{
}

// decode IRNSS SPS NAV frame ([15]) -------------------------------------------
static void decode_IRN_NAV(sdr_ch_t *ch, const uint8_t *syms, int rev)
{
    double time = ch->time - TOFF_I5S;
    uint8_t buff[584], bits[297] = {0}, data[36];
    
    // decode block-interleave (73 x 8)
    for (int i = 0, k = 0; i < 73; i++) {
        for (int j = 0; j < 8; j++) {
            buff[k++] = (syms[16+j*73+i] ^ (uint8_t)rev) * 255;
        }
    }
    // decode 1/2 FEC (584 syms -> 297 bits -> 286 bits)
    sdr_decode_conv(buff, 584, bits);
    
    if (test_CRC(bits, 286)) {
        ch->nav->ssync = ch->nav->fsync = ch->lock;
        ch->nav->rev = rev;
        sdr_pack_bits(bits, 286, 0, data);
        int sf = getbitu(data, 27, 2) + 1;
        if (sf == 1) {
            ch->week = getbitu(data, 30, 10) + 1024 + GPST_IRT_W;
        }
        update_tow(ch, getbitu(data, 8, 17) * 12.0 + TOFF_I5S);
        if (sf >= 1 && sf <= 4) {
            ch->nav->type = sf; // L5-SPS SF NO
            memcpy(ch->nav->data + 37 * (sf - 1), data, 36); // L5-SPS SF (286 bits)
        }
        ch->nav->stat = 1;
        ch->nav->count[0]++;
        char str[256];
        hex_str(data, 286, str);
        sdr_log(3, "$IRNAV,%.3f,%s,%d,%s", time, ch->sig, ch->prn, str);
    }
    else {
        unsync_nav(ch);
        ch->nav->count[1]++;
        sdr_log(3, "$LOG,%.3f,%s,%d,IRNAV FRAME ERROR", time, ch->sig, ch->prn);
    }
}

// decode I5S nav data ([15]) -------------------------------------------------
static void decode_I5S(sdr_ch_t *ch)
{
    static const uint8_t preamb[] = {
        1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0 // 0xEB90
    };
    if (!sync_symb(ch, 20)) { // sync symbol
        return;
    }
    uint8_t *syms = ch->nav->syms + SDR_MAX_NSYM - 616;
    
    if (ch->nav->fsync > 0) { // sync IRNSS SPS NAV subframe
        if (ch->lock == ch->nav->fsync + 12000) {
            int rev = sync_frame(ch, preamb, 16, 0, syms, 600);
            if (rev == ch->nav->rev) {
                decode_IRN_NAV(ch, syms, rev);
            }
        }
        else if (ch->lock > ch->nav->fsync + 12000) {
            unsync_nav(ch);
        }
    }
    else if (ch->lock >= 20 * 616 + 1000) {
        // sync and decode IRNSS SPS NAV subframe
        int rev = sync_frame(ch, preamb, 16, 0, syms, 600);
        if (rev >= 0) {
            decode_IRN_NAV(ch, syms, rev);
        }
    }
}

// decode ISS nav data ([15]) -------------------------------------------------
static void decode_ISS(sdr_ch_t *ch)
{
    decode_I5S(ch);
}

//------------------------------------------------------------------------------
//  Decode navigation data in the correlation history of the tracking GNSS
//  signals. The decoded subframe or message in the navigation data are saved to
//  ch->nav->data asi the packed bits format.
//
//  args:
//      ch       (IO) SDR receiver channel
//
//  returns:
//      none
//
void sdr_nav_decode(sdr_ch_t *ch)
{
    if (!strcmp(ch->sig, "L1CA")) {
        decode_L1CA(ch);
    }
    else if (!strcmp(ch->sig, "L1S")) {
        decode_L1S(ch);
    }
    else if (!strcmp(ch->sig, "L1CB")) {
        decode_L1CB(ch);
    }
    else if (!strcmp(ch->sig, "L1CD")) {
        decode_L1CD(ch);
    }
    else if (!strcmp(ch->sig, "L1CP")) {
        decode_L1CP(ch);
    }
    else if (!strcmp(ch->sig, "L2CM")) {
        decode_L2CM(ch);
    }
    else if (!strcmp(ch->sig, "L5I")) {
        decode_L5I(ch);
    }
    else if (!strcmp(ch->sig, "L5Q")) {
        decode_L5Q(ch);
    }
    else if (!strcmp(ch->sig, "L6D")) {
        decode_L6D(ch);
    }
    else if (!strcmp(ch->sig, "L6E")) {
        decode_L6E(ch);
    }
    else if (!strcmp(ch->sig, "L5SI")) {
        decode_L5SI(ch);
    }
    else if (!strcmp(ch->sig, "L5SQ")) {
        decode_L5SQ(ch);
    }
    else if (!strcmp(ch->sig, "L5SIV")) {
        decode_L5SIV(ch);
    }
    else if (!strcmp(ch->sig, "L5SQV")) {
        decode_L5SQV(ch);
    }
    else if (!strcmp(ch->sig, "G1CA")) {
        decode_G1CA(ch);
    }
    else if (!strcmp(ch->sig, "G2CA")) {
        decode_G2CA(ch);
    }
    else if (!strcmp(ch->sig, "G1OCD")) {
        decode_G1OCD(ch);
    }
    else if (!strcmp(ch->sig, "G1OCP")) {
        decode_G1OCP(ch);
    }
    else if (!strcmp(ch->sig, "G3OCD")) {
        decode_G3OCD(ch);
    }
    else if (!strcmp(ch->sig, "G3OCP")) {
        decode_G3OCP(ch);
    }
    else if (!strcmp(ch->sig, "E1B")) {
        decode_E1B(ch);
    }
    else if (!strcmp(ch->sig, "E1C")) {
        decode_E1C(ch);
    }
    else if (!strcmp(ch->sig, "E5AI")) {
        decode_E5AI(ch);
    }
    else if (!strcmp(ch->sig, "E5AQ")) {
        decode_E5AQ(ch);
    }
    else if (!strcmp(ch->sig, "E5BI")) {
        decode_E5BI(ch);
    }
    else if (!strcmp(ch->sig, "E5BQ")) {
        decode_E5BQ(ch);
    }
    else if (!strcmp(ch->sig, "E6B")) {
        decode_E6B(ch);
    }
    else if (!strcmp(ch->sig, "E6C")) {
        decode_E6C(ch);
    }
    else if (!strcmp(ch->sig, "B1I")) {
        decode_B1I(ch);
    }
    else if (!strcmp(ch->sig, "B1CD")) {
        decode_B1CD(ch);
    }
    else if (!strcmp(ch->sig, "B1CP")) {
        decode_B1CP(ch);
    }
    else if (!strcmp(ch->sig, "B2I")) {
        decode_B2I(ch);
    }
    else if (!strcmp(ch->sig, "B2AD")) {
        decode_B2AD(ch);
    }
    else if (!strcmp(ch->sig, "B2AP")) {
        decode_B2AP(ch);
    }
    else if (!strcmp(ch->sig, "B2BI")) {
        decode_B2BI(ch);
    }
    else if (!strcmp(ch->sig, "B3I")) {
        decode_B3I(ch);
    }
    else if (!strcmp(ch->sig, "I1SD")) {
        decode_I1SD(ch);
    }
    else if (!strcmp(ch->sig, "I1SP")) {
        decode_I1SP(ch);
    }
    else if (!strcmp(ch->sig, "I5S")) {
        decode_I5S(ch);
    }
    else if (!strcmp(ch->sig, "ISS")) {
        decode_ISS(ch);
    }
}

