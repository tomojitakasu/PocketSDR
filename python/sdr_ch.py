#
#  Pocket SDR Python Library - GNSS SDR Receiver Channel Functions
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-12-24  1.0  new
#  2022-01-13  1.1  support tracking of L6D, L6E
#
from math import *
import numpy as np
from sdr_func import *
import sdr_code, sdr_nav

# constants --------------------------------------------------------------------
T_SRCH     = 300.0           # average of signal search interval (s)
T_ACQ      = 0.010           # non-coherent integration time for acquisition (s)
T_DLL      = 0.001           # non-coherent integration time for DLL (s)
T_CN0      = 1.0             # averaging time for C/N0 (s)
T_FPULLIN  = 1.0             # frequency pullin time (s)
T_NPULLIN  = 1.5             # navigation data pullin time (s)
B_DLL      = 0.5             # band-width of DLL filter (Hz)
B_PLL      = 5.0             # band-width of PLL filter (Hz)
B_FLL      = (10.0, 2.0)     # band-width of FLL filter (Hz) (wide, narrow)
SP_CORR    = 0.5             # default correlator spacing (chip)
MAX_DOP    = 5000.0          # default max Doppler for acquisition (Hz)
THRES_CN0  = (35.0, 32.0)    # C/N0 threshold (dB-Hz) (lock, lost)

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
#      sp_corr  (I) Correlator spacing (chips) (optional)
#      add_corr (I) Flag for additional correlator for plot (optional)
#      nav_opt  (I) Navigation data options (optional)
#
#  returns:
#      ch       Receiver channel
#
def ch_new(sig, prn, fs, fi, max_dop=MAX_DOP, sp_corr=SP_CORR, add_corr=False,
    nav_opt=''):
    ch = Obj()
    ch.state = 'SRCH'               # channel state
    ch.time = 0.0                   # receiver time
    ch.sig = sig.upper()            # signal type
    ch.prn = prn                    # PRN number
    ch.code = sdr_code.gen_code(sig, prn) # primary code
    ch.sec_code = sdr_code.sec_code(sig, prn) # secondary code
    ch.fc = sdr_code.sig_freq(sig)  # carrier frequency (Hz)
    ch.fs = fs                      # sampling freqency (Hz)
    ch.fi = shift_freq(sig, prn, fi) # IF frequency (Hz)
    ch.T = sdr_code.code_cyc(sig)   # code cycle (period) (s)
    ch.N = int(fs * ch.T)           # number of samples in a code cycle
    ch.fd = 0.0                     # Doppler frequency (Hz)
    ch.coff = 0.0                   # code offset (s)
    ch.adr = 0.0                    # accumulated Doppler (cyc)
    ch.cn0 = 0.0                    # C/N0 (dB-Hz)
    ch.lock = 0                     # lock count
    ch.lost = 0                     # signal lost count
    ch.costas = not (ch.sig == 'L6D' or ch.sig == 'L6E') # Costas PLL flag
    ch.acq = acq_new(ch.code, ch.T, fs, ch.N, max_dop)
    ch.trk = trk_new(ch.sig, ch.code, ch.T, fs, sp_corr, add_corr)
    ch.nav = sdr_nav.nav_new(nav_opt)
    return ch

#-------------------------------------------------------------------------------
#  Update a receiver channel. A receiver channel is a state machine which has
#  the following internal states indicated as ch.state. By calling the function,
#  the receiver channel search and track GNSS signals and decode navigation
#  data in the signals. The results of the signal acquisition, trackingare and
#  navigation data decoding are output as log messages. The internal status are
#  also accessed as object instance variables of the receiver channel after
#  calling the function. The function should be called in the cycle of GNSS
#  signal code with 2-cycle samples of digitized IF data (which are overlapped
#  between previous and current). 
#
#    'SRCH' : signal acquisition state
#    'LOCK' : signal tracking state
#    'IDLE' : waiting for a next signal acquisition cycle
#
#  args:
#      ch       (I) Receiver channel
#      time     (I) Sampling time of the end of digitized IF data (s)
#      data     (I) 2-cycle samples of digitized IF data as complex64 ndarray
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
    acq.code_fft = sdr_code.gen_code_fft(code, T, 0.0, fs, N, N) # (code + ZP) DFT
    acq.fds = dop_bins(T, max_dop)  # Doppler search bins
    acq.P_sum = np.zeros((len(acq.fds), N)) # non-coherent sum of correlations
    acq.n_sum = 0                   # number of non-coherent sum
    return acq

