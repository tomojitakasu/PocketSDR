//
//  Unit tests for sdr_ch.c.
//
#include "test_sdr.h"

extern double sdr_t_acq;

// pack signed I/Q values into sdr_cpx8_t --------------------------------------
static sdr_cpx8_t pack_cpx8(int re, int im)
{
    return (sdr_cpx8_t)((((uint8_t)im & 0x0F) << 4) | ((uint8_t)re & 0x0F));
}

// allocate zeroed IF data buffer ----------------------------------------------
static sdr_buff_t *new_zero_buff(int N)
{
    sdr_buff_t *buff = sdr_buff_new(N, 2);
    
    TEST_ASSERT_TRUE(buff != NULL);
    for (int i = 0; i < N; i++) {
        buff->data[i] = pack_cpx8(0, 0);
    }
    return buff;
}

// test sdr_ch_new() and sdr_ch_free() -----------------------------------------
static void test_sdr_ch_new_free_api(void)
{
    sdr_ch_t *ch = sdr_ch_new("L1CA", 1, 4e6, 0.0);
    
    TEST_ASSERT_TRUE(ch != NULL);
    TEST_ASSERT_EQ_INT(SDR_STATE_IDLE, ch->state);
    TEST_ASSERT_EQ_INT(1, ch->prn);
    TEST_ASSERT_TRUE(!strcmp(ch->sig, "L1CA"));
    TEST_ASSERT_TRUE(!strcmp(ch->sat, "G01"));
    TEST_ASSERT_EQ_INT(1023, ch->len_code);
    TEST_ASSERT_EQ_INT(1, ch->len_sec_code);
    TEST_ASSERT_NEAR(4e6, ch->fs, 1e-6);
    TEST_ASSERT_NEAR(1e-3, ch->T, 1e-15);
    TEST_ASSERT_EQ_INT(4000, ch->N);
    TEST_ASSERT_TRUE(ch->acq != NULL);
    TEST_ASSERT_TRUE(ch->trk != NULL);
    TEST_ASSERT_TRUE(ch->nav != NULL);
    TEST_ASSERT_TRUE(ch->data != NULL);
    TEST_ASSERT_EQ_INT(4, ch->trk->npos);
    TEST_ASSERT_EQ_INT(0, ch->trk->nposx);
    sdr_ch_free(ch);
    
    ch = sdr_ch_new("l1ca", 1, 4e6, 0.0);
    TEST_ASSERT_TRUE(ch != NULL);
    TEST_ASSERT_TRUE(!strcmp(ch->sig, "L1CA"));
    sdr_ch_free(ch);
    
    TEST_ASSERT_TRUE(sdr_ch_new("UNKNOWN", 1, 4e6, 0.0) == NULL);
    TEST_ASSERT_TRUE(sdr_ch_new("L1CA", 0, 4e6, 0.0) == NULL);
    sdr_ch_free(NULL);
}

// test sdr_ch_set_corr() ------------------------------------------------------
static void test_sdr_ch_set_corr_api(void)
{
    sdr_ch_t *ch = sdr_ch_new("L1CA", 1, 4e6, 0.0);
    double stat[8], pos[SDR_MAX_CORR], P[SDR_MAX_CORR], I[SDR_MAX_CORR];
    sdr_cpx_t C[SDR_MAX_CORR];
    int npos;
    
    TEST_ASSERT_TRUE(ch != NULL);
    
    sdr_ch_set_corr(ch, 5, 2e-6);
    npos = sdr_ch_corr_stat(ch, stat, pos, C, P, I);
    
    TEST_ASSERT_EQ_INT(9, npos);
    TEST_ASSERT_EQ_INT(4, (int)stat[6]);
    TEST_ASSERT_EQ_INT(5, ch->trk->nposx);
    TEST_ASSERT_NEAR(-3.2, pos[4], 1e-9);
    TEST_ASSERT_NEAR(0.0, pos[6], 1e-9);
    TEST_ASSERT_NEAR(3.2, pos[8], 1e-9);
    
    sdr_ch_set_corr(ch, SDR_N_CORRX + 10, 1e-6);
    npos = sdr_ch_corr_stat(ch, stat, pos, C, P, NULL);
    TEST_ASSERT_EQ_INT(4 + SDR_N_CORRX, npos);
    TEST_ASSERT_EQ_INT(SDR_N_CORRX, ch->trk->nposx);
    
    sdr_ch_free(ch);
}

// test sdr_ch_corr_stat() -----------------------------------------------------
static void test_sdr_ch_corr_stat_api(void)
{
    sdr_ch_t *ch = sdr_ch_new("L1CA", 1, 4e6, 0.0);
    double stat[8], pos[SDR_MAX_CORR], P[SDR_MAX_CORR], I[SDR_MAX_CORR];
    sdr_cpx_t C[SDR_MAX_CORR];
    int npos;
    
    TEST_ASSERT_TRUE(ch != NULL);
    
    ch->state = SDR_STATE_LOCK;
    ch->lock = 7;
    ch->cn0 = 42.5;
    ch->coff = 2.5e-4;
    ch->fd = -123.0;
    ch->trk->C[0][0] = 10.0f;
    ch->trk->C[0][1] = -2.0f;
    ch->trk->aveP[0] = 25.0;
    ch->trk->aveI[0] = 4.0;
    
    npos = sdr_ch_corr_stat(ch, stat, pos, C, P, I);
    
    TEST_ASSERT_EQ_INT(4, npos);
    TEST_ASSERT_EQ_INT(SDR_STATE_LOCK, (int)stat[0]);
    TEST_ASSERT_NEAR(4e6, stat[1], 1e-6);
    TEST_ASSERT_NEAR(7e-3, stat[2], 1e-12);
    TEST_ASSERT_NEAR(42.5, stat[3], 1e-12);
    TEST_ASSERT_NEAR(0.25, stat[4], 1e-12);
    TEST_ASSERT_NEAR(-123.0, stat[5], 1e-12);
    TEST_ASSERT_EQ_INT(4, (int)stat[6]);
    TEST_ASSERT_NEAR(10.0, C[0][0], 1e-6);
    TEST_ASSERT_NEAR(-2.0, C[0][1], 1e-6);
    TEST_ASSERT_NEAR(25.0, P[0], 1e-12);
    TEST_ASSERT_NEAR(4.0, I[0], 1e-12);
    
    sdr_ch_free(ch);
}

