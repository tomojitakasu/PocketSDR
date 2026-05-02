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
    printf("Usage: pocket_calib [-ts time] [-te time] [-f freq] -g file -n file [-o file]\n");
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

// read geometry file ----------------------------------------------------------
static int read_geom(const char *file, double *ant_pos)
{
    FILE *fp;
    char buff[256], *p;
    double pos[3];
    int nant = 0;
    
    if (!(fp = fopen(file, "r"))) return 0;
    while (fgets(buff, sizeof(buff), fp) && nant < SDR_MAX_RFCH) {
        if ((p = strchr(buff, '#'))) *p = '\0';
        if (sscanf(buff, "%lf %lf %lf", pos, pos + 1, pos + 2) == 3) {
            matcpy(ant_pos + nant * 3, pos, 3, 1);
            nant++;
        }
    }
    fclose(fp);
    return nant;
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

// write output -----------------------------------------------------------------
static void write_result(FILE *fp, const char *geom_file, const char *nav_file,
    const char *obs_files[], int nant, int freq, const double *rr, int nep,
    const double *bias, const double *rpy, double rms)
{
    double pos[3];
    ecef2pos(rr, pos);
    
    fprintf(fp, "# ARRAY CALIBRATION by %s\n", PROG_NAME);
    fprintf(fp, "# GEOMETRY FILE   : %s\n", geom_file);
    for (int i = 0; i < nant; i++) {
        fprintf(fp, "# OBS FILE CH%d    : %s\n", i + 1, obs_files[i]);
    }
    fprintf(fp, "# NAV FILE        : %s\n", nav_file);
    fprintf(fp, "# POSTION LLH     : %.9f %.9f %.3f\n", pos[0] * R2D,
        pos[1] * R2D, pos[2]);
    fprintf(fp, "# FREQ INDEX      : %d\n", freq);
    fprintf(fp, "# NUM EPOCHS      : %d\n", nep);
    fprintf(fp, "# RESIDUALS RMS   : %9.4f m\n", rms);
    fprintf(fp, "# ATTITUDE ROLL   : %9.4f deg\n", rpy[0] * R2D);
    fprintf(fp, "# ATTITUDE PITCH  : %9.4f deg\n", rpy[1] * R2D);
    fprintf(fp, "# ATTITUDE YAW    : %9.4f deg (CCW+ from N; compass = -yaw)\n",
        rpy[2] * R2D);
    fprintf(fp, "# H/W DELAY       :\n");
    fprintf(fp, "#  %-4s %12s %12s\n", "CH", "DELAY(ns)", "DELAY(m)");
    for (int i = 0; i < nant; i++) {
        fprintf(fp, "   CH%-2d %12.4f %12.4f\n", i + 1, bias[i] / CLIGHT * 1e9,
            bias[i]);
    }
}

//-----------------------------------------------------------------------------
//
//   Synopsis
//
//     pocket_calib [-ts time] [-te time] [-f freq] -g file -n file [-o file]
//         obs1 obs2 [obs3 ...]
//
//   Description
//
//     Estimates antenna array hardware delays (CH1 reference) and attitude
//     (roll/pitch/yaw) from per-element RINEX OBS files and a RINEX NAV file
//     using array_calib() (see sdr_array.c). Results are written to a text
//     file (stdout by default).
//
//   Options ([]: default)
//
//     -ts time, -te time
//         Start/end time to use (GPST, YYYY/MM/DD HH:MM:SS). [all]
//
//     -f freq
//         frequency index (0:L1,1:L2, ...) [0]
//
//     -g file
//         Array geometry file: one "x y z" per line in body frame (m). The
//         body frame is X=right, Y=forward, Z=up. The i-th line is the
//         position of element CH(i). First line is CH1 (reference).
//
//     -n file
//         RINEX navigation data file.
//
//     -o file
//         Output text file. [stdout]
//
//     obs1 obs2 [obs3 ...]
//         Per-element RINEX OBS files. The k-th file is element CH(k).
//
int main(int argc, char **argv)
{
    nav_t nav = {};
    obs_t obs = {};
    FILE *fp = stdout;
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
    if (!*geom_file || !*nav_file || nobs < 2) {
        show_usage();
    }
    // read geometry file
    if ((nant = read_geom(geom_file, ant_pos)) != nobs) {
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

    double bias[SDR_MAX_RFCH] = {0}, rpy[3] = {0}, rms = 0.0;
    int n_ok = 0;
    for (int k = 0; k < nep; k++) {
        sdr_array_calib(array, obs_ep[k], nobs_ep[k], &nav, rr);
    }
    sdr_array_stat(array, rpy, bias, &rms, &n_ok);

    sdr_array_free(array);
    free(obs_ep); free(nobs_ep);
    freeobs(&obs); freenav(&nav, NAV_MASK);

    if (n_ok <= 0) {
        fprintf(stderr, "array_calib() error\n");
        return -1;
    }
    printf("TIME = %.3f s\n", (sdr_get_tick() - tick) * 1e-3);

    // write output
    if (*out_file && !(fp = fopen(out_file, "w"))) {
        fprintf(stderr, "output file open error %s\n", out_file);
        return -1;
    }
    write_result(fp, geom_file, nav_file, obs_files, nant, freq, rr, n_ok, bias,
        rpy, rms);
    
    if (fp != stdout) fclose(fp);
    
    return 0;
}
