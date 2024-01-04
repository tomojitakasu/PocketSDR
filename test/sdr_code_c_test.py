#!/usr/bin/env python3
#
#  Unit test for sdr_code.c
#
import os, sys, time, platform
sys.path.append('../python')
sys.path.append('.')
import numpy as np
import sdr_code
from ctypes import *
from numpy import ctypeslib

# load external library --------------------------------------------------------
dir = os.path.dirname(__file__)
env = platform.platform()
try:
    if 'Windows' in env:
        libsdr = cdll.LoadLibrary(dir + '/../lib/win32/libsdr.so')
    elif 'Linux' in env:
        libsdr = cdll.LoadLibrary(dir + '/../lib/linux/libsdr.so')
    else:
        raise
except:
    libsdr = None
else:
    libsdr.sdr_func_init(c_char_p((dir + '/../python/fftw_wisdom.txt').encode()))

# sdr_gen_code() by libsdr -----------------------------------------------------
def sdr_gen_code(sig, prn):
    N = c_int()
    libsdr.sdr_gen_code.restype = POINTER(c_int8)
    p = libsdr.sdr_gen_code(sig.encode(), c_int32(prn), byref(N))
    return np.array(p[:N.value], dtype='int8')

# sdr_sec_code() by libsdr -----------------------------------------------------
def sdr_sec_code(sig, prn):
    N = c_int()
    libsdr.sdr_sec_code.restype = POINTER(c_int8)
    p = libsdr.sdr_sec_code(sig.encode(), c_int32(prn), byref(N))
    return np.array(p[:N.value], dtype='int8')

# sdr_res_code() by libsdr -----------------------------------------------------
def sdr_res_code(code, T, coff, fs, N, Nz):
    code = np.array(code, dtype='int8')
    code_res = np.zeros(N + Nz, dtype='float32')
    libsdr.sdr_res_code.argtypes = [
        ctypeslib.ndpointer('int8'), c_int32, c_double, c_double, c_double,
        c_int32, c_int32, ctypeslib.ndpointer('float32')]
    libsdr.sdr_res_code(code, len(code), T, coff, fs, N, Nz, code_res)
    return np.array(code_res, dtype='complex64')

# sdr_gen_code_fft() by libsdr -------------------------------------------------
def sdr_gen_code_fft(code, T, coff, fs, N, Nz):
    code = np.array(code, dtype='int8')
    code_fft = np.zeros(N + Nz, dtype='complex64')
    libsdr.sdr_gen_code_fft.argtypes = [
        ctypeslib.ndpointer('int8'), c_int32, c_double, c_double, c_double,
        c_int32, c_int32, ctypeslib.ndpointer('complex64')]
    libsdr.sdr_gen_code_fft(code, len(code), T, coff, fs, N, Nz, code_fft)
    return code_fft

# dummy function to measure overhead -------------------------------------------
def dummy_func(code, N, Nz):
    code1 = np.array(code, dtype='int8')
    code2 = np.zeros(N + Nz, dtype='complex64')

