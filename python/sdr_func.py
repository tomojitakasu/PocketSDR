#
#  Pocket SDR Python Library - Fundamental GNSS SDR Functions
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-12-01  1.0  new
#  2021-12-24  1.1  fix several problems
#  2022-01-13  1.2  add API unpack_data()
#                   DOP_STEP: 0.25 -> 0.5
#  2022-01-20  1.3  use external library for mix_carr(), corr_std(), corr_fft()
#                   modify API search_sig(), search_code(), mix_carr() 
#  2022-01-25  1.4  support TCP client/server for log stream
#  2022-05-18  1.5  support API changes of sdr_func.c
#                   support np.fromfile() without offset option
#
from math import *
from ctypes import *
import time, os, re, platform
import numpy as np
from numpy import ctypeslib
import scipy.fftpack as fft
import sdr_code, sdr_rtk

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
    libsdr.sdr_func_init(c_char_p((dir + '/fftw_wisdom.txt').encode()))

# constants --------------------------------------------------------------------
DOP_STEP = 0.5     # Doppler frequency search step (* 1 / code cycle)
LIBSDR_ENA = True  # enable flag of LIBSDR

# global variable --------------------------------------------------------------
carr_tbl = []      # carrier lookup table 
log_lvl = 3        # log level
log_str = None     # log stream

#-------------------------------------------------------------------------------
#  Read digitalized IF (inter-frequency) data from file. Supported file format
#  is signed byte (int8) for I-sampling (real-sampling) or interleaved singned
#  byte for IQ-sampling (complex-sampling).
#
#  args:
#      file     (I) Digitalized IF data file path
#      fs       (I) Sampling frequency (Hz)
#      IQ       (I) Sampling type (1: I-sampling, 2: IQ-sampling)
#      T        (I) Sample period (s)
#      toff=0.0 (I) Time offset from the beginning (s) (optional)
#
#  returns:
#      data     Digitized IF data as complex64 ndarray (length == 0: read error)
#
def read_data(file, fs, IQ, T, toff=0.0):
    off = int(fs * toff * IQ)
    cnt = int(fs * T * IQ) if T > 0.0 else -1 # all if T=0.0
    
    f = open(file, 'rb')
    f.seek(off, os.SEEK_SET)
    raw = np.fromfile(f, dtype=np.int8, count=cnt)
    f.close()
    
    if len(raw) < cnt:
        return np.array([], dtype='complex64')
    elif IQ == 1: # I-sampling
        return np.array(raw, dtype='complex64')
    else: # IQ-sampling
        return np.array(raw[0::2] - raw[1::2] * 1j, dtype='complex64')

#-------------------------------------------------------------------------------
#  Parallel code search in digitized IF data.
#
#  args:
#      code_fft (I) Code DFT (with or w/o zero-padding)
#      T        (I) Code cycle (period) (s)
#      buff     (I) Buffer of IF data as complex64 ndarray
#      ix       (I) Index of buffer
#      fs       (I) Sampling frequency (Hz)
#      fi       (I) IF frequency (Hz)
#      fds      (I) Doppler frequency bins as ndarray (Hz)
#
#  returns:
#      P        Correlation powers in the Doppler frequencies - Code offset
#               space as float32 2D-ndarray
#
def search_code(code_fft, T, buff, ix, fs, fi, fds):
    N = int(fs * T)
    P = np.zeros((len(fds), N), dtype='float32')
    
    for i in range(len(fds)):
        C = corr_fft(buff, ix, len(code_fft), fs, fi + fds[i], 0.0, code_fft)[:N]
        P[i] = np.abs(C) ** 2
    return P

# max correlation power and C/N0 -----------------------------------------------
def corr_max(P, T):
    ix = np.unravel_index(np.argmax(P), P.shape)
    P_max = P[ix[0]][ix[1]]
    P_ave = np.mean(P)
    cn0 = 10.0 * log10((P_max - P_ave) / P_ave / T) if P_ave > 0.0 else 0.0
    return P_max, ix, cn0

# fine Doppler frequency by quadratic fitting ----------------------------------
def fine_dop(P, fds, ix):
    if ix == 0 or ix == len(fds) - 1:
        return fds[ix]
    p = np.polyfit(fds[ix-1:ix+2], P[ix-1:ix+2], 2)
    return -p[1] / (2.0 * p[0])

# shift IF frequency for GLONASS FDMA ------------------------------------------
def shift_freq(sig, fcn, fi):
    if sig == 'G1CA':
        fi += 0.5625e6 * fcn
    elif sig == 'G2CA':
        fi += 0.4375e6 * fcn
    return fi

# doppler search bins ----------------------------------------------------------
def dop_bins(T, dop, max_dop):
    return np.arange(dop - max_dop, dop + max_dop + DOP_STEP / T, DOP_STEP / T)

# mix carrier and standard correlator ------------------------------------------
def corr_std(buff, ix, N, fs, fc, phi, code, pos):
    if libsdr and LIBSDR_ENA:
        corr = np.empty(len(pos), dtype='complex64')
        pos = np.array(pos, dtype='int32')
        libsdr.sdr_corr_std.argtypes = [
            ctypeslib.ndpointer('complex64'), c_int32, c_int32, c_double,
            c_double, c_double, ctypeslib.ndpointer('complex64'),
            ctypeslib.ndpointer('int32'), c_int32,
            ctypeslib.ndpointer('complex64')]
        libsdr.sdr_corr_std(buff, ix, N, fs, fc, phi, code, pos, len(pos), corr)
        return corr
    else:
        data = mix_carr(buff, ix, N, fs, fc, phi)
        return corr_std_(data, code, pos)

