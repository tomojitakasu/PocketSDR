//
//  Unit tests for sdr_nav.c.
//
#include "test_sdr.h"

int test_CRC(const uint8_t *bits, int len_bits);
int test_CRC16_GLO(const uint8_t *bits, int len_bits);

// setup L1CA symbol history powers --------------------------------------------
static void set_l1ca_symbol_history(sdr_ch_t *ch, double latest,
    double previous)
{
    for (int i = 0; i < 200; i++) {
        int block = i / 20;
        double v = block == 0 ? latest : (block == 1 ? previous : latest);
        int ix = SDR_N_HIST - 1 - i;
        
        ch->trk->P[ix][0] = (float)v;
        ch->trk->P[ix][1] = 0.0f;
    }
}

// test sdr_nav_new(), init() and free() ----------------------------------------
static void test_sdr_nav_new_free_init_api(void)
{
    sdr_nav_t *nav = sdr_nav_new();
    
    TEST_ASSERT_TRUE(nav != NULL);
    nav->ssync = 10;
    nav->fsync = 20;
    nav->rev = 1;
    nav->seq = 2;
    nav->type = 3;
    nav->stat = 4;
    nav->nerr = 5;
    nav->coff = 0.25;
    memset(nav->syms, 0xAA, sizeof(nav->syms));
    memset(nav->data, 0x55, sizeof(nav->data));
    for (int i = 0; i < 16; i++) nav->lock_sf[i] = i + 1;
    
    sdr_nav_init(nav);
    
    TEST_ASSERT_EQ_INT(0, nav->ssync);
    TEST_ASSERT_EQ_INT(0, nav->fsync);
    TEST_ASSERT_EQ_INT(0, nav->rev);
    TEST_ASSERT_EQ_INT(0, nav->seq);
    TEST_ASSERT_EQ_INT(0, nav->type);
    TEST_ASSERT_EQ_INT(0, nav->stat);
    TEST_ASSERT_EQ_INT(0, nav->nerr);
    TEST_ASSERT_NEAR(0.0, nav->coff, 1e-15);
    TEST_ASSERT_EQ_INT(0, nav->syms[0]);
    TEST_ASSERT_EQ_INT(0, nav->syms[SDR_MAX_NSYM-1]);
    TEST_ASSERT_EQ_INT(0, nav->data[0]);
    TEST_ASSERT_EQ_INT(0, nav->data[SDR_MAX_DATA-1]);
    TEST_ASSERT_EQ_INT(0, nav->lock_sf[0]);
    TEST_ASSERT_EQ_INT(0, nav->lock_sf[15]);
    
    sdr_nav_free(nav);
    sdr_nav_free(NULL);
}

// test CRC helper wrappers ----------------------------------------------------
static void test_crc_helpers(void)
{
    uint8_t crc24_bits[24] = {0};
    uint8_t glo_bits[250] = {0};
    
    TEST_ASSERT_EQ_INT(1, test_CRC(crc24_bits, 24));
    crc24_bits[23] = 1;
    TEST_ASSERT_EQ_INT(0, test_CRC(crc24_bits, 24));
    
    TEST_ASSERT_EQ_INT(1, test_CRC16_GLO(glo_bits, 250));
    glo_bits[0] = 1;
    TEST_ASSERT_EQ_INT(0, test_CRC16_GLO(glo_bits, 250));
}

// test sdr_nav_decode() L1CA symbol sync --------------------------------------
static void test_sdr_nav_decode_l1ca_symbol_sync(void)
{
    sdr_ch_t ch;
    sdr_trk_t trk;
    sdr_nav_t nav;
    
    memset(&ch, 0, sizeof(ch));
    memset(&trk, 0, sizeof(trk));
    memset(&nav, 0, sizeof(nav));
    strcpy(ch.sig, "L1CA");
    strcpy(ch.sat, "G01");
    ch.prn = 1;
    ch.trk = &trk;
    ch.nav = &nav;
    ch.lock = 200;
    ch.time = 1.0;
    sdr_nav_init(ch.nav);
    memset(ch.nav->syms, 0xAA, sizeof(ch.nav->syms));
    set_l1ca_symbol_history(&ch, 1.0, -1.0);
    
    sdr_nav_decode(&ch);
    
    TEST_ASSERT_EQ_INT(180, ch.nav->ssync);
    TEST_ASSERT_EQ_INT(0, ch.nav->fsync);
    TEST_ASSERT_EQ_INT(0, ch.nav->stat);
    
    sdr_nav_decode(&ch);
    
    TEST_ASSERT_EQ_INT(0, ch.nav->syms[SDR_MAX_NSYM-1]);
    TEST_ASSERT_EQ_INT(0xAA, ch.nav->syms[SDR_MAX_NSYM-2]);
}

