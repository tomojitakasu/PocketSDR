//
//  Pocket SDR C Library - Forward Error Correction (FEC) Functions
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-07-08  1.0  port sdr_fec.py to C
//  2024-01-26  1.1  sdr_decode_rs() returns number of error bits
//  2026-06-05  1.2  remove libfec dependency
//
#include "pocket_sdr.h"

// constants -------------------------------------------------------------------
#define VIT_K       7
#define VIT_STATES  (1 << (VIT_K - 1))
#define VIT_INF     1000000000

#define RS_N        255
#define RS_K        223
#define RS_NROOTS   (RS_N - RS_K)
#define RS_A0       255
#define RS_GF_POLY  0x187
#define RS_FCR      112
#define RS_PRIM     11
#define RS_IPRIM    116

// type definitions ------------------------------------------------------------
typedef struct {
    uint8_t alpha_to[RS_N + 1];
    uint8_t index_of[RS_N + 1];
    uint8_t genpoly[RS_NROOTS + 1];
    uint8_t tal[256];
    uint8_t tal1[256];
    int init;
} rs_ccsds_t;

// global variables ------------------------------------------------------------
static rs_ccsds_t RS_CCSDS = {0};

// reduce GF(256) exponent modulo 255 ------------------------------------------
static int rs_mod255(int x)
{
    while (x < 0) x += RS_N;
    while (x >= RS_N) {
        x -= RS_N;
        x = (x >> 8) + (x & RS_N);
    }
    return x;
}

// multiply GF(256) values -----------------------------------------------------
static uint8_t rs_mul(const rs_ccsds_t *rs, uint8_t a, uint8_t b)
{
    if (a == 0 || b == 0) return 0;
    return rs->alpha_to[rs_mod255(rs->index_of[a] + rs->index_of[b])];
}

// divide GF(256) values -------------------------------------------------------
static uint8_t rs_div(const rs_ccsds_t *rs, uint8_t a, uint8_t b)
{
    if (a == 0) return 0;
    return rs->alpha_to[rs_mod255(rs->index_of[a] - rs->index_of[b])];
}

// convert conventional to CCSDS dual basis ------------------------------------
static uint8_t rs_to_dual(uint8_t x)
{
    static const uint8_t tal[] = {
        0x8D, 0xEF, 0xEC, 0x86, 0xFA, 0x99, 0xAF, 0x7B
    };
    uint8_t y = 0;
    
    for (int j = 0; j < 8; j++) {
        for (int k = 0; k < 8; k++) {
            if (x & (1 << k)) y ^= tal[7-k] & (1 << j);
        }
    }
    return y;
}

// initialize CCSDS RS tables --------------------------------------------------
static void rs_init_ccsds(rs_ccsds_t *rs)
{
    if (rs->init) return;
    
    rs->index_of[0] = RS_A0;
    rs->alpha_to[RS_A0] = 0;
    int sr = 1;
    for (int i = 0; i < RS_N; i++) {
        rs->index_of[sr] = (uint8_t)i;
        rs->alpha_to[i] = (uint8_t)sr;
        sr <<= 1;
        if (sr & 0x100) sr ^= RS_GF_POLY;
        sr &= RS_N;
    }
    for (int i = 0; i < 256; i++) {
        rs->tal[i] = rs_to_dual((uint8_t)i);
        rs->tal1[rs->tal[i]] = (uint8_t)i;
    }
    rs->genpoly[0] = 1;
    for (int i = 0, root = RS_FCR * RS_PRIM; i < RS_NROOTS; i++, root += RS_PRIM) {
        rs->genpoly[i+1] = 1;
        for (int j = i; j > 0; j--) {
            if (rs->genpoly[j]) {
                int idx = rs->index_of[rs->genpoly[j]] + root;
                rs->genpoly[j] = rs->genpoly[j-1] ^ rs->alpha_to[rs_mod255(idx)];
            } else {
                rs->genpoly[j] = rs->genpoly[j-1];
            }
        }
        rs->genpoly[0] = rs->alpha_to[rs_mod255(rs->index_of[rs->genpoly[0]] +
            root)];
    }
    for (int i = 0; i <= RS_NROOTS; i++) {
        rs->genpoly[i] = rs->index_of[rs->genpoly[i]];
    }
    rs->init = 1;
}

// compute CCSDS RS syndromes --------------------------------------------------
static int rs_syndromes(const rs_ccsds_t *rs, const uint8_t *data, uint8_t *syn)
{
    int syn_error = 0;
    
    for (int i = 0; i < RS_NROOTS; i++) {
        uint8_t x = rs->alpha_to[rs_mod255((RS_FCR + i) * RS_PRIM)];
        syn[i] = data[0];
        for (int j = 1; j < RS_N; j++) {
            syn[i] = rs_mul(rs, syn[i], x) ^ data[j];
        }
        syn_error |= syn[i];
    }
    return syn_error;
}

