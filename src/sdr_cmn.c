//
//  Pocket SDR C library - SDR Common Functions.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2021-10-03  0.1  new
//  2022-01-04  1.0  support Windows API.
//  2022-05-17  1.1  add API sdr_cpx_malloc(), sdr_cpx_free()
//
#ifdef WIN32
#include <windows.h>
#endif
#include "pocket_sdr.h"

//------------------------------------------------------------------------------
//  Allocate memory. If no memory allocated, it exits the AP immediately with
//  an error message.
//  
//  args:
//      size     (I)  memory size (bytes)
//
//  return:
//      memory pointer allocated.
//
void *sdr_malloc(size_t size)
{
    void *p;
    
    if (!(p = calloc(size, 1))) {
        fprintf(stderr, "memory allocation error size=%d\n", (int)size);
        exit(-1);
    }
    return p;
}

//------------------------------------------------------------------------------
//  Free memory allocated by sdr_malloc()
//  
//  args:
//      p        (I)  memory pointer allocated.
//
//  return:
//      none
//
void sdr_free(void *p)
{
    free(p);
}

//------------------------------------------------------------------------------
//  Allocate memory for complex array. If no memory allocated, it exits the AP
//  immediately with an error message.
//  
//  args:
//      N        (I)  size of complex array
//
//  return:
//      complex array allocated.
//
sdr_cpx_t *sdr_cpx_malloc(int N)
{
    sdr_cpx_t *cpx;
    
    if (!(cpx = (sdr_cpx_t *)fftwf_malloc(sizeof(sdr_cpx_t) * N))) {
        fprintf(stderr, "sdr_cpx_t memory allocation error N=%d\n", N);
        exit(-1);
    }
    return cpx;
}

//------------------------------------------------------------------------------
//  Free memory allocated by sdr_cpx_malloc().
//  
//  args:
//      cpx      (I)  complex array
//
//  return:
//      none
//
void sdr_cpx_free(sdr_cpx_t *cpx)
{
    fftwf_free(cpx);
}

//------------------------------------------------------------------------------
//  Get system tick (msec).
//  
//  args:
//      none
//
//  return:
//      system tick (ms)
//
uint32_t sdr_get_tick(void)
{
#ifdef WIN32
    return (uint32_t)timeGetTime();
#else
    struct timeval tv = {0};
    
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000u + tv.tv_usec / 1000u;
#endif
}

//------------------------------------------------------------------------------
//  Sleep for milli-seconds.
//  
//  args:
//      msec     (I)  mill-seconds to sleep
//
//  return:
//      none
//
void sdr_sleep_msec(int msec)
{
#ifdef WIN32
    Sleep(msec < 5 ? 1 : msec);
#else
    struct timespec ts = {0};
    
    if (msec <= 0) return;
    ts.tv_nsec = (long)(msec * 1000000);
    nanosleep(&ts, NULL);
#endif
}

