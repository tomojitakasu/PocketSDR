#
#  Pocket SDR Python Library - NB-LDPC Decoder Functions
#
#  References:
#  [1] BeiDou Navigation Satellite System Signal In Space Interface Control
#      Document Open Service Signal B1C (Version 1.0), December, 2017
#  [2] E.Li et al., Trellis-based Extended Min-Sum algorithm for non-binary LDPC
#      codes and its hardware structure, IEEE Trans. on Communications, 2013
#
#  Author:
#  T.TAKASU
#
#  History:
#  2024-01-25  1.0  new
#
from math import *
import numpy as np

# constants --------------------------------------------------------------------
N_GF = 6            # number of GF(q) bits
Q_GF = 1<<N_GF      # number of GF(q) elements
MAX_ITER = 15       # max number of iterations
NM_EMS = 4          # LLR truncation size of EMS
ERR_PROB = 1e-5     # error probability of input codes

# GF(q) tables -----------------------------------------------------------------
GF_VEC = ( # power -> vector ([1])
     1,  2,  4,  8, 16, 32,  3,  6, 12, 24, 48, 35,  5, 10, 20, 40,
    19, 38, 15, 30, 60, 59, 53, 41, 17, 34,  7, 14, 28, 56, 51, 37,
     9, 18, 36, 11, 22, 44, 27, 54, 47, 29, 58, 55, 45, 25, 50, 39,
    13, 26, 52, 43, 21, 42, 23, 46, 31, 62, 63, 61, 57, 49, 33)

GF_POW = ( # vector -> power ([1])
     0,  0,  1,  6,  2, 12,  7, 26,  3, 32, 13, 35,  8, 48, 27, 18,
     4, 24, 33, 16, 14, 52, 36, 54,  9, 45, 49, 38, 28, 41, 19, 56,
     5, 62, 25, 11, 34, 31, 17, 47, 15, 23, 53, 51, 37, 44, 55, 40,
    10, 61, 46, 30, 50, 22, 39, 43, 29, 60, 42, 21, 20, 59, 57, 58)

GF_MUL = [] # multiply

# initialize GF(q) table -------------------------------------------------------
def init_table():
    global GF_MUL
    if len(GF_MUL):
        return
    GF_MUL = np.zeros((Q_GF, Q_GF), dtype='uint8')
    for i in range(1, Q_GF):
        for j in range(1, Q_GF):
            GF_MUL[i][j] = GF_VEC[(GF_POW[i] + GF_POW[j]) % (Q_GF - 1)]

# convert binary codes to GF(q) codes ------------------------------------------
def bin2gf(syms):
    n = len(syms) // N_GF
    code = np.zeros(n, dtype='uint8')
    for i in range(n):
        for j in range(N_GF):
            code[i] = (code[i] << 1) + syms[i*N_GF+j]
    return code

# convert GF(q) codes to binary codes ------------------------------------------
def gf2bin(code):
    n = len(code)
    syms = np.zeros(n * N_GF, dtype='uint8')
    for i in range(n):
        for j in range(N_GF):
            syms[i*N_GF+j] = (code[i] >> (N_GF - 1 - j)) & 1
    return syms

# Tanner graph edges -----------------------------------------------------------
def graph_edge(H_idx, H_ele):
    ie, je, he = [], [], []
    for i in range(len(H_idx)):
        for j in range(len(H_idx[i])):
            ie.append(i)
            je.append(H_idx[i][j])
            he.append(H_ele[i][j])
    return ie, je, he, len(he) # CN-index, VN-index, H_ij of edges

# initialize LLR ---------------------------------------------------------------
def init_LLR(code, err_prob):
    L = np.zeros((len(code), Q_GF), dtype='float32')
    for i in range(len(code)):
        for j in range(Q_GF):
            nerr = bin(code[i] ^ j).count('1')
            L[i][j] = -log(err_prob) * nerr
    return L

# parity check -----------------------------------------------------------------
def check_parity(ie, je, he, m, code):
    s = np.zeros(m, dtype='uint8')
    for i in range(len(ie)):
        s[ie[i]] ^= GF_MUL[he[i]][code[je[i]]]
    return np.all(s == 0)

# permute VN->CN message -------------------------------------------------------
def permute_V2C(h, V2C):
    V2C_p = np.zeros(Q_GF, dtype='float32')
    for i in range(Q_GF):
        V2C_p[GF_MUL[h][i]] = V2C[i]
    return V2C_p

# permute CN->VN message -------------------------------------------------------
def permute_C2V(h, C2V):
    C2V_p = np.zeros(Q_GF, dtype='float32')
    for i in range(Q_GF):
        C2V_p[i] = C2V[GF_MUL[h][i]]
    return C2V_p

# extended-min-sum (EMS) of LLRs ([2]) -----------------------------------------
def ext_min_sum(L1, L2):
    if len(L1) == 0:
        return L2
    idx1 = np.argsort(L1)
    idx2 = np.argsort(L2)
    maxL = L1[idx1[NM_EMS-1]] + L2[idx2[NM_EMS-1]]
    Ls = np.full(Q_GF, maxL, dtype='float32')
    
    for i in idx1[:NM_EMS]:
        for j in idx2[:NM_EMS]:
            if L1[i] + L2[j] < Ls[i^j]:
                Ls[i^j] = L1[i] + L2[j]
    return Ls

# decode NB-LDPC ---------------------------------------------------------------
def decode_NB_LDPC(H_idx, H_ele, m, n, syms):
    
    # initialze GF(q) tables
    init_table()
    
    # convert binary codes to GF(q) codes
    code = bin2gf(syms)
    
    # Tanner graph edges
    ie, je, he, ne = graph_edge(H_idx, H_ele)
    V2C = np.zeros((ne, Q_GF), dtype='float32')
    C2V = np.zeros((ne, Q_GF), dtype='float32')
    
    # initialize LLR and VN->CN messages
    L = init_LLR(code, ERR_PROB)
    for i in range(ne):
        V2C[i] = permute_V2C(he[i], L[je[i]])
    
    for iter in range(MAX_ITER):
        # parity check
        if check_parity(ie, je, he, m, code):
            syms_dec = gf2bin(code)
            nerr = np.count_nonzero(syms_dec ^ syms)
            return syms_dec[:m*N_GF], nerr
        
        # update check nodes
        for i in range(ne):
            Ls = []
            for j in range(ne):
                if ie[i] == ie[j] and i != j:
                    Ls = ext_min_sum(Ls, V2C[j])
            Ls -= np.min(Ls)
            C2V[i] = permute_C2V(he[i], Ls)
        
        # update variable nodes
        for i in range(ne):
            Ls = L[je[i]].copy()
            for j in range(ne):
                if je[i] == je[j] and i != j:
                    Ls += C2V[j]
            Ls -= np.min(Ls)
            V2C[i] = permute_V2C(he[i], Ls)
        
        # update LLR and GF(q) codes
        for i in range(n):
            for j in range(ne):
                if i == je[j]:
                    L[i] += C2V[j]
            L[i] -= np.min(L[i])
            code[i] = np.argmin(L[i])
    
    return gf2bin(code)[:m*N_GF], -1

