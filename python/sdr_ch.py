#
#  Pocket SDR Python Library - GNSS SDR Receiver Channel Functions
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-12-08  1.0  new
#
from math import *
import numpy as np
import scipy.fftpack as fft
from sdr_func import *
import sdr_code
import sdr_nav

# constants --------------------------------------------------------------------
T_SRCH     = 30.0            # average of signal search interval (s)
T_ACQ      = 0.010           # non-coherent integration time for acquisition (s)
T_DLL      = 0.001           # non-coherent integration time for DLL (s)
T_CN0      = 1.0             # averaging time for C/N0 (s)
T_FPULLIN  = 1.5             # frequency pullin time (s)
T_NPULLIN  = 2.0             # navigation data pullin time (s)
#B_DLL      = 8.0             # band-width of DLL (Hz)
#B_DLL      = 80.0             # band-width of DLL (Hz)
B_DLL      = 20.0             # band-width of DLL (Hz)
B_PLL      = 20.0            # band-width of PLL (Hz)
B_FLL      = (100.0, 20.0)   # band-width of FLL (Hz) (wide, narrow)
SP_CORR    = 0.5             # default correlator spacing (chip)
MAX_DOP    = 5000.0          # default max Doppler search for acquisition (Hz)
THRES_CN0  = (35.0, 31.0)    # C/N0 threshold (dB-Hz) (lock, lost)

# general object classes -------------------------------------------------------
class Obj: pass

#-------------------------------------------------------------------------------
#  Generate new receiver channel.
#
#  args:
#      sig      (I) Signal type as string ('L1CA', 'L1CB', 'L1CP', ....)
#      prn      (I) PRN number
#      fs       (I) Sampling frequency (Hz)
#      fi       (I) IF frequency (Hz)
#      max_dop  (I) Max Doppler frequency for acquisition (Hz) (optional)
#      sp_corr  (I) Correlor spacing (chips) (optional)
#      add_corr (I) Flag for additional correlators for plot (optional)
#      nav_opt  (I) Navigation data options (optional)
#
#  returns:
#      ch       Receiver channel
#
def ch_new(sig, prn, fs, fi, max_dop=MAX_DOP, sp_corr=SP_CORR, add_corr=False,
    nav_opt=''):
    ch = Obj()
    ch.state = 'SRCH'               # channel state
    ch.time = ch.T0 = 0.0           # receiver time
    ch.sig = sig.upper()            # signal type
    ch.prn = prn                    # PRN number
    ch.code = sdr_code.gen_code(sig, prn) # primary code
    ch.sec_code = sdr_code.sec_code(sig, prn) # secondary code
    freq = sdr_code.sig_freq(sig)
    ch.freq = shift_freq(sig, prn, freq) # carrier freqency (Hz)
    ch.fs = fs                      # sampling freqency (Hz)
    ch.fi = shift_freq(sig, prn, fi) # IF frequency (Hz)
    ch.T = sdr_code.code_cyc(sig)   # code cycle (period) (s)
    ch.N = int(fs * ch.T)           # number of samples in a code cycle
    ch.phi = 0.0                    # carrier phase (cyc)
    ch.fd = 0.0                     # Doppler frequency (Hz)
    ch.coff = 0.0                   # Code offset (s)
    ch.adr = 0.0                    # Accumulated Doppler (cyc)
    ch.cn0 = 0.0                    # C/N0 (dB-Hz)
    ch.lock = 0                     # lock count
    ch.acq = acq_new(ch.code, ch.T, ch.fs, ch.N, max_dop)
    ch.trk = trk_new(ch.code, ch.T, ch.fs, sp_corr, add_corr)
    ch.nav = sdr_nav.nav_new(nav_opt)
    return ch

