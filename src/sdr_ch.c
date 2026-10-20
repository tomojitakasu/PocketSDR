// 
//  Pocket SDR C Library - GNSS SDR Receiver Channel Functions
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-07-08  1.0  port sdr_ch.py to C
//  2022-07-16  1.1  modify API sdr_ch_new()
//  2023-12-16  1.2  reduce memory usage
//  2023-12-28  1.3  support type and API changes.
//  2024-01-12  1.4  ch->state: const char * -> int
//  2024-01-16  1.5  add doppler assist for acquisition
//  2024-03-20  1.6  modify API sdr_ch_update()
//  2024-04-28  1.7  modify API sdr_ch_new()
//  2024-06-06  1.8  modify API sdr_ch_new()
//  2024-06-10  1.9  add API sdr_ch_stat_req(), sdr_ch_stat_get()
//  2024-08-26  1.10 support sdr_corr_std(), sdr_corr_fft() API changes
//  2024-12-30  1.11 support Bump-jump for BOC modulation
//  2025-11-19  1.12 improve carrier-phase coherency
//  2026-05-02  1.13 support E5 AltBOC
//  2026-06-15  1.14 update loss-of-lock detection logic
//  2026-07-05  1.15 use fused mix carrier and standard correlator
//  2026-07-05  1.16 support sdr_corr_fft() API change
//                   optimize CSK() peak search and buffer usage
//  2026-07-05  1.17 use standard correlator for L6D/E EPL correlations
//  2026-07-05  1.18 decode L6 CSK by chip-domain FFT correlation
//  2026-07-05  1.19 fix polarity of L6D/E EPL correlations
//  2026-07-05  1.20 narrow CSK peak search range after L6 frame sync
//  2026-07-08  1.21 support sdr_corr_std() API change for int8 code values
//  2026-07-08  1.22 E5 AltBOC SSB sub-carriers: -1/+1 square -> 4-level stairs
//                   support sdr_corr_std_cpx_code() API change
//  2026-07-14  1.23 fix E5ABQ bank selection for correlation windows straddling
//                   the code-cycle boundary (sec-code chip pairs differ)
//  2026-07-17  1.24 correlate E5 AltBOC sidebands separately with real code
//                   banks and combine them after correlation
//                   support sdr_corr_std() API change
//  2026-07-19  1.25 add pre-combined E5ABQ replicas (a+b, a-b) to correlate
//                   both sidebands in one pass once the polarity is calibrated
//  2026-07-19  1.26 improve low-C/N0 acquisition and pilot tracking
//
#include <ctype.h>
#include <math.h>
#include "pocket_sdr.h"

// constants and macros --------------------------------------------------------
#define SP_CORR    0.25     // correlator spacing (chip)
#define T_ACQ      0.02     // non-coherent integration time for acquisition (s)
#define T_ACQ_EXT  0.10     // integration time for assisted acquisition (s)
#define T_COH      0.02     // coherent integration time for pilot PLL (s)
#define T_DLL      0.02     // non-coherent integration time for DLL (s)
#define T_CN0      0.5      // averaging time for C/N0 (s)
#define T_FPULLIN  1.0      // frequency pull-in time (s)
#define T_FPULLIN_W 0.5     // wide-band FLL time (s) (narrow-band FLL afterwards)
#define T_NPULLIN  1.5      // navigation data pull-in time (s)
#define B_DLL      0.25     // band-width of DLL filter (Hz)
#define B_PLL      5.0      // band-width of PLL filter (Hz)
#define B_FLL_W    5.0      // band-width of FLL filter (Hz) (wide)
#define B_FLL_N    2.0      // band-width of FLL filter (Hz) (narrow)
#define MAX_DOP    5000.0   // max Doppler for acquisition (Hz)
#define THRES_CN0_L 34.0    // C/N0 threshold (dB-Hz) (lock)
#define THRES_CN0_U 25.0    // C/N0 threshold (dB-Hz) (lost)
#define THRES_CN0_EXT 30.0  // C/N0 threshold (dB-Hz) (assisted acquisition)
#define THRES_CN0_L6 32.5   // C/N0 threshold (dB-Hz) (L6D/E lost)
#define THRES_PLI  0.25     // carrier lock detector threshold (cos 2*phi)
#define LOST_TH    4        // lost decision count (C/N0/PLI windows of T_CN0)
#define THRES_SYNC 0.02     // threshold for sec-code sync
#define THRES_LOST 0.002    // threshold for sec-code lost
#define THRES_SEC_RATIO 0.8 // threshold for sec-code soft correlation ratio
#define POS_CORR_N -120.0   // N-correlator position (samples)
#define FILT_CN0   0.5      // filter parameter for C/N0
#define BUMP_K     1.3      // bump-jump threshold
#define ACQ_INT    10       // acquisition interval (ms)
#define CSK_WIN    280      // L6 CSK correlation window margin (chips)
#define CSK_NFFT   10800    // L6 CSK chip-domain FFT size (>= 10230 + 2 * CSK_WIN)

#define DPI        (2.0 * PI)
#define SQR(x)     ((x) * (x))
#define MAX(x, y)  ((x) > (y) ? (x) : (y))
#define MIN(x, y)  ((x) < (y) ? (x) : (y))
#define SIGN(x)    ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

// global variables ------------------------------------------------------------
double sdr_sp_corr = SP_CORR;
double sdr_t_acq   = T_ACQ;
double sdr_t_acq_ext = T_ACQ_EXT;
double sdr_t_coh   = T_COH;
double sdr_t_dll   = T_DLL;
double sdr_b_dll   = B_DLL;
double sdr_b_pll   = B_PLL;
double sdr_b_fll_w = B_FLL_W;
double sdr_b_fll_n = B_FLL_N;
double sdr_max_dop = MAX_DOP;
double sdr_thres_cn0_l = THRES_CN0_L;
double sdr_thres_cn0_u = THRES_CN0_U;
double sdr_thres_cn0_ext = THRES_CN0_EXT;
double sdr_thres_pli = THRES_PLI; // carrier lock detector threshold
int sdr_lost_th = LOST_TH; // lost decision count
int sdr_bump_jump = 0;
double sdr_e5ab_off = 0.0;  // E5b-E5a group-delay (s)
double sdr_bump_k = BUMP_K; // bump-jump threshold

// upper cases of signal string ------------------------------------------------
static void sig_upper(const char *sig, char *Sig)
{
    int i;
    
    for (i = 0; i < 15; i++) {
        if (!(Sig[i] = (char)toupper(sig[i]))) break;
    }
    Sig[i] = '\0';
}

