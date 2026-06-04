//
//  Unit tests for sdr_code_gal.c data tables.
//
#include "test_sdr.h"

extern const char *code_gal_E1B[];
extern const char *code_gal_E1C[];
extern const char *code_gal_E6B[];
extern const char *code_gal_E6C[];
extern const char *code_gal_CS4;
extern const char *code_gal_CS20;
extern const char *code_gal_CS25;
extern const char *code_gal_CS100[];

// check hexadecimal character -------------------------------------------------
static int is_hex_char(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

// assert hexadecimal string length and contents -------------------------------
static void assert_hex_string(const char *str, int len)
{
    TEST_ASSERT_TRUE(str != NULL);
    TEST_ASSERT_EQ_INT(len, strlen(str));
    
    for (int i = 0; i < len; i++) {
        TEST_ASSERT_TRUE(is_hex_char(str[i]));
    }
}

// assert hexadecimal code table contents --------------------------------------
static void assert_hex_table(const char *table[], int n, int len)
{
    for (int i = 0; i < n; i++) {
        assert_hex_string(table[i], len);
    }
    TEST_ASSERT_TRUE(strcmp(table[0], table[n-1]) != 0);
}

// test Galileo E1 primary code tables -----------------------------------------
static void test_e1_primary_code_tables(void)
{
    assert_hex_table(code_gal_E1B, 50, 1023);
    assert_hex_table(code_gal_E1C, 50, 1023);
    
    TEST_ASSERT_TRUE(!strncmp(code_gal_E1B[0], "F5D710130573541B", 16));
    TEST_ASSERT_TRUE(!strncmp(code_gal_E1C[0], "B39340CA1C817D81", 16));
}

// test Galileo E6 primary code tables -----------------------------------------
static void test_e6_primary_code_tables(void)
{
    assert_hex_table(code_gal_E6B, 50, 1279);
    assert_hex_table(code_gal_E6C, 50, 1279);
    
    TEST_ASSERT_TRUE(!strncmp(code_gal_E6B[0], "E6648AA5EFF0907A", 16));
    TEST_ASSERT_TRUE(!strncmp(code_gal_E6C[0], "F5A3D656F9DAC534", 16));
}

// test Galileo secondary code tables ------------------------------------------
static void test_secondary_code_tables(void)
{
    assert_hex_string(code_gal_CS4, 1);
    assert_hex_string(code_gal_CS20, 5);
    assert_hex_string(code_gal_CS25, 7);
    assert_hex_table(code_gal_CS100, 100, 25);
    
    TEST_ASSERT_TRUE(!strcmp(code_gal_CS4, "E"));
    TEST_ASSERT_TRUE(!strcmp(code_gal_CS20, "842E9"));
    TEST_ASSERT_TRUE(!strcmp(code_gal_CS25, "380AD90"));
    TEST_ASSERT_TRUE(!strcmp(code_gal_CS100[0], "83F6F69D8F6E15411FB8C9B1C"));
    TEST_ASSERT_TRUE(!strcmp(code_gal_CS100[99], "64310BAD8EB5B36E38646AF01"));
}

// main ------------------------------------------------------------------------
int main(void)
{
    TEST_RUN(test_e1_primary_code_tables);
    TEST_RUN(test_e6_primary_code_tables);
    TEST_RUN(test_secondary_code_tables);
    
    return 0;
}
