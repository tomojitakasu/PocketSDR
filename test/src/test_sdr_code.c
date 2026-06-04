//
//  Unit tests for sdr_code.c.
//
#include "test_sdr.h"

// assert code chips are +/-1 --------------------------------------------------
static void assert_chips_pm1(const int8_t *code, int N)
{
    TEST_ASSERT_TRUE(code != NULL);
    TEST_ASSERT_TRUE(N > 0);
    
    for (int i = 0; i < N; i++) {
        TEST_ASSERT_TRUE(code[i] == 1 || code[i] == -1);
    }
}

// test sdr_gen_code() ---------------------------------------------------------
static void test_sdr_gen_code_api(void)
{
    int len = -1;
    int8_t *code = sdr_gen_code("L1CA", 1, &len);
    
    TEST_ASSERT_TRUE(code != NULL);
    TEST_ASSERT_EQ_INT(1023, len);
    assert_chips_pm1(code, len);
    
    len = -1;
    TEST_ASSERT_TRUE(sdr_gen_code("l1ca", 1, &len) != NULL);
    TEST_ASSERT_EQ_INT(1023, len);
    
    len = -1;
    TEST_ASSERT_TRUE(sdr_gen_code("UNKNOWN", 1, &len) == NULL);
    
    len = -1;
    TEST_ASSERT_TRUE(sdr_gen_code("L1CA", 0, &len) == NULL);
}

// test sdr_sec_code() ---------------------------------------------------------
static void test_sdr_sec_code_api(void)
{
    int len = -1;
    int8_t *code = sdr_sec_code("L1CA", 1, &len);
    
    TEST_ASSERT_TRUE(code != NULL);
    TEST_ASSERT_EQ_INT(1, len);
    TEST_ASSERT_EQ_INT(1, code[0]);
    
    len = -1;
    code = sdr_sec_code("L1CP", 1, &len);
    assert_chips_pm1(code, len);
    
    len = -1;
    TEST_ASSERT_TRUE(sdr_sec_code("UNKNOWN", 1, &len) == NULL);
    TEST_ASSERT_EQ_INT(0, len);
}

// test code attribute APIs ----------------------------------------------------
static void test_code_attributes_api(void)
{
    TEST_ASSERT_NEAR(1e-3, sdr_code_cyc("L1CA"), 1e-15);
    TEST_ASSERT_NEAR(10e-3, sdr_code_cyc("L1CP"), 1e-15);
    TEST_ASSERT_NEAR(0.0, sdr_code_cyc("UNKNOWN"), 1e-15);
    
    TEST_ASSERT_EQ_INT(1023, sdr_code_len("L1CA"));
    TEST_ASSERT_EQ_INT(4092, sdr_code_len("E1B"));
    TEST_ASSERT_EQ_INT(1023, sdr_code_len("l1ca"));
    TEST_ASSERT_EQ_INT(0, sdr_code_len("UNKNOWN"));
    
    TEST_ASSERT_NEAR(1575.42e6, sdr_sig_freq("L1CA"), 1.0);
    TEST_ASSERT_NEAR(1602.0e6, sdr_sig_freq("G1CA"), 1.0);
    TEST_ASSERT_NEAR(0.0, sdr_sig_freq("UNKNOWN"), 1e-15);
}

// test sdr_sat_id() -----------------------------------------------------------
static void test_sdr_sat_id_api(void)
{
    char sat[16] = "";
    
    sdr_sat_id("L1CA", 1, sat);
    TEST_ASSERT_TRUE(!strcmp(sat, "G01"));
    
    sdr_sat_id("L1CA", 120, sat);
    TEST_ASSERT_TRUE(!strcmp(sat, "S20"));
    
    sdr_sat_id("L6D", 193, sat);
    TEST_ASSERT_TRUE(!strcmp(sat, "J01"));
    
    sdr_sat_id("G1CA", -7, sat);
    TEST_ASSERT_TRUE(!strcmp(sat, "R-7"));
    
    sdr_sat_id("E1B", 1, sat);
    TEST_ASSERT_TRUE(!strcmp(sat, "E01"));
    
    sdr_sat_id("B1I", 1, sat);
    TEST_ASSERT_TRUE(!strcmp(sat, "C01"));
    
    sdr_sat_id("I5S", 1, sat);
    TEST_ASSERT_TRUE(!strcmp(sat, "I01"));
    
    sdr_sat_id("UNKNOWN", 1, sat);
    TEST_ASSERT_TRUE(!strcmp(sat, "???"));
    
    sdr_sat_id("B1CP", 1, sat);
    TEST_ASSERT_TRUE(!strcmp(sat, "???"));
}