// generate E5a-Q × SC_E5aI chip pattern (AltBOC 4-level stairs, 4× chip rate) -
static int gen_e5aq_chip(int prn, int8_t **code_I, int8_t **code_Q, int *N)
{
    static const int8_t SC_E5aI_I[] = { 11, -27,  27, -11, -11,  27, -27,  11};
    static const int8_t SC_E5aI_Q[] = {-27,  11,  11, -27,  27, -11, -11,  27};
    int n;
    int8_t *c = sdr_gen_code("E5AQ", prn, &n);
    if (!c) return 0;
    *N = n * 4;
    *code_I = (int8_t *)sdr_malloc(*N);
    *code_Q = (int8_t *)sdr_malloc(*N);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 4; j++) {
            int k = (i * 4 + j) % 8;
            (*code_I)[i*4+j] = c[i] * SC_E5aI_I[k];
            (*code_Q)[i*4+j] = c[i] * SC_E5aI_Q[k];
        }
    }
    return 1;
}

// generate E5b-Q × SC_E5bI chip pattern (AltBOC 4-level stairs, 4× chip rate) -
static int gen_e5bq_chip(int prn, int8_t **code_I, int8_t **code_Q, int *N)
{
    static const int8_t SC_E5bI_I[] = { 11, -27,  27, -11, -11,  27, -27,  11};
    static const int8_t SC_E5bI_Q[] = { 27, -11, -11,  27, -27,  11,  11, -27};
    int n;
    int8_t *c = sdr_gen_code("E5BQ", prn, &n);
    if (!c) return 0;
    *N = n * 4;
    *code_I = (int8_t *)sdr_malloc(*N);
    *code_Q = (int8_t *)sdr_malloc(*N);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 4; j++) {
            int k = (i * 4 + j) % 8;
            (*code_I)[i*4+j] = c[i] * SC_E5bI_I[k];
            (*code_Q)[i*4+j] = c[i] * SC_E5bI_Q[k];
        }
    }
    return 1;
}

// E5ABQ acquisition FFT (E5aQ-only) -------------------------------------------
static void gen_e5abq_code_fft(int prn, double T, double fs, int N,
    sdr_cpx_t *code_fft)
{
    int8_t *code_I, *code_Q;
    int len;
    if (gen_e5aq_chip(prn, &code_I, &code_Q, &len)) {
        sdr_gen_code_fft(code_I, code_Q, len, T, 0.0, fs, N, N, code_fft);
        sdr_free(code_I);
        sdr_free(code_Q);
    }
}

// E5ABQ tracking code banks (aI,aQ,bI,bQ,pI,pQ,mI,mQ; a=E5aQ*SC_E5aI,
// b=E5bQ*SC_E5bI, p=a+b, m=a-b: p/m are the pre-combined sideband replicas) ----
static void gen_e5abq_banks(int prn, double T, double fs, int N, int8_t *code)
{
    int8_t *I[2], *Q[2];
    int len[2];
    if (!gen_e5aq_chip(prn, &I[0], &Q[0], &len[0])) return;
    if (!gen_e5bq_chip(prn, &I[1], &Q[1], &len[1])) {
        sdr_free(I[0]); sdr_free(Q[0]);
        return;
    }
    sdr_cpx16_t *tmp = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N);

    for (int m = 0; m < 2; m++) { // m=0:a, m=1:b (E5b-E5a group-delay applied)
        for (int k = 0; k < SDR_N_CODES; k++) {
            double coff = -k / fs / SDR_N_CODES - (m ? sdr_e5ab_off : 0.0);
            int8_t *pI = code + ((m * 2    ) * SDR_N_CODES + k) * N;
            int8_t *pQ = code + ((m * 2 + 1) * SDR_N_CODES + k) * N;
            sdr_res_code(I[m], Q[m], len[m], T, coff, fs, N, 0, tmp);
            for (int s = 0; s < N; s++) {
                pI[s] = tmp[s].I;
                pQ[s] = tmp[s].Q;
            }
        }
    }
    // pre-combined replicas: bank 4,5 = a+b (k=+1), bank 6,7 = a-b (k=-1)
    for (int k = 0; k < SDR_N_CODES; k++) {
        const int8_t *aI = code + (0 * SDR_N_CODES + k) * N;
        const int8_t *aQ = code + (1 * SDR_N_CODES + k) * N;
        const int8_t *bI = code + (2 * SDR_N_CODES + k) * N;
        const int8_t *bQ = code + (3 * SDR_N_CODES + k) * N;
        int8_t *pI = code + (4 * SDR_N_CODES + k) * N;
        int8_t *pQ = code + (5 * SDR_N_CODES + k) * N;
        int8_t *mI = code + (6 * SDR_N_CODES + k) * N;
        int8_t *mQ = code + (7 * SDR_N_CODES + k) * N;
        for (int s = 0; s < N; s++) { // |a|,|b| <= SDR_ALTBOC_SCALE: fits int8
            pI[s] = (int8_t)(aI[s] + bI[s]);
            pQ[s] = (int8_t)(aQ[s] + bQ[s]);
            mI[s] = (int8_t)(aI[s] - bI[s]);
            mQ[s] = (int8_t)(aQ[s] - bQ[s]);
        }
    }
    sdr_free(tmp);
    sdr_free(I[0]); sdr_free(Q[0]); sdr_free(I[1]); sdr_free(Q[1]);
}

// calibrate E5ABQ sideband combination polarity by voting stronger combination -
static void cal_e5abq_pol(sdr_ch_t *ch, const sdr_cpx_t *C1a,
    const sdr_cpx_t *C1b, int sig1, int sig2)
{
    sdr_trk_t *trk = ch->trk;

    for (int part = 0; part < 2; part++) {
        double dot = C1a[part][0] * C1b[part][0] + C1a[part][1] * C1b[part][1];
        double pwr = SQR(C1a[part][0]) + SQR(C1a[part][1]) +
            SQR(C1b[part][0]) + SQR(C1b[part][1]);
        if (pwr <= 0.0) continue;
        // |Ca+s*Cb|^2 - |Ca-s*Cb|^2 = 4*s*dot (confidence-weighted vote)
        trk->e5b_score += 2.0 * (part ? sig2 : sig1) * dot / pwr;
    }
    if (++trk->e5b_cnt >= 64) {
        trk->e5b_pol = trk->e5b_score < 0.0 ? -1 : 1;
        sdr_log(4, "$LOG,%.3f,%s,%d,E5B POL %+d (%.1f)", ch->time, ch->sig,
            ch->prn, trk->e5b_pol, trk->e5b_score);
    }
}

