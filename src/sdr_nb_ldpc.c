//
//  Pocket SDR C Library - NB-LDPC Decoder Functions
//
//  References:
//  [1] BeiDou Navigation Satellite System Signal In Space Interface Control
//      Document Open Service Signal B1C (Version 1.0), December, 2017
//  [2] E.Li et al., Trellis-based Extended Min-Sum algorithm for non-binary LDPC
//      codes and its hardware structure, IEEE Trans. on Communications, 2013
//  [3] A.Voicila et al., Low-complexity decoding for non-binary LDPC codes in
//      high order fields, IEEE Trans. on Communications, 2010
//  [4] D.Declercq and M.Fossorier, Decoding algorithms for nonbinary LDPC codes
//      over GF(q), IEEE Trans. on Communications, 2007
//
//  Author:
//  T.TAKASU
//
//  History:
//  2024-01-25  1.0  port sdr_nb_ldpc.py to C
//  2026-06-13  1.1  hard-decision -> soft-decision
//  2026-10-26  1.2  redesign decoder: extrinsic EMS (NM=8) with compensation
//                   offset, input LLR normalization, cached Tanner graph and
//                   layered check node schedule; add sum-product retry on EMS
//                   failure
//
#include <math.h>
#include "pocket_sdr.h"

// constants --------------------------------------------------------------------
#define N_GF        6           // number of GF(q) bits
#define Q_GF        (1<<N_GF)   // number of GF(q) elements
#define MAX_ITER    15          // max number of iterations
#define NM_EMS      8           // LLR truncation size of EMS
#ifndef EMS_OFFSET
#define EMS_OFFSET  2.0f        // compensation offset of truncated LLRs ([3])
#endif
#ifndef LLR_AVE
#define LLR_AVE     4.0f        // normalized average of input bit |LLR|
#endif
#ifndef MAX_ITER_SP
#define MAX_ITER_SP 15          // max iterations of sum-product retry (0: off)
#endif
#define P_FLOOR     1e-15       // floor of symbol probability relative to max
#define DC_H        4           // row weight of H-matrix
#define MAX_H_M     128         // max rows of H-matrix
#define MAX_H_N     256         // max columns of H-matrix
#define MAX_VDEG    8           // max degree of variable node
#define MAX_GRAPH   8           // max number of cached Tanner graphs
#define MAX_LLR     1e30f       // LLR of impossible GF(q) element

// type definitions --------------------------------------------------------------
typedef struct {                // truncated LLR message (llr ascending, llr[0]=0)
    uint8_t sym[NM_EMS];
    float llr[NM_EMS];
} nb_msg_t;

typedef struct {                // Tanner graph
    const uint8_t (*H_idx)[4];  // cache key
    int m, n, ne;               // numbers of checks, variables and edges
    uint16_t *ce_var;           // edge (check-major) -> variable index
    uint8_t *ce_h, *ce_hinv;    // edge -> H element and its inverse
    int *ve_ptr;                // variable -> index range in ve_edge
    uint16_t *ve_edge;          // variable-major edge list
} nb_graph_t;

// GF(q) tables -----------------------------------------------------------------
static const uint8_t GF_VEC[Q_GF] = { // power -> vector ([1])
     1,  2,  4,  8, 16, 32,  3,  6, 12, 24, 48, 35,  5, 10, 20, 40,
    19, 38, 15, 30, 60, 59, 53, 41, 17, 34,  7, 14, 28, 56, 51, 37,
     9, 18, 36, 11, 22, 44, 27, 54, 47, 29, 58, 55, 45, 25, 50, 39,
    13, 26, 52, 43, 21, 42, 23, 46, 31, 62, 63, 61, 57, 49, 33
};
static const uint8_t GF_POW[Q_GF] = { // vector -> power ([1])
     0,  0,  1,  6,  2, 12,  7, 26,  3, 32, 13, 35,  8, 48, 27, 18,
     4, 24, 33, 16, 14, 52, 36, 54,  9, 45, 49, 38, 28, 41, 19, 56,
     5, 62, 25, 11, 34, 31, 17, 47, 15, 23, 53, 51, 37, 44, 55, 40,
    10, 61, 46, 30, 50, 22, 39, 43, 29, 60, 42, 21, 20, 59, 57, 58
};
static uint8_t GF_MUL[Q_GF][Q_GF] = {{0}}; // multiply
static uint8_t GF_INV[Q_GF] = {0};         // inverse

