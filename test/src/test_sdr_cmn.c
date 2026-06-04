//
//  Unit tests for sdr_cmn.c.
//
#include "test_sdr.h"

typedef struct {
    sdr_mutex_t mtx;
    int value;
} thread_arg_t;

// thread entry for sdr_thread_create() ----------------------------------------
static void *thread_func(void *arg)
{
    thread_arg_t *ctx = (thread_arg_t *)arg;
    
    sdr_mutex_lock(&ctx->mtx);
    ctx->value += 1;
    sdr_mutex_unlock(&ctx->mtx);
    return NULL;
}

// sdr_malloc(), sdr_free() ----------------------------------------------------
static void test_sdr_malloc_free(void)
{
    uint8_t *p = (uint8_t *)sdr_malloc(32);
    
    TEST_ASSERT_TRUE(p != NULL);
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQ_INT(0, p[i]); // sdr_malloc() zero-clears memory.
    }
    memset(p, 0xA5, 32);
    sdr_free(p);
    
    // Edge case: free(NULL) must be accepted by the C runtime via sdr_free().
    sdr_free(NULL);
}

// sdr_get_time() --------------------------------------------------------------
static void test_sdr_get_time(void)
{
    double t[6] = {0};
    
    sdr_get_time(t);
    TEST_ASSERT_TRUE(t[0] >= 2020.0 && t[0] <= 2100.0);
    TEST_ASSERT_TRUE(t[1] >= 1.0 && t[1] <= 12.0);
    TEST_ASSERT_TRUE(t[2] >= 1.0 && t[2] <= 31.0);
    TEST_ASSERT_TRUE(t[3] >= 0.0 && t[3] < 24.0);
    TEST_ASSERT_TRUE(t[4] >= 0.0 && t[4] < 60.0);
    TEST_ASSERT_TRUE(t[5] >= 0.0 && t[5] < 61.0);
}

// sdr_get_tick(), sdr_sleep_msec() -------------------------------------------
static void test_sdr_tick_sleep(void)
{
    uint32_t t0, t1;
    
    t0 = sdr_get_tick();
    sdr_sleep_msec(0);  // Edge case: no-op.
    sdr_sleep_msec(-1); // Edge case: no-op.
    t1 = sdr_get_tick();
    TEST_ASSERT_TRUE((uint32_t)(t1 - t0) < 1000u);
    
    t0 = sdr_get_tick();
    sdr_sleep_msec(5);
    t1 = sdr_get_tick();
    TEST_ASSERT_TRUE((uint32_t)(t1 - t0) < 1000u);
}

// sdr_get_name(), sdr_get_ver() ----------------------------------------------
static void test_sdr_name_ver(void)
{
    TEST_ASSERT_TRUE(sdr_get_name() != NULL);
    TEST_ASSERT_TRUE(sdr_get_ver() != NULL);
    TEST_ASSERT_TRUE(!strcmp(SDR_LIB_NAME, sdr_get_name()));
    TEST_ASSERT_TRUE(!strcmp(SDR_LIB_VER, sdr_get_ver()));
}

// sdr_mutex_init(), sdr_mutex_lock(), sdr_mutex_unlock() ----------------------
static void test_sdr_mutex(void)
{
    sdr_mutex_t mtx;
    int value = 0;
    
    sdr_mutex_init(&mtx);
    sdr_mutex_lock(&mtx);
    value++;
    sdr_mutex_unlock(&mtx);
    TEST_ASSERT_EQ_INT(1, value);
    
#ifdef WIN32
    // Edge case supported by the Windows implementation.
    sdr_mutex_init(NULL);
    sdr_mutex_lock(NULL);
    sdr_mutex_unlock(NULL);
#endif
}

// sdr_thread_create(), sdr_thread_join() -------------------------------------
static void test_sdr_thread(void)
{
    sdr_thread_t thread;
    thread_arg_t arg;
    
    memset(&arg, 0, sizeof(arg));
    sdr_mutex_init(&arg.mtx);
    TEST_ASSERT_EQ_INT(1, sdr_thread_create(&thread, thread_func, &arg));
    sdr_thread_join(thread);
    TEST_ASSERT_EQ_INT(1, arg.value);
    
#ifdef WIN32
    // Edge case supported by the Windows implementation.
    sdr_thread_join((sdr_thread_t)0);
#endif
}

// main ------------------------------------------------------------------------
int main(void)
{
    TEST_RUN(test_sdr_malloc_free);
    TEST_RUN(test_sdr_get_time);
    TEST_RUN(test_sdr_tick_sleep);
    TEST_RUN(test_sdr_name_ver);
    TEST_RUN(test_sdr_mutex);
    TEST_RUN(test_sdr_thread);
    return 0;
}