# sdr_gen_code() -------------------------------------------------------------------
def test_01():
    print('test_01:')
    
    err = 0
    for prn in range(1, 211):
        code = sdr_gen_code('L1CA', prn)
        code_ref = sdr_code.gen_code('L1CA', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L1CA : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(184, 192):
        code = sdr_gen_code('L1S', prn)
        code_ref = sdr_code.gen_code('L1S', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L1S  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(203, 207):
        code = sdr_gen_code('L1CB', prn)
        code_ref = sdr_code.gen_code('L1CB', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L1CB : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_gen_code('L1CP', prn)
        code_ref = sdr_code.gen_code('L1CP', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L1CP : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_gen_code('L1CD', prn)
        code_ref = sdr_code.gen_code('L1CD', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L1CD : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_gen_code('L2CM', prn)
        code_ref = sdr_code.gen_code('L2CM', prn)
        if not np.all(code == code_ref):
            err = 1
    for prn in range(159, 211):
        code = sdr_gen_code('L2CM', prn)
        code_ref = sdr_code.gen_code('L2CM', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L2CM : %s' % ('NG' if err else 'OK'))
   
    #err = 0
    #for prn in range(1, 64):
    #    code = sdr_gen_code('L2CL', prn)
    #    code_ref = sdr_code.gen_code('L2CL', prn)
    #    if not np.all(code == code_ref):
    #        err = 1
    #for prn in range(159, 211):
    #    code = sdr_gen_code('L2CL', prn)
    #    code_ref = sdr_code.gen_code('L2CL', prn)
    #    if not np.all(code == code_ref):
    #        err = 1
    #print('sdr_gen_code() L2CL : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_gen_code('L5I', prn)
        code_ref = sdr_code.gen_code('L5I', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L5I  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_gen_code('L5Q', prn)
        code_ref = sdr_code.gen_code('L5Q', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L5Q  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(184, 190):
        code = sdr_gen_code('L5SI', prn)
        code_ref = sdr_code.gen_code('L5SI', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L5SI : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(184, 190):
        code = sdr_gen_code('L5SIV', prn)
        code_ref = sdr_code.gen_code('L5SIV', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L5SIV: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(184, 190):
        code = sdr_gen_code('L5SQ', prn)
        code_ref = sdr_code.gen_code('L5SQ', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L5SQ : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(193, 202):
        code = sdr_gen_code('L6D', prn)
        code_ref = sdr_code.gen_code('L6D', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L6D  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(203, 212):
        code = sdr_gen_code('L6E', prn)
        code_ref = sdr_code.gen_code('L6E', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() L6E  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(-7, 7):
        code = sdr_gen_code('G1CA', 1)
        code_ref = sdr_code.gen_code('G1CA', 1)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() G1CA : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(-7, 7):
        code = sdr_gen_code('G2CA', 1)
        code_ref = sdr_code.gen_code('G2CA', 1)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() G2CA : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(0, 64):
        code = sdr_gen_code('G3OCD', 1)
        code_ref = sdr_code.gen_code('G3OCD', 1)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() G3OCD: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(0, 64):
        code = sdr_gen_code('G3OCP', 1)
        code_ref = sdr_code.gen_code('G3OCP', 1)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() G3OCP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_gen_code('E1B', prn)
        code_ref = sdr_code.gen_code('E1B', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() E1B  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_gen_code('E1C', prn)
        code_ref = sdr_code.gen_code('E1C', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() E1C  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_gen_code('E5aI', prn)
        code_ref = sdr_code.gen_code('E5aI', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() E5aI : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_gen_code('E5aQ', prn)
        code_ref = sdr_code.gen_code('E5aQ', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() E5aQ : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_gen_code('E5bI', prn)
        code_ref = sdr_code.gen_code('E5bI', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() E5bI : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_gen_code('E5bQ', prn)
        code_ref = sdr_code.gen_code('E5bQ', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() E5bQ : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_gen_code('E6B', prn)
        code_ref = sdr_code.gen_code('E6B', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() E6B  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_gen_code('E6C', prn)
        code_ref = sdr_code.gen_code('E6C', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() E6C  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_gen_code('B1I', prn)
        code_ref = sdr_code.gen_code('B1I', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() B1I  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_gen_code('B1CD', prn)
        code_ref = sdr_code.gen_code('B1CD', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() B1CD : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_gen_code('B1CP', prn)
        code_ref = sdr_code.gen_code('B1CP', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() B1CP : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_gen_code('B2I', prn)
        code_ref = sdr_code.gen_code('B2I', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() B2I  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_gen_code('B2aD', prn)
        code_ref = sdr_code.gen_code('B2aD', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() B2aD : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_gen_code('B2aP', prn)
        code_ref = sdr_code.gen_code('B2aP', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() B2aP : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_gen_code('B2BI', prn)
        code_ref = sdr_code.gen_code('B2BI', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() B2bI : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_gen_code('B3I', prn)
        code_ref = sdr_code.gen_code('B3I', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() B3I  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 15):
        code = sdr_gen_code('I5S', prn)
        code_ref = sdr_code.gen_code('I5S', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() I5S  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 15):
        code = sdr_gen_code('ISS', prn)
        code_ref = sdr_code.gen_code('ISS', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_gen_code() ISS  : %s' % ('NG' if err else 'OK'))
    
# sdr_sec_code() -------------------------------------------------------------------
def test_02():
    print('test_02:')
    
    err = 0
    for prn in range(1, 210):
        code = sdr_sec_code('L1CP', prn)
        code_ref = sdr_code.sec_code('L1CP', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() L1CP : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 210):
        code = sdr_sec_code('L5I', prn)
        code_ref = sdr_code.sec_code('L5I', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() L5I  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(184, 190):
        code = sdr_sec_code('L5SI', prn)
        code_ref = sdr_code.sec_code('L5SI', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() L5SI : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(184, 190):
        code = sdr_sec_code('L5SIV', prn)
        code_ref = sdr_code.sec_code('L5SIV', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() L5SIV: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(184, 190):
        code = sdr_sec_code('L5SQ', prn)
        code_ref = sdr_code.sec_code('L5SQ', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() L5SI : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(184, 190):
        code = sdr_sec_code('L5SQV', prn)
        code_ref = sdr_code.sec_code('L5SQV', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() L5SQV: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(-7, 7):
        code = sdr_sec_code('G1CA', prn)
        code_ref = sdr_code.sec_code('G1CA', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() G1CA : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(-7, 7):
        code = sdr_sec_code('G2CA', prn)
        code_ref = sdr_code.sec_code('G2CA', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() G2CA : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(0, 63):
        code = sdr_sec_code('G3OCD', prn)
        code_ref = sdr_code.sec_code('G3OCD', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() G3OCD: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(0, 63):
        code = sdr_sec_code('G3OCP', prn)
        code_ref = sdr_code.sec_code('G3OCP', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() G3OCP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_sec_code('E1C', prn)
        code_ref = sdr_code.sec_code('E1C', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() E1C  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_sec_code('E5aI', prn)
        code_ref = sdr_code.sec_code('E5aI', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() E5aI : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_sec_code('E5aQ', prn)
        code_ref = sdr_code.sec_code('E5aQ', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() E5aQ : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_sec_code('E5bI', prn)
        code_ref = sdr_code.sec_code('E5bI', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() E5bI : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_sec_code('E5bQ', prn)
        code_ref = sdr_code.sec_code('E5bQ', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() E5bQ : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_sec_code('E6C', prn)
        code_ref = sdr_code.sec_code('E6C', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() E6C  : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 63):
        code = sdr_sec_code('B1I', prn)
        code_ref = sdr_code.sec_code('B1I', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() B1I  : %s' % ('NG' if err else 'OK'))

    err = 0
    for prn in range(1, 63):
        code = sdr_sec_code('B1CP', prn)
        code_ref = sdr_code.sec_code('B1CP', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() B1CP : %s' % ('NG' if err else 'OK'))

    err = 0
    for prn in range(1, 63):
        code = sdr_sec_code('B2I', prn)
        code_ref = sdr_code.sec_code('B2I', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() B2I  : %s' % ('NG' if err else 'OK'))

    err = 0
    for prn in range(1, 63):
        code = sdr_sec_code('B2aD', prn)
        code_ref = sdr_code.sec_code('B2aD', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() B2aD : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 63):
        code = sdr_sec_code('B2aP', prn)
        code_ref = sdr_code.sec_code('B2aP', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() B2aP : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 63):
        code = sdr_sec_code('B3I', prn)
        code_ref = sdr_code.sec_code('B3I', prn)
        if not np.all(code == code_ref):
            err = 1
    print('sdr_sec_code() B3I  : %s' % ('NG' if err else 'OK'))

# sdr_res_code() -------------------------------------------------------------------
def test_03():
    print('test_03:')
    
    code = sdr_code.gen_code('L1CA', 10)
    T = sdr_code.code_cyc('L1CA')
    coff = 0.3e-3
    fs = 12.0e6
    N = 12000
    Nz = 12000
    
    code_res = sdr_res_code(code, T, coff, fs, N, Nz)
    code_ref = sdr_code.res_code(code, T, coff, fs, N, Nz)
    
    err = not np.all(code_res == code_ref)
    print('sdr_res_code()      : %s' % ('NG' if err else 'OK'))

# sdr_gen_code_fft() -------------------------------------------------------------------
def test_04():
    print('test_04:')
    
    code = sdr_code.gen_code('L1CA', 15)
    T = sdr_code.code_cyc('L1CA')
    coff = 0.5e-3
    fs = 12.0e6
    N = 12000
    Nz = 12000
    
    code_fft = sdr_gen_code_fft(code, T, coff, fs, N, Nz)
    code_ref = sdr_code.gen_code_fft(code, T, coff, fs, N, Nz)
    
    err = not np.all(np.abs(code_fft - code_ref) < 1e-3)
    print('sdr_gen_code_fft()  : %s' % ('NG' if err else 'OK'))

# performance ----------------------------------------------------------------------
def test_05():
    code = sdr_code.gen_code('L1CA', 19)
    T = sdr_code.code_cyc('L1CA')
    coff = 0.3e-3
    fs = 12.0e6
    n = 10000
    
    print('test_05: performance')
    print('%6s   %18s   %18s %s' % ('N', 'sdr_res_code()', 'sdr_gen_code_fft()', '(ms)'))
    print('%6s   %8s  %8s   %8s  %8s' % ('', 'Python', 'C', 'Python', 'C'))
    
    for N in (6000, 8000, 12000, 24000, 48000):
        fs = N * 1e3
        Nz = N
        
        tt = time.time()
        for i in range(n):
            code_res = sdr_code.res_code(code, T, coff, fs, N, Nz)
        t1 = (time.time() - tt) * 1e3 / n
            
        tt = time.time()
        for i in range(n):
            code_res = sdr_res_code(code, T, coff, fs, N, Nz)
        t2 = (time.time() - tt) * 1e3 / n
        
        tt = time.time()
        for i in range(n):
            code_fft = sdr_code.gen_code_fft(code, T, coff, fs, N, Nz)
        t3 = (time.time() - tt) * 1e3 / n
        
        tt = time.time()
        for i in range(n):
            code_fft = sdr_gen_code_fft(code, T, coff, fs, N, Nz)
        t4 = (time.time() - tt) * 1e3 / n
            
        tt = time.time()
        for i in range(n):
            dummy_func(code, N, Nz)
        t5 = (time.time() - tt) * 1e3 / n
            
        print('%6d   %8.4f  %8.4f   %8.4f  %8.4f' % (N, t1, t2 - t5, t3, t4 - t5))

# main -------------------------------------------------------------------------
if __name__ == '__main__':
    test_01()
    test_02()
    test_03()
    test_04()
    test_05()
    
