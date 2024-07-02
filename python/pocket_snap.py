#!/usr/bin/env python3
#
#  Pocket SDR Python AP - Snapshot Positioning
#
#  Author:
#  T.TAKASU
#
#  History:
#  2022-02-14  1.0  new
#  2022-07-08  1.1  add option -v
#
import sys, math, time, re
import numpy as np
import sdr_func, sdr_code
from sdr_rtk import *

# constants --------------------------------------------------------------------
THRES_CN0 = 37.0    # threshold to lock signal (dB-Hz)
EL_MASK   = 15.0    # elevation mask (deg)
MAX_DOP   = 5000.0  # max Doppler freq. to search signal (Hz)
MAX_DFREQ = 500.0   # max freq. offset of ref oscillator (Hz)
VERP      = 0       # verpose display flag

# global variables -------------------------------------------------------------
code_fft = {}       # code FFT caches

# show usage -------------------------------------------------------------------
def show_usage():
    print('Usage: pocket_snap.py [-ts time] [-pos lat,lon,hgt] [-ti sec] [-toff toff]')
    print('       [-f freq] [-fi freq] [-tint tint] [-sys sys[,...]] [-v] -nav file')
    print('       [-out file] file')
    exit()

# euclid norm ------------------------------------------------------------------
def norm(x):
    return math.sqrt(np.dot(x, x))

# position string --------------------------------------------------------------
def pos_str(rr):
    pos = ecef2pos(rr)
    return '%13.9f %14.9f %12.3f' % (pos[0] * R2D, pos[1] * R2D, pos[2])

# select satellite -------------------------------------------------------------
def sel_sat(time, sys, prn, rr, nav):
    if len(rr) == 0: # no coarse position
        return PI / 2.0, 0.0
    rs, dts, var, svh = satpos(time, time, satno(sys, prn), nav)
    if norm(rs) < 1e-3 or svh:
        return 0.0, 0.0
    rho, e = geodist(rs, rr)
    azel = satazel(ecef2pos(rr), e)
    return azel[1], np.dot(rs[3:], e) # el, rrate

# satellite position, velocity and clock rate ----------------------------------
def sat_pos(time, data, nav):
    spos = []
    for i in range(len(data)):
        rs, dts, var, svh = satpos(time, time, data[i][0], nav)
        if svh != 0:
            rs[:] = 0.0
        spos.append([rs[:3], rs[3:], CLIGHT * dts[0], CLIGHT * dts[1]])
    return spos

# fine code offset -------------------------------------------------------------
def fine_coff(sig, fs, P, coffs, ix):
    T = sdr_code.code_cyc(sig) / len(sdr_code.gen_code(sig, 1)) # (s/chip)
    E = np.sqrt(P[(ix - 1) % len(P)])
    L = np.sqrt(P[(ix + 1) % len(P)])
    return coffs[ix] + (L - E) / (L + E) * (T / 2.0 - 1.0 / fs)

# search signal ----------------------------------------------------------------
def search_sig(data, sig, sys, prn, dif, fs, fi, rrate):
    sat = satno(sys, prn)
    T = sdr_code.code_cyc(sig)
    N = int(fs * T)
    
    # generate code FFT
    if sat not in code_fft:
        code = sdr_code.gen_code(sig, prn)
        code_fft[sat] = sdr_code.gen_code_fft(code, T, 0.0, fs, N, N)
    
    # doppler search bins
    if rrate == 0.0:
        fds = sdr_func.dop_bins(T, 0.0, MAX_DOP)
    else:
        dop = -rrate / CLIGHT * sdr_code.sig_freq(sig)
        fds = sdr_func.dop_bins(T, dop, MAX_DFREQ)
    
    # parallel code search
    P = np.zeros((len(fds), N), dtype='float32')
    for i in range(0, len(dif) - len(code_fft[sat]) + 1, N):
        P += sdr_func.search_code(code_fft[sat], T, dif, i, fs, fi, fds)
    
    # max correlation power
    P_max, ix, cn0 = sdr_func.corr_max(P, T)
    
    if cn0 >= THRES_CN0:
        dop = sdr_func.fine_dop(P.T[ix[1]], fds, ix[0])
        rrate = -dop * CLIGHT / sdr_code.sig_freq(sig)
        coff = fine_coff(sig, fs, P[ix[0]], np.arange(0, T, 1.0 / fs), ix[1])
        if VERP:
            print('%s : SIG=%-5s C/N0=%5.1f dB-Hz DOP=%9.3f Hz COFF=%12.9f ms' %
                (satno2id(sat), sig, cn0, dop, coff * 1e3))
        data.append([sat, rrate, coff])