# new signal tracking ----------------------------------------------------------
def trk_new(sig, code, T, fs, sp_corr, add_corr):
    trk = Obj()
    pos = int(sp_corr * T / len(code) * fs) + 1
    trk.pos = [0, -pos, pos, -80]   # correlator positions {P,E,L,N} (samples)
    if add_corr:
        trk.pos += range(-40, 41)   # additional correlator positions
    trk.C = np.zeros(len(trk.pos), dtype='complex64') # correlator outputs
    trk.P = np.zeros(2000, dtype='complex64') # history of P correlator outputs
    trk.err_phas = 0.0              # carrier phase error (cyc)
    trk.sumP = trk.sumE = trk.sumL = trk.sumN = 0.0 # sum of correlator outputs
    if sig == 'L6D' or sig == 'L6E':
        trk.code = sdr_code.gen_code_fft(code, T, 0.0, fs, int(fs * T))
    else:
        trk.code = sdr_code.res_code(code, T, 0.0, fs, int(fs * T))
    return trk

# initialize signal tracking ---------------------------------------------------
def trk_init(trk):
    trk.err_phas = 0.0
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
    fc = ch.fi + ch.fd     # IF carrier frequency with Doppler (Hz)
    ch.adr += ch.fd * tau  # accumulated Doppler (cyc)
    ch.coff -= ch.fd / ch.fc * tau # carrier-aided code offset (s)
    ch.time = time
    
    # code position (samples) and carrier phase (cyc)
    i = int(ch.coff * ch.fs + 0.5) % ch.N
    phi = ch.fi * tau + ch.adr + fc * i / ch.fs
    
    # mix carrier
    data_carr = mix_carr(data[i:i+ch.N], ch.fs, fc, phi)
    
    if ch.sig == 'L6D' or ch.sig == 'L6E':
        # FFT correlator
        C = corr_fft(data_carr, ch.trk.code) / ch.N
        
        # decode L6 CSK
        ch.trk.C = CSK(ch, C)
    else:
        # standard correlator
        ch.trk.C = corr_std(data_carr, ch.trk.code, ch.trk.pos)
    
    # add P correlator outputs to histroy
    add_buff(ch.trk.P, ch.trk.C[0])
    ch.lock += 1
    
    # FLL/PLL, DLL and update C/N0
    if ch.lock * ch.T <= T_FPULLIN:
        FLL(ch)
    else:
        PLL(ch)
    DLL(ch)
    CN0(ch)
    
    # decode navigation data
    if ch.lock * ch.T >= T_NPULLIN:
        sdr_nav.nav_decode(ch)
    
    if ch.cn0 < THRES_CN0[1]: # signal lost
        ch.state = 'IDLE'
        ch.lost += 1
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
            B = B_FLL[0] if ch.lock * ch.T < T_FPULLIN / 2 else B_FLL[1]
            err_freq = atan(cross / dot) if ch.costas else atan2(cross, dot)
            ch.fd -= B / 0.25 * err_freq / 2.0 / pi

# PLL --------------------------------------------------------------------------
def PLL(ch):
    IP = ch.trk.C[0].real
    QP = ch.trk.C[0].imag
    if IP != 0.0:
        err_phas = (atan(QP / IP) if ch.costas else atan2(QP, IP)) / 2.0 / pi
        W = B_PLL / 0.53
        ch.fd += 1.4 * W * (err_phas - ch.trk.err_phas) + W * W * err_phas * ch.T
        ch.trk.err_phas = err_phas

# DLL --------------------------------------------------------------------------
def DLL(ch):
    N = np.max([1, int(T_DLL / ch.T)])
    ch.trk.sumE += ch.trk.C[1]
    ch.trk.sumL += ch.trk.C[2]
    if ch.lock % N == 0:
        E = np.abs(ch.trk.sumE)
        L = np.abs(ch.trk.sumL)
        err_code = (E - L) / (E + L) / 2.0 * ch.T / len(ch.code) # (s)
        ch.coff -= B_DLL / 0.25 * err_code * ch.T * N
        ch.trk.sumE = ch.trk.sumL = 0.0

# update C/N0 ------------------------------------------------------------------
def CN0(ch):
    ch.trk.sumP += np.abs(ch.trk.C[0]) ** 2
    ch.trk.sumN += np.abs(ch.trk.C[3]) ** 2
    if ch.lock % int(T_CN0 / ch.T) == 0:
        if ch.trk.sumN > 0.0:
            cn0 = 10.0 * log10(ch.trk.sumP / ch.trk.sumN / ch.T)
            ch.cn0 += 0.5 * (cn0 - ch.cn0)
        ch.trk.sumP = ch.trk.sumN = 0.0

# decode L6 CSK ----------------------------------------------------------------
def CSK(ch, C):
    R = ch.N / (len(ch.code) // 2) # samples / chips
    n = int(280 * R)
    C = np.hstack([C[-n:], C[:n]])
    
    # interpolate correlation powers
    P = np.interp(np.arange(-256, 257) * R, np.arange(-n, n), np.abs(C))
    
    # decode CSK and add symbol to buffer
    ix = np.argmax(P) - 256
    add_buff(ch.nav.syms, 255 - ix % 256)
    
    # generate correlator outputs
    return np.interp(ix * R + np.array(ch.trk.pos) / 2, np.arange(-n, n), C)
