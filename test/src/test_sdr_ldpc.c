//
//  Unit tests for sdr_ldpc.c.
//
#include "test_sdr.h"

// get hexadecimal digit value -------------------------------------------------
static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

// convert hexadecimal string to bits ------------------------------------------
static int hex_to_bits(const char *hex, uint8_t *bits, int max_bits)
{
    int n = 0;
    
    for (int i = 0; hex[i] && n + 4 <= max_bits; i++) {
        int val = hex_value(hex[i]);
        
        for (int j = 0; j < 4; j++) {
            bits[n++] = (uint8_t)((val >> (3 - j)) & 1);
        }
    }
    return n;
}

// assert bit arrays are equal -------------------------------------------------
static void assert_bits_eq(const uint8_t *a, const uint8_t *b, int n)
{
    for (int i = 0; i < n; i++) {
        TEST_ASSERT_EQ_INT(a[i], b[i]);
    }
}

// assert decoded prefix is inverted source bits -------------------------------
static void assert_bits_inv_prefix(const uint8_t *dec, const uint8_t *src, int n)
{
    for (int i = 0; i < n; i++) {
        TEST_ASSERT_EQ_INT(src[i] ^ 1, dec[i]);
    }
}

// convert hard bits to soft symbols ------------------------------------------
static void bits_to_soft(const uint8_t *bits, int n, uint8_t *soft)
{
    for (int i = 0; i < n; i++) {
        soft[i] = bits[i] ? 255 : 0;
    }
}

// flip bit positions ----------------------------------------------------------
static void flip_positions(uint8_t *syms, const int *pos, int npos)
{
    for (int i = 0; i < npos; i++) {
        syms[pos[i]] = (uint8_t)(255 - syms[pos[i]]);
    }
}

// test sdr_decode_LDPC() unknown type -----------------------------------------
static void test_sdr_decode_ldpc_unknown_type(void)
{
    uint8_t syms[1] = {0};
    uint8_t dec[1] = {0};
    
    TEST_ASSERT_EQ_INT(-1, sdr_decode_LDPC("UNKNOWN", syms, 0, dec));
}

// test sdr_decode_LDPC() CNV2 SF3 without errors ------------------------------
static void test_sdr_decode_ldpc_cnv2_sf3_no_error(void)
{
    const char *hex =
        "3DEEE0D7E71FF77FBFF86A28B9CBC1691F97FFFFFFFFFFFFFF"
        "FFFFFFFFFFFFF5D2977C0718B9DEC318F680AF7205923021BA"
        "37F5D6DBB0F80BBC1FFE32F1378EAE0938718";
    uint8_t org[548];
    uint8_t syms[548];
    uint8_t dec[274];
    
    TEST_ASSERT_EQ_INT(548, hex_to_bits(hex, org, sizeof(org)));
    bits_to_soft(org, 548, syms);
    memset(dec, 0, sizeof(dec));
    
    TEST_ASSERT_EQ_INT(0, sdr_decode_LDPC("CNV2_SF3", syms, 548, dec));
    assert_bits_inv_prefix(dec, org, 274);
}

// test sdr_decode_LDPC() CNV2 SF3 correction ----------------------------------
static void test_sdr_decode_ldpc_cnv2_sf3_corrects_errors(void)
{
    const char *hex =
        "3DEEE0D7E71FF77FBFF86A28B9CBC1691F97FFFFFFFFFFFFFF"
        "FFFFFFFFFFFFF5D2977C0718B9DEC318F680AF7205923021BA"
        "37F5D6DBB0F80BBC1FFE32F1378EAE0938718";
    const int err_pos[] = {3, 47, 128, 277, 416};
    uint8_t org[548];
    uint8_t syms[548];
    uint8_t dec[274];
    
    TEST_ASSERT_EQ_INT(548, hex_to_bits(hex, org, sizeof(org)));
    bits_to_soft(org, 548, syms);
    flip_positions(syms, err_pos, sizeof(err_pos) / sizeof(err_pos[0]));
    
    TEST_ASSERT_EQ_INT(5, sdr_decode_LDPC("CNV2_SF3", syms, 548, dec));
    assert_bits_inv_prefix(dec, org, 274);
}

