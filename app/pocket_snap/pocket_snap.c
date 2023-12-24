//
//  Pocket SDR C AP - Snapshot Positioning
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-08-04  1.0  port pocket_snap.py to C
//
#include "rtklib.h"
#include "pocket_sdr.h"

// constants --------------------------------------------------------------------
#define THRES_CN0  38.0    // threshold to lock signal (dB-Hz)
#define EL_MASK    15.0    // elevation mask (deg)
#define MAX_DOP    5000.0  // max Doppler freq. to search signal (Hz)
#define MAX_DFREQ  500.0   // max freq. offset of ref oscillator (Hz)
#define MAX_SAT    256     // max number of satellites

#define FFTW_WISDOM "../python/fftw_wisdom.txt"

#define ROUND(x)   floor(x + 0.5)

// function prototype in rtklib_wrap.c ------------------------------------------
double ionmodel_nav(gtime_t time, const nav_t *nav, const double *pos,
    const double *azel);
double navgettgd(int sat, const nav_t *nav);

// type definition --------------------------------------------------------------
typedef struct {           // satellite data type
    int sat;               // satellite number
    double rrate;          // range rate (m/s)
    double coff;           // code offset (s)
} data_t;

// global variables -------------------------------------------------------------
static sdr_cpx_t *code_fft[MAX_SAT] = {NULL}; // code FFT caches
static int VERP = 0;       // verpose display flag

// show usage -------------------------------------------------------------------
static void show_usage(void)
{
    printf("Usage: pocket_snap.py [-ts time] [-pos lat,lon,hgt] [-ti sec] [-toff toff]\n");
    printf("       [-f freq] [-fi freq] [-tint tint] [-sys sys[,...]] [-v] [-w file]\n");
    printf("       -nav file [-out file] file\n");
    exit(0);
}

// position string --------------------------------------------------------------
static void pos_str(const double *rr, char *str)
{
    double pos[3];
    ecef2pos(rr, pos);
    sprintf(str, "%13.9f %14.9f %12.3f", pos[0] * R2D, pos[1] * R2D, pos[2]);
}

// select satellite -------------------------------------------------------------
static double sel_sat(gtime_t time, int sys, int prn, const double *rr,
    const nav_t *nav, double *rrate)
{
    if (norm(rr, 3) < 1e-3) { // no coarse position
        return PI / 2.0;
    }
    double rs[6] = {0}, dts[2], var, rho, e[3], pos[3], azel[2];
    int svh = 0;
    satpos(time, time, satno(sys, prn), EPHOPT_BRDC, nav, rs, dts, &var, &svh);
    if (norm(rs, 3) < 1e-3 || svh) {
        return 0.0;
    }
    rho = geodist(rs, rr, e);
    ecef2pos(rr, pos);
    satazel(pos, e, azel);
    *rrate = dot(rs + 3, e, 3);
    return azel[1];
}

// satellite position, velocity and clock rate ----------------------------------
static void sat_pos(gtime_t time, const data_t *data, int N, const nav_t *nav,
    double spos[][8])
{
    for (int i = 0; i < N; i++) {
        double rs[6] = {0}, dts[2], var;
        int svh = 0;
        satpos(time, time, data[i].sat, EPHOPT_BRDC, nav, rs, dts, &var, &svh);
        if (svh != 0) {
            memset(rs, 0, sizeof(double) * 6);
        }
        matcpy(spos[i], rs, 6, 1);
        spos[i][6] = CLIGHT * dts[0];
        spos[i][7] = CLIGHT * dts[1];
    }
}

// fine code offset -------------------------------------------------------------
static double fine_coff(const char *sig, double fs, const float *P, int N,
    double coff, int ix)
{
    int len_code;
    (void)sdr_gen_code(sig, 1, &len_code);
    double T = sdr_code_cyc(sig) / len_code; // (s/chip)
    double E = sqrt(P[(ix - 1) % N]);
    double L = sqrt(P[(ix + 1) % N]);
    return coff + (L - E) / (L + E) * (T / 2.0 - 1.0 / fs);
}