#-------------------------------------------------------------------------------
#  Update a receiver channel. A receiver channel is a state machine having the
#  following internal states indicated as ch.state.
#
#    state  : description
#    'SRCH' : searching signal
#    'LOCK' : tracking signal
#    'IDLE' : waiting for next signal search cycle
#
#  args:
#      ch       (I) Receiver channel
#      time     (I) Sampling time of the end of digitized IF data (s)
#      data     (I) 2 cycle samples of digitized IF data as complex64 ndarray
#
#  returns:
#      None
#
def ch_update(ch, time, data):
    if ch.state == 'SRCH':
        search_sig(ch, time, data)
    elif ch.state == 'LOCK':
        track_sig(ch, time, data)
    elif np.random.rand() * T_SRCH / ch.T < 1.0: # IDLE
        ch.state = 'SRCH'

# new signal acquisition -------------------------------------------------------
def acq_new(code, T, fs, N, max_dop):
    acq = Obj()
    acq.code_fft = sdr_code.gen_code_fft(code, T, fs, N, N) # (code + ZP) DFT
    acq.fds = dop_bins(T, max_dop)  # Doppler search bins
    acq.P_sum = np.zeros((len(acq.fds), N)) # non-coherent sum of correlations
    acq.n_sum = 0                   # numberof non-coherent sum
    return acq

# new signal tracking ----------------------------------------------------------
def trk_new(code, T, fs, sp_corr, add_corr):
    trk = Obj()
    pos = int(sp_corr * T / len(code) * fs) + 1
    trk.pos = [0, -pos, pos, -80]   # correlator position {P,E,L,N} (samples)
    if add_corr:
        trk.pos += range(-40, 41)   # additional correlator positions
    trk.err_phas = 0.0              # carrier phase error (cyc)
    trk.err_code = 0.0              # code offset error (s)
    trk.C = np.zeros(len(trk.pos), dtype='complex64') # correlator output
    trk.P = np.zeros(2000, dtype='complex64') # history of P correlator outputs
    trk.sumP = trk.sumE = trk.sumL = trk.sumN = 0.0 # sum of correlator outputs
    return trk

# initialize signal tracking ---------------------------------------------------
def trk_init(trk):
    trk.err_phas = trk.err_code = 0.0
    trk.sumP = trk.sumE = trk.sumL = trk.sumN = 0.0
    trk.C[:] = 0.0
    trk.P[:] = 0.0

# search signal ----------------------------------------------------------------
def search_sig(ch, time, data):
    ch.time = time
    
    # parallel code search and non-coherent integration
    ch.acq.P_sum += search_code(ch.acq.code_fft, ch.T, data, ch.fs, ch.fi,
        ch.acq.fds)
    ch.acq.n_sum += 1
    
    if ch.acq.n_sum * ch.T >= T_ACQ:
        
        # search max correlation power
        P_max, ix, cn0 = corr_max(ch.acq.P_sum, ch.T)
        
        if cn0 >= THRES_CN0[0]:
            fd = ch.acq.fds[ix[0]]
            coff = ix[1] / ch.fs
            start_track(ch, fd, coff, cn0)
            log(3, '$LOG,%.3f,%s,%d,SIGNAL FOUND (%.1f,%.1f,%.7f)' % (ch.time,
                ch.sig, ch.prn, cn0, fd, coff))
        else:
            ch.state = 'IDLE'
            log(3, '$LOG,%.3f,%s,%d,SIGNAL NOT FOUND (%.1f)' % (ch.time, ch.sig,
                ch.prn, cn0))
        ch.acq.P_sum[:][:] = 0.0
        ch.acq.n_sum = 0

# start tracking ---------------------------------------------------------------
def start_track(ch, fd, coff, cn0):
    ch.state = 'LOCK'
    ch.lock = 0
    ch.fd = fd
    ch.coff = coff
    ch.adr = 0.0
    ch.cn0 = cn0
    trk_init(ch.trk)
    sdr_nav.nav_init(ch.nav)

