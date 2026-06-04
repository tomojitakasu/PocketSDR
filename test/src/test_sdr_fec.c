//
//  Unit tests for sdr_fec.c.
//
#include "test_sdr.h"

#ifdef __cplusplus
extern "C" {
#endif
void encode_rs_ccsds(uint8_t *data, uint8_t *parity, int pad);
#ifdef __cplusplus
}
#endif

// compute parity of byte ------------------------------------------------------
static int parity_u8(uint8_t x)
{
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return x & 1;
}

// encode convolutional test symbols -------------------------------------------
static void encode_conv_test_bits(const uint8_t *bits, int nbits,
    uint8_t *syms)
{
    const uint8_t poly1 = 0x4F;
    const uint8_t poly2 = 0x6D;
    uint8_t state = 0;
    
    for (int i = 0; i < nbits + 6; i++) {
        uint8_t bit = i < nbits ? (bits[i] & 1) : 0;
        
        state = (uint8_t)((state << 1) | bit);
        syms[i*2  ] = parity_u8(state & poly1) ? 255 : 0;
        syms[i*2+1] = parity_u8(state & poly2) ? 255 : 0;
    }
}

// initialize CCSDS Reed-Solomon codeword --------------------------------------
static void init_rs_codeword(uint8_t syms[255])
{
    for (int i = 0; i < 223; i++) {
        syms[i] = (uint8_t)(i * 17 + 3);
    }
    memset(syms + 223, 0, 32);
    encode_rs_ccsds(syms, syms + 223, 0);
}

// assert symbol arrays are equal ----------------------------------------------
static void assert_same_syms(const uint8_t *a, const uint8_t *b, int n)
{
    for (int i = 0; i < n; i++) {
        TEST_ASSERT_EQ_INT(a[i], b[i]);
    }
}

// test sdr_decode_conv() ------------------------------------------------------
static void test_sdr_decode_conv_api(void)
{
    const uint8_t bits[] = {
        1, 0, 1, 1, 0, 0, 1, 1,
        1, 0, 0, 1, 0, 1, 0, 1
    };
    uint8_t syms[(int)(sizeof(bits) + 6) * 2];
    uint8_t dec[sizeof(bits)];
    
    encode_conv_test_bits(bits, sizeof(bits), syms);
    memset(dec, 0, sizeof(dec));
    
    sdr_decode_conv(syms, sizeof(syms), dec);
    
    assert_same_syms(dec, bits, sizeof(bits));
}

// test sdr_decode_conv() edge cases -------------------------------------------
static void test_sdr_decode_conv_edge_api(void)
{
    uint8_t syms[10] = {0};
    uint8_t dec[4] = {0xAA, 0xAA, 0xAA, 0xAA};
    
    sdr_decode_conv(syms, sizeof(syms), dec);
    
    TEST_ASSERT_EQ_INT(0xAA, dec[0]);
    TEST_ASSERT_EQ_INT(0xAA, dec[1]);
    TEST_ASSERT_EQ_INT(0xAA, dec[2]);
    TEST_ASSERT_EQ_INT(0xAA, dec[3]);
}

// test sdr_decode_rs() without errors -----------------------------------------
static void test_sdr_decode_rs_no_error_api(void)
{
    uint8_t syms[255];
    uint8_t org[255];
    
    init_rs_codeword(syms);
    memcpy(org, syms, sizeof(org));
    
    TEST_ASSERT_EQ_INT(0, sdr_decode_rs(syms));
    assert_same_syms(syms, org, 255);
}

// test sdr_decode_rs() with 32 corrected symbols ------------------------------
static void test_sdr_decode_rs_32_corrected_bits_api(void)
{
    uint8_t syms[255];
    uint8_t org[255];
    
    init_rs_codeword(syms);
    memcpy(org, syms, sizeof(org));
    
    for (int i = 0; i < 16; i++) {
        syms[i * 13] ^= 0x03; // 16 symbol errors, 2 bits each.
    }
    
    TEST_ASSERT_EQ_INT(32, sdr_decode_rs(syms));
    assert_same_syms(syms, org, 255);
}

// test sdr_decode_rs() uncorrectable case -------------------------------------
static void test_sdr_decode_rs_uncorrectable_api(void)
{
    uint8_t syms[255];
    
    init_rs_codeword(syms);
    
    for (int i = 0; i < 33; i++) {
        syms[i] ^= 0x01;
    }
    
    TEST_ASSERT_EQ_INT(-1, sdr_decode_rs(syms));
}

// main ------------------------------------------------------------------------
int main(void)
{
    TEST_RUN(test_sdr_decode_conv_api);
    TEST_RUN(test_sdr_decode_conv_edge_api);
    TEST_RUN(test_sdr_decode_rs_no_error_api);
    TEST_RUN(test_sdr_decode_rs_32_corrected_bits_api);
    TEST_RUN(test_sdr_decode_rs_uncorrectable_api);
    
    return 0;
}
