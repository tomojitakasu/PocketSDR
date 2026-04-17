//
//  Pocket SDR C AP - Antenna Array Live Calibration.
//
//  Reads a per-element RINEX OBS file and a RINEX NAV file, then calls
//  array_calib() to estimate the per-element hardware delay (CH1 reference)
//  and the array attitude. Results are written to a text file.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2026-04-17  1.0  new
//
#include "pocket_sdr.h"

// constants -------------------------------------------------------------------
#define PROG_NAME   "pocket_calib"
#define MAX_EP      86400           // max number of epochs
#define EPS_TIME    1e-3            // epoch time tolerance (s)

// show version -----------------------------------------------------------------
static void print_ver(void)
{
    printf("%s ver.%s\n", PROG_NAME, sdr_get_ver());
    exit(0);
}

// show usage -------------------------------------------------------------------
static void show_usage(void)
{
    printf("Usage: pocket_calib [-ts time] [-te time] [-f freq] [-pos x,y,z]\n");
    printf("       [-log file] [-level lvl]\n");
    printf("       -geom file -nav file [-out file] obs1 obs2 [obs3 ...]\n");
    exit(0);
}

// parse time (YYYY/MM/DD HH:MM:SS) --------------------------------------------
static gtime_t parse_time(const char *str)
{
    double ep[6] = {0};
    char buff[64] = "";
    for (char *s = buff; s - buff < 63; s++, str++) {
        if (!(*s = strchr("/:_-", *str) ? ' ' : *str)) break;
    }
    sscanf(buff, "%lf %lf %lf %lf %lf %lf", ep, ep+1, ep+2, ep+3, ep+4, ep+5);
    return utc2gpst(epoch2time(ep));
}

// convert file path for Windows -----------------------------------------------
static const char *conv_path(const char *path)
{
#ifdef WIN32
    static char buff[1024] = "";
    const char *p = path;
    for (char *q = buff; q - buff < 1023; p++, q++) {
        if (!(*q = (*p == '/') ? '\\' : *p)) break;
    }
    return buff;
#else
    return path;
#endif
}

// read geometry file ----------------------------------------------------------
//   one line per element: "x y z" (meters in body frame). Returns narr, <=0 on
//   failure.
static int read_geom(const char *file, double *ant_pos, int max_narr)
{
    FILE *fp = fopen(conv_path(file), "r");
    if (!fp) return 0;
    
    char line[256];
    int narr = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '%' || *p == '\0' || *p == '\n' || *p == '\r') {
            continue;
        }
        if (narr >= max_narr) break;
        double x = 0.0, y = 0.0, z = 0.0;
        if (sscanf(p, "%lf %lf %lf", &x, &y, &z) == 3 ||
            sscanf(p, "%lf,%lf,%lf", &x, &y, &z) == 3) {
            ant_pos[narr*3+0] = x;
            ant_pos[narr*3+1] = y;
            ant_pos[narr*3+2] = z;
            narr++;
        }
    }
    fclose(fp);
    return narr;
}

// compare two epoch times ------------------------------------------------------
static int same_epoch(gtime_t t1, gtime_t t2)
{
    return fabs(timediff(t1, t2)) < EPS_TIME;
}

// group obs data by epoch ------------------------------------------------------
//   Populates obs_ep[k] = pointer to k-th epoch's obsd_t array, nobs[k] = count.
//   obs->data must be sorted by time.
static int group_epochs(obs_t *obs, obsd_t **obs_ep, int *nobs, int max_ep)
{
    int nep = 0, i = 0;
    while (i < obs->n && nep < max_ep) {
        int j = i;
        while (j < obs->n && same_epoch(obs->data[j].time, obs->data[i].time)) {
            j++;
        }
        obs_ep[nep] = obs->data + i;
        nobs[nep] = j - i;
        nep++;
        i = j;
    }
    return nep;
}