# track signal -----------------------------------------------------------------
def track_sig(ch, time, data):
    tau = time - ch.time   # time interval (s)
    ch.time = time
    fc = ch.fi + ch.fd     # IF frequency (Hz)
    ch.adr += ch.fd * tau  # accumulated Doppler (cyc)
    
    #ch.coff += 0.01 * ch.fd / ch.freq * tau
    
    # advance code offset
    i = int(ch.coff * ch.fs)
    phi = ch.fi * tau + ch.adr + fc * i / ch.fs
    coff = ch.coff - i / ch.fs
    
    # mix carrier and resample code
    data_carr = mix_carr(data[i:i+ch.N], ch.fs, fc, phi)
    code = sdr_code.res_code(ch.code, ch.T, coff, ch.fs, ch.N)
    
    # correlator outputs
    ch.trk.C = corr_std(data_carr, code, ch.trk.pos)
    
    # add correction outputs to histroy
    add_buff(ch.trk.P, ch.trk.C[0])
    ch.lock += 1
    
    # FLL/PLL, DLL and update C/N0
    if ch.lock * ch.T <= T_FPULLIN:
        FLL(ch)
    else:
        PLL(ch)
    DLL(ch)
    CN0(ch)
    
    ch.coff = np.mod(ch.coff, ch.T)
    
    # decode navigation data
    if ch.lock * ch.T >= T_NPULLIN:
        sdr_nav.nav_decode(ch)
    
    # test signal lost
    if ch.cn0 < THRES_CN0[1]:
        ch.state = 'IDLE'
        log(3, '$LOG,%.3f,%s,%d,SIGNAL LOST (%s, %.1f)' % (ch.time, ch.sig,
            ch.prn, ch.sig, ch.cn0))

# FLL --------------------------------------------------------------------------
def FLL(ch):
    if ch.lock >= 2:
        IP1 = ch.trk.P[-1].real
        QP1 = ch.trk.P[-1].imag
        IP2 = ch.trk.P[-2].real
        QP2 = ch.trk.P[-2].imag
        dot   = IP1 * IP2 + QP1 * QP2
        cross = IP1 * QP2 - QP1 * IP2
        if dot != 0.0:
            err_freq = atan(cross / dot) / 2.0 / pi / ch.T
            B = B_FLL[0] if ch.lock * ch.T < T_FPULLIN / 2 else B_FLL[1]
            ch.fd -= B * ch.T * err_freq

# PLL --------------------------------------------------------------------------
def PLL(ch):
    IP = ch.trk.C[0].real
    QP = ch.trk.C[0].imag
    if IP != 0.0:
        err_phas = -atan(QP / IP) / 2.0 / pi
        err_freq = (err_phas - ch.trk.err_phas) / ch.T
        ch.fd -= B_PLL * ch.T * err_phas + B_PLL * ch.T * err_freq # 2nd-order
        ch.trk.err_phas = err_phas

# DLL --------------------------------------------------------------------------
def DLL(ch):
    N = np.max([1, int(T_DLL / ch.T)])
    ch.trk.sumE += ch.trk.C[1]
    ch.trk.sumL += ch.trk.C[2]
    if ch.lock % N == 0:
        E = np.abs(ch.trk.sumE)
        L = np.abs(ch.trk.sumL)
        err_code = (E - L) / (E + L) * ch.trk.pos[2] / ch.fs
        ch.coff -= err_code * B_DLL / 0.25 * ch.T * N # 1st-order
        ch.trk.sumE = ch.trk.sumL = 0.0

# update C/N0 ------------------------------------------------------------------
def CN0(ch):
    ch.trk.sumP += abs(ch.trk.C[0]) ** 2
    ch.trk.sumN += abs(ch.trk.C[3]) ** 2
    if ch.lock % int(T_CN0 / ch.T) == 0:
        if ch.trk.sumN > 0.0:
            cn0 = 10.0 * log10(ch.trk.sumP / ch.trk.sumN / ch.T)
            ch.cn0 += 0.3 * (cn0 - ch.cn0)
        ch.trk.sumP = ch.trk.sumN = 0.0

