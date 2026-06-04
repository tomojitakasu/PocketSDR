//
//  Unit tests for sdr_nb_ldpc.c.
//
#include "test_sdr.h"

static const uint8_t H_REP_IDX[3][4] = {
    {0, 1, 1, 1},
    {0, 2, 2, 2},
    {0, 3, 3, 3}
};
static const uint8_t H_REP_ELE[3][4] = {
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1}
};

// convert GF(64) value to bits ------------------------------------------------
static void gf_to_bits(uint8_t val, uint8_t *bits)
{
    for (int i = 0; i < 6; i++) {
        bits[i] = (uint8_t)((val >> (5 - i)) & 1);
    }
}

// set repetition-code symbols -------------------------------------------------
static void set_rep_code(uint8_t val, uint8_t *syms)
{
    for (int i = 0; i < 4; i++) {
        gf_to_bits(val, syms + i * 6);
    }
}

// assert decoded repetition-code symbols --------------------------------------
static void assert_rep_decoded(uint8_t val, const uint8_t *dec)
{
    uint8_t bits[6];
    
    gf_to_bits(val, bits);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 6; j++) {
            TEST_ASSERT_EQ_INT(bits[j], dec[i * 6 + j]);
        }
    }
}

// flip bit positions ----------------------------------------------------------
static void flip_bits(uint8_t *bits, const int *pos, int npos)
{
    for (int i = 0; i < npos; i++) {
        bits[pos[i]] ^= 1;
    }
}

// test sdr_decode_NB_LDPC() empty input ---------------------------------------
static void test_sdr_decode_nb_ldpc_empty_api(void)
{
    static const uint8_t H_idx[1][4] = {{0}};
    static const uint8_t H_ele[1][4] = {{0}};
    uint8_t syms[1] = {0};
    uint8_t dec[1] = {0xAA};
    
    TEST_ASSERT_EQ_INT(0, sdr_decode_NB_LDPC(H_idx, H_ele, 0, 0, syms, dec));
    TEST_ASSERT_EQ_INT(0xAA, dec[0]);
}

// test sdr_decode_NB_LDPC() without errors ------------------------------------
static void test_sdr_decode_nb_ldpc_no_error_api(void)
{
    uint8_t syms[24];
    uint8_t dec[18];
    
    set_rep_code(0x15, syms);
    memset(dec, 0xAA, sizeof(dec));
    
    TEST_ASSERT_EQ_INT(0, sdr_decode_NB_LDPC(H_REP_IDX, H_REP_ELE, 3, 4,
        syms, dec));
    assert_rep_decoded(0x15, dec);
}

// test sdr_decode_NB_LDPC() check-symbol correction ---------------------------
static void test_sdr_decode_nb_ldpc_corrects_check_symbol_error_api(void)
{
    const int err_pos[] = {6 + 2};
    uint8_t syms[24];
    uint8_t dec[18];
    
    set_rep_code(0x15, syms);
    flip_bits(syms, err_pos, sizeof(err_pos) / sizeof(err_pos[0]));
    
    TEST_ASSERT_EQ_INT(1, sdr_decode_NB_LDPC(H_REP_IDX, H_REP_ELE, 3, 4,
        syms, dec));
    assert_rep_decoded(0x15, dec);
}

// test sdr_decode_NB_LDPC() information-symbol correction ---------------------
static void test_sdr_decode_nb_ldpc_corrects_info_symbol_error_api(void)
{
    const int err_pos[] = {3};
    uint8_t syms[24];
    uint8_t dec[18];
    
    set_rep_code(0x2A, syms);
    flip_bits(syms, err_pos, sizeof(err_pos) / sizeof(err_pos[0]));
    
    TEST_ASSERT_EQ_INT(1, sdr_decode_NB_LDPC(H_REP_IDX, H_REP_ELE, 3, 4,
        syms, dec));
    assert_rep_decoded(0x2A, dec);
}

// main ------------------------------------------------------------------------
int main(void)
{
    TEST_RUN(test_sdr_decode_nb_ldpc_empty_api);
    TEST_RUN(test_sdr_decode_nb_ldpc_no_error_api);
    TEST_RUN(test_sdr_decode_nb_ldpc_corrects_check_symbol_error_api);
    TEST_RUN(test_sdr_decode_nb_ldpc_corrects_info_symbol_error_api);
    
    return 0;
}
