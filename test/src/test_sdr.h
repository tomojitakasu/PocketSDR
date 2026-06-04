//
//  Lightweight unit-test helpers for Pocket SDR C modules.
//
#ifndef TEST_SDR_H
#define TEST_SDR_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pocket_sdr.h"

#define TEST_ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

#define TEST_ASSERT_EQ_INT(exp, act) do { \
    int test_exp_ = (int)(exp); \
    int test_act_ = (int)(act); \
    if (test_exp_ != test_act_) { \
        fprintf(stderr, "%s:%d: expected %d, got %d\n", __FILE__, __LINE__, \
            test_exp_, test_act_); \
        exit(1); \
    } \
} while (0)

#define TEST_ASSERT_NEAR(exp, act, tol) do { \
    double test_exp_ = (double)(exp); \
    double test_act_ = (double)(act); \
    double test_tol_ = (double)(tol); \
    if (fabs(test_exp_ - test_act_) > test_tol_) { \
        fprintf(stderr, "%s:%d: expected %.12g, got %.12g, tol %.12g\n", \
            __FILE__, __LINE__, test_exp_, test_act_, test_tol_); \
        exit(1); \
    } \
} while (0)

#define TEST_RUN(fn) do { \
    printf("%s ... ", #fn); \
    fflush(stdout); \
    fn(); \
    printf("OK\n"); \
} while (0)

#define TEST_MAIN(fn) \
int main(void) \
{ \
    TEST_RUN(fn); \
    return 0; \
}

#endif