// lock for GF(q) table and Tanner graph cache initialization ---------------------
static sdr_mutex_t init_mtx = SDR_MUTEX_INIT;

// get index of min value in array -----------------------------------------------
static int argmin(const float *L, int n)
{
    int j = 0;
    for (int i = 1; i < n; i++) {
        if (L[i] < L[j]) j = i;
    }
    return j;
}

// initialize GF(q) tables --------------------------------------------------------
static void init_table(void)
{
    if (GF_MUL[1][1]) {
        return;
    }
    for (int i = 1; i < Q_GF; i++) {
        for (int j = 1; j < Q_GF; j++) {
            GF_MUL[i][j] = GF_VEC[(GF_POW[i] + GF_POW[j]) % (Q_GF - 1)];
        }
        GF_INV[i] = GF_VEC[(Q_GF - 1 - GF_POW[i]) % (Q_GF - 1)];
    }
}

// convert soft binary codes to GF(q) hard-decision codes -------------------------
static void soft2gf(const uint8_t *syms, int n, uint8_t *code)
{
    memset(code, 0, n);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < N_GF; j++) {
            code[i] = (code[i] << 1) + (syms[i*N_GF+j] >= 128 ? 1 : 0);
        }
    }
}

// convert GF(q) codes to binary codes --------------------------------------------
static void gf2bin(const uint8_t *code, int n, uint8_t *syms)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < N_GF; j++) {
            syms[i*N_GF+j] = (code[i] >> (N_GF - 1 - j)) & 1;
        }
    }
}

// generate Tanner graph ----------------------------------------------------------
static int gen_graph(nb_graph_t *g, const uint8_t H_idx[][4],
    const uint8_t H_ele[][4], int m, int n)
{
    int ne = m * DC_H, vdeg[MAX_H_N] = {0};

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < DC_H; j++) {
            if (H_idx[i][j] >= n || !H_ele[i][j]) return 0;
            if (++vdeg[H_idx[i][j]] > MAX_VDEG) return 0;
        }
    }
    g->m = m;
    g->n = n;
    g->ne = ne;
    g->ce_var = (uint16_t *)sdr_malloc(sizeof(uint16_t) * ne * 2);
    g->ve_edge = g->ce_var + ne;
    g->ce_h = (uint8_t *)sdr_malloc(sizeof(uint8_t) * ne * 2);
    g->ce_hinv = g->ce_h + ne;
    g->ve_ptr = (int *)sdr_malloc(sizeof(int) * (n + 1));

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < DC_H; j++) {
            g->ce_var[i*DC_H+j] = H_idx[i][j];
            g->ce_h[i*DC_H+j] = H_ele[i][j];
            g->ce_hinv[i*DC_H+j] = GF_INV[H_ele[i][j]];
            g->ve_ptr[H_idx[i][j]+1]++;
        }
    }
    for (int i = 0; i < n; i++) {
        g->ve_ptr[i+1] += g->ve_ptr[i];
    }
    int pos[MAX_H_N];
    memcpy(pos, g->ve_ptr, sizeof(int) * n);
    for (int i = 0; i < ne; i++) {
        g->ve_edge[pos[g->ce_var[i]]++] = (uint16_t)i;
    }
    return 1;
}

// get cached Tanner graph --------------------------------------------------------
static const nb_graph_t *get_graph(const uint8_t H_idx[][4],
    const uint8_t H_ele[][4], int m, int n)
{
    static nb_graph_t graphs[MAX_GRAPH] = {{0}};

    if (m <= 0 || m > MAX_H_M || n <= 0 || n > MAX_H_N) return NULL;

    for (int i = 0; i < MAX_GRAPH; i++) {
        if (graphs[i].H_idx == H_idx && graphs[i].m == m && graphs[i].n == n) {
            return graphs + i;
        }
        if (!graphs[i].H_idx) {
            if (!gen_graph(graphs + i, H_idx, H_ele, m, n)) return NULL;
            graphs[i].H_idx = H_idx;
            return graphs + i;
        }
    }
    return NULL;
}

