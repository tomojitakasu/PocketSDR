//
//  Minimal receiver wrapper stubs for isolated unit tests.
//
#include "test_sdr.h"

void sdr_rcv_array_calib(sdr_rcv_t *rcv, const obsd_t *obs, int nobs,
    const nav_t *nav, const double *rr)
{
    (void)rcv;
    (void)obs;
    (void)nobs;
    (void)nav;
    (void)rr;
}