// search signal ----------------------------------------------------------------
static int search_sig(data_t *data, const char *sig, int sys, int prn,
    const sdr_cpx_t *dif, int len_dif, double fs, double fi, double rrate)
{
    int sat = satno(sys, prn);
    double T = sdr_code_cyc(sig);
    int N = (int)(fs * T), len_code;
    
    // generate code FFT
    if (!code_fft[sat-1]) {
        int8_t *code = sdr_gen_code(sig, prn, &len_code);
        code_fft[sat-1] = sdr_cpx_malloc(2 * N);
        sdr_gen_code_fft(code, len_code, T, 0.0, fs, N, N, code_fft[sat-1]);
    }
    // doppler search bins
    float *fds;
    int len_fds;
    if (rrate == 0.0) {
        fds = sdr_dop_bins(T, 0.0, MAX_DOP, &len_fds);
    }
    else {
        float dop = -rrate / CLIGHT * sdr_sig_freq(sig);
        fds = sdr_dop_bins(T, dop, MAX_DFREQ, &len_fds);
    }
    // parallel code search
    float *P = (float *)sdr_malloc(sizeof(float) * 2 * N * len_fds);
    
    for (int i = 0; i < len_dif - 2 * N + 1; i += N) {
        sdr_search_code(code_fft[sat-1], T, dif, len_dif, i, 2 * N, fs, fi, fds,
            len_fds, P);
    }
    // max correlation power
    int ix[2] = {0};
    float cn0 = sdr_corr_max(P, 2 * N, N, len_fds, T, ix);
    if (cn0 >= THRES_CN0) {
        double dop, rrate, coff;
        dop = sdr_fine_dop(P, 2 * N, fds, len_fds, ix);
        rrate = -dop * CLIGHT / sdr_sig_freq(sig);
        coff = fine_coff(sig, fs, P + ix[0] * 2 * N, N, ix[1] / fs, ix[1]);
        if (VERP) {
            char str[32];
            satno2id(sat, str);
            printf("%s : SIG=%-5s C/N0=%5.1f dB-Hz DOP=%9.3f Hz COFF=%12.9f ms\n",
                str, sig, cn0, dop, coff * 1e3);
            fflush(stdout);
        }
        data->sat = sat;
        data->rrate = rrate;
        data->coff = coff;
    }
    sdr_free(fds);
    sdr_free(P);
    return cn0 >= THRES_CN0;
}

// search signals ---------------------------------------------------------------
static int search_sigs(gtime_t time, int ssys, const sdr_cpx_t *dif, int len_dif,
    double fs, double fi, const double *rr, const nav_t *nav, data_t *data)
{
    if (VERP) {
        printf("search_sigs\n");
    }
    int n = 0;
    if (ssys & SYS_GPS) {
        for (int prn = 1; prn <= 32; prn++) {
            double el, rrate = 0.0;
            el = sel_sat(time, SYS_GPS, prn, rr, nav, &rrate);
            if (el >= EL_MASK * D2R) {
                n += search_sig(data + n, "L1CA", SYS_GPS, prn, dif, len_dif, fs,
                    fi, rrate);
            }
        }
    }
    if (ssys & SYS_GAL) {
        for (int prn = 1; prn <= 36; prn++) {
            double el, rrate = 0.0;
            el = sel_sat(time, SYS_GAL, prn, rr, nav, &rrate);
            if (el >= EL_MASK * D2R) {
                n += search_sig(data + n, "E1C", SYS_GAL, prn, dif, len_dif, fs,
                    fi, rrate);
            }
        }
    }
    if (ssys & SYS_CMP) {
        for (int prn = 19; prn <= 46; prn++) {
            double el, rrate = 0.0;
            el = sel_sat(time, SYS_CMP, prn, rr, nav, &rrate);
            if (el >= EL_MASK * D2R) {
                n += search_sig(data + n, "B1CP", SYS_CMP, prn, dif, len_dif, fs,
                    fi, rrate);
            }
        }
    }
    if (ssys & SYS_QZS) {
        for (int prn = 193; prn <= 199; prn++) {
            double el, rrate = 0.0;
            el = sel_sat(time, SYS_QZS, prn, rr, nav, &rrate);
            if (el >= EL_MASK * D2R) {
                n += search_sig(data + n, "L1CP", SYS_QZS, prn, dif, len_dif, fs,
                    fi, rrate);
            }
        }
    }
    return n;
}

// drdot/dx ---------------------------------------------------------------------
static void drdot_dx(const double *rs, const double *vs, const double *x,
    double *drdot)
{
    double dx = 10.0, rho, rdot, e[3], x1[3], x2[3], x3[3], e1[3], e2[3], e3[3];
    rho = geodist(rs, x, e);
    rdot = dot(vs, e, 3);
    memcpy(x1, x, sizeof(double) * 3);
    memcpy(x2, x, sizeof(double) * 3);
    memcpy(x3, x, sizeof(double) * 3);
    x1[0] += dx;
    x2[1] += dx;
    x3[2] += dx;
    rho = geodist(rs, x1, e1);
    rho = geodist(rs, x2, e2);
    rho = geodist(rs, x3, e3);
    drdot[0] = (dot(vs, e1, 3) - rdot) / dx;
    drdot[1] = (dot(vs, e2, 3) - rdot) / dx;
    drdot[2] = (dot(vs, e3, 3) - rdot) / dx;
    drdot[3] = 1.0;
}