// test sdr_ch_corr_hist() -----------------------------------------------------
static void test_sdr_ch_corr_hist_api(void)
{
    sdr_ch_t *ch = sdr_ch_new("L1CA", 1, 4e6, 0.0);
    double stat[2];
    sdr_cpx_t P[SDR_N_HIST];
    int n;
    
    TEST_ASSERT_TRUE(ch != NULL);
    
    ch->time = 12.25;
    for (int i = 0; i < SDR_N_HIST; i++) {
        ch->trk->P[i][0] = (float)i;
        ch->trk->P[i][1] = (float)-i;
    }
    
    n = sdr_ch_corr_hist(ch, 3.5 * ch->T, stat, P);
    TEST_ASSERT_EQ_INT(3, n);
    TEST_ASSERT_NEAR(12.25, stat[0], 1e-12);
    TEST_ASSERT_NEAR(ch->T, stat[1], 1e-15);
    TEST_ASSERT_NEAR(SDR_N_HIST - 3, P[0][0], 1e-6);
    TEST_ASSERT_NEAR(-(SDR_N_HIST - 3), P[0][1], 1e-6);
    TEST_ASSERT_NEAR(SDR_N_HIST - 1, P[2][0], 1e-6);
    
    n = sdr_ch_corr_hist(ch, 10000.0 * ch->T, stat, P);
    TEST_ASSERT_EQ_INT(SDR_N_HIST, n);
    
    sdr_ch_free(ch);
}

// test sdr_ch_update() in IDLE state ------------------------------------------
static void test_sdr_ch_update_idle_api(void)
{
    sdr_ch_t *ch = sdr_ch_new("L1CA", 1, 4e6, 0.0);
    sdr_buff_t *buff;
    
    TEST_ASSERT_TRUE(ch != NULL);
    buff = new_zero_buff(ch->N * 2);
    
    sdr_ch_update(ch, 1.0, buff, 0);
    
    TEST_ASSERT_EQ_INT(SDR_STATE_IDLE, ch->state);
    TEST_ASSERT_EQ_INT(0, ch->lock);
    TEST_ASSERT_NEAR(0.0, ch->time, 1e-12);
    
    sdr_buff_free(buff);
    sdr_ch_free(ch);
}

// test sdr_ch_update() in SEARCH state ----------------------------------------
static void test_sdr_ch_update_search_api(void)
{
    sdr_ch_t *ch = sdr_ch_new("L1CA", 1, 4e6, 0.0);
    sdr_buff_t *buff;
    double old_t_acq = sdr_t_acq;
    
    TEST_ASSERT_TRUE(ch != NULL);
    buff = new_zero_buff(ch->N * 2);
    
    sdr_t_acq = ch->T;
    ch->state = SDR_STATE_SRCH;
    sdr_ch_update(ch, 1.0, buff, 0);
    
    TEST_ASSERT_EQ_INT(SDR_STATE_IDLE, ch->state);
    TEST_ASSERT_EQ_INT(0, ch->acq->n_sum);
    TEST_ASSERT_TRUE(ch->acq->P_sum == NULL);
    
    sdr_t_acq = old_t_acq;
    sdr_buff_free(buff);
    sdr_ch_free(ch);
}

// test sdr_ch_update() in LOCK state ------------------------------------------
static void test_sdr_ch_update_lock_api(void)
{
    sdr_ch_t *ch = sdr_ch_new("L1CA", 1, 4e6, 0.0);
    sdr_buff_t *buff;
    
    TEST_ASSERT_TRUE(ch != NULL);
    buff = new_zero_buff(ch->N * 2);
    
    ch->state = SDR_STATE_LOCK;
    ch->time = 0.0;
    ch->fd = 0.0;
    ch->coff = 0.0;
    ch->cn0 = 45.0;
    
    sdr_ch_update(ch, ch->T, buff, 0);
    
    TEST_ASSERT_EQ_INT(SDR_STATE_LOCK, ch->state);
    TEST_ASSERT_EQ_INT(1, ch->lock);
    TEST_ASSERT_NEAR(ch->T, ch->time, 1e-12);
    TEST_ASSERT_TRUE(isfinite(ch->fd));
    TEST_ASSERT_TRUE(isfinite(ch->coff));
    
    sdr_buff_free(buff);
    sdr_ch_free(ch);
}

// main ------------------------------------------------------------------------
int main(void)
{
    sdr_func_init("");
    
    TEST_RUN(test_sdr_ch_new_free_api);
    TEST_RUN(test_sdr_ch_set_corr_api);
    TEST_RUN(test_sdr_ch_corr_stat_api);
    TEST_RUN(test_sdr_ch_corr_hist_api);
    TEST_RUN(test_sdr_ch_update_idle_api);
    TEST_RUN(test_sdr_ch_update_search_api);
    TEST_RUN(test_sdr_ch_update_lock_api);
    
    return 0;
}