// NB-LDPC parity check -----------------------------------------------------------
static int check_parity(const nb_graph_t *g, const uint8_t *code)
{
    for (int i = 0; i < g->ne; i += DC_H) {
        uint8_t s = 0;
        for (int j = 0; j < DC_H; j++) {
            s ^= GF_MUL[g->ce_h[i+j]][code[g->ce_var[i+j]]];
        }
        if (s) return 0;
    }
    return 1;
}

// initialize posterior LLRs by soft binary codes with scale normalization --------
static void init_LLR(const uint8_t *syms, int n, float *P)
{
    double ave = 0.0;

    for (int i = 0; i < n * N_GF; i++) {
        ave += fabs(syms[i] - 127.5);
    }
    float scale = LLR_AVE / (float)(ave / (n * N_GF)); // min of ave = 0.5

    for (int i = 0; i < n; i++) {
        float absL[N_GF];
        uint8_t hard = 0;

        for (int j = 0; j < N_GF; j++) {
            uint8_t sym = syms[i*N_GF+j];
            hard = (hard << 1) + (sym >= 128 ? 1 : 0);
            absL[j] = fabsf(sym - 127.5f) * scale;
        }
        for (int x = 0; x < Q_GF; x++) {
            float cost = 0.0f;
            uint8_t diff = (uint8_t)x ^ hard;

            for (int j = 0; j < N_GF; j++) {
                if (diff & (1 << (N_GF - 1 - j))) cost += absL[j];
            }
            P[i*Q_GF+x] = cost;
        }
    }
}

// truncate LLRs to NM smallest elements (ascending, normalized to llr[0]=0) ------
static void trunc_LLR(const float *met, nb_msg_t *msg)
{
    int nk = 0;

    for (int x = 0; x < Q_GF; x++) {
        float L = met[x];

        if (nk >= NM_EMS && L >= msg->llr[NM_EMS-1]) continue;
        int k = nk < NM_EMS ? nk++ : NM_EMS - 1;
        for ( ; k > 0 && msg->llr[k-1] > L; k--) {
            msg->llr[k] = msg->llr[k-1];
            msg->sym[k] = msg->sym[k-1];
        }
        msg->llr[k] = L;
        msg->sym[k] = (uint8_t)x;
    }
    for (int k = NM_EMS - 1; k >= 0; k--) {
        msg->llr[k] -= msg->llr[0];
    }
}

// combine messages by extended-min-sum (out = min(u + w)) ([2],[3]) --------------
static void comb_msg(const nb_msg_t *u, const nb_msg_t *w, nb_msg_t *out)
{
    float best[Q_GF];

    for (int x = 0; x < Q_GF; x++) {
        best[x] = MAX_LLR;
    }
    for (int i = 0; i < NM_EMS; i++) {
        for (int j = 0; j < NM_EMS; j++) {
            uint8_t s = u->sym[i] ^ w->sym[j];
            float L = u->llr[i] + w->llr[j];

            if (L < best[s]) best[s] = L;
        }
    }
    trunc_LLR(best, out);
}

// update check node and posteriors by layered schedule ([3]) ---------------------
static void update_check(const nb_graph_t *g, int i, float *P, float *EC2V,
    uint8_t *code)
{
    nb_msg_t V2C[DC_H], C2V[DC_H], f1, b2;
    float met[Q_GF], ext[Q_GF];

    // extrinsic VN->CN messages in edge domain (y = h * x)
    for (int j = 0; j < DC_H; j++) {
        int e = i * DC_H + j;
        const float *Pv = P + g->ce_var[e] * Q_GF;
        const float *E = EC2V + e * Q_GF;
        const uint8_t *mul = GF_MUL[g->ce_h[e]];

        for (int x = 0; x < Q_GF; x++) {
            met[mul[x]] = Pv[x] - E[x];
        }
        trunc_LLR(met, V2C + j);
    }
    // CN->VN messages by forward-backward recursion
    comb_msg(V2C + 2, V2C + 3, &b2);
    comb_msg(V2C + 1, &b2, C2V);
    comb_msg(V2C + 0, &b2, C2V + 1);
    comb_msg(V2C + 0, V2C + 1, &f1);
    comb_msg(&f1, V2C + 3, C2V + 2);
    comb_msg(&f1, V2C + 2, C2V + 3);

    // update posteriors with offset compensation of truncated LLRs
    for (int j = 0; j < DC_H; j++) {
        int e = i * DC_H + j, v = g->ce_var[e];
        float *Pv = P + v * Q_GF, *E = EC2V + e * Q_GF;
        const uint8_t *inv = GF_MUL[g->ce_hinv[e]];
        float fill = C2V[j].llr[NM_EMS-1] + EMS_OFFSET;

        for (int x = 0; x < Q_GF; x++) {
            ext[x] = fill;
        }
        for (int k = 0; k < NM_EMS; k++) {
            ext[inv[C2V[j].sym[k]]] = C2V[j].llr[k];
        }
        for (int x = 0; x < Q_GF; x++) {
            Pv[x] += ext[x] - E[x];
            E[x] = ext[x];
        }
        code[v] = (uint8_t)argmin(Pv, Q_GF);
    }
}