// test sdr_decode_LDPC() IRNV1 SF3 correction ---------------------------------
static void test_sdr_decode_ldpc_irnv1_sf3_corrects_errors(void)
{
    const char *hex =
        "FD555555555555555555555555555555555555555555555555"
        "5555555555554601CE1A297425A2EB11C5D1891FB31895EE56"
        "E776EC3249CC5FC6D9369B16D21E07040554B";
    const int err_pos[] = {5, 73, 201, 352};
    uint8_t org[548];
    uint8_t syms[548];
    uint8_t dec[274];
    
    TEST_ASSERT_EQ_INT(548, hex_to_bits(hex, org, sizeof(org)));
    bits_to_soft(org, 548, syms);
    flip_positions(syms, err_pos, sizeof(err_pos) / sizeof(err_pos[0]));
    
    TEST_ASSERT_EQ_INT(4, sdr_decode_LDPC("IRNV1_SF3", syms, 548, dec));
    assert_bits_inv_prefix(dec, org, 274);
}

// test sdr_decode_LDPC() BCNV2 without errors ---------------------------------
static void test_sdr_decode_ldpc_bcnv2_no_error(void)
{
    const char *hex =
        "5CA92F4075A0006DDCC02A522FFD64695BB0008809750F4B6C"
        "0087B26E469E3B6FD2C19F13FAD5D6C360E72ED54C2607B594"
        "A4CF0EDB258BBD81AFDF27A700E0BB872ACAA9B0A73B";
    uint8_t org[576];
    uint8_t syms[576];
    uint8_t dec[288];
    
    TEST_ASSERT_EQ_INT(576, hex_to_bits(hex, org, sizeof(org)));
    bits_to_soft(org, 576, syms);
    memset(dec, 0, sizeof(dec));
    
    TEST_ASSERT_EQ_INT(0, sdr_decode_LDPC("BCNV2", syms, 576, dec));
    assert_bits_eq(dec, org, 288);
}

// test sdr_decode_LDPC() BCNV2 correction -------------------------------------
static void test_sdr_decode_ldpc_bcnv2_corrects_errors(void)
{
    const char *hex =
        "5CA92F4075A0006DDCC02A522FFD64695BB0008809750F4B6C"
        "0087B26E469E3B6FD2C19F13FAD5D6C360E72ED54C2607B594"
        "A4CF0EDB258BBD81AFDF27A700E0BB872ACAA9B0A73B";
    const int err_pos[] = {0, 67, 145, 263, 511};
    uint8_t org[576];
    uint8_t syms[576];
    uint8_t dec[288];
    
    TEST_ASSERT_EQ_INT(576, hex_to_bits(hex, org, sizeof(org)));
    bits_to_soft(org, 576, syms);
    flip_positions(syms, err_pos, sizeof(err_pos) / sizeof(err_pos[0]));
    
    TEST_ASSERT_EQ_INT(5, sdr_decode_LDPC("BCNV2", syms, 576, dec));
    assert_bits_eq(dec, org, 288);
}

// main ------------------------------------------------------------------------
int main(void)
{
    TEST_RUN(test_sdr_decode_ldpc_unknown_type);
    TEST_RUN(test_sdr_decode_ldpc_cnv2_sf3_no_error);
    TEST_RUN(test_sdr_decode_ldpc_cnv2_sf3_corrects_errors);
    TEST_RUN(test_sdr_decode_ldpc_irnv1_sf3_corrects_errors);
    TEST_RUN(test_sdr_decode_ldpc_bcnv2_no_error);
    TEST_RUN(test_sdr_decode_ldpc_bcnv2_corrects_errors);
    
    return 0;
}