# search signals ---------------------------------------------------------------
def search_sigs(time, ssys, dif, fs, fi, rr, nav):
    if VERP:
        print('search_sigs')
    data = []
    if ssys & SYS_GPS:
        for prn in range(1, 33):
            el, rrate = sel_sat(time, SYS_GPS, prn, rr, nav)
            if el >= EL_MASK * D2R:
                search_sig(data, 'L1CA', SYS_GPS, prn, dif, fs, fi, rrate)
    if ssys & SYS_GAL:
        for prn in range(1, 37):
            el, rrate = sel_sat(time, SYS_GAL, prn, rr, nav)
            if el >= EL_MASK * D2R:
                search_sig(data, 'E1C' , SYS_GAL, prn, dif, fs, fi, rrate)
    if ssys & SYS_CMP:
        for prn in range(19, 47):
            el, rrate = sel_sat(time, SYS_CMP, prn, rr, nav)
            if el >= EL_MASK * D2R:
                search_sig(data, 'B1CP', SYS_CMP, prn, dif, fs, fi, rrate)
    if ssys & SYS_QZS:
        for prn in range(193, 200):
            el, rrate = sel_sat(time, SYS_QZS, prn, rr, nav)
            if el >= EL_MASK * D2R:
                search_sig(data, 'L1CP', SYS_QZS, prn, dif, fs, fi, rrate)
    return data

# drdot/dx ---------------------------------------------------------------------
def drdot_dx(rs, vs, x):
    dx = 10.0
    rho, e = geodist(rs, x[:3])
    rdot = np.dot(vs, e)
    rho, e1 = geodist(rs, x[:3] + [dx, 0, 0])
    rho, e2 = geodist(rs, x[:3] + [0, dx, 0])
    rho, e3 = geodist(rs, x[:3] + [0, 0, dx])
    return [(np.dot(vs, e1) - rdot) / dx, (np.dot(vs, e2) - rdot) / dx,
            (np.dot(vs, e3) - rdot) / dx, 1.0]

# position by Doppler ----------------------------------------------------------
def pos_dop(data, spos):
    if VERP:
        print('pos_dop')
    N = len(data)
    x = np.zeros(4)
    v = np.zeros(N)
    H = np.zeros((N, 4))
    
    for i in range(10):
        n = 0
        for j in range(N):
            if norm(spos[j][0]) > 1e-3:
                rho, e = geodist(spos[j][0], x)
                v[n] = data[j][1] - (np.dot(spos[j][1], e) + x[3] - spos[j][3])
                H[n,:] = drdot_dx(spos[j][0], spos[j][1], x)
                n += 1
        if n < 4:
            return np.zeros(3)
        dx, res, rank, s = np.linalg.lstsq(H[:n,:], v[:n], rcond=None)
        if VERP:
            print('(%d) N=%2d  POS=%s  RES=%10.3f m/s' % (i, n, pos_str(x), norm(v)))
        x += dx
        if norm(dx) < 1.0:
            return x[:3]
    return np.zeros(3)

# resolve ms ambiguity in code offset -----------------------------------------
def res_coff_amb(data, spos, rr):
    if VERP:
        print('res_coff_amb')
    N = len(data)
    tau = np.ones(N) * 1e9
    for i in range(N):
        if norm(spos[i][0]) > 1e-3:
            r, e = geodist(spos[i][0], rr)
            tau[i] = (r - spos[i][2]) / CLIGHT
    idx = np.argmin(tau)
    coff_ref = data[idx][2]
    tau_ref = tau[idx]
    for i in range(N):
        if norm(spos[i][0]) > 1e-3:
            off = (tau[i] - tau_ref) - (data[i][2] - coff_ref)
            data[i][2] += np.round(off * 1e3) * 1e-3 
            if VERP:
                print('%s - %s: N=%8.5f -> %8.5f' % (satno2id(data[i][0]),
                    satno2id(data[idx][0]), off * 1e3, np.round(off * 1e3)))
    
