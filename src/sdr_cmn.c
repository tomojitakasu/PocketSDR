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
//  2022-07-08  1.2  add API sdr_get_time()
//                   move API sdr_cpx_malloc(), sdr_cpx_free() to sdr_func.c
//
#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
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
//  Get current time in UTC.
//  
//  args:
//      t        (O)  Current time as {year, month, day, hour, minute, second}
//
//  return:
//      none
//
void sdr_get_time(double *t)
{
#ifdef WIN32
    SYSTEMTIME ts;
    
    GetSystemTime(&ts); /* in UTC */
    t[0] = ts.wYear;
    t[1] = ts.wMonth;
    t[2] = ts.wDay;
    t[3] = ts.wHour;
    t[4] = ts.wMinute;
    t[5] = ts.wSecond + ts.wMilliseconds * 1E-3;
#else
    struct timeval tv;
    struct tm *tt;
    
    if (!gettimeofday(&tv, NULL) && (tt = gmtime(&tv.tv_sec))) {
        t[0] = tt->tm_year + 1900;
        t[1] = tt->tm_mon + 1;
        t[2] = tt->tm_mday;
        t[3] = tt->tm_hour;
        t[4] = tt->tm_min;
        t[5] = tt->tm_sec + tv.tv_usec * 1E-6;
    }
#endif
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