// get index of max value in array -------------------------------------------------
static int argmax(const double *p, int n)
{
    int j = 0;
    for (int i = 1; i < n; i++) {
        if (p[i] > p[j]) j = i;
    }
    return j;
}

// Walsh-Hadamard transform (in-place, self-inverse up to factor Q) ----------------
static void walsh_trans(double *p)
{
    for (int h = 1; h < Q_GF; h *= 2) {
        for (int i = 0; i < Q_GF; i += h * 2) {
            for (int j = i; j < i + h; j++) {
                double a = p[j], b = p[j+h];
                p[j] = a + b;
                p[j+h] = a - b;
            }
        }
    }
}

// normalize symbol probabilities with scale-invariant floor ----------------------
static void norm_prob(double *p)
{
    double pmax = p[argmax(p, Q_GF)], sum = 0.0;

    if (pmax <= 0.0) { // all zero or roundoff negatives
        for (int x = 0; x < Q_GF; x++) {
            p[x] = 1.0 / Q_GF;
        }
        return;
    }
    double pmin = pmax * P_FLOOR; // also clears roundoff negatives

    for (int x = 0; x < Q_GF; x++) {
        if (p[x] < pmin) p[x] = pmin;
        sum += p[x];
    }
    for (int x = 0; x < Q_GF; x++) {
        p[x] /= sum;
    }
}