// test sdr_nav_decode() L1CA symbol lost --------------------------------------
static void test_sdr_nav_decode_l1ca_symbol_lost(void)
{
    sdr_ch_t ch;
    sdr_trk_t trk;
    sdr_nav_t nav;
    
    memset(&ch, 0, sizeof(ch));
    memset(&trk, 0, sizeof(trk));
    memset(&nav, 0, sizeof(nav));
    strcpy(ch.sig, "L1CA");
    strcpy(ch.sat, "G01");
    ch.prn = 1;
    ch.trk = &trk;
    ch.nav = &nav;
    ch.nav->ssync = 180;
    ch.nav->rev = 1;
    ch.lock = 200;
    ch.tow = 1234;
    ch.tow_v = 1;
    set_l1ca_symbol_history(&ch, 0.0, 0.0);
    
    sdr_nav_decode(&ch);
    
    TEST_ASSERT_EQ_INT(0, ch.nav->ssync);
    TEST_ASSERT_EQ_INT(0, ch.nav->rev);
    TEST_ASSERT_EQ_INT(0, ch.nav->stat);
}

// test sdr_nav_decode() pilot TOW handling ------------------------------------
static void test_sdr_nav_decode_pilot_tow_api(void)
{
    sdr_ch_t ch;
    sdr_trk_t trk;
    sdr_nav_t nav;
    
    memset(&ch, 0, sizeof(ch));
    memset(&trk, 0, sizeof(trk));
    memset(&nav, 0, sizeof(nav));
    strcpy(ch.sig, "B1CP");
    ch.trk = &trk;
    ch.nav = &nav;
    ch.lock = 100;
    ch.len_sec_code = 1800;
    ch.tow = 1;
    ch.tow_v = 1;
    
    sdr_nav_decode(&ch);
    
    TEST_ASSERT_EQ_INT(-1, ch.tow);
    TEST_ASSERT_EQ_INT(0, ch.tow_v);
    
    trk.sec_sync = 100;
    ch.lock = 1900;
    sdr_nav_decode(&ch);
    
    TEST_ASSERT_EQ_INT(14000, ch.tow);
    TEST_ASSERT_EQ_INT(2, ch.tow_v);
    
    strcpy(ch.sig, "B2AP");
    trk.sec_sync = 7;
    ch.lock = 1007;
    ch.len_sec_code = 1000;
    ch.tow = -1;
    ch.tow_v = 0;
    sdr_nav_decode(&ch);
    
    TEST_ASSERT_EQ_INT(900, ch.tow);
    TEST_ASSERT_EQ_INT(2, ch.tow_v);
}

// test sdr_nav_decode() unknown and empty inputs ------------------------------
static void test_sdr_nav_decode_unknown_and_empty_api(void)
{
    sdr_ch_t ch;
    sdr_trk_t trk;
    sdr_nav_t nav;
    
    memset(&ch, 0, sizeof(ch));
    memset(&trk, 0, sizeof(trk));
    memset(&nav, 0, sizeof(nav));
    ch.trk = &trk;
    ch.nav = &nav;
    ch.tow = 123;
    ch.tow_v = 1;
    nav.stat = 7;
    
    strcpy(ch.sig, "UNKNOWN");
    sdr_nav_decode(&ch);
    TEST_ASSERT_EQ_INT(123, ch.tow);
    TEST_ASSERT_EQ_INT(1, ch.tow_v);
    TEST_ASSERT_EQ_INT(7, nav.stat);
    
    strcpy(ch.sig, "I1SP");
    sdr_nav_decode(&ch);
    TEST_ASSERT_EQ_INT(123, ch.tow);
    TEST_ASSERT_EQ_INT(1, ch.tow_v);
    TEST_ASSERT_EQ_INT(7, nav.stat);
}

// main ------------------------------------------------------------------------
int main(void)
{
    sdr_func_init("");
    
    TEST_RUN(test_sdr_nav_new_free_init_api);
    TEST_RUN(test_crc_helpers);
    TEST_RUN(test_sdr_nav_decode_l1ca_symbol_sync);
    TEST_RUN(test_sdr_nav_decode_l1ca_symbol_lost);
    TEST_RUN(test_sdr_nav_decode_pilot_tow_api);
    TEST_RUN(test_sdr_nav_decode_unknown_and_empty_api);
    
    return 0;
}