// estimate approximate receiver position by single-point solution ------------
//   Scans epochs (CH1 only) until pntpos() succeeds.
static int est_rr(const obs_t *obs, const nav_t *nav, double *rr)
{
    prcopt_t opt = prcopt_default;
    opt.mode = PMODE_SINGLE;
    opt.navsys = SYS_ALL;
    opt.sateph = EPHOPT_BRDC;

    for (int i = 0; i < obs->n; ) {
        gtime_t t0 = obs->data[i].time;
        obsd_t buf[MAXOBS];
        int n = 0, n_ch1 = 0;
        while (i + n < obs->n && same_epoch(obs->data[i+n].time, t0)) {
            if (obs->data[i+n].rcv == 1 && n_ch1 < MAXOBS) {
                buf[n_ch1++] = obs->data[i+n];
            }
            n++;
        }
        i += n;
        if (n_ch1 < 4) continue;

        sol_t sol;
        memset(&sol, 0, sizeof(sol));
        double azel[2*MAXSAT] = {0};
        char msg[128] = "";
        if (pntpos(buf, n_ch1, nav, &opt, &sol, azel, NULL, msg)) {
            rr[0] = sol.rr[0];
            rr[1] = sol.rr[1];
            rr[2] = sol.rr[2];
            return 1;
        }
    }
    return 0;
}

// write output -----------------------------------------------------------------
static void write_result(FILE *fp, const char *gfile, const char *nfile,
    char * const *ofiles, int narr, double freq, const double *rr, int nep,
    int nsd, const double *bias, const double *rpy, double cp_rms)
{
    char tstr[64];
    time_t now = time(NULL);
    strftime(tstr, sizeof(tstr), "%Y/%m/%d %H:%M:%S %Z", localtime(&now));
    
    double pos[3];
    ecef2pos(rr, pos);
    
    fprintf(fp, "# ARRAY CALIBRATION by POCKET_CALIB\n");
    fprintf(fp, "# Date/Time       : %s\n", tstr);
    fprintf(fp, "# Geometry file   : %s\n", gfile);
    fprintf(fp, "# Nav file        : %s\n", nfile);
    for (int i = 0; i < narr; i++) {
        fprintf(fp, "# Obs files   CH%d : %s\n", i + 1, ofiles[i]);
    }
    fprintf(fp, "# Calib freq (MHz): %.3f\n", freq / 1e6);
    fprintf(fp, "# Receiver LLH    : %.9f %.9f %.3f\n",
        pos[0] * R2D, pos[1] * R2D, pos[2]);
    fprintf(fp, "# Num epochs      : %d\n", nep);
    fprintf(fp, "# Num SD meas     : %d\n", nsd);
    fprintf(fp, "# SD CP RMS (m)   : %.4f\n", cp_rms);
    fprintf(fp, "#\n");
    fprintf(fp, "# Attitude [deg]  : roll=%.3f pitch=%.3f yaw=%.3f\n",
        rpy[0] * R2D, rpy[1] * R2D, rpy[2] * R2D);
    fprintf(fp, "#\n");
    fprintf(fp, "# H/W delay (CH1 reference):\n");
    fprintf(fp, "#   %-4s %12s %12s %12s\n", "CH", "delay(ns)", "delay(m)",
        "delay(cyc)");
    double lam = CLIGHT / freq;
    for (int i = 0; i < narr; i++) {
        fprintf(fp, "   CH%-2d %12.4f %12.4f %12.4f\n", i + 1,
            bias[i] * 1e9, bias[i] * CLIGHT, bias[i] * CLIGHT / lam);
    }
    fflush(fp);
}

