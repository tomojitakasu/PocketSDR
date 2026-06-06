//
//  Unit tests for sdr_fec.c.
//
#include "test_sdr.h"

// constants -------------------------------------------------------------------
#define TEST_RS_N       255
#define TEST_RS_K       223
#define TEST_RS_NROOTS  32
#define TEST_RS_A0      255
#define TEST_RS_GF_POLY 0x187
#define TEST_RS_FCR     112
#define TEST_RS_PRIM    11

// global variables ------------------------------------------------------------
static uint8_t TEST_RS_ALPHA_TO[TEST_RS_N + 1];
static uint8_t TEST_RS_INDEX_OF[TEST_RS_N + 1];
static uint8_t TEST_RS_GENPOLY[TEST_RS_NROOTS + 1];
static uint8_t TEST_RS_TAL[256];
static uint8_t TEST_RS_TAL1[256];
static int TEST_RS_INIT = 0;

// reduce GF(256) exponent modulo 255 ------------------------------------------
static int test_rs_mod255(int x)
{
    while (x < 0) x += TEST_RS_N;
    while (x >= TEST_RS_N) {
        x -= TEST_RS_N;
        x = (x >> 8) + (x & TEST_RS_N);
    }
    return x;
}

// convert conventional to CCSDS dual basis ------------------------------------
static uint8_t test_rs_to_dual(uint8_t x)
{
    static const uint8_t tal[] = {
        0x8D, 0xEF, 0xEC, 0x86, 0xFA, 0x99, 0xAF, 0x7B
    };
    uint8_t y = 0;
    
    for (int j = 0; j < 8; j++) {
        for (int k = 0; k < 8; k++) {
            if (x & (1 << k)) y ^= tal[7-k] & (1 << j);
        }
    }
    return y;
}

// initialize CCSDS RS test tables ---------------------------------------------
static void test_rs_init_ccsds(void)
{
    if (TEST_RS_INIT) return;
    
    TEST_RS_INDEX_OF[0] = TEST_RS_A0;
    TEST_RS_ALPHA_TO[TEST_RS_A0] = 0;
    int sr = 1;
    for (int i = 0; i < TEST_RS_N; i++) {
        TEST_RS_INDEX_OF[sr] = (uint8_t)i;
        TEST_RS_ALPHA_TO[i] = (uint8_t)sr;
        sr <<= 1;
        if (sr & 0x100) sr ^= TEST_RS_GF_POLY;
        sr &= TEST_RS_N;
    }
    for (int i = 0; i < 256; i++) {
        TEST_RS_TAL[i] = test_rs_to_dual((uint8_t)i);
        TEST_RS_TAL1[TEST_RS_TAL[i]] = (uint8_t)i;
    }
    TEST_RS_GENPOLY[0] = 1;
    for (int i = 0, root = TEST_RS_FCR * TEST_RS_PRIM; i < TEST_RS_NROOTS;
        i++, root += TEST_RS_PRIM) {
        TEST_RS_GENPOLY[i+1] = 1;
        for (int j = i; j > 0; j--) {
            if (TEST_RS_GENPOLY[j]) {
                int idx = TEST_RS_INDEX_OF[TEST_RS_GENPOLY[j]] + root;
                TEST_RS_GENPOLY[j] = TEST_RS_GENPOLY[j-1] ^
                    TEST_RS_ALPHA_TO[test_rs_mod255(idx)];
            } else {
                TEST_RS_GENPOLY[j] = TEST_RS_GENPOLY[j-1];
            }
        }
        TEST_RS_GENPOLY[0] =
            TEST_RS_ALPHA_TO[test_rs_mod255(TEST_RS_INDEX_OF[TEST_RS_GENPOLY[0]] +
            root)];
    }
    for (int i = 0; i <= TEST_RS_NROOTS; i++) {
        TEST_RS_GENPOLY[i] = TEST_RS_INDEX_OF[TEST_RS_GENPOLY[i]];
    }
    TEST_RS_INIT = 1;
}

// encode CCSDS RS(255,223) test codeword --------------------------------------
static void encode_rs_ccsds_test(uint8_t *data, uint8_t *parity)
{
    uint8_t cparity[TEST_RS_NROOTS] = {0};
    
    test_rs_init_ccsds();
    for (int i = 0; i < TEST_RS_K; i++) {
        uint8_t cdata = TEST_RS_TAL1[data[i]];
        uint8_t feedback = TEST_RS_INDEX_OF[cdata ^ cparity[0]];
        if (feedback != TEST_RS_A0) {
            for (int j = 1; j < TEST_RS_NROOTS; j++) {
                cparity[j] ^= TEST_RS_ALPHA_TO[test_rs_mod255(feedback +
                    TEST_RS_GENPOLY[TEST_RS_NROOTS-j])];
            }
        }
        memmove(cparity, cparity + 1, TEST_RS_NROOTS - 1);
        cparity[TEST_RS_NROOTS-1] = (feedback == TEST_RS_A0) ? 0 :
            TEST_RS_ALPHA_TO[test_rs_mod255(feedback + TEST_RS_GENPOLY[0])];
    }
    for (int i = 0; i < TEST_RS_NROOTS; i++) {
        parity[i] = TEST_RS_TAL[cparity[i]];
    }
}

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
    encode_rs_ccsds_test(syms, syms + 223);
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

// test sdr_decode_conv() with soft symbols ------------------------------------
static void test_sdr_decode_conv_soft_api(void)
{
    const uint8_t bits[] = {
        0, 1, 1, 0, 1, 0, 0, 1,
        1, 1, 0, 0, 0, 1, 0, 1
    };
    uint8_t syms[(int)(sizeof(bits) + 6) * 2];
    uint8_t dec[sizeof(bits)];
    
    encode_conv_test_bits(bits, sizeof(bits), syms);
    for (int i = 0; i < (int)sizeof(syms); i++) {
        syms[i] = syms[i] ? 205 : 50;
    }
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
    TEST_RUN(test_sdr_decode_conv_soft_api);
    TEST_RUN(test_sdr_decode_conv_edge_api);
    TEST_RUN(test_sdr_decode_rs_no_error_api);
    TEST_RUN(test_sdr_decode_rs_32_corrected_bits_api);
    TEST_RUN(test_sdr_decode_rs_uncorrectable_api);
    
    return 0;
}