// test sdr_sig_boc() ----------------------------------------------------------
static void test_sdr_sig_boc_api(void)
{
    TEST_ASSERT_EQ_INT(0, sdr_sig_boc("L1CA"));
    TEST_ASSERT_EQ_INT(1, sdr_sig_boc("L1CP"));
    TEST_ASSERT_EQ_INT(1, sdr_sig_boc("e1b"));
    TEST_ASSERT_EQ_INT(0, sdr_sig_boc("UNKNOWN"));
}

// test sdr_res_code() ---------------------------------------------------------
static void test_sdr_res_code_api(void)
{
    const int8_t code_I[] = {1, -1, 1, -1};
    const int8_t code_Q[] = {-1, -1, 1, 1};
    sdr_cpx16_t code_res[6];
    
    sdr_res_code(code_I, NULL, 4, 1.0, 0.0, 4.0, 4, 2, code_res);
    
    TEST_ASSERT_EQ_INT(1, code_res[0].I);
    TEST_ASSERT_EQ_INT(1, code_res[0].Q);
    TEST_ASSERT_EQ_INT(-1, code_res[1].I);
    TEST_ASSERT_EQ_INT(-1, code_res[1].Q);
    TEST_ASSERT_EQ_INT(1, code_res[2].I);
    TEST_ASSERT_EQ_INT(1, code_res[2].Q);
    TEST_ASSERT_EQ_INT(-1, code_res[3].I);
    TEST_ASSERT_EQ_INT(-1, code_res[3].Q);
    TEST_ASSERT_EQ_INT(0, code_res[4].I);
    TEST_ASSERT_EQ_INT(0, code_res[4].Q);
    TEST_ASSERT_EQ_INT(0, code_res[5].I);
    TEST_ASSERT_EQ_INT(0, code_res[5].Q);
    
    sdr_res_code(code_I, code_Q, 4, 1.0, 0.0, 4.0, 4, 2, code_res);
    
    TEST_ASSERT_EQ_INT(1, code_res[0].I);
    TEST_ASSERT_EQ_INT(-1, code_res[0].Q);
    TEST_ASSERT_EQ_INT(-1, code_res[1].I);
    TEST_ASSERT_EQ_INT(-1, code_res[1].Q);
    TEST_ASSERT_EQ_INT(1, code_res[2].I);
    TEST_ASSERT_EQ_INT(1, code_res[2].Q);
    TEST_ASSERT_EQ_INT(-1, code_res[3].I);
    TEST_ASSERT_EQ_INT(1, code_res[3].Q);
    TEST_ASSERT_EQ_INT(0, code_res[4].I);
    TEST_ASSERT_EQ_INT(0, code_res[4].Q);
}

// test sdr_gen_code_fft() -----------------------------------------------------
static void test_sdr_gen_code_fft_api(void)
{
    const int8_t code_I[] = {1, 1, 1, 1};
    const int8_t code_Q[] = {1, 1, 1, 1};
    sdr_cpx_t code_fft[4];
    
    sdr_gen_code_fft(code_I, NULL, 4, 1.0, 0.0, 4.0, 4, 0, code_fft);
    
    TEST_ASSERT_NEAR(4.0, code_fft[0][0], 1e-5);
    TEST_ASSERT_NEAR(0.0, code_fft[0][1], 1e-5);
    for (int i = 1; i < 4; i++) {
        TEST_ASSERT_NEAR(0.0, code_fft[i][0], 1e-5);
        TEST_ASSERT_NEAR(0.0, code_fft[i][1], 1e-5);
    }
    
    sdr_gen_code_fft(code_I, code_Q, 4, 1.0, 0.0, 4.0, 4, 0, code_fft);
    
    TEST_ASSERT_NEAR(4.0, code_fft[0][0], 1e-5);
    TEST_ASSERT_NEAR(-4.0, code_fft[0][1], 1e-5);
    for (int i = 1; i < 4; i++) {
        TEST_ASSERT_NEAR(0.0, code_fft[i][0], 1e-5);
        TEST_ASSERT_NEAR(0.0, code_fft[i][1], 1e-5);
    }
}

// main ------------------------------------------------------------------------
int main(void)
{
    sdr_func_init("");
    
    TEST_RUN(test_sdr_gen_code_api);
    TEST_RUN(test_sdr_sec_code_api);
    TEST_RUN(test_code_attributes_api);
    TEST_RUN(test_sdr_sat_id_api);
    TEST_RUN(test_sdr_sig_boc_api);
    TEST_RUN(test_sdr_res_code_api);
    TEST_RUN(test_sdr_gen_code_fft_api);
    
    return 0;
}
