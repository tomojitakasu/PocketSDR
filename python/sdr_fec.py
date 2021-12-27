#!/usr/bin/env python
#
#  PocketSDR Python Library - Forward Error Correction (FEC) Functions
#
#  References:
#  [1] LIBFEC: Clone of Phil Karn's libfec with capability ot build on x86-64
#      (https://github.com/quiet/libfec)
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-12-24  1.0  new
#
import os
from ctypes import *
import numpy as np
import sdr_func

# constants --------------------------------------------------------------------
POLY_CONV = (0x4F, 0x6D)  # convolution code polynomials (G1, G2)
NONE = np.array([], dtype='uint8')

# load LIBFEC ([1]) ------------------------------------------------------------
dir = os.path.dirname(__file__)
try:
    libfec = cdll.LoadLibrary(dir + '/../lib/win32/libfec.so')
except:
    libfec = cdll.LoadLibrary(dir + '/../lib/linux/libfec.so')
#libfec = cdll.LoadLibrary('libfec.so')

#-------------------------------------------------------------------------------
#  Encode convolution code (K=7, R=1/2, Poly=G1:0x4F,G2:0x6D).
#
#  args:
#      data     (I) Data as uint8 ndarray (0 or 1).
#
#  returns:
#      enc_data Encoded data as uint8 ndarray (0 or 1).
#               (len(enc_data) = (len(data) + 6) * 2)
#
def encode_conv(data):
    N = len(data)
    
    if N <= 0 or data.dtype != 'uint8':
        print('encode_conv: data length or type error')
        return NONE
    
    enc_data = np.zeros((N + 6) * 2, dtype='uint8')
    R = 0
    for i in range(N + 6):
        R = (R << 1) + ((data[i] & 1) if i < N else 0)
        enc_data[i*2+0] = sdr_func.xor_bits(R & POLY_CONV[0])
        enc_data[i*2+1] = sdr_func.xor_bits(R & POLY_CONV[1])
    
    return enc_data

#-------------------------------------------------------------------------------
#  Decode convolution code (K=7, R=1/2, Poly=G1:0x4F,G2:0x6D).
#
#  args:
#      data     (I) Data as uint8 ndarray (0 to 255 for soft-decision).
#
#  returns:
#      dec_data Decoded data as uint8 ndarray (0 or 1).
#               (len(dec_data) = len(data) / 2 - 6)
#
def decode_conv(data):
    N = len(data) // 2 - 6
    
    if N <= 0 or data.dtype != 'uint8':
        print('decode_conv: data length or type error')
        return NONE
    
    # initialize Viterbi decoder
    libfec.create_viterbi27.restype = c_void_p
    dec = libfec.create_viterbi27(N)
    if dec == None:
        print('decode_conv: deocoder create error')
        return NONE
    
    # set polynomial
    p = np.array(POLY_CONV, dtype='int').ctypes.data_as(POINTER(c_int))
    libfec.set_viterbi27_polynomial(p)
    
    # update decoder with demodulated symbols
    p = data.ctypes.data_as(POINTER(c_uint8))
    if libfec.update_viterbi27_blk(c_void_p(dec), p, N + 6) != 0:
        print('decode_conv: decoder update error')
        return NONE
    
    # Viterbi chainback
    bits = np.zeros((N + 7) // 8, dtype='uint8')
    p = bits.ctypes.data_as(POINTER(c_uint8))
    if libfec.chainback_viterbi27(c_void_p(dec), p, N, 0) != 0:
        print('decode_conv: decoder chainback error')
        return NONE
    
    # delete decoder
    libfec.delete_viterbi27(c_void_p(dec))
    
    dec_data = np.zeros(N, dtype='uint8')
    for i in range(N):
        dec_data[i] = (bits[i // 8] >> (7 - i % 8)) & 1
    
    return dec_data

#-------------------------------------------------------------------------------
#  Encode Reed-Solomon RS(255,223) code.
#
#  args:
#      syms     (IO) Data symbols as uint8 ndarray (length = 255).
#                    syms[0:223] should be set by input data. syms[223:255] are
#                    set by RS parity before returning the function.
#
#  returns:
#      None
#
def encode_rs(syms):
    if len(syms) < 255 or syms.dtype != 'uint8':
        print('encode_rs: data length or type error')
        return
    
    parity = np.zeros(32, dtype='uint8')
    p = syms.ctypes.data_as(POINTER(c_uint8))
    q = parity.ctypes.data_as(POINTER(c_uint8))
    
    # encode RS-CCSDS
    libfec.encode_rs_ccsds(p, q, 0)
    
    syms[223:] = parity

#-------------------------------------------------------------------------------
#  Decode Reed-Solomon RS(255,223) code.
#
#  args:
#      syms     (IO) RS-encoded data symbols as uint8 ndarray (length = 255).
#                    Symbol errors are corrected before returning the function.
#
#  returns:
#      nerr     Number of error symbols corrected. (-1: too many erros)
#
def decode_rs(syms):
    if len(syms) < 255 or syms.dtype != 'uint8':
        print('decode_rs: data length or type error')
        return -1
    
    p = syms.ctypes.data_as(POINTER(c_uint8))
    
    # decode RS-CCSDS
    return libfec.decode_rs_ccsds(p, None, 0, 0)