// position by Doppler ----------------------------------------------------------
static int pos_dop(const data_t *data, int N, double spos[][8], double *rr)
{
    if (VERP) {
        printf("pos_dop\n");
    }
    double x[4] = {0};
    
    for (int i = 0; i < 10; i++) {
        double rho, e[3], v[MAX_SAT] = {0}, H[MAX_SAT*4] = {0}, dx[4], Q[4*4];
        int n = 0;
        for (int j = 0; j < N; j++) {
            if (norm(spos[j], 3) > 1e-3) {
                rho = geodist(spos[j], x, e);
                v[n] = data[j].rrate - (dot(spos[j] + 3, e, 3) + x[3] -
                    spos[j][7]);
                drdot_dx(spos[j], spos[j] + 3, x, H + 4 * n);
                n++;
            }
        }
        if (n < 4 || lsq(H, v, 4, n, dx, Q)) {
            return 0;
        }
        if (VERP) {
            char str[64];
            pos_str(x, str);
            printf("(%d) N=%2d  POS=%s  RES=%10.3f m/s\n", i, n, str,
                norm(v, n));
        }
        for (int i = 0; i < 4; i++) {
            x[i] += dx[i];
        }
        if (norm(dx, 4) < 1.0) {
            matcpy(rr, x, 3, 1);
            return 1;
        }
    }
    return 0;
}

// resolve ms ambiguity in code offset -----------------------------------------
static void res_coff_amb(data_t *data, int N, double spos[][8], const double *rr)
{
    if (VERP) {
        printf("res_coff_amb\n");
    }
    double e[3], r, tau[MAX_SAT], tau_min = 1e9;
    int idx = 0;
    for (int i = 0; i < N; i++) {
        if (norm(spos[i], 3) > 1e-3) {
            r = geodist(spos[i], rr, e);
            tau[i] = (r - spos[i][6]) / CLIGHT;
            if (tau[i] < tau_min) {
                tau_min = tau[i];
                idx = i;
            }
        }
    }
    double coff_ref = data[idx].coff;
    double tau_ref = tau[idx];
    for (int i = 0; i < N; i ++) {
        if (norm(spos[i], 3) > 1e-3) {
            double off = (tau[i] - tau_ref) - (data[i].coff - coff_ref);
            data[i].coff += ROUND(off * 1e3) * 1e-3;
            if (VERP) {
                char sat1[16], sat2[16];
                satno2id(data[i].sat, sat1);
                satno2id(data[idx].sat, sat2);
                printf("%s - %s: N=%8.5f -> %8.5f\n", sat1, sat2, off * 1e3,
                    ROUND(off * 1e3));
            }
        }
    }
}

// estimate position by code offsets ------------------------------------------
static int pos_coff(gtime_t time, const data_t *data, int N, double *rr,
    const nav_t *nav, double *dtr)
{
    if (VERP) {
        printf("pos_coff\n");
    }
    double x[5] = {0};
    matcpy(x, rr, 5, 1);
    
    for (int i = 0; i < 10; i++) {
        int n = 0;
        double pos[3], dx[5], v[MAX_SAT], H[MAX_SAT*5], Q[5*5];
        ecef2pos(x, pos);
        for (int j = 0; j < N; j++) {
            gtime_t ts = timeadd(time, x[4]);
            double rs[6] = {0}, dts[2], var, rho, e[3], azel[2];
            int svh = 0;
            satpos(ts, time, data[j].sat, EPHOPT_BRDC, nav, rs, dts, &var, &svh);
            rho = geodist(rs, x, e);
            satazel(pos, e, azel);
            if (norm(rs, 3) > 1e-3 && svh == 0 && azel[1] >= EL_MASK * D2R) {
                v[n] = CLIGHT * data[j].coff - (rho + x[3] - CLIGHT * dts[0] +
                    ionmodel_nav(ts, nav, pos, azel) +
                    tropmodel(ts, pos, azel, 0.7) + navgettgd(data[j].sat, nav));
                H[n*5  ] = -e[0];
                H[n*5+1] = -e[1];
                H[n*5+2] = -e[2];
                H[n*5+3] = 1.0;
                H[n*5+4] = dot(rs + 3, e, 3);
                n++;
            }
        }
        if (n < 5 || lsq(H, v, 5, n, dx, Q)) {
            break;
        }
        if (VERP) {
            char str[64];
            pos_str(x, str);
            printf("(%d) N=%2d  POS=%s  CLK=%9.6f  DT=%9.6f  RES=%10.3f \n",
                i, n, str, x[3] / CLIGHT, x[4], sqrt(dot(v, v, n) / n));
        }
        for (int i = 0; i < 5; i++) {
            x[i] += dx[i];
        }
        if (norm(dx, 3) < 1e-3) {
            if (sqrt(dot(v, v, 3) / n) > 1e3) {
                break;
            }
            matcpy(rr, x, 3, 1);
            *dtr = x[3] / CLIGHT - x[4];
            return n;
        }
    }
    memset(rr, 0, sizeof(double) * 3);
    return 0;
}