// mix carrier and correlate E5 AltBOC sidebands (a,b) and combine them ---------
static void corr_e5abq(sdr_ch_t *ch, const sdr_buff_t *buff, int ix, double fc,
    sdr_cpx_t *C1)
{
    sdr_trk_t *trk = ch->trk;
    sdr_cpx_t Ca[SDR_MAX_CORR], Cb[SDR_MAX_CORR], C1a[2], C1b[2];
    int n = trk->npos + trk->nposx, NB = ch->N * SDR_N_CODES;
    int NS = (ch->N + 1) * SDR_N_CODES;
    double coff = ch->coff * ch->fs;

    if (trk->sec_sync <= 0) { // E5aQ sideband only until sec-code sync
        sdr_corr_std(buff, ix, ch->N, ch->fs, fc, ch->phi, trk->code,
            trk->code_sum, trk->code + NB, trk->code_sum + NS,
            SDR_ALTBOC_SCALE, coff, trk->pos, n, 0, trk->C, C1);
        return;
    }
    // sec-code chip pairs for window parts split by the code-cycle boundary
    int N1 = ch->len_sec_code, N2 = ch->len_sec_code2;
    int idx = (ch->lock - trk->sec_sync + N1) % N1, idx2 = (idx + 1) % N1;
    int sa1 = ch->sec_code[idx], sa2 = ch->sec_code[idx2];
    int sb1 = ch->sec_code2[idx % N2], sb2 = ch->sec_code2[idx2 % N2];

    if (trk->e5b_pol) { // calibrated: one correlation with pre-combined replica
        int j1 = trk->e5b_pol * sa1 * sb1 > 0 ? 4 : 6; // bank a+b or a-b
        int j2 = trk->e5b_pol * sa2 * sb2 > 0 ? 4 : 6; // after the wrap-around
        sdr_corr_std2(buff, ix, ch->N, ch->fs, fc, ch->phi,
            trk->code + NB * j1, trk->code_sum + NS * j1,
            trk->code + NB * (j1 + 1), trk->code_sum + NS * (j1 + 1),
            trk->code + NB * j2, trk->code_sum + NS * j2,
            trk->code + NB * (j2 + 1), trk->code_sum + NS * (j2 + 1),
            SDR_ALTBOC_SCALE * 2, coff, trk->pos, n, sa1 * sa2, trk->C, C1);
        return;
    }
    int pol = trk->e5b_score < 0.0 ? -1 : 1; // running estimate while calib.

    sdr_corr_std(buff, ix, ch->N, ch->fs, fc, ch->phi, trk->code,
        trk->code_sum, trk->code + NB, trk->code_sum + NS, SDR_ALTBOC_SCALE,
        coff, trk->pos, n, sa1 * sa2, Ca, C1a);
    sdr_corr_std(buff, ix, ch->N, ch->fs, fc, ch->phi, trk->code + NB * 2,
        trk->code_sum + NS * 2, trk->code + NB * 3, trk->code_sum + NS * 3,
        SDR_ALTBOC_SCALE, coff, trk->pos, n, sb1 * sb2, Cb, C1b);

    // combine sidebands (C = (Ca + pol * sec_a * sec_b * Cb) / 2)
    int k1 = pol * sa1 * sb1, k2 = pol * sa2 * sb2;
    for (int i = 0; i < n; i++) {
        trk->C[i][0] = (Ca[i][0] + k1 * Cb[i][0]) * 0.5f;
        trk->C[i][1] = (Ca[i][1] + k1 * Cb[i][1]) * 0.5f;
    }
    for (int i = 0; i < 2; i++) {
        C1[0][i] = (C1a[0][i] + k1 * C1b[0][i]) * 0.5f;
        C1[1][i] = (C1a[1][i] + k2 * C1b[1][i]) * 0.5f;
    }
    cal_e5abq_pol(ch, C1a, C1b, sa1 * sb1, sa2 * sb2);
}

// new signal acquisition ------------------------------------------------------
static sdr_acq_t *acq_new(const char *sig, int prn, const int8_t *code,
    int len_code, double T, double fs, int N)
{
    sdr_acq_t *acq = (sdr_acq_t *)sdr_malloc(sizeof(sdr_acq_t));
    
    acq->code_fft = sdr_cpx_malloc(2 * N);
    if (!strcmp(sig, "E5ABQ")) {
        gen_e5abq_code_fft(prn, T, fs, N, acq->code_fft);
    } else {
        sdr_gen_code_fft(code, NULL, len_code, T, 0.0, fs, N, N, acq->code_fft);
    }
    acq->fd_ext = 0.0;
    acq->fd_ext_valid = 0;
    acq->fds = sdr_dop_bins(T, 0.0f, (float)sdr_max_dop, &acq->len_fds);
    acq->P_sum = NULL;
    acq->n_sum = 0;
    return acq;
}

// free signal acquisition -----------------------------------------------------
static void acq_free(sdr_acq_t *acq)
{
    if (!acq) return;
    sdr_cpx_free(acq->code_fft);
    sdr_free(acq->fds);
    sdr_free(acq->P_sum);
    sdr_free(acq);
}

// generate L6 CSK code FFT in chip-domain (see CSK()) ---------------------------
static void gen_csk_code_fft(const int8_t *code, int len_code, int slot,
    sdr_cpx_t *code_fft)
{
    int L = len_code / 2, W = CSK_WIN, M = CSK_NFFT;
    sdr_cpx_t *cpx = sdr_cpx_malloc(M);

    // code chips periodically extended by +-W chips and zero-padded
    memset(cpx, 0, sizeof(sdr_cpx_t) * M);
    for (int i = 0; i < L + 2 * W; i++) {
        cpx[i][0] = (float)code[((i - W + L) % L) * 2 + slot];
    }
    if (sdr_cpx_fft(cpx, M, SDR_FFT_FORWARD, code_fft)) {
        for (int i = 0; i < M; i++) {
            code_fft[i][1] = -code_fft[i][1]; // complex conjugate
        }
    }
    sdr_cpx_free(cpx);
}