// decode NB-LDPC by sum-product with Walsh-domain convolution ([1],[4]) -----------
static int decode_SP(const nb_graph_t *g, const uint8_t *syms, uint8_t *code)
{
    int n = g->n, ne = g->ne, ok = 0;
    float *L = (float *)sdr_malloc(sizeof(float) * n * Q_GF);
    double *Pc = (double *)sdr_malloc(sizeof(double) * (n + ne * 2) * Q_GF);
    double *V2C = Pc + n * Q_GF, *C2V = V2C + ne * Q_GF;

    // channel symbol probabilities by normalized LLRs
    init_LLR(syms, n, L);
    for (int i = 0; i < n; i++) {
        for (int x = 0; x < Q_GF; x++) {
            Pc[i*Q_GF+x] = exp(-(double)L[i*Q_GF+x]);
        }
        norm_prob(Pc + i * Q_GF);
    }
    sdr_free(L);

    // initialize VN->CN messages in edge domain (y = h * x)
    for (int e = 0; e < ne; e++) {
        const double *pv = Pc + g->ce_var[e] * Q_GF;
        const uint8_t *mul = GF_MUL[g->ce_h[e]];

        for (int x = 0; x < Q_GF; x++) {
            V2C[e*Q_GF+mul[x]] = pv[x];
        }
    }
    for (int iter = 0; iter < MAX_ITER_SP && !ok; iter++) {
        // update check nodes (XOR convolution as product in Walsh domain)
        for (int i = 0; i < g->ne; i += DC_H) {
            double T[DC_H][Q_GF], f1[Q_GF], b2[Q_GF], *out;

            for (int j = 0; j < DC_H; j++) {
                memcpy(T[j], V2C + (i + j) * Q_GF, sizeof(T[j]));
                walsh_trans(T[j]);
            }
            for (int x = 0; x < Q_GF; x++) {
                f1[x] = T[0][x] * T[1][x];
                b2[x] = T[2][x] * T[3][x];
            }
            for (int j = 0; j < DC_H; j++) {
                out = C2V + (i + j) * Q_GF;
                for (int x = 0; x < Q_GF; x++) {
                    out[x] = j == 0 ? T[1][x] * b2[x] :
                             j == 1 ? T[0][x] * b2[x] :
                             j == 2 ? f1[x] * T[3][x] : f1[x] * T[2][x];
                }
                walsh_trans(out); // inverse up to factor Q absorbed by norm
                norm_prob(out);
            }
        }
        // update variable nodes and GF(q) codes
        for (int i = 0; i < n; i++) {
            int nd = g->ve_ptr[i+1] - g->ve_ptr[i];
            const uint16_t *edge = g->ve_edge + g->ve_ptr[i];
            double post[Q_GF], out[Q_GF];

            for (int x = 0; x < Q_GF; x++) {
                post[x] = Pc[i*Q_GF+x];
            }
            for (int d = 0; d < nd; d++) {
                const double *c2v = C2V + edge[d] * Q_GF;
                const uint8_t *mul = GF_MUL[g->ce_h[edge[d]]];

                for (int x = 0; x < Q_GF; x++) {
                    post[x] *= c2v[mul[x]];
                }
            }
            code[i] = (uint8_t)argmax(post, Q_GF);

            for (int d = 0; d < nd; d++) {
                const uint8_t *mul = GF_MUL[g->ce_h[edge[d]]];

                for (int x = 0; x < Q_GF; x++) {
                    out[x] = Pc[i*Q_GF+x];
                }
                for (int f = 0; f < nd; f++) {
                    if (f == d) continue;
                    const double *c2v = C2V + edge[f] * Q_GF;
                    const uint8_t *mulf = GF_MUL[g->ce_h[edge[f]]];

                    for (int x = 0; x < Q_GF; x++) {
                        out[x] *= c2v[mulf[x]];
                    }
                }
                norm_prob(out);
                for (int x = 0; x < Q_GF; x++) {
                    V2C[edge[d]*Q_GF+mul[x]] = out[x];
                }
            }
        }
        ok = check_parity(g, code);
    }
    sdr_free(Pc);
    return ok;
}

// decode NB-LDPC by soft-decision input ------------------------------------------
int sdr_decode_NB_LDPC(const uint8_t H_idx[][4], const uint8_t H_ele[][4],
    int m, int n, const uint8_t *syms, uint8_t *syms_dec)
{
    uint8_t code[MAX_H_N];
    int nerr = -1;

    if (m <= 0 || n <= 0) {
        return (m == 0 && n == 0) ? 0 : -1;
    }
    // initialize GF(q) tables and get cached Tanner graph (channels run in
    // separate threads: see ch_thread() in sdr_rcv.c)
    sdr_mutex_lock(&init_mtx);
    init_table();
    const nb_graph_t *g = get_graph(H_idx, H_ele, m, n);
    sdr_mutex_unlock(&init_mtx);
    if (!g) return -1;

    // convert soft binary codes to GF(q) hard-decision codes
    soft2gf(syms, n, code);

    if (!check_parity(g, code)) {
        // initialize normalized posterior LLRs (EC2V cleared by sdr_malloc)
        float *P = (float *)sdr_malloc(sizeof(float) * (n + g->ne) * Q_GF);
        float *EC2V = P + n * Q_GF;

        init_LLR(syms, n, P);

        for (int iter = 0; iter < MAX_ITER; iter++) {
            for (int i = 0; i < g->m; i++) {
                update_check(g, i, P, EC2V, code);
            }
            if (check_parity(g, code)) break;
        }
        sdr_free(P);

        // sum-product retry on EMS failure
        if (MAX_ITER_SP > 0 && !check_parity(g, code)) {
            decode_SP(g, syms, code);
        }
    }
    if (check_parity(g, code)) {
        nerr = 0;
        for (int i = 0; i < n * N_GF; i++) {
            uint8_t sym = (code[i/N_GF] >> (N_GF-1-i%N_GF)) & 1;
            if (sym != (syms[i] >= 128 ? 1 : 0)) nerr++;
        }
        gf2bin(code, m, syms_dec);
    }
    return nerr;
}
