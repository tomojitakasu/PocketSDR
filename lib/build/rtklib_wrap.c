/*
* Wrapper functions of RTKLIB for librtk.so
*/
#include "rtklib.h"

/* new observation data ------------------------------------------------------*/
obs_t *obsnew(void)
{
    obs_t *obs;
    
    if (!(obs = (obs_t *)calloc(1, sizeof(obs_t)))) {
        return NULL;
    }
    return obs;
}

/* free observation data -----------------------------------------------------*/
void obsfree(obs_t *obs)
{
    if (!obs) return;
    freeobs(obs);
    free(obs);
}

/* get observation data ------------------------------------------------------*/
obsd_t *obsget(obs_t *obs, int idx)
{
    if (!obs || idx < 0 || idx >= obs->n) return NULL;
    return obs->data + idx;
}

/* new navigation data -------------------------------------------------------*/
nav_t *navnew(void)
{
    nav_t *nav;
    
    if (!(nav = (nav_t *)calloc(1, sizeof(nav_t)))) {
        return NULL;
    }
    return nav;
}

/* free navigation data ------------------------------------------------------*/
void navfree(nav_t *nav)
{
    if (!nav) return;
    freenav(nav, 0xFF);
    free(nav);
}

/* get ephemeris -------------------------------------------------------------*/
eph_t *navgeteph(nav_t *nav, int idx)
{
    if (!nav || idx < 0 || idx >= nav->n) return NULL;
    return nav->eph + idx;
}

/* get GLONASS ephemeris -----------------------------------------------------*/
geph_t *navgetgeph(nav_t *nav, int idx)
{
    if (!nav || idx < 0 || idx >= nav->ng) return NULL;
    return nav->geph + idx;
}

/* new station parameter -----------------------------------------------------*/
sta_t *stanew(void)
{
    sta_t *sta;
    
    if (!(sta = (sta_t *)calloc(1, sizeof(sta_t)))) {
        return NULL;
    }
    return sta;
}

/* new solution --------------------------------------------------------------*/
sol_t *solnew(void)
{
    sol_t *sol;
    
    if (!(sol = (sol_t *)calloc(1, sizeof(sol_t)))) {
        return NULL;
    }
    return sol;
}

/* free solution -------------------------------------------------------------*/
void solfree(sol_t *sol)
{
    if (!sol) return;
    free(sol);
}

/* new stream ----------------------------------------------------------------*/
stream_t *strnew(void)
{
    stream_t *str;
    
    if (!(str = (stream_t *)calloc(1, sizeof(stream_t)))) {
        return NULL;
    }
    strinit(str);
    return str;
}

/* free stream ---------------------------------------------------------------*/
void strfree(stream_t *str)
{
    if (!str) return;
    free(str);
}

/* dummy user functions ------------------------------------------------------*/
int showmsg(const char *msg, ...)
{
    return 0;
}

void settspan(gtime_t ts, gtime_t te)
{
}

void settime(gtime_t time)
{
}