// new signal tracking ---------------------------------------------------------
static sdr_trk_t *trk_new(const char *sig, int prn, const int8_t *code,
    int len_code, double T, double fs)
{
    sdr_trk_t *trk = (sdr_trk_t *)sdr_malloc(sizeof(sdr_trk_t));
    double sc = T / sdr_code_len(sig) * fs; // sample / chip
    double sp = sdr_sp_corr * sc; // correlator spacing (sample)
    int npos = 0;
    trk->pos[npos++] = 0.0;        // P
    trk->pos[npos++] = -0.5 * sp;  // E
    trk->pos[npos++] =  0.5 * sp;  // L
    trk->pos[npos++] = POS_CORR_N; // N
    if (sdr_bump_jump && sdr_sig_boc(sig)) {
        double vsp;
        if (!strcmp(sig, "E5ABQ")) {
            vsp = sc / 3;
        } else if (!strcmp(sig, "G1OCP") || !strcmp(sig, "G2OCP")) {
            vsp = sc / 4;
        } else {
            vsp = sc / 2;
        }
        trk->pos[npos++] = -vsp; // VE
        trk->pos[npos++] =  vsp; // VL
    }
    trk->npos = npos;
    
    trk->nposx = 0;
    trk->sec_sync = trk->sec_pol = 0;
    trk->csk_ref = -1;
    trk->err_phas = trk->err_code = 0.0;
    trk->phas_acc = trk->code_int = 0.0;
    trk->sumP = trk->sumN = trk->sumVE = trk->sumVL = 0.0;
    trk->sumPs = trk->sumD = 0.0;
    trk->Cs[0] = trk->Cs[1] = 0.0f;
    trk->coh_n = 0;
    memset(trk->sumC, 0, sizeof(double) * SDR_MAX_CORR);
    memset(trk->sumI, 0, sizeof(double) * SDR_MAX_CORR);
    memset(trk->aveP, 0, sizeof(double) * SDR_MAX_CORR);
    memset(trk->aveI, 0, sizeof(double) * SDR_MAX_CORR);
    int N = (int)(fs * T);
    if (!strcmp(sig, "L6D") || !strcmp(sig, "L6E")) {
        trk->code_fft = sdr_cpx_malloc(CSK_NFFT);
        gen_csk_code_fft(code, len_code, !strcmp(sig, "L6E") ? 1 : 0,
            trk->code_fft);
    }
    int nb = !strcmp(sig, "E5ABQ") ? 8 : 1; // number of code banks
    trk->code = (int8_t *)sdr_malloc(sizeof(int8_t) * N * SDR_N_CODES * nb);
    if (nb == 8) {
        gen_e5abq_banks(prn, T, fs, N, trk->code);
    } else {
        sdr_cpx16_t *tmp = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * N);
        for (int i = 0; i < SDR_N_CODES; i++) {
            double coff = -i / fs / SDR_N_CODES;
            int8_t *p = trk->code + i * N;
            sdr_res_code(code, NULL, len_code, T, coff, fs, N, 0, tmp);
            for (int s = 0; s < N; s++) {
                p[s] = tmp[s].I;
            }
        }
        sdr_free(tmp);
    }
    // prefix sums of code banks (bias correction in sdr_corr_std())
    trk->code_sum = (int32_t *)sdr_malloc(sizeof(int32_t) * (N + 1) *
        SDR_N_CODES * nb);
    for (int i = 0; i < SDR_N_CODES * nb; i++) {
        const int8_t *p = trk->code + i * N;
        int32_t *cs = trk->code_sum + i * (N + 1);
        for (int s = 0; s < N; s++) {
            cs[s+1] = cs[s] + p[s];
        }
    }
    trk->code_scale = sdr_code_scale(sig);
    return trk;
}

// free signal tracking --------------------------------------------------------
static void trk_free(sdr_trk_t *trk)
{
    if (!trk) return;
    sdr_free(trk->code);
    sdr_free(trk->code_sum);
    sdr_cpx_free(trk->code_fft);
    sdr_free(trk);
}

//------------------------------------------------------------------------------
//  Generate new receiver channel.
//
//  args:
//      sig      (I)  Signal ID as string ('L1CA', 'L1CB', 'L1CP', ....)
//      prn      (I)  PRN number
//      fs       (I)  Sampling frequency (Hz)
//      fi       (I)  IF carrier frequency (Hz)
//
//  return:
//      Receiver channel (NULL: error)
//
sdr_ch_t *sdr_ch_new(const char *sig, int prn, double fs, double fi)
{
    sdr_ch_t *ch = (sdr_ch_t *)sdr_malloc(sizeof(sdr_ch_t));
    
    ch->state = SDR_STATE_IDLE;
    ch->time = 0.0;
    sig_upper(sig, ch->sig);
    ch->prn = prn;
    sdr_sat_id(ch->sig, prn, ch->sat);
    if (!(ch->code = sdr_gen_code(sig, prn, &ch->len_code)) ||
        !(ch->sec_code = sdr_sec_code(sig, prn, &ch->len_sec_code))) {
        sdr_free(ch);
        return NULL;
    }
    ch->sec_code2 = NULL;
    ch->len_sec_code2 = 0;
    if (!strcmp(ch->sig, "E5ABQ") &&
        !(ch->sec_code2 = sdr_sec_code("E5BQ", prn, &ch->len_sec_code2))) {
        sdr_free(ch);
        return NULL;
    }
    ch->fc = sdr_shift_freq(sig, prn, sdr_sig_freq(sig));
    ch->fs = fs;
    ch->fi = sdr_shift_freq(sig, prn, fi);
    ch->T = sdr_code_cyc(sig);
    ch->N = (int)(fs * ch->T);
    ch->fd = ch->coff = ch->phi = ch->adr = ch->cn0 = ch->pli = 0.0;
    ch->lock = ch->lost = ch->lost_cnt = ch->pli_valid = 0;
    ch->costas = strcmp(ch->sig, "L6D") && strcmp(ch->sig, "L6E");
    ch->pilot = sdr_sig_pilot(ch->sig);
    ch->obs_idx = -1;
    ch->acq = acq_new(ch->sig, ch->prn, ch->code, ch->len_code, ch->T, fs,
        ch->N);
    ch->trk = trk_new(ch->sig, ch->prn, ch->code, ch->len_code, ch->T, fs);
    ch->nav = sdr_nav_new();
    ch->data = (sdr_cpx16_t *)sdr_malloc(sizeof(sdr_cpx16_t) * ch->N);
    sdr_mutex_init(&ch->mtx);
    return ch;
}

//------------------------------------------------------------------------------
//  Free receiver channel.
//
//  args:
//      ch       (I) Receiver channel
//
//  return:
//      none
//
void sdr_ch_free(sdr_ch_t *ch)
{
    if (!ch) return;
    acq_free(ch->acq);
    trk_free(ch->trk);
    sdr_nav_free(ch->nav);
    sdr_free(ch->data);
    sdr_cpx_free(ch->corr);
    sdr_free(ch);
}

// initialize signal tracking --------------------------------------------------
static void trk_init(sdr_trk_t *trk)
{
    trk->err_phas = trk->err_code = 0.0;
    trk->phas_acc = trk->code_int = 0.0;
    trk->sec_sync = trk->sec_pol = 0;
    trk->csk_ref = -1;
    trk->sumP = trk->sumN = trk->sumVE = trk->sumVL = 0.0;
    trk->sumPs = trk->sumD = 0.0;
    trk->Cs[0] = trk->Cs[1] = 0.0f;
    trk->coh_n = 0;
    memset(trk->C, 0, sizeof(sdr_cpx_t) * SDR_MAX_CORR);
    trk->C0[0] = trk->C0[1] = trk->C1[0] = trk->C1[1] = 0.0;
    memset(trk->P, 0, sizeof(sdr_cpx_t) * SDR_N_HIST);
    memset(trk->sumC, 0, sizeof(double) * SDR_MAX_CORR);
    memset(trk->sumI, 0, sizeof(double) * SDR_MAX_CORR);
    memset(trk->aveP, 0, sizeof(double) * SDR_MAX_CORR);
    memset(trk->aveI, 0, sizeof(double) * SDR_MAX_CORR);
}