// find RS error locator polynomial -------------------------------------------
static int rs_find_locator(const rs_ccsds_t *rs, const uint8_t *syn,
    uint8_t *lambda)
{
    uint8_t b[RS_NROOTS + 1] = {0};
    int L = 0, m = 1;
    uint8_t bb = 1;
    
    memset(lambda, 0, RS_NROOTS + 1);
    lambda[0] = b[0] = 1;
    
    for (int n = 0; n < RS_NROOTS; n++) {
        uint8_t d = syn[n];
        for (int i = 1; i <= L; i++) {
            d ^= rs_mul(rs, lambda[i], syn[n-i]);
        }
        if (d == 0) {
            m++;
            continue;
        }
        uint8_t t[RS_NROOTS + 1];
        memcpy(t, lambda, sizeof(t));
        uint8_t coef = rs_div(rs, d, bb);
        for (int i = 0; i + m <= RS_NROOTS; i++) {
            if (b[i]) lambda[i+m] ^= rs_mul(rs, coef, b[i]);
        }
        if (2 * L <= n) {
            L = n + 1 - L;
            memcpy(b, t, sizeof(b));
            bb = d;
            m = 1;
        } else {
            m++;
        }
    }
    return L;
}

// search roots of RS error locator polynomial ---------------------------------
static int rs_find_roots(const rs_ccsds_t *rs, const uint8_t *lambda,
    int deg_lambda, int *root, int *loc)
{
    int count = 0;
    
    for (int i = 1, k = RS_IPRIM - 1; i <= RS_N; i++, k = rs_mod255(k +
        RS_IPRIM)) {
        uint8_t q = 1;
        for (int j = 1; j <= deg_lambda; j++) {
            q ^= rs_mul(rs, lambda[j], rs->alpha_to[rs_mod255(i * j)]);
        }
        if (q == 0) {
            root[count] = i;
            loc[count] = k;
            if (++count == deg_lambda) break;
        }
    }
    return count;
}

// compute RS error evaluator polynomial ---------------------------------------
static void rs_find_evaluator(const rs_ccsds_t *rs, const uint8_t *syn,
    const uint8_t *lambda, uint8_t *omega)
{
    memset(omega, 0, RS_NROOTS);
    for (int i = 0; i < RS_NROOTS; i++) {
        for (int j = 0; j <= i; j++) {
            omega[i] ^= rs_mul(rs, syn[i-j], lambda[j]);
        }
    }
}

// correct RS codeword symbols -------------------------------------------------
static int rs_correct_errors(const rs_ccsds_t *rs, uint8_t *data,
    const uint8_t *lambda, const uint8_t *omega, const int *root,
    const int *loc, int count)
{
    for (int j = count - 1; j >= 0; j--) {
        uint8_t num = 0, den = 0;
        for (int i = 0; i < RS_NROOTS; i++) {
            num ^= rs_mul(rs, omega[i], rs->alpha_to[rs_mod255(root[j] * i)]);
        }
        for (int i = 0; i < RS_NROOTS; i += 2) {
            den ^= rs_mul(rs, lambda[i+1], rs->alpha_to[rs_mod255(root[j] * i)]);
        }
        if (den == 0) return -1;
        uint8_t mag = rs_mul(rs, rs_div(rs, num, den),
            rs->alpha_to[rs_mod255(root[j] * (RS_FCR - 1))]);
        data[loc[j]] ^= mag;
    }
    return count;
}

// decode CCSDS RS(255,223) data ----------------------------------------------
static int rs_decode_ccsds(uint8_t *data)
{
    rs_ccsds_t *rs = &RS_CCSDS;
    uint8_t cdata[RS_N], syn[RS_NROOTS], lambda[RS_NROOTS + 1];
    uint8_t omega[RS_NROOTS];
    int root[RS_NROOTS], loc[RS_NROOTS];
    
    rs_init_ccsds(rs);
    for (int i = 0; i < RS_N; i++) {
        cdata[i] = rs->tal1[data[i]];
    }
    if (!rs_syndromes(rs, cdata, syn)) return 0;
    
    int deg_lambda = rs_find_locator(rs, syn, lambda);
    if (deg_lambda <= 0 || deg_lambda > RS_NROOTS / 2) return -1;
    
    int count = rs_find_roots(rs, lambda, deg_lambda, root, loc);
    if (count != deg_lambda) return -1;
    
    rs_find_evaluator(rs, syn, lambda, omega);
    if (rs_correct_errors(rs, cdata, lambda, omega, root, loc, count) < 0) {
        return -1;
    }
    if (rs_syndromes(rs, cdata, syn)) return -1;
    
    for (int i = 0; i < RS_N; i++) {
        data[i] = rs->tal[cdata[i]];
    }
    return count;
}

