#
#  Pocket SDR Python Library - Fundamental GNSS SDR Functions
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-12-01  1.0  new
#
from math import *
import time
import numpy as np
import scipy.fftpack as fft
import sdr_code

dop_max = 5000.0  # max doppler search (Hz)
carr_tbl = []     # carrier lookup table 

# read digital IF data ----------------------------------------------------------
def read_data(file, fs, IQ, T, toff=0.0):
    cnt, off = int(fs * T), int(fs * toff)
    if IQ == 1: # I
        raw = np.fromfile(file, dtype=np.int8, offset=off, count=cnt)
        data = np.array(raw, dtype='complex64')
    else: # IQ
        raw = np.fromfile(file, dtype=np.int8, offset=off * 2, count=cnt * 2)
        data = np.array(raw[0::2] + raw[1::2] * 1j, dtype='complex64')
    return data

# mix carrier ------------------------------------------------------------------
def mix_carr(data, fs, fc, phi):
    N = 256 # carrier lookup table size
    global carr_tbl
    if len(carr_tbl) == 0:
        carr_tbl = np.array(np.exp(-2j * np.pi * np.arange(N) / N),
                       dtype='complex64')
    ix = ((fc * np.arange(len(data)) / fs + phi) * N).astype('int')
    return data * carr_tbl[ix % N]
    
# FFT correlator ---------------------------------------------------------------
def corr_fft(data, fs, fc, code_fft):
    data = mix_carr(data, fs, fc, 0.0)
    return np.abs(fft.ifft(fft.fft(data) * code_fft)) ** 2

# search code ------------------------------------------------------------------
def search_code(code, T, data, fs, fi, zero_pad=True):
    
    # resample and FFT code
    N = int(fs * T)
    code = sdr_code.res_code(code, T, 0.0, fs, N, N if zero_pad else 0)
    code_fft = np.conj(fft.fft(code))
    
    # parallel code search with zero-padding
    dops = np.arange(-dop_max, dop_max + 0.5 / T, 0.5 / T)
    P = np.zeros((len(dops), N))
    for i in range(len(dops)):
        for j in range(0, len(data) - len(code) + 1, N):
            Pc = corr_fft(data[j:j+len(code)], fs, fi + dops[i], code_fft)
            P[i] += Pc[0:N]
    
    return P, dops, np.arange(0, T, 1.0 / fs)

# max correlation power --------------------------------------------------------
def corr_max(P, T):
    ix = np.unravel_index(np.argmax(P), P.shape)
    P_max = P[ix[0]][ix[1]]
    P_ave = np.mean(P)
    cn0 = 10.0 * log10(P_max / P_ave / T) if P_ave > 0.0 else 0.0
    return P_max, ix, cn0

# search signal ----------------------------------------------------------------
def search_sig(sig, prn, data, fs, fi, zero_pad=True):
    # generate code
    code, T, Tc = sdr_code.gen_code(sig, prn)
    
    if len(code) == 0:
        return [], [0.0], (0, 0), 0.0, 0.0
    
    # search code
    P, dops, coffs = search_code(code, T, data, fs, fi, zero_pad=zero_pad)
    
    # max correlation power
    P_max, ix, cn0 = corr_max(P, T)
    
    return P / P_max, dops, coffs, ix, cn0, Tc
    