// initial carrier NCO phase ----------------------------------------------------
static double init_phi(double fi, double time)
{
    // phi = frac(fi * time) on the common receiver time origin to cancel the
    // start-time dependent fractional carrier phase bias between CHs
    // (time = cycle count * SDR_CYC; split the product to avoid precision
    // loss for large time)
    int64_t n = (int64_t)floor(time / SDR_CYC + 0.5);
    int64_t s = n / 1000, r = n % 1000;
    int64_t k = ((int64_t)floor(fi) * r) % 1000; // floor(fi) * s: integer
    double fi_f = fi - floor(fi);
    double phi = fi_f * s + (fi_f * r + k) * 1e-3;
    return phi - floor(phi);
}

// start tracking --------------------------------------------------------------
static void start_track(sdr_ch_t *ch, double time, double fd, double coff,
    double cn0)
{
    ch->state = SDR_STATE_LOCK;
    ch->time = time;
    ch->lock = 0;
    ch->fd = fd;
    ch->coff = coff;
    ch->phi = init_phi(ch->fi, time);
    ch->adr = 0.0;
    ch->cn0 = cn0;
    ch->pli = 1.0;
    ch->lost_cnt = 0;
    ch->pli_valid = 0;
    ch->week = 0;
    ch->tow = -1;
    ch->tow_v = 0;
    trk_init(ch->trk);
    sdr_nav_init(ch->nav);
}

// search signal ---------------------------------------------------------------
static void search_sig(sdr_ch_t *ch, double time, const sdr_buff_t *buff,
    int ix)
{
    float *fds = ch->acq->fds;
    float *fd_ext_bins = NULL;
    int assisted = ch->acq->fd_ext_valid;
    int n = ch->acq->len_fds;
    
    if (assisted) { // assist by external Doppler
        int nw = (ch->acq->fd_ext_n > 0) ? ch->acq->fd_ext_n : 3;
        fd_ext_bins = (float *)sdr_malloc(sizeof(float) * nw);
        for (int k = 0; k < nw; k++) {
            fd_ext_bins[k] = (float)(ch->acq->fd_ext
                + (k - nw/2) * 0.5 / ch->T);
        }
        fds = fd_ext_bins;
        n = nw;
    }
    if (!ch->acq->P_sum || ch->acq->n_sum == 0) {
        sdr_free(ch->acq->P_sum);
        ch->acq->P_sum = (float *)sdr_malloc(sizeof(float) * 2 * ch->N * n);
    }
    // parallel code search and non-coherent integration
    sdr_search_code(ch->acq->code_fft, ch->T, buff, ix, 2 * ch->N, ch->fs,
        ch->fi, fds, n, ch->acq->P_sum);
    ch->acq->n_sum++;
    
    double t_acq = assisted ? sdr_t_acq_ext : sdr_t_acq;
    if (ch->acq->n_sum * ch->T >= t_acq) {
        int idx[2];
        
        // search max correlation power
        float cn0 = sdr_corr_max(ch->acq->P_sum, 2 * ch->N, ch->N, n, ch->T,
            idx);
        
        double thres = sdr_thres_cn0_l;
        if (assisted) {
            double lost = !strncmp(ch->sig, "L6", 2) ? THRES_CN0_L6 :
                sdr_thres_cn0_u;
            thres = MAX(sdr_thres_cn0_ext, lost + 1.0);
        }
        if (cn0 >= thres) {
            double fd = sdr_fine_dop(ch->acq->P_sum, 2 * ch->N, fds, n, idx);
            double coff = idx[1] / ch->fs;
            start_track(ch, time, fd, coff, cn0);
            sdr_log(3, "$LOG,%.3f,%s,%d,SIGNAL FOUND (%.1f,%.1f,%.7f)", ch->time,
                ch->sig, ch->prn, cn0, fd, coff * 1e3);
        } else {
            sdr_sleep_msec(ACQ_INT);
            ch->state = SDR_STATE_IDLE;
            sdr_log(4, "$LOG,%.3f,%s,%d,SIGNAL NOT FOUND (%.1f)", time, ch->sig,
                ch->prn, cn0);
        }
        sdr_free(ch->acq->P_sum);
        ch->acq->P_sum = NULL;
        ch->acq->n_sum = 0;
        ch->acq->fd_ext = 0.0;
        ch->acq->fd_ext_valid = 0;
        ch->acq->fd_ext_n = 0;
    }
    sdr_free(fd_ext_bins);
}

// sync and remove secondary code ----------------------------------------------
static void sync_sec_code(sdr_ch_t *ch, int N)
{
    if (ch->trk->sec_sync == 0) {
        float P = 0.0, R = 0.0;
        for (int i = 0; i < N; i++) {
            int j = i % ch->len_sec_code;
            P += ch->trk->P[SDR_N_HIST-N+i][0] * ch->sec_code[j] / N;
            R += fabsf(ch->trk->P[SDR_N_HIST-N+i][0]) / N;
        }
        if (fabsf(P) >= THRES_SEC_RATIO * R && R >= THRES_SYNC) {
            ch->trk->sec_sync = ch->lock;
            ch->trk->sec_pol = (P > 0.0f) ? 1 : -1;
        }
    } else if ((ch->lock - ch->trk->sec_sync) % N == 0) {
        float P = 0.0;
        for (int i = 0; i < N; i++) {
            P += ch->trk->P[SDR_N_HIST-N+i][0] / N;
        }
        if (fabsf(P) < THRES_LOST) {
            ch->trk->sec_sync = ch->trk->sec_pol = 0;
            ch->trk->e5b_pol = ch->trk->e5b_cnt = 0; // re-calibrate E5b pol.
            ch->trk->e5b_score = 0.0;
        }
    }
    if (ch->trk->sec_sync > 0) {
        int idx = (ch->lock - ch->trk->sec_sync - 1 + N) % N % ch->len_sec_code;
        int8_t C = ch->sec_code[idx] * ch->trk->sec_pol;
        for (int i = 0; i < ch->trk->npos + ch->trk->nposx; i++) {
            ch->trk->C[i][0] *= C;
            ch->trk->C[i][1] *= C;
        }
        ch->trk->P[SDR_N_HIST-1][0] *= C;
        ch->trk->P[SDR_N_HIST-1][1] *= C;
    }
}

