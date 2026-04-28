/*
*  Wrapper functions of RTKLIB for librtk.so
*
*  References:
*  [1] RTKLIB: An Open Source Program Package for GNSS Positioning
*      (https://github.com/tomojitakasu/RTKLIB)
*
*  Author:
*  T.TAKASU
*
*  History:
*  2021-12-24  1.0  new
*  2022-02-08  1.1  add get_const_int(), navgettgd()
*/
#include "rtklib.h"

/* get constant int ----------------------------------------------------------*/
int get_const_int(const char *name)
{
    if (!strcmp(name, "MAXSAT")) {
        return MAXSAT;
    }
    else if (!strcmp(name, "MAXSTA")) {
        return MAXSTA;
    }
    else if (!strcmp(name, "MAXANT")) {
        return MAXANT;
    }
    else if (!strcmp(name, "MAXOBS")) {
        return MAXOBS;
    }
    else if (!strcmp(name, "NFREQ")) {
        return NFREQ;
    }
    else if (!strcmp(name, "NEXOBS")) {
        return NEXOBS;
    }
    else if (!strcmp(name, "NSYS")) {
        return NSYS;
    }
    else if (!strcmp(name, "SNR_UNIT")) {
        return SNR_UNIT;
    }
    return 0;
}

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

/* iono model with navigation d ---------------------------------------------*/
double ionmodel_nav(gtime_t time, nav_t *nav, const double *pos,
    const double *azel)
{
    return ionmodel(time, nav->ion_gps, pos, azel);
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

/* get TGD -------------------------------------------------------------------*/
double navgettgd(int sat, const nav_t *nav)
{
    int i;
    
    if (nav) {
        for (i = 0; i < nav->n; i++) {
            if (nav->eph[i].sat == sat) return CLIGHT * nav->eph[i].tgd[0];
        }
    }
    return 0.0;
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