// write solution header --------------------------------------------------------
static void write_head(FILE *fp, const char *file, double tint, double fs)
{
    fprintf(fp, "%% SNAPSHOT POSITION by POCKET_SNAP\n");
    fprintf(fp, "%% INPUT FILE    : %s\n",  file);
    fprintf(fp, "%% SAMPLING TIME : %.1f ms / SNAPSHOT\n", tint * 1e3);
    fprintf(fp, "%% SAMPLING FREQ : %.3f MHz\n", fs / 1e6);
    fprintf(fp, "%%  %-21s  %13s %12s %12s %4s %4s\n", "UTC", "latitude(deg)",
        "longitude(deg)", "height(m)", "Q", "ns");
}

// parse time -------------------------------------------------------------------
static gtime_t parse_time(const char *str)
{
    double ep[6] = {0};
    char buff[64] = "";
    for (char *s = buff; s - buff < 63; s++, str++) {
        if (!(*s = strchr("/:_-", *str) ?  ' ' : *str)) break;
    }
    sscanf(buff, "%lf %lf %lf %lf %lf %lf", ep, ep + 1, ep + 2, ep + 3, ep + 4,
        ep + 5);
    return utc2gpst(epoch2time(ep));
}

// parse navigation system ------------------------------------------------------
static int parse_sys(const char *str)
{
    int sys = SYS_NONE;
    for (const char *s = str; *s; s++) {
        if (*s == 'G') {
            sys |= SYS_GPS;
        }
        else if (*s == 'E') {
            sys |= SYS_GAL;
        }
        else if (*s == 'J') {
            sys |= SYS_QZS;
        }
        else if (*s == 'C') {
            sys |= SYS_CMP;
        }
    }
    return sys;
}

// get capture time by file path ------------------------------------------------
static gtime_t path_time(const char *file)
{
    double epoch[6] = {0};
    const char *p = strchr(file, '_');
    sscanf(p + 1, "%4lf%2lf%2lf_%2lf%2lf%2lf", epoch, epoch + 1, epoch + 2,
        epoch + 3, epoch + 4, epoch + 5);
    return utc2gpst(epoch2time(epoch));
}

// convert file path for Windows -------------------------------------------------
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