//-----------------------------------------------------------------------------
//
//   Synopsis
//
//     pocket_calib [-ts time] [-te time] [-f freq] [-pos x,y,z]
//         -geom file -nav file [-out file] obs1 obs2 [obs3 ...]
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
//         Start/end time to use (UTC, YYYY/MM/DD HH:MM:SS). [all]
//
//     -f freq
//         Calibration carrier frequency in MHz. [1575.42 (L1)]
//
//     -pos x,y,z
//         Approximate receiver position in ECEF (m). [from obs1 RINEX header
//         or single-point solution]
//
//     -geom file
//         Array geometry file: one "x y z" per line in body frame (m). The
//         i-th line is the position of element CH(i). First line is CH1
//         (reference).
//
//     -nav file
//         RINEX navigation data file.
//
//     -out file
//         Output text file. [stdout]
//
//     obs1 obs2 [obs3 ...]
//         Per-element RINEX OBS files. The k-th file is element CH(k).
//
int main(int argc, char **argv)
{
    gtime_t ts = {0, 0}, te = {0, 0};
    double freq = FREQ1, rr[3] = {0};
    double ant_pos[SDR_MAX_RFCH*3] = {0}, bias[SDR_MAX_RFCH] = {0}, rpy[3] = {0};
    char * const *ofiles_arg;
    const char *gfile = "", *nfile = "", *ofile = "", *lfile = "";
    int has_pos = 0, log_level = 3;

    char *obs_files[SDR_MAX_RFCH] = {NULL};
    int narr_obs = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-ts") && i + 1 < argc) {
            ts = parse_time(argv[++i]);
        }
        else if (!strcmp(argv[i], "-te") && i + 1 < argc) {
            te = parse_time(argv[++i]);
        }
        else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            freq = atof(argv[++i]) * 1e6;
        }
        else if (!strcmp(argv[i], "-pos") && i + 1 < argc) {
            if (sscanf(argv[++i], "%lf,%lf,%lf", rr, rr+1, rr+2) == 3) {
                has_pos = 1;
            }
        }
        else if (!strcmp(argv[i], "-geom") && i + 1 < argc) {
            gfile = argv[++i];
        }
        else if (!strcmp(argv[i], "-nav") && i + 1 < argc) {
            nfile = argv[++i];
        }
        else if (!strcmp(argv[i], "-out") && i + 1 < argc) {
            ofile = argv[++i];
        }
        else if (!strcmp(argv[i], "-log") && i + 1 < argc) {
            lfile = argv[++i];
        }
        else if (!strcmp(argv[i], "-level") && i + 1 < argc) {
            log_level = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-v")) {
            print_ver();
        }
        else if (argv[i][0] == '-') {
            show_usage();
        }
        else if (narr_obs < SDR_MAX_RFCH) {
            obs_files[narr_obs++] = argv[i];
        }
    }
    sdr_log_level(log_level);
    if (*lfile && !sdr_log_open(lfile)) {
        fprintf(stderr, "log file open error %s\n", lfile);
        return -1;
    }
    ofiles_arg = obs_files;
    
    if (!*gfile || !*nfile || narr_obs < 2) {
        show_usage();
    }
    sdr_log(3, "$LOG,CALIB_START,freq=%.3fMHz,narr=%d", freq / 1e6, narr_obs);
    int narr = read_geom(gfile, ant_pos, SDR_MAX_RFCH);
    if (narr < 2) {
        sdr_log(2, "$LOG,ERROR,geom file read error or too few elements: %s",
            gfile);
        fprintf(stderr, "geometry file read error or too few elements: %s\n",
            gfile);
        return -1;
    }
    if (narr != narr_obs) {
        sdr_log(2, "$LOG,ERROR,element count mismatch: geom=%d obs=%d", narr,
            narr_obs);
        fprintf(stderr, "element count mismatch: geom=%d, obs=%d\n", narr,
            narr_obs);
        return -1;
    }
    sdr_log(4, "$LOG,GEOM,file=%s,narr=%d", gfile, narr);
    for (int i = 0; i < narr; i++) {
        sdr_log(4, "$LOG,GEOM_CH%d,%.4f,%.4f,%.4f", i + 1,
            ant_pos[i*3], ant_pos[i*3+1], ant_pos[i*3+2]);
    }
    // read RINEX NAV
    nav_t nav;
    sta_t sta;
    memset(&nav, 0, sizeof(nav));
    memset(&sta, 0, sizeof(sta));
    if (readrnxt(conv_path(nfile), 0, ts, te, 0.0, "", NULL, &nav, NULL) <= 0
        || (nav.n <= 0 && nav.ng <= 0)) {
        sdr_log(2, "$LOG,ERROR,nav data read error or empty %s", nfile);
        fprintf(stderr, "nav data read error or empty %s\n", nfile);
        return -1;
    }
    sdr_log(4, "$LOG,NAV,file=%s,n=%d,ng=%d", nfile, nav.n, nav.ng);
    // read per-element RINEX OBS (rcv = 1..narr)
    obs_t obs;
    memset(&obs, 0, sizeof(obs));
    for (int i = 0; i < narr; i++) {
        int n0 = obs.n;
        int r = readrnxt(conv_path(obs_files[i]), i + 1, ts, te, 0.0, "", &obs,
            NULL, i == 0 ? &sta : NULL);
        if (r <= 0 || obs.n <= n0) {
            sdr_log(2, "$LOG,ERROR,obs data read error or empty %s",
                obs_files[i]);
            fprintf(stderr, "obs data read error or empty %s\n", obs_files[i]);
            freeobs(&obs); freenav(&nav, 0xFF);
            return -1;
        }
        sdr_log(4, "$LOG,OBS_CH%d,file=%s,n=%d", i + 1, obs_files[i],
            obs.n - n0);
    }
    if (obs.n <= 0) {
        sdr_log(2, "$LOG,ERROR,no observation data");
        fprintf(stderr, "no observation data\n");
        freeobs(&obs); freenav(&nav, 0xFF);
        return -1;
    }
    sortobs(&obs);

    // receiver position
    if (!has_pos) {
        if (norm(sta.pos, 3) > 1.0) {
            rr[0] = sta.pos[0];
            rr[1] = sta.pos[1];
            rr[2] = sta.pos[2];
            sdr_log(4, "$LOG,POS_SRC,header");
        }
        else if (!est_rr(&obs, &nav, rr)) {
            sdr_log(2, "$LOG,ERROR,receiver position not available");
            fprintf(stderr, "receiver position not available\n");
            freeobs(&obs); freenav(&nav, 0xFF);
            return -1;
        }
        else {
            sdr_log(4, "$LOG,POS_SRC,SPP");
        }
    }
    else {
        sdr_log(4, "$LOG,POS_SRC,user");
    }
    double pos0[3];
    ecef2pos(rr, pos0);
    sdr_log(3, "$LOG,POS,%.9f,%.9f,%.3f",
        pos0[0] * R2D, pos0[1] * R2D, pos0[2]);

    // group by epochs
    obsd_t **obs_ep = (obsd_t **)malloc(sizeof(obsd_t *) * MAX_EP);
    int *nobs_ep = (int *)malloc(sizeof(int) * MAX_EP);
    int nep = group_epochs(&obs, obs_ep, nobs_ep, MAX_EP);
    if (nep >= MAX_EP) {
        sdr_log(2, "$LOG,ERROR,epoch count exceeds MAX_EP=%d", MAX_EP);
        fprintf(stderr, "epoch count exceeds MAX_EP=%d; trailing epochs "
            "dropped\n", MAX_EP);
        free(obs_ep); free(nobs_ep);
        freeobs(&obs); freenav(&nav, 0xFF);
        return -1;
    }

    sdr_log(3, "$LOG,OBS,narr=%d,nep=%d,n=%d", narr, nep, obs.n);
    printf("pocket_calib: narr=%d nep=%d n_obs=%d freq=%.3fMHz\n",
        narr, nep, obs.n, freq / 1e6);

    // run calibration
    uint32_t t0 = tickget();
    double cp_rms = 0.0;
    int nsd = array_calib(obs_ep, nobs_ep, nep, &nav, ant_pos, narr, freq, rr,
        bias, rpy, &cp_rms);
    double dt = (tickget() - t0) * 1e-3;

    if (nsd <= 0) {
        sdr_log(2, "$LOG,ERROR,array_calib failed (insufficient data or no "
            "convergence)");
        fprintf(stderr, "array_calib() failed (insufficient data or no "
            "convergence)\n");
        free(obs_ep); free(nobs_ep);
        freeobs(&obs); freenav(&nav, 0xFF);
        return -1;
    }
    sdr_log(3, "$LOG,CALIB_DONE,time=%.3fs,nsd=%d", dt, nsd);
    printf("calib time = %.3f s, nsd = %d\n", dt, nsd);
    
    // write output
    FILE *fp = stdout;
    if (*ofile && !(fp = fopen(conv_path(ofile), "w"))) {
        sdr_log(2, "$LOG,ERROR,output file open error %s", ofile);
        fprintf(stderr, "output file open error %s\n", ofile);
        free(obs_ep); free(nobs_ep);
        freeobs(&obs); freenav(&nav, 0xFF);
        return -1;
    }
    write_result(fp, gfile, nfile, ofiles_arg, narr, freq, rr, nep, nsd, bias,
        rpy, cp_rms);
    if (fp != stdout) fclose(fp);

    sdr_log(3, "$LOG,RPY,%.4f,%.4f,%.4f",
        rpy[0] * R2D, rpy[1] * R2D, rpy[2] * R2D);
    for (int i = 0; i < narr; i++) {
        sdr_log(3, "$LOG,BIAS_CH%d,%.4f,%.4f", i + 1,
            bias[i] * 1e9, bias[i] * CLIGHT);
    }

    free(obs_ep); free(nobs_ep);
    freeobs(&obs); freenav(&nav, 0xFF);
    sdr_log_close();
    return 0;
}