// FLL ------------------------------------------------------------------------
static void FLL(sdr_ch_t *ch)
{
    if (ch->lock >= 2) {
        double IP1 = ch->trk->C[0][0];
        double QP1 = ch->trk->C[0][1];
        double IP2 = ch->trk->C0[0];
        double QP2 = ch->trk->C0[1];
        double dot   = IP1 * IP2 + QP1 * QP2;
        double cross = IP1 * QP2 - QP1 * IP2;
        if (dot != 0.0) {
            double B = ch->lock * ch->T < T_FPULLIN_W ? sdr_b_fll_w : sdr_b_fll_n;
            double err_freq = ch->costas ? atan(cross / dot) : atan2(cross, dot);
            ch->fd -= B / 0.25 * err_freq / DPI;
        }
    }
    ch->trk->C0[0] = ch->trk->C[0][0];
    ch->trk->C0[1] = ch->trk->C[0][1];
}

// PLL (3rd-order, a3=1.1, b3=2.4, Bn=W/0.7845) --------------------------------
static void PLL(sdr_ch_t *ch, double IP, double QP, double dt, int costas)
{
    if (IP != 0.0 || (!costas && QP != 0.0)) {
        double err_phas = (costas ? atan(QP / IP) : atan2(QP, IP)) / DPI;
        double W = sdr_b_pll / 0.7845;
        ch->trk->phas_acc += W * W * W * err_phas * dt;
        ch->fd += 2.4 * W * (err_phas - ch->trk->err_phas) +
            1.1 * W * W * err_phas * dt + ch->trk->phas_acc * dt;
        ch->trk->err_phas = err_phas;
    }
}

// DLL (2nd-order, zeta=0.707, Bn=W/0.53) --------------------------------------
static void DLL(sdr_ch_t *ch)
{
    int N = MAX(1, (int)(sdr_t_dll / ch->T));
    double sgn = ch->trk->C[0][0] >= 0.0 ? 1.0 : -1.0; // sign of IP (data wipe-off)
    for (int i = 0; i < ch->trk->npos + ch->trk->nposx; i++) {
        ch->trk->sumC[i] += SQR(ch->trk->C[i][0]) + SQR(ch->trk->C[i][1]);
        ch->trk->sumI[i] += ch->trk->C[i][0] * sgn;
    }
    if (ch->lock % N == 0) {
        double E = sqrt(ch->trk->sumC[1]);
        double L = sqrt(ch->trk->sumC[2]);
        if (E + L > 0.0) {
            double err_code = (E - L) / (E + L) * 0.5f * ch->T / ch->len_code;
            double W = sdr_b_dll / 0.53;
            double dt = ch->T * N;
            ch->trk->code_int += W * W * err_code * dt;
            ch->coff -= (1.414 * W * err_code + ch->trk->code_int) * dt;
            ch->trk->err_code = err_code;
        }
        for (int i = 0; i < ch->trk->npos + ch->trk->nposx; i++) {
            ch->trk->aveP[i] = ch->trk->sumC[i] / N;
            ch->trk->aveI[i] = ch->trk->sumI[i] / N;
            ch->trk->sumC[i] = 0.0;
            ch->trk->sumI[i] = 0.0;
        }
    }
}

// bump-jump for BOC modulation ------------------------------------------------
static void bump_jump(sdr_ch_t *ch)
{
    double coff = ch->coff;
    double step;
    if (!strcmp(ch->sig, "E5ABQ")) {
        step = ch->T / ch->len_code / 3;
    } else if (!strcmp(ch->sig, "G1OCP") || !strcmp(ch->sig, "G2OCP")) {
        step = ch->T / ch->len_code / 2;
    } else {
        step = ch->T / ch->len_code;
    }
    if (ch->trk->sumVL > sdr_bump_k * ch->trk->sumP &&
        ch->trk->sumP > sdr_bump_k * ch->trk->sumVE) {
        ch->coff += step;
    } else if (ch->trk->sumVE > sdr_bump_k * ch->trk->sumP &&
        ch->trk->sumP > sdr_bump_k * ch->trk->sumVL) {
        ch->coff -= step;
    }
    if (ch->coff != coff) {
        sdr_log(3, "$LOG,%.3f,%s,%d,FALSE LOCK (%.2f,%.2f,%.2f) COFF (%.7f->%.7f) K=%.2f",
            ch->time, ch->sig, ch->prn, ch->trk->sumVE, ch->trk->sumP,
            ch->trk->sumVL, coff * 1e3, ch->coff * 1e3, sdr_bump_k);
    }
    ch->trk->sumVE = ch->trk->sumVL = 0.0;
}

// update C/N0 and carrier lock indicator (returns 1 at decision window) --------
static int CN0(sdr_ch_t *ch, double IPs, double QPs, int pli_upd)
{
    ch->trk->sumP +=
        SQR(ch->trk->P[SDR_N_HIST-1][0]) + SQR(ch->trk->P[SDR_N_HIST-1][1]);
    if (pli_upd) {
        ch->trk->sumD += SQR(IPs) - SQR(QPs);
        ch->trk->sumPs += SQR(IPs) + SQR(QPs);
    }
    ch->trk->sumN += SQR(ch->trk->C[3][0]) + SQR(ch->trk->C[3][1]);
    if (ch->trk->npos >= 6) {
        ch->trk->sumVE += SQR(ch->trk->C[4][0]) + SQR(ch->trk->C[4][1]);
        ch->trk->sumVL += SQR(ch->trk->C[5][0]) + SQR(ch->trk->C[5][1]);
    }
    if (ch->lock % (int)(T_CN0 / ch->T) != 0) return 0;
    
    if (ch->trk->sumPs > 0.0) {
        ch->pli = ch->trk->sumD / ch->trk->sumPs; // cos 2*phi in [-1,1]
        ch->pli_valid = 1;
    }
    if (ch->trk->sumN > 0.0) {
        double S = MAX(ch->trk->sumP - ch->trk->sumN,
            0.01 * ch->trk->sumN);
        double cn0 = 10.0 * log10(S / ch->trk->sumN / ch->T);
        ch->cn0 += FILT_CN0 * (cn0 - ch->cn0);
    }
    if (ch->trk->npos >= 6) {
        bump_jump(ch);
    }
    ch->trk->sumP = ch->trk->sumN = 0.0;
    ch->trk->sumPs = ch->trk->sumD = 0.0;
    return 1;
}