//-------------------------------------------------------------------------------
//
//   Synopsis
// 
//     pocket_snap [-ts time] [-pos lat,lon,hgt] [-ti sec] [-toff toff]
//         [-f freq] [-fi freq] [-tint tint] [-sys sys[,...]] [-v] [-w file]
//         -nav file [-out file] file
// 
//   Description
// 
//     Snapshot positioning with GNSS signals in digitized IF file.
// 
//   Options ([]: default)
//  
//     -ts time
//         Captured start time in UTC as YYYY/MM/DD HH:mm:ss format.
//         [parsed by file name]
//
//     -pos lat,lon,hgt
//         Coarse receiver position as latitude, longitude in degree and
//         height in m. [no coarse position]
// 
//     -ti sec
//         Time interval of positioning in seconds. (0.0: single) [0.0]
// 
//     -toff toff
//         Time offset from the start of digital IF data in seconds. [0.0]
// 
//     -f freq
//         Sampling frequency of digital IF data in MHz. [12.0]
//
//     -fi freq
//         IF frequency of digital IF data in MHz. The IF frequency equals 0, the
//         IF data is treated as IQ-sampling (zero-IF). [0.0]
//
//     -tint tint
//         Integration time for signal search in msec. [20.0]
//
//     -sys sys[,...]
//         Select navigation system(s) (G=GPS,E=Galileo,J=QZSS,C=BDS). [G]
//
//     -v
//         Enable verpose status display.
//
//     -w file
//         Specify FFTW wisdowm file. [../python/fftw_wisdom.txt]
//
//     -nav file
//         RINEX navigation data file.
//
//     -out file
//         Output solution file as RTKLIB solution format.
//
//     file
//         Digitized IF data file.
//
int main(int argc, char **argv)
{
    gtime_t ts = {0};
    FILE *fp = stdout;
    nav_t nav = {0};
    data_t data[MAX_SAT] = {0};
    double ti = 0.0, toff = 0.0, fs = 6e6, fi = 0.0, tint = 0.02, rr[3] = {0};
    char *file = "", *nfile = "", *ofile = "", *fftw_wisdom = FFTW_WISDOM;
    int ssys = SYS_GPS;
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-ts") && i + 1 < argc) {
            ts = parse_time(argv[++i]);
        }
        else if (!strcmp(argv[i], "-pos") && i + 1 < argc) {
            double pos[3] = {0};
            sscanf(argv[++i], "%lf,%lf,%lf", pos, pos + 1, pos + 2);
            pos[0] *= D2R;
            pos[1] *= D2R;
            pos2ecef(pos, rr);
        }
        else if (!strcmp(argv[i], "-ti") && i + 1 < argc) {
            ti = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-toff") && i + 1 < argc) {
            toff = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-f") && i + 1 < argc) {
            fs = atof(argv[++i]) * 1e6;
        }
        else if (!strcmp(argv[i], "-fi") && i + 1 < argc) {
            fi = atof(argv[++i]) * 1e6;
        }
        else if (!strcmp(argv[i], "-tint") && i + 1 < argc) {
            tint = atof(argv[++i]) * 1e-3;
        }
        else if (!strcmp(argv[i], "-sys") && i + 1 < argc) {
            ssys = parse_sys(argv[++i]);
        }
        else if (!strcmp(argv[i], "-nav") && i + 1 < argc) {
            nfile = argv[++i];
        }
        else if (!strcmp(argv[i], "-out") && i + 1 < argc) {
            ofile = argv[++i];
        }
        else if (!strcmp(argv[i], "-v")) {
            VERP = 1;
        }
        else if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            fftw_wisdom = argv[++i];
        }
        else if (argv[i][0] == '-') {
            show_usage();
        }
        else {
            file = argv[i];
        }
    }
    // read RINEX NAV
    if (readrnx(conv_path(nfile), 0, "", NULL, &nav, NULL) == -1) {
        fprintf(stderr, "nav data read error %s\n", nfile);
        exit(-1);
    }
    // get capture time by file path
    if (ts.time == 0) {
        ts = path_time(file);
    }
    if (*ofile) {
        if (!(fp = fopen(conv_path(ofile), "w"))) {
            fprintf(stderr, "file open error %s\n", ofile);
            freenav(&nav, 0xFF);
            exit(-1);
        }
        write_head(fp, file, tint, fs);
    }
    sdr_func_init(fftw_wisdom);
    
    uint32_t t0 = tickget();
    
    for (int i = 0; i < 100000; i++) {
        gtime_t tt = timeadd(ts, toff + ti * i);
        
        if (ti <= 0.0 && i >= 1) { // single snapshot
            break;
        }
        // read DIF data
        int N, IQ = (fi > 0) ? 1 : 2;
        sdr_cpx_t *dif = sdr_read_data(file, fs, IQ, tint, toff + ti * i, &N);
        if (!dif) {
            break;
        }
        // search signals
        int n = search_sigs(tt, ssys, dif, N, fs, fi, rr, &nav, data);
        if (n <= 0) {
            continue;
        }
        sdr_cpx_free(dif);
        
        // satellite position and velocity
        double spos[MAX_SAT][8];
        sat_pos(tt, data, n, &nav, spos);
        
        if (norm(rr, 3) == 0) {
            // position by doppler
            if (!pos_dop(data, n, spos, rr)) {
                sdr_cpx_free(dif);
                continue;
            }
            // force height = 0
            double pos[3];
            ecef2pos(rr, pos);
            pos[2] = 0.0;
            pos2ecef(pos, rr);
        }
        // resolve ms ambiguity in code offsets
        res_coff_amb(data, n, spos, rr);
        
        // estimate position by code offsets
        double dtr = 0.0;
        int ns = pos_coff(tt, data, n, rr, &nav, &dtr);
        
        // write solution
        char tstr[64], str[128];
        time2str(gpst2utc(timeadd(tt, -dtr)), tstr, 3);
        pos_str(rr, str);
        fprintf(fp, "%s   %s %4d %4d\n", tstr, str, 5, ns);
        fflush(fp);
    }
    printf("TIME (s) = %.3f\n", (tickget() - t0) * 1e-3);
    freenav(&nav, 0xFF);
    fclose(fp);
    return 0;
}