// compute parity of one byte --------------------------------------------------
static int parity_u8(uint8_t x)
{
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return x & 1;
}

// count bits of "1" -----------------------------------------------------------
static int count_bits(uint8_t bits)
{
    int n = 0;
    
    for (int i = 0; i < 8; i++) {
        if ((bits >> i) & 1) n++;
    }
    return n;
}

//------------------------------------------------------------------------------
//  Decode convolution code (K=7, R=1/2, Poly=G1:0x4F,G2:0x6D).
//
//  args:
//      data     (I) Data as uint8_t array (0 to 255 for soft-decision).
//      N        (I) Data length
//      dec_data (O) Decoded data as uint8_t array (0 or 1).
//                   (len(dec_data) = len(data) / 2 - 6)
//  return:
//      none
//
void sdr_decode_conv(const uint8_t *data, int N, uint8_t *dec_data)
{
    int nbits = N / 2 - (VIT_K - 1);
    int steps = N / 2;
    
    if (nbits <= 0 || N % 2) {
        fprintf(stderr, "sdr_decode_conv() error N=%d\n", N);
        return;
    }
    uint8_t br0[32], br1[32];
    uint8_t *dec = (uint8_t *)sdr_malloc((size_t)steps * VIT_STATES);
    unsigned int *metric = (unsigned int *)sdr_malloc(VIT_STATES *
        sizeof(unsigned int));
    unsigned int *new_metric = (unsigned int *)sdr_malloc(VIT_STATES *
        sizeof(unsigned int));
    
    for (int s = 0; s < 32; s++) {
        br0[s] = parity_u8((uint8_t)((2 * s) & 0x4F)) ? 255 : 0;
        br1[s] = parity_u8((uint8_t)((2 * s) & 0x6D)) ? 255 : 0;
    }
    for (int s = 0; s < VIT_STATES; s++) metric[s] = 63;
    metric[0] = 0;
    
    for (int t = 0; t < steps; t++) {
        uint8_t sym0 = data[t*2], sym1 = data[t*2+1];
        
        for (int i = 0; i < 32; i++) {
            unsigned int bm = (br0[i] ^ sym0) + (br1[i] ^ sym1);
            unsigned int m0 = metric[i] + bm;
            unsigned int m1 = metric[i+32] + (510 - bm);
            uint8_t d = m0 > m1;
            new_metric[2*i] = d ? m1 : m0;
            dec[t*VIT_STATES+2*i] = d;
            
            m0 = metric[i] + (510 - bm);
            m1 = metric[i+32] + bm;
            d = m0 > m1;
            new_metric[2*i+1] = d ? m1 : m0;
            dec[t*VIT_STATES+2*i+1] = d;
        }
        unsigned int *tmp = metric;
        metric = new_metric;
        new_metric = tmp;
    }
    uint8_t *bits = (uint8_t *)sdr_malloc((size_t)(nbits + 7) / 8);
    int endstate = 0; // terminated by 6 zero tail bits.
    
    memset(bits, 0, (size_t)(nbits + 7) / 8);
    for (int i = nbits - 1; i >= 0; i--) {
        int state = (endstate >> 2) & 63;
        int k = dec[(i + 6) * VIT_STATES + state] & 1;
        endstate = (endstate >> 1) | (k << 7);
        bits[i >> 3] = (uint8_t)endstate;
    }
    for (int i = 0; i < nbits; i++) {
        dec_data[i] = (bits[i / 8] >> (7 - i % 8)) & 1;
    }
    
    sdr_free(bits);
    sdr_free(new_metric);
    sdr_free(metric);
    sdr_free(dec);
}

//------------------------------------------------------------------------------
//  Decode Reed-Solomon RS(255,223) code.
//
//  args:
//      syms     (IO) RS-encoded data symbols as uint8_t array (length = 255).
//                    Symbol errors are corrected before returning the function.
//
//  return:
//      Number of error bits corrected. (-1: too many errors)
//
int sdr_decode_rs(uint8_t *syms)
{
    uint8_t buff[RS_N];
    int nerr = 0;
    
    memcpy(buff, syms, RS_N);
    if (rs_decode_ccsds(syms) < 0) {
        return -1;
    }
    for (int i = 0; i < RS_N; i++) {
        nerr += count_bits(syms[i] ^ buff[i]);
    }
    return nerr;
}