// test signal lost (see doc/update_lost_decision.md) --------------------------
static void test_lost(sdr_ch_t *ch)
{
    double t_cn0 = !strncmp(ch->sig, "L6", 2) ? THRES_CN0_L6 : sdr_thres_cn0_u;
    int bad = ch->cn0 < t_cn0 ||
        (ch->costas && ch->pli_valid && ch->pli < sdr_thres_pli);
    ch->lost_cnt = bad ? ch->lost_cnt + 1 : 0;
    if (ch->lost_cnt < sdr_lost_th) return;
    
    ch->state = SDR_STATE_IDLE;
    ch->lock = 0;
    ch->trk->sec_sync = ch->trk->sec_pol = 0;
    ch->nav->ssync = ch->nav->fsync = ch->nav->rev = 0;
    ch->lost++;
    sdr_sat_id(ch->sig, ch->prn, ch->sat); // for GLONASS FDMA
    sdr_log(3, "$LOG,%.3f,%s,%d,SIGNAL LOST (%.1f,%.2f)", ch->time, ch->sig,
        ch->prn, ch->cn0, ch->pli);
}

// decode L6 CSK by chip-domain FFT correlation (returns code shift (chips)) ----
static int CSK(sdr_ch_t *ch, const sdr_cpx16_t *IQ, double coff_frc)
{
    static __thread sdr_cpx_t *cpx = NULL;
    int W = CSK_WIN, M = CSK_NFFT;
    if (!cpx) {
        cpx = sdr_cpx_malloc(M * 2);
    }
    // bin carrier-mixed IF data to code chips (TDM slots: L6D=0, L6E=1)
    int L = ch->len_code / 2;
    sdr_bin_csk(IQ, ch->N, ch->len_code, !strcmp(ch->sig, "L6E") ? 1 : 0,
        coff_frc, cpx);
    memset(cpx + L, 0, sizeof(sdr_cpx_t) * (M - L));
    // correlate with periodically extended code (X[s] = cpx[M - W + s])
    if (sdr_cpx_fft(cpx, M, SDR_FFT_FORWARD, cpx + M)) {
        sdr_cpx_mul(cpx + M, ch->trk->code_fft, M, 1.0f / M / M, cpx + M);
        (void)sdr_cpx_fft(cpx + M, M, SDR_FFT_BACKWARD, cpx);
    }
    // narrow peak search range by code shift reference after L6 frame sync
    int i0 = -255, i1 = 255, ref = ch->trk->csk_ref;
    if (ch->nav->fsync > 0 && ref >= 0) {
        i0 = ref - 255;
        i1 = ref;
    }
    // detect correlation peak
    double P_max = 0.0;
    int ix = i0;
    for (int i = i0; i <= i1; i++) {
        const float *c = cpx[M - W + i];
        double P = SQR(c[0]) + SQR(c[1]); // squared magnitude
        if (P <= P_max) continue;
        P_max = P;
        ix = i;
    }
    // add CSK symbol to buffer
    uint8_t sym = (uint8_t)(255 - ix % 256);
    sdr_add_buff(ch->nav->syms, SDR_MAX_NSYM, &sym, sizeof(sym));

    // update code shift reference (ix = ref - symbol, see decode_L6_frame())
    if (ch->nav->fsync <= 0) {
        ch->trk->csk_ref = -1;
    } else if (ref < 0) {
        int off = (int)floor(ch->nav->coff * 10230 / ch->T + 0.5) + 1;
        ref = ix + (uint8_t)(sym + off);
        ch->trk->csk_ref = (ref < 0 || ref > 255) ? -1 : ref;
    }
    return ix;
}

// update TOW ------------------------------------------------------------------
static void update_tow(sdr_ch_t *ch, double sec)
{
    if (ch->tow < 0) return;
    ch->tow += (int)floor(sec / 1e-3 + 0.5);
    
    // the week has to follow the TOW rollover until the next week is decoded
    if (ch->tow >= 86400 * 7 * 1000) {
        ch->tow -= 86400 * 7 * 1000;
        if (ch->week > 0) ch->week++;
    } else if (ch->tow < 0) {
        ch->tow += 86400 * 7 * 1000;
        if (ch->week > 0) ch->week--;
    }
}

// adjust code offset (0 <= ch->coff < ch->T) ----------------------------------
static void adj_coff(sdr_ch_t *ch)
{
    if (ch->coff >= ch->T) {
        ch->coff -= ch->T;
        update_tow(ch, -ch->T);
        ch->lock--;
        memmove(ch->trk->P + 1, ch->trk->P, sizeof(sdr_cpx_t) *
            (SDR_N_HIST - 1));
    } else if (ch->coff < 0.0) {
        ch->coff += ch->T;
        update_tow(ch, ch->T);
        ch->lock++;
        memmove(ch->trk->P, ch->trk->P + 1, sizeof(sdr_cpx_t) *
            (SDR_N_HIST - 1));
    } else {
        return;
    }
    if (!strcmp(ch->sig, "E5ABQ")) { // re-calibrate E5b combination polarity
        ch->trk->e5b_pol = ch->trk->e5b_cnt = 0;
        ch->trk->e5b_score = 0.0;
    }
}

