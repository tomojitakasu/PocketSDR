//
//  Pocket SDR C Library - NB-LDPC Decoder Functions
//
//  References:
//  [1] BeiDou Navigation Satellite System Signal In Space Interface Control
//      Document Open Service Signal B1C (Version 1.0), December, 2017
//  [2] E.Li et al., Trellis-based Extended Min-Sum algorithm for non-binary LDPC
//      codes and its hardware structure, IEEE Trans. on Communications, 2013
//
//  Author:
//  T.TAKASU
//
//  History:
//  2024-01-25  1.0  port sdr_nb_ldpc.py to C
//
#include <math.h>
#include "pocket_sdr.h"

// constants --------------------------------------------------------------------
#define N_GF        6           // number of GF(q) bits
#define Q_GF        (1<<N_GF)   // number of GF(q) elements
#define MAX_ITER    15          // max number of iterations
#define NM_EMS      4           // LLR truncation size of EMS
#define ERR_PROB    1e-5        // error probability of input codes

#define MAX_H_M     128         // max rows of H-matrix
#define MAX_H_N     256         // max colums of H-matrix
#define MAX_EDGE    1024        // max number of Tanner graph edges

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
static uint8_t GF_MUL[Q_GF][Q_GF] = {{0}}; // multipy

// get index of min value in array ---------------------------------------------
static int argmin(const float *L, int n)
{
    int j = 0;
    for (int i = 1; i < n; i++) {
        if (L[i] < L[j]) j = i;
    }
    return j;
}

// swap index ------------------------------------------------------------------
static void swap_idx(int *idx, int i, int j)
{
    int idx_tmp = idx[i];
    idx[i] = idx[j];
    idx[j] = idx_tmp;
}

// get index of sorted array ---------------------------------------------------
static void argsort(const float *L, int n, int *idx)
{
    for (int i = 0; i < n; i++) {
        idx[i] = i;
    }
    for (int i = 0; i < n - 1; i++) { // bubble sort
        for (int j = n - 2; j >= i; j--) {
            if (L[idx[j]] > L[idx[j+1]]) swap_idx(idx, j, j + 1);
        }
    }
}

// initialize GF(q) table ------------------------------------------------------
static void init_table(void)
{
    if (GF_MUL[1][1]) {
        return;
    }
    for (int i = 1; i < Q_GF; i++) {
        for (int j = 1; j < Q_GF; j++) {
            GF_MUL[i][j] = GF_VEC[(GF_POW[i] + GF_POW[j]) % (Q_GF - 1)];
        }
    }
}

// convert binary codes to GF(q) codes -----------------------------------------
static void bin2gf(const uint8_t *syms, int n, uint8_t *code)
{
    memset(code, 0, n);
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < N_GF; j++) {
            code[i] = (code[i] << 1) + (syms[i*N_GF+j] & 1);
        }
    }
}

// convert GF(q) codes to binary codes -----------------------------------------
static void gf2bin(const uint8_t *code, int n, uint8_t *syms)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < N_GF; j++) {
            syms[i*N_GF+j] = (code[i] >> (N_GF - 1 - j)) & 1;
        }
    }
}

// Tanner graph edges ----------------------------------------------------------
static int graph_edge(const uint8_t H_idx[][4], const uint8_t H_ele[][4], int m,
    int *ie, int *je, uint8_t *he)
{
    int ne = 0;
    
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < 4 && ne < MAX_EDGE; j++) {
            ie[ne] = i;
            je[ne] = H_idx[i][j];
            he[ne++] = H_ele[i][j];
        }
    }
    return ne;
}

// copy LLR (L1 = L2.copy()) ---------------------------------------------------
static void copy_LLR(float *L1, const float *L2)
{
    for (int i = 0; i < Q_GF; i++) {
        L1[i] = L2[i];
    }
}

// add LLR (L1 += L2) ----------------------------------------------------------
static void add_LLR(float *L1, const float *L2)
{
    for (int i = 0; i < Q_GF; i++) {
        L1[i] += L2[i];
    }
}

// normalize LLR (L -= min(L)) -------------------------------------------------
static void norm_LLR(float *L)
{
    float minL = L[argmin(L, Q_GF)];
    
    for (int i = 0; i < Q_GF; i++) {
        L[i] -= minL;
    }
}

// initialize LLR --------------------------------------------------------------
static void init_LLR(const uint8_t *code, int n, float err_prob, float **L)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < Q_GF; j++) {
            int nerr = 0;
            for (int k = 0; k < N_GF; k++) {
                if ((code[i] ^ j) & (1<<k)) nerr++;
            }
            L[i][j] = -logf(err_prob) * nerr;
        }
    }
}

// NB-LDPC parity check --------------------------------------------------------
static int check_parity(const int *ie, const int *je, const uint8_t *he, int ne,
    int m, const uint8_t *code)
{
    uint8_t s[MAX_H_M] = {0};
    
    for (int i = 0; i < ne; i++) {
        s[ie[i]] ^= GF_MUL[he[i]][code[je[i]]];
    }
    for (int i = 0; i < m; i++) {
        if (s[i] != 0) return 0;
    }
    return 1;
}