# estimate position by code offsets ------------------------------------------
def pos_coff(time, data, rr, nav):
    if VERP:
        print('pos_coff')
    N = len(data)
    x = np.zeros(5)
    x[:3] = rr
    v = np.zeros(N)
    H = np.zeros((N, 5))
    
    for i in range(10):
        n = 0
        pos = ecef2pos(x)
        for j in range(N):
            ts = timeadd(time, x[4])
            rs, dts, var, svh = satpos(ts, time, data[j][0], nav)
            rho, e = geodist(rs, x)
            azel = satazel(pos, e)
            if norm(rs) > 1e-3 and svh == 0 and azel[1] >= EL_MASK * D2R:
                v[n] = CLIGHT * data[j][2] - (rho + x[3] - CLIGHT * dts[0] + \
                    ionmodel(ts, nav, pos, azel) + tropmodel(ts, pos, azel) + \
                    get_tgd(data[j][0], nav))
                H[n,:] = np.hstack([-e, 1.0, np.dot(rs[3:], e)])
                n += 1
        if n < 5:
            break
        dx, res, rank, s = np.linalg.lstsq(H[:n,:], v[:n], rcond=None)
        if VERP:
            print('(%d) N=%2d  POS=%s  CLK=%9.6f  DT=%9.6f  RES=%10.3f m' % (i,
                n, pos_str(x), x[3] / CLIGHT, x[4], np.sqrt(np.dot(v, v) / n)))
        x += dx
        if norm(dx) < 1e-3:
            if np.sqrt(np.dot(v, v) / n) > 1e3:
                break
            return x[:3], x[3] / CLIGHT - x[4], n
    return np.zeros(3), 0.0, 0

# write solution header --------------------------------------------------------
def write_head(fp, file, tint, fs):
    fp.write('% SNAPSHOT POSITION by POCKET_SNAP.PY\n')
    fp.write('%% INPUT FILE    : %s\n' % (file))
    fp.write('%% SAMPLING TIME : %.1f ms / SNAPSHOT\n' % (tint * 1e3))
    fp.write('%% SAMPLING FREQ : %.3f MHz\n' % (fs / 1e6))
    fp.write('%%  %-21s  %13s %12s %12s %4s %4s\n' % ('UTC', 'latitude(deg)',
        'longitude(deg)', 'height(m)', 'Q', 'ns'))

# parse time -------------------------------------------------------------------
def parse_time(str):
    return utc2gpst(epoch2time([float(s) for s in re.split('[/:_-]', str)]))

# parse navigation system ------------------------------------------------------
def parse_sys(str):
    sys = SYS_NONE
    for s in str.split(','):
        if s == 'G':
            sys |= SYS_GPS
        elif s == 'E':
            sys |= SYS_GAL
        elif s == 'J':
            sys |= SYS_QZS
        elif s == 'C':
            sys |= SYS_CMP
    return sys

# get capture time by file path ------------------------------------------------
def path_time(file):
    m = re.search('_(....)(..)(..)_(..)(..)(..).bin', file)
    return utc2gpst(epoch2time([float(s) for s in m.group(1, 2, 3, 4, 5, 6)]))