# mix carrier and FFT correlator -----------------------------------------------
def corr_fft(buff, ix, N, fs, fc, phi, code_fft):
    if libsdr and LIBSDR_ENA:
        corr = np.empty(N, dtype='complex64')
        libsdr.sdr_corr_fft.argtypes = [
            ctypeslib.ndpointer('complex64'), c_int32, c_int32, c_double,
            c_double, c_double, ctypeslib.ndpointer('complex64'),
            ctypeslib.ndpointer('complex64')]
        libsdr.sdr_corr_fft(buff, ix, N, fs, fc, phi, code_fft, corr)
        return corr
    else:
        data = mix_carr(buff, ix, N, fs, fc, phi)
        return corr_fft_(data, code_fft)

# mix carrier ------------------------------------------------------------------
def mix_carr(buff, ix, N, fs, fc, phi):
    if libsdr and LIBSDR_ENA:
        data = np.empty(N, dtype='complex64')
        libsdr.sdr_mix_carr.argtypes = [
            ctypeslib.ndpointer('complex64'), c_int32, c_int32, c_double,
            c_double, c_double, ctypeslib.ndpointer('complex64')]
        libsdr.sdr_mix_carr(buff, ix, N, fs, fc, phi, data)
        return data
    else:
        global carr_tbl
        if len(carr_tbl) == 0:
            carr_tbl = np.array(np.exp(-2j * np.pi * np.arange(256) / 256),
                           dtype='complex64')
        i = ((fc / fs * np.arange(N) + phi) * 256).astype('uint8')
        return buff[ix:ix+N] * carr_tbl[i]

# standard correlator ----------------------------------------------------------
def corr_std_(data, code, pos):
    N = len(data)
    corr = np.zeros(len(pos), dtype='complex64')
    for i in range(len(pos)):
        if pos[i] > 0:
            corr[i] = np.dot(data[pos[i]:], code[:-pos[i]]) / (N - pos[i])
        elif pos[i] < 0:
            corr[i] = np.dot(data[:pos[i]], code[-pos[i]:]) / (N + pos[i])
        else:
            corr[i] = np.dot(data, code) / N
    return corr

# FFT correlator ---------------------------------------------------------------
def corr_fft_(data, code_fft):
    return fft.ifft(fft.fft(data) * code_fft) / len(data)

# open log ---------------------------------------------------------------------
def log_open(path):
    global log_str
    m = re.search(r'([^:]*):([^:]+)', path)
    if not m: # file
        log_str = sdr_rtk.stropen(sdr_rtk.STR_FILE, sdr_rtk.STR_MODE_W, path)
    elif not m.group(1): # TCP server
        log_str = sdr_rtk.stropen(sdr_rtk.STR_TCPSVR, sdr_rtk.STR_MODE_W, path)
    elif not m.group(2): # TCP client
        log_str = sdr_rtk.stropen(sdr_rtk.STR_TCPCLI, sdr_rtk.STR_MODE_W, path)
    if not log_str:
        print('log stream open error %s' % (path))

# close log --------------------------------------------------------------------
def log_close():
    global log_str
    sdr_rtk.strclose(log_str)
    log_str = None

# set log level ----------------------------------------------------------------
def log_level(level):
    global log_lvl
    log_lvl = level

# output log -------------------------------------------------------------------
def log(level, msg):
    global log_str, log_lvl
    if log_lvl == 0:
        print(msg)
    elif log_str and level <= log_lvl:
        buff = np.frombuffer((msg + '\r\n').encode(), dtype='uint8')
        sdr_rtk.strwrite(log_str, buff)

# parse numbers list and range -------------------------------------------------
def parse_nums(str):
    nums = []
    for ss in str.split(','):
        s = ss.split('-')
        if len(s) >= 4:
            if s[0] == '' and s[2] == '': # -n--m
                nums += range(-int(s[1]), -int(s[3]) + 1)
        elif len(s) >= 3:
            if s[0] == '': # -n-m
                nums += range(-int(s[1]), int(s[2]) + 1)
        elif len(s) >= 2:
            if s[0] == '': # -n
                nums += [-int(s[1])]
            else: # n-m
                nums += range(int(s[0]), int(s[1]) + 1)
        else: # n
            nums += [int(s[0])]
    return nums

# add item to buffer -----------------------------------------------------------
def add_buff(buff, item):
    buff[:-1], buff[-1] = buff[1:], item

# pack bits to uint8 ndarray ---------------------------------------------------
def pack_bits(data, nz=0):
    if nz > 0:
        data = np.hstack([[0] * nz, data])
    N = len(data)
    buff = np.zeros((N + 7) // 8, dtype='uint8')
    for i in range(N):
        buff[i // 8] |= (data[i] << (7 - i % 8))
    return buff

# unpack uint8 ndarray to bits ------------------------------------------------
def unpack_bits(data, N):
    buff = np.zeros(N, dtype='uint8')
    for i in range(np.min([N, len(data) * 8])):
        buff[i] = (data[i // 8] >> (7 - i % 8)) & 1
    return buff

# unpack data to bits ----------------------------------------------------------
def unpack_data(data, N):
    buff = np.zeros(N, dtype='uint8')
    for i in range(N):
        buff[i] = (data >> (N - 1 - i)) & 1
    return buff

# exclusive-or of all bits ------------------------------------------------------
def xor_bits(X):
    return bin(X).count('1') % 2

# hex string --------------------------------------------------------------------
def hex_str(data):
    str = ''
    for i in range(len(data)):
        str += '%02X' % (data[i])
    return str