// track signal ----------------------------------------------------------------
static void track_sig(sdr_ch_t *ch, double time, const sdr_buff_t *buff, int ix)
{
    double tau = time - ch->time;   // time interval (s) 
    double fc = ch->fi + ch->fd;    // IF carrier frequency with Doppler (Hz)
    ch->adr += ch->fd * tau;        // accumulated Doppler (cyc)
    ch->coff -= ch->fd / ch->fc * tau; // carrier-aided code offset (s)
    ch->phi += fc * tau;
    ch->phi -= floor(ch->phi); // 0 <= phi < 1 (cyc)
    ch->time = time;
    
    // adjust code offset
    adj_coff(ch);
    
    if (!strcmp(ch->sig, "L6D") || !strcmp(ch->sig, "L6E")) {
        int i = (int)floor(ch->coff * ch->fs);
        
        // mix carrier
        sdr_mix_carr(buff, ix + i, ch->N, ch->fs, fc, ch->phi + fc * i / ch->fs,
            ch->data);
        
        // decode L6 CSK
        int csk = CSK(ch, ch->data, ch->coff * ch->fs - i);
        
        // standard correlator with CSK-shifted code (carrier mixing fused)
        // (fixed code polarity: window is symbol-aligned and circular)
        double R = (double)ch->N / (ch->len_code / 2); // samples / chip
        sdr_cpx_t C1[2];
        sdr_corr_std(buff, ix + i, ch->N, ch->fs, fc,
            ch->phi + fc * i / ch->fs, ch->trk->code, ch->trk->code_sum,
            NULL, NULL, ch->trk->code_scale, ch->coff * ch->fs - i + csk * R,
            ch->trk->pos, ch->trk->npos + ch->trk->nposx, 1, ch->trk->C, C1);
        
        // add P correlator outputs to history 
        sdr_add_buff(ch->trk->P, SDR_N_HIST, ch->trk->C[0], sizeof(sdr_cpx_t));
    } else {
        sdr_cpx_t C1[2];
        
        if (!strcmp(ch->sig, "E5ABQ")) {
            // correlate E5aQ/E5bQ sidebands separately and combine them
            corr_e5abq(ch, buff, ix, fc, C1);
        } else {
            // standard correlator (carrier mixing fused)
            sdr_corr_std(buff, ix, ch->N, ch->fs, fc, ch->phi, ch->trk->code,
                ch->trk->code_sum, NULL, NULL, ch->trk->code_scale,
                ch->coff * ch->fs, ch->trk->pos,
                ch->trk->npos + ch->trk->nposx, 0, ch->trk->C, C1);
        }
        for (int i = 0; i < 2; i++) {
            C1[0][i] = (C1[0][i] + ch->trk->C1[i]) / ch->N;
            ch->trk->C1[i] = C1[1][i];
        }
        sdr_add_buff(ch->trk->P, SDR_N_HIST, C1[0], sizeof(sdr_cpx_t));
    }
    update_tow(ch, ch->T);
    ch->lock++;
    
    // sync and remove secondary code
    if (ch->len_sec_code >= 2 && ch->lock * ch->T >= T_NPULLIN) {
        sync_sec_code(ch, ch->len_sec_code);
    }
    // FLL/PLL, DLL and update C/N0
    double IP = ch->trk->C[0][0];
    double QP = ch->trk->C[0][1];
    double IPs = ch->trk->P[SDR_N_HIST-1][0];
    double QPs = ch->trk->P[SDR_N_HIST-1][1];
    int pli_upd = 1;
    if (ch->lock * ch->T <= T_FPULLIN) {
        FLL(ch);
        ch->trk->Cs[0] = ch->trk->Cs[1] = 0.0f;
        ch->trk->coh_n = 0;
    } else if (ch->pilot && ch->trk->sec_sync > 0) {
        int K = MAX(1, (int)(sdr_t_coh / ch->T + 0.5));
        double dt = K * ch->T;
        if (sdr_b_pll * dt < 0.4) {
            ch->trk->Cs[0] += (float)IP;
            ch->trk->Cs[1] += (float)QP;
            ch->trk->coh_n++;
            pli_upd = 0;
            if (ch->trk->coh_n >= K) {
                IPs = ch->trk->Cs[0];
                QPs = ch->trk->Cs[1];
                PLL(ch, IPs, QPs, dt, 0);
                ch->trk->Cs[0] = ch->trk->Cs[1] = 0.0f;
                ch->trk->coh_n = 0;
                pli_upd = 1;
            }
        } else {
            PLL(ch, IP, QP, ch->T, ch->costas);
            ch->trk->Cs[0] = ch->trk->Cs[1] = 0.0f;
            ch->trk->coh_n = 0;
        }
    } else {
        PLL(ch, IP, QP, ch->T, ch->costas);
        ch->trk->Cs[0] = ch->trk->Cs[1] = 0.0f;
        ch->trk->coh_n = 0;
    }
    DLL(ch);
    int cn0_upd = CN0(ch, IPs, QPs, pli_upd);
    
    // decode navigation data
    if (ch->lock * ch->T >= T_NPULLIN) {
        sdr_nav_decode(ch);
    }
    // test signal lost (only on a C/N0/PLI decision window)
    if (cn0_upd) {
        test_lost(ch);
    }
}

//------------------------------------------------------------------------------
//  Update a receiver channel. A receiver channel is a state machine which has
//  the following internal states indicated as ch.state. By calling the function,
//  the receiver channel search and track GNSS signals and decode navigation
//  data in the signals. The results of the signal acquisition, tracking and
//  navigation data decoding are output as log messages. The internal status are
//  also accessed as object instance variables of the receiver channel after
//  calling the function. The function should be called in the cycle of GNSS
//  signal code with 2-cycle samples of digitized IF data (which are overlapped
//  between previous and current). 
//
//    SDR_STATE_SRCH : signal acquisition state
//    SDR_STATE_LOCK : signal tracking state
//    SDR_STATE_IDLE : waiting for a next signal acquisition cycle
//
//  args:
//      ch       (I)  Receiver channel
//      time     (I)  Sampling time of the end of digitized IF data (s)
//      buff     (I)  IF data buffer
//      ix       (I)  index of IF data buffer
//
//  return:
//      none
//
void sdr_ch_update(sdr_ch_t *ch, double time, const sdr_buff_t *buff, int ix)
{
    if (ch->state == SDR_STATE_SRCH) {
        search_sig(ch, time, buff, ix);
    } else if (ch->state == SDR_STATE_LOCK) {
        sdr_mutex_lock(&ch->mtx);
        track_sig(ch, time, buff, ix);
        sdr_mutex_unlock(&ch->mtx);
    }
}

// set receiver channel correlator ---------------------------------------------
void sdr_ch_set_corr(sdr_ch_t *ch, int nposx, double width)
{
    nposx = MIN(nposx, SDR_N_CORRX);
    
    sdr_mutex_lock(&ch->mtx);
    for (int i = 0, j = ch->trk->npos; i < nposx; i++, j++) {
        ch->trk->pos[j] = (double)(i - (nposx - 1) / 2) / nposx * width * ch->fs;
    }
    ch->trk->nposx = nposx;
    sdr_mutex_unlock(&ch->mtx);
}

// get receiver correlator status ----------------------------------------------
int sdr_ch_corr_stat(sdr_ch_t *ch, double *stat, double *pos, sdr_cpx_t *C,
    double *P, double *I)
{
    sdr_mutex_lock(&ch->mtx);
    int npos = ch->trk->npos + ch->trk->nposx;
    stat[0] = ch->state;
    stat[1] = ch->fs;
    stat[2] = ch->lock * ch->T;
    stat[3] = ch->cn0;
    stat[4] = ch->coff * 1e3;
    stat[5] = ch->fd;
    stat[6] = ch->trk->npos;
    memcpy(pos, ch->trk->pos, sizeof(double) * npos);
    memcpy(C, ch->trk->C, sizeof(sdr_cpx_t) * npos);
    memcpy(P, ch->trk->aveP, sizeof(double) * npos);
    if (I) memcpy(I, ch->trk->aveI, sizeof(double) * npos);
    sdr_mutex_unlock(&ch->mtx);
    return npos;
}

// get receiver correlator history ---------------------------------------------
int sdr_ch_corr_hist(sdr_ch_t *ch, double tspan, double *stat, sdr_cpx_t *P)
{
    sdr_mutex_lock(&ch->mtx);
    int n = MIN((int)(tspan / ch->T), SDR_N_HIST);
    stat[0] = ch->time;
    stat[1] = ch->T;
    memcpy(P, ch->trk->P + SDR_N_HIST - n, sizeof(sdr_cpx_t) * n);
    sdr_mutex_unlock(&ch->mtx);
    return n;
}
