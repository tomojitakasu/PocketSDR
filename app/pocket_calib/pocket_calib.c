//
//  Pocket SDR C AP - Antenna Array Live Calibration.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2026-04-17  1.0  new
//
#include "pocket_sdr.h"

#define PROG_NAME   "pocket_calib"
#define MAX_EP      3600            // max number of epochs
#define NAV_MASK    0xFF            // navigation data mask

// show version -----------------------------------------------------------------
static void print_ver(void)
{
    printf("%s ver.%s\n", PROG_NAME, sdr_get_ver());
    exit(0);
}

// show usage -------------------------------------------------------------------
static void show_usage(void)
{
    printf("Usage: pocket_calib [-ts time] [-te time] [-f freq] -g file -n file -o file\n");
    printf("       obs1 obs2 [obs3 ...]\n");
    exit(0);
}

// parse time (YYYY/MM/DD HH:MM:SS) --------------------------------------------
static gtime_t parse_time(const char *str)
{
    double ep[6] = {0};
    char buff[64];
    
    snprintf(buff, sizeof(buff), "%.63s", str);
    for (char *p = buff; *p; p++) {
        if (strchr("/:_-", *p)) *p = ' ';
    }
    sscanf(buff, "%lf%lf%lf%lf%lf%lf", ep, ep+1, ep+2, ep+3, ep+4, ep+5);
    return epoch2time(ep);
}

// group obs data by epoch ------------------------------------------------------
static int group_epochs(obs_t *obs, obsd_t **obs_ep, int *nobs)
{
    int n = 0;
    
    for (int i = 0, j = 0; i < obs->n && n < MAX_EP; i = j) {
        gtime_t ti = obs->data[i].time; 
        while (j < obs->n && fabs(timediff(obs->data[j].time, ti)) < 1e-3) {
            j++;
        }
        obs_ep[n] = obs->data + i;
        nobs[n++] = j - i;
    }
    if (n >= MAX_EP) {
        fprintf(stderr, "epoch limit exceeded (MAX_EP=%d)\n", MAX_EP);
    }
    return n;
}

// estimate receiver position --------------------------------------------------
static int est_pos(const obs_t *obs, const nav_t *nav, double *rr)
{
    prcopt_t opt = prcopt_default;
    opt.navsys = SYS_ALL;
    obsd_t buf[MAXOBS];
    sol_t sol;
    double azel[2*MAXSAT];
    char msg[128];
    
    for (int i = 0, j = 0; i < obs->n; i = j) {
        gtime_t ti = obs->data[i].time;
        int n = 0;
        while (j < obs->n && fabs(timediff(obs->data[j].time, ti)) < 1e-3) {
            if (obs->data[j].rcv == 1 && n < MAXOBS) {
                buf[n++] = obs->data[j];
            }
            j++;
        }
        if (pntpos(buf, n, nav, &opt, &sol, azel, NULL, msg)) {
            matcpy(rr, sol.rr, 3, 1);
            return 1;
        }
    }
    return 0;
}

// main (see doc/command_ref.md) -----------------------------------------------
int main(int argc, char **argv)
{
    nav_t nav = {};
    obs_t obs = {};
    gtime_t ts = {0, 0}, te = {0, 0};
    const char *geom_file = "", *nav_file = "", *out_file = "";
    const char *obs_files[SDR_MAX_RFCH];
    double ant_pos[SDR_MAX_RFCH*3] = {0}, rr[3] = {0};
    int nant = 0, nobs = 0, freq = 0;
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-ts") && i + 1 < argc) {
            ts = parse_time(argv[++i]);
        } else if (!strcmp(argv[i], "-te") && i + 1 < argc) {
            te = parse_time(argv[++i]);
        } else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            freq = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-g") && i + 1 < argc) {
            geom_file = argv[++i];
        } else if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            nav_file = argv[++i];
        } else if (!strcmp(argv[i], "-o") && i + 1 < argc) {
            out_file = argv[++i];
        } else if (!strcmp(argv[i], "-v")) {
            print_ver();
        } else if (argv[i][0] == '-') {
            show_usage();
        } else if (nobs < SDR_MAX_RFCH) {
            obs_files[nobs++] = argv[i];
        }
    }
    if (!*geom_file || !*nav_file || !*out_file || nobs < 2) {
        show_usage();
    }
    // read geometry file
    if ((nant = sdr_array_geom_load(geom_file, ant_pos, SDR_MAX_RFCH)) != nobs) {
        fprintf(stderr, "number of antenna elements error %d %d\n", nant, nobs);
        return -1;
    }
    // read RINEX obs and nav
    if (!readrnxt(nav_file, 0, ts, te, 0.0, "", NULL, &nav, NULL)) {
        fprintf(stderr, "nav data read error %s\n", nav_file);
        return -1;
    }
    for (int i = 0; i < nobs; i++) {
        if (!readrnxt(obs_files[i], i + 1, ts, te, 0.0, "", &obs, NULL, NULL)) {
            fprintf(stderr, "obs data read error %s\n", obs_files[i]);
            freenav(&nav, NAV_MASK);
            return -1;
        }
    }
    sortobs(&obs);
    
    uint32_t tick = sdr_get_tick();
    
    // estimate receiver position
    if (!est_pos(&obs, &nav, rr)) {
        fprintf(stderr, "receiver position not available\n");
        freeobs(&obs);
        freenav(&nav, NAV_MASK);
        return -1;
    }
    // group by epochs
    obsd_t **obs_ep = (obsd_t **)malloc(sizeof(obsd_t *) * MAX_EP);
    int *nobs_ep = (int *)malloc(sizeof(int) * MAX_EP);
    if (!obs_ep || !nobs_ep) {
        fprintf(stderr, "malloc error\n");
        freeobs(&obs);
        freenav(&nav, NAV_MASK);
        return -1;
    }
    int nep = group_epochs(&obs, obs_ep, nobs_ep);

    // per-epoch EKF calibration
    int ant_ena[SDR_MAX_RFCH] = {0};
    for (int i = 0; i < nant; i++) ant_ena[i] = 1;
    sdr_array_t *array = sdr_array_new(nant, freq);
    sdr_array_ant_pos(array, ant_pos, ant_ena);
    sdr_array_run(array, 1);

    for (int k = 0; k < nep; k++) {
        sdr_array_calib(array, obs_ep[k], nobs_ep[k], &nav, rr);
    }
    double rpy_chk[3], bias_chk[SDR_MAX_RFCH], rms_chk;
    int n_ok = 0;
    sdr_array_stat(array, rpy_chk, bias_chk, &rms_chk, &n_ok);

    if (n_ok <= 0) {
        fprintf(stderr, "array_calib() error\n");
        sdr_array_free(array);
        free(obs_ep); free(nobs_ep);
        freeobs(&obs); freenav(&nav, NAV_MASK);
        return -1;
    }
    printf("TIME = %.3f s\n", (sdr_get_tick() - tick) * 1e-3);

    if (!sdr_array_save(array, out_file)) {
        fprintf(stderr, "output file write error %s\n", out_file);
    }
    sdr_array_free(array);
    free(obs_ep); free(nobs_ep);
    freeobs(&obs); freenav(&nav, NAV_MASK);

    return 0;
}
