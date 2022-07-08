//
//  Pocket SDR C Library - Forward Error Correction (FEC) Functions
//
//  References:
//  [1] LIBFEC: Clone of Phil Karn's libfec with capability ot build on x86-64
//      (https://github.com/quiet/libfec)
//
//  Author:
//  T.TAKASU
//
//  History:
//  2022-07-08  1.0  port sdr_fec.py to C
//
#include "pocket_sdr.h"

// function prototype of LIBFEC ([1]) ------------------------------------------
int decode_rs_ccsds(uint8_t *data, int *eras_pos, int no_eras, int pad);
void *create_viterbi27(int len);
void set_viterbi27_polynomial(int polys[2]);
int init_viterbi27(void *vp, int starting_state);
int update_viterbi27_blk(void *vp, unsigned char sym[], int npairs);
int chainback_viterbi27(void *vp, unsigned char *data, unsigned int nbits,
    unsigned int endstate);
void delete_viterbi27(void *vp);

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
    static int poly[] = {0x4F, 0x6D};
    int n = N / 2 - 6;
    void *dec;
    
    // initialize Viterbi decoder
    if (n <= 0 || !(dec = create_viterbi27(n))) {
        fprintf(stderr, "create_viterbi27() error n=%d\n", n);
        return;
    }
    // set polynomial
    set_viterbi27_polynomial(poly);
    
    // update decoder with demodulated symbols
    if (update_viterbi27_blk(dec, (unsigned char *)data, n + 6)) {
        fprintf(stderr, "update_viterbi27_blk() error\n");
        delete_viterbi27(dec);
        return;
    }
    uint8_t *bits = (uint8_t *)sdr_malloc((n + 7) / 8);
    
    // Viterbi chainback
    if (chainback_viterbi27(dec, (unsigned char *)bits, n, 0)) {
        fprintf(stderr, "chainback_viterbi27() error\n");
        delete_viterbi27(dec);
        sdr_free(bits);
        return;
    }
    // delete decoder
    delete_viterbi27(dec);
    
    for (int i = 0; i < n; i++) {
        dec_data[i] = (bits[i / 8] >> (7 - i % 8)) & 1;
    }
    sdr_free(bits);
}

//------------------------------------------------------------------------------
//  Decode Reed-Solomon RS(255,223) code.
//
//  args:
//      syms     (IO) RS-encoded data symbols as uint8_t array (length = 255).
//                    Symbol errors are corrected before returning the function.
//
//  return:
//      Number of error symbols corrected. (-1: too many erros)
//
int sdr_decode_rs(uint8_t *syms)
{
    // decode RS-CCSDS (RS(255,223))
    return decode_rs_ccsds(syms, NULL, 0, 0);
}