// permute VN->CN message ------------------------------------------------------
static void permute_V2C(uint8_t h, const float *V2C, float *V2C_p)
{
    for (int i = 0; i < Q_GF; i++) {
        V2C_p[GF_MUL[h][i]] = V2C[i];
    }
}

// permute CN->VN message ------------------------------------------------------
static void permute_C2V(uint8_t h, const float *C2V, float *C2V_p)
{
    for (int i = 0; i < Q_GF; i++) {
        C2V_p[i] = C2V[GF_MUL[h][i]];
    }
}

// extended-min-sum (EMS) of LLRs (Ls = min(L1 + L2)) ([2]) --------------------
static void ext_min_sum(const float *L1, const float *L2, float *Ls)
{
    if (L1[0] < 0.0) { // first sum
        copy_LLR(Ls, L2);
        return;
    }
    float L[Q_GF], maxL;
    int idx1[Q_GF], idx2[Q_GF];
    
    argsort(L1, Q_GF, idx1);
    argsort(L2, Q_GF, idx2);
    maxL = L1[idx1[NM_EMS-1]] + L2[idx2[NM_EMS-1]];
    
    for (int i = 0; i < Q_GF; i++) {
        L[i] = maxL;
    }
    for (int k = 0; k < NM_EMS; k++) {
        for (int m = 0; m < NM_EMS; m++) {
            int i = idx1[k], j = idx2[m];
            if (L1[i] + L2[j] < L[i^j]) {
                L[i^j] = L1[i] + L2[j];
            }
        }
    }
    copy_LLR(Ls, L);
}

// decode NB-LDPC --------------------------------------------------------------
int sdr_decode_NB_LDPC(const uint8_t H_idx[][4], const uint8_t H_ele[][4],
    int m, int n, const uint8_t *syms, uint8_t *syms_dec)
{
    float *V2C[MAX_EDGE], *C2V[MAX_EDGE], *L[MAX_H_N], Ls[Q_GF];
    uint8_t code[MAX_H_N], he[MAX_EDGE];
    int iter, ne, ie[MAX_EDGE], je[MAX_EDGE], nerr = -1;
    
    // initialize GF(q) tables
    init_table();
    
    // convert binary codes to GF(q) codes
    bin2gf(syms, n, code);
    
    // Tanner graph edges
    ne = graph_edge(H_idx, H_ele, m, ie, je, he);
    for (int i = 0; i < ne; i++) {
        V2C[i] = (float *)sdr_malloc(sizeof(float) * Q_GF);
        C2V[i] = (float *)sdr_malloc(sizeof(float) * Q_GF);
    }
    // initialize LLR and VN->CN messages
    for (int i = 0; i < n; i++) {
        L[i] = (float *)sdr_malloc(sizeof(float) * Q_GF);
    }
    init_LLR(code, n, ERR_PROB, L);
    
    for (int i = 0; i < ne; i++) {
        permute_V2C(he[i], L[je[i]], V2C[i]);
    }
    for (iter = 0; iter < MAX_ITER; iter++) {
        // parity check
        if (check_parity(ie, je, he, ne, m, code)) {
            nerr = 0;
            for (int i = 0; i < n * N_GF; i++) {
                uint8_t sym = (code[i/N_GF] >> (N_GF-1-i%N_GF)) & 1;
                if (sym != syms[i]) nerr++;
            }
            gf2bin(code, m, syms_dec);
            break;
        }
        // update check nodes
        for (int i = 0; i < ne; i++) {
            Ls[0] = -1.0; // initial LLR
            for (int j = 0; j < ne; j++) {
                if (ie[i] == ie[j] && i != j) {
                    ext_min_sum(Ls, V2C[j], Ls);
                }
            }
            norm_LLR(Ls);
            permute_C2V(he[i], Ls, C2V[i]);
        }
        // update variable nodes
        for (int i = 0; i < ne; i++) {
            copy_LLR(Ls, L[je[i]]);
            for (int j = 0; j < ne; j++) {
                if (je[i] == je[j] && i != j) {
                    add_LLR(Ls, C2V[j]);
                }
            }
            norm_LLR(Ls);
            permute_V2C(he[i], Ls, V2C[i]);
        }
        // update LLR and GF(q) codes
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < ne; j++) {
                if (i == je[j]) {
                    add_LLR(L[i], C2V[j]);
                }
            }
            norm_LLR(L[i]);
            code[i] = (uint8_t)argmin(L[i], Q_GF);
        }
    }
    for (int i = 0; i < ne; i++) {
        sdr_free(V2C[i]);
        sdr_free(C2V[i]);
    }
    for (int i = 0; i < n; i++) {
        sdr_free(L[i]);
    }
    return nerr;
}