#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     pocket_snap.py [-ts time] [-pos lat,lon,hgt] [-ti sec] [-toff toff]
#         [-f freq] [-fi freq] [-tint tint] [-sys sys[,...]] -nav file
#         [-out file] file
# 
#   Description
# 
#     Snapshot positioning with GNSS signals in digitized IF file.
# 
#   Options ([]: default)
#  
#     -ts time
#         Captured start time in UTC as YYYY/MM/DD HH:mm:ss format.
#         [parsed by file name]
#
#     -pos lat,lon,hgt
#         Coarse receiver position as latitude, longitude in degree and
#         height in m. [no coarse position]
# 
#     -ti sec
#         Time interval of positioning in seconds. (0.0: single) [0.0]
# 
#     -toff toff
#         Time offset from the start of digital IF data in seconds. [0.0]
# 
#     -f freq
#         Sampling frequency of digital IF data in MHz. [12.0]
#
#     -fi freq
#         IF frequency of digital IF data in MHz. The IF frequency equals 0, the
#         IF data is treated as IQ-sampling (zero-IF). [0.0]
#
#     -tint tint
#         Integration time for signal search in msec. [20.0]
#
#     -sys sys[,...]
#         Select navigation system(s) (G=GPS,E=Galileo,J=QZSS,C=BDS). [G]
#
#     -v
#         Enable verpose status display.
#
#     -nav file
#         RINEX navigation data file.
#
#     -out file
#         Output solution file as RTKLIB solution format.
#
#     file
#         Digitized IF data file.
#
if __name__ == '__main__':
    ts, ti, toff, fs, fi, tint = GTIME(), 0.0, 0.0, 6.0, 0.0, 0.020
    rr = []
    file, nfile, ofile = '', '', ''
    fp = sys.stdout
    ssys = SYS_GPS
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-ts':
            i += 1
            ts = parse_time(sys.argv[i])
        elif sys.argv[i] == '-pos':
            i += 1
            pos = [float(s) for s in sys.argv[i].split(',')]
            pos[0] *= D2R
            pos[1] *= D2R
            rr = pos2ecef(pos)
        elif sys.argv[i] == '-ti':
            i += 1
            ti = float(sys.argv[i])
        elif sys.argv[i] == '-toff':
            i += 1
            toff = float(sys.argv[i])
        elif sys.argv[i] == '-f':
            i += 1
            fs = float(sys.argv[i]) * 1e6
        elif sys.argv[i] == '-fi':
            i += 1
            fi = float(sys.argv[i]) * 1e6
        elif sys.argv[i] == '-tint':
            i += 1
            tint = float(sys.argv[i]) * 1e-3
        elif sys.argv[i] == '-sys':
            i += 1
            ssys = parse_sys(sys.argv[i])
        elif sys.argv[i] == '-nav':
            i += 1
            nfile = sys.argv[i]
        elif sys.argv[i] == '-out':
            i += 1
            ofile = sys.argv[i]
        elif sys.argv[i] == '-v':
            VERP = 1
        elif sys.argv[i][0] == '-':
            show_usage()
        else:
            file = sys.argv[i];
        i += 1
    
    # read RINEX NAV
    obs, nav = readrnx(nfile)
    if not nav:
        print('nav data read error %s' % (nfile))
        exit()
    
    # get capture time by file path
    if ts.time == 0:
        ts = path_time(file)
    
    if ofile:
        fp = open(ofile, 'w')
        write_head(fp, file, tint, fs)
    
    t0 = time.time()
    
    for i in range(100000):
        tt = timeadd(ts, toff + ti * i)
        
        if ti <= 0.0 and i >= 1: # single snapshot
            break
        
        # read DIF data
        IQ = 1 if fi > 0 else 2
        dif = sdr_func.read_data(file, fs, IQ, tint, toff + ti * i)
        if len(dif) == 0:
            break
        
        # search signals
        data = search_sigs(tt, ssys, dif, fs, fi, rr, nav)
        if len(data) <= 0:
            continue
        
        # satellite position and velocity
        spos = sat_pos(tt, data, nav)
        
        if len(rr) == 0:
            # position by doppler
            rr = pos_dop(data, spos)
            
            # force height = 0
            pos = ecef2pos(rr)
            pos[2] = 0.0
            rr = pos2ecef(pos)
        
        # resolve ms ambiguity in code offsets
        res_coff_amb(data, spos, rr)
        
        # estimate position by code offsets
        rr, dtr, ns = pos_coff(tt, data, rr, nav)
        
        # write solution
        fp.write('%s   %s %4d %4d\n' % (time2str(gpst2utc(timeadd(tt, -dtr)), 3),
            pos_str(rr), 5, ns))
         
    print('TIME (s) = %.3f' % (time.time() - t0))
    navfree(nav)
    obsfree(obs)
    fp.close()

