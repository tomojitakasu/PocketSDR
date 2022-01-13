#!/usr/bin/env python3
#
#  Pocket SDR Python AP - GNSS Signal Tracking
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-12-01  1.0  new
#  2022-01-13  1.1  add input from stdout
#                   add options -IQ, -e, -yl, -q
#
import sys, math, time
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from sdr_func import *
import sdr_code, sdr_ch

# plot settings ----------------------------------------------------------------
window = 'PocketSDR - GNSS SIGNAL TRACKING'
size   = (9, 6)
ylim   = 0.3
fc, bc, lc, gc = 'darkblue', 'w', 'k', 'grey'
#fc, bc, lc, gc = 'darkgreen', 'w', '#404040', 'grey'
#fc, bc, lc, gc = 'yellow', '#000040', 'silver', 'grey'
mpl.rcParams['toolbar'] = 'None'
mpl.rcParams['font.size'] = 9
mpl.rcParams['text.color'] = lc
mpl.rcParams['axes.facecolor'] = bc
mpl.rcParams['axes.edgecolor'] = lc
rect0  = [0.080, 0.040, 0.840, 0.910]
rect1  = [0.080, 0.568, 0.490, 0.380]
rect2  = [0.650, 0.568, 0.280, 0.380]
rect3  = [0.080, 0.198, 0.840, 0.320]
rect4  = [0.080, 0.040, 0.840, 0.110]

# read IF data -----------------------------------------------------------------
def read_data(fp, fs, IQ, T):
    cnt = int(fs * T * IQ)
    
    if fp == None:
        raw = np.frombuffer(sys.stdin.buffer.read(cnt), dtype='int8')
    else:
        raw = np.frombuffer(fp.read(cnt), dtype='int8')
    
    if IQ == 1: # I
        return np.array(raw, dtype='complex64')
    else: # IQ
        #return np.array(raw[0::2] + raw[1::2] * 1j, dtype='complex64')
        return np.array(raw[0::2] - raw[1::2] * 1j, dtype='complex64')

# print receiver channel status header -----------------------------------------
def print_head():
    print('%9s %5s %4s %5s %9s %5s %-13s %10s %8s %11s %4s %4s %4s %4s %3s' %
        ('TIME(s)', 'SIG', 'PRN', 'STATE', 'LOCK(s)', 'C/N0', '(dB-Hz)',
        'COFF(ms)', 'DOP(Hz)', 'ADR(cyc)', 'SYNC', '#NAV', '#ERR', '#LOL', 'NER'))

# update receiver channel status -----------------------------------------------
def update_stat(prns, ch, ncol):
    if ncol > 0: # cursor up
        print('\033[%dF' % (ncol), end='')
    ncol = 0
    for i in range(len(prns)):
        nav_sync = ('B' if ch[i].nav.ssync > 0 else '-') + \
            ('F' if ch[i].nav.fsync > 0 else '-') + \
            ('R' if ch[i].nav.rev else '-')
        print('%s%9.2f %5s %4d %5s %9.2f %5.1f %-13s %10.7f %8.1f %11.1f  %s %4d %4d %4d %3d%s' %
            ('\033[32m' if ch[i].state == 'LOCK' else '',
            ch[i].time, ch[i].sig, prns[i], ch[i].state, ch[i].lock * ch[i].T,
            ch[i].cn0, cn0_bar(ch[i].cn0), ch[i].coff * 1e3, ch[i].fd, ch[i].adr,
            nav_sync, ch[i].nav.count[0], ch[i].nav.count[1], ch[i].lost,
            ch[i].nav.nerr, '\033[0m' if ch[i].state == 'LOCK' else ''))
        ncol += 1
    return ncol

# C/N0 bar ---------------------------------------------------------------------
def cn0_bar(cn0):
    return '|' * np.min([int((cn0 - 30.0) / 1.5), 13])

# initialize plot --------------------------------------------------------------
def init_plot(sig, prn, ch, env, file):
    fig = plt.figure(window, figsize=size, facecolor=bc)
    ax0 = fig.add_axes(rect0)
    ax0.axis('off')
    ax0.set_title('SIG = %s, PRN = %3d, FILE = %s' % (sig, prn, file),
        fontsize=10)
    Tc = ch.T / sdr_code.code_len(sig)
    pos = np.array(ch.trk.pos) / ch.fs / Tc
    ax1, p1 = plot_corr_env (fig, rect1, env, pos)
    ax2, p2 = plot_corr_IQ  (fig, rect2)
    ax3, p3 = plot_corr_time(fig, rect3)
    ax4, p4 = plot_nav_data (fig, rect4)
    return fig, (ax0, ax1, ax2, ax3, ax4), ([], p1, p2, p3, p4)

# update plot -----------------------------------------------------------------
def update_plot(fig, ax, p, ch, env, toff, tspan):
    update_corr_env (ax[1], p[1], ch, env)
    update_corr_IQ  (ax[2], p[2], ch, tspan)
    update_corr_time(ax[3], p[3], ch, toff, tspan)
    update_nav_data (ax[4], p[4], ch)
    plt.pause(1e-3)

# plot correlation envelope ---------------------------------------------------
def plot_corr_env(fig, rect, env, pos):
    ax = fig.add_axes(rect)
    p0 = ax.plot([], [], '-', color=gc, lw=0.4)
    p1 = ax.plot([], [], '.', color=gc, ms=2)
    p2 = ax.plot([], [], '.', color=fc, ms=10)
    xl = [pos[4], pos[-1]]
    if env:
        yl = [0, ylim]
        text = 'SQRT(I^2+Q^2)'
    else:
        yl = [-ylim / 3.0, ylim]
        text = 'I * sign(IP)'
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.vlines(0.0, yl[0], yl[1], color=lc, lw=0.4)
    ax.hlines(0.0, xl[0], xl[1], color=lc, lw=0.4)
    ax.plot(0.0, 0.0, '.', color=lc, ms=6)
    ax.grid(True, lw=0.4)
    set_axcolor(ax, lc)
    ax.text(0.03, 0.95, text, ha='left', va='top', transform=ax.transAxes)
    ax.text(1.00, -0.04, '(chip)', ha='left', va='top', transform=ax.transAxes)
    p3 = ax.text(0.97, 0.95, '', color=fc, ha='right', va='top',
        transform=ax.transAxes)
    return ax, (p0, p1, p2, p3)

# update correlation envelope --------------------------------------------------
def update_corr_env(ax, p, ch, env):
    Tc = ch.T / sdr_code.code_len(sig)
    pos = np.array(ch.trk.pos) / ch.fs / Tc
    if env:
        EPL = np.abs(ch.trk.C[:3])
        COR = np.abs(ch.trk.C[4:])
    else:
        sign = np.sign(ch.trk.C[0].real)
        EPL = ch.trk.C[:3].real * sign
        COR = ch.trk.C[4:].real * sign
    p[0][0].set_data(pos[4:], COR)
    p[1][0].set_data(pos[4:], COR)
    p[2][0].set_data(pos[:3], EPL)
    p[3].set_text('E=%6.3f P=%6.3f L=%6.3f' % (EPL[1], EPL[0], EPL[2]))

# plot correlation I-Q ---------------------------------------------------------
def plot_corr_IQ(fig, rect):
    ax = fig.add_axes(rect)
    p0 = ax.plot([], [], '.', color=gc, ms=1)
    p1 = ax.plot([], [], '.', color=fc, ms=10)
    ax.grid(True, lw=0.4)
    ax.grid(True, lw=0.4)
    ax.vlines(0.0, -ylim, ylim, color=lc, lw=0.4)
    ax.hlines(0.0, -ylim, ylim, color=lc, lw=0.4)
    ax.plot(0.0, 0.0, '.', color=lc, ms=6)
    ax.set_xlim([-ylim, ylim])
    ax.set_ylim([-ylim, ylim])
    ax.set_aspect('equal')
    set_axcolor(ax, lc)
    ax.text(0.05, 0.95, 'IP - QP', ha='left', va='top', transform=ax.transAxes)
    p2 = ax.text(0.95, 0.95, '', ha='right', va='top', color=fc,
        transform=ax.transAxes)
    return ax, (p0, p1, p2)

# update correlation I-Q -------------------------------------------------------
def update_corr_IQ(ax, p, ch, tspan):
    N = np.min([int(tspan / ch.T), len(ch.trk.P)])
    p[0][0].set_data(ch.trk.P[-N:].real, ch.trk.P[-N:].imag)
    p[1][0].set_data(ch.trk.P[-1].real, ch.trk.P[-1].imag)
    p[2].set_text('IP=%6.3f\nQP=%6.3f' % (ch.trk.P[-1].real, ch.trk.P[-1].imag))

# plot correlation to time ------------------------------------------------------
def plot_corr_time(fig, rect):
    ax = fig.add_axes(rect)
    p0 = ax.plot([], [], '-', color=gc, lw=0.3, ms=2)
    p1 = ax.plot([], [], '-', color=fc, lw=0.5, ms=2)
    p2 = ax.plot([], [], '.', color=gc, ms=10)
    p3 = ax.plot([], [], '.', color=fc, ms=10)
    p4 = ax.plot([], [], '.', color='r', ms=3)
    ax.grid(True, lw=0.4)
    ax.set_ylim(-ylim, ylim)
    set_axcolor(ax, lc)
    ax.text(0.955, 0.95, 'IP', ha='right', va='top', color=fc,
        transform=ax.transAxes)
    ax.text(0.985, 0.95, 'QP', ha='right', va='top', color=gc,
        transform=ax.transAxes)
    ax.text(0.985, 0.05, '(s)', ha='right', va='bottom', transform=ax.transAxes)
    p5 = ax.text(0.015, 0.95, '', ha='left', va='top', transform=ax.transAxes)
    p6 = ax.text(0.015, 0.05, '', ha='left', va='bottom', transform=ax.transAxes)
    return ax, (p0, p1, p2, p3, p4, p5, p6)

# update correlation to time ----------------------------------------------------
def update_corr_time(ax, p, ch, toff, tspan):
    N = np.min([int(tspan / ch.T), len(ch.trk.P)])
    time = ch.time + np.arange(-N+1, 1) * ch.T
    P = ch.trk.P[-N:]
    p[0][0].set_data(time, P.imag)
    p[1][0].set_data(time, P.real)
    p[2][0].set_data(ch.time, P[-1].imag)
    p[3][0].set_data(ch.time, P[-1].real)
    #p[4][0].set_data(ch.nav.tsyms, np.zeros(len(ch.nav.tsyms))) # for debug
    p[5].set_text(('T=%7.3f s COFF=%10.7f ms DOP=%8.1f Hz ADR=%10.1f cyc ' +
        'C/N0=%5.1f dB-Hz') % (ch.time, ch.coff * 1e3, ch.fd, ch.adr, ch.cn0))
    nav_sync = ('B' if ch.nav.ssync > 0 else '-') + \
        ('F' if ch.nav.fsync > 0 else '-') + ('R' if ch.nav.rev else '-')
    p[6].set_text('SYNC=%s #NAV=%4d #ERR=%2d #LOL=%2d NER=%2d SEQ=%6d' % (nav_sync,
        ch.nav.count[0], ch.nav.count[1], ch.lost, ch.nav.nerr, ch.nav.seq))
    t0 = toff if ch.lock < N else ch.time - N * ch.T
    ax.set_xlim(t0, t0 + N * ch.T * 1.008)

# plot nav data ----------------------------------------------------------------
def plot_nav_data(fig, rect):
    ax = fig.add_axes(rect)
    ax.grid(False)
    ax.set_xticks([])
    ax.set_yticks([])
    p0 = ax.text(-0.035, 0.48, 'NAV\nDATA', ha='center', va='center',
        transform=ax.transAxes)
    p1 = ax.text(0.01, 0.92, '', ha='left', va='top', color=fc,
        transform=ax.transAxes, fontname='monospace')
    return ax, (p0, p1)

# update nav data --------------------------------------------------------------
def update_nav_data(ax, p, ch):
    N = len(ch.nav.data)
    text = ''
    for i in range(0 if N <= 4 else N - 4, N):
        text += '%7.2f: ' % (ch.nav.data[i][0])
        for j in range(len(ch.nav.data[i][1])):
            text += '%02X' % (ch.nav.data[i][1][j])
            if j >= 42:
                text += '...'
                break
        text += '\n'
    p[1].set_text(text)

# set axes colors ---------------------------------------------------------------
def set_axcolor(ax, color):
    ax.tick_params(color=color)
    plt.setp(ax.get_xticklabels(), color=color)
    plt.setp(ax.get_yticklabels(), color=color)

# show usage -------------------------------------------------------------------
def show_usage():
    print('Usage: pocket_trk.py [-sig sig] [-prn prn[,...]] [-p] [-e] [-toff toff] [-f freq]\n' +
          '           [-fi freq] [-IQ] [-ti tint] [-ts tspan] [-yl ylim] [-log file] [-q] file')
    exit()

#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     pocket_trk.py [-sig sig] [-prn prn[,...]] [-p] [-e] [-toff toff] [-f freq]
#         [-fi freq] [-IQ] [-ti tint] [-ts tspan] [-yl ylim] [-log file] [-q] [file]
# 
#   Description
# 
#     It tracks GNSS signals in digital IF data and decode navigation data in
#     the signals.
#     If single PRN number by -prn option, it plots correlation power and
#     correlation shape of the specified GNSS signal. If multiple PRN numbers
#     specified by -prn option, it plots C/N0 for each PRN.
# 
#   Options ([]: default)
#  
#     -sig sig
#         GNSS signal type (L1CA, L1CB, L1CP, L1CD, L2CM, L5I, L5Q, L5SI, L5SQ,
#         L6D, L6E, G1CA, G2CA, E1B, E1C, E5AI, E5AQ, E5BI, E5BQ, E6B, E6C, B1I,
#         B1CD, B1CP, B2I, B2AD, B2AP, B2BI, B3I). [L1CA]
# 
#     -prn prn[,...]
#         PRN numbers of the GNSS signal separated by ','. A PRN number can be a
#         PRN number range like 1-32 with start and end PRN numbers. For GLONASS
#         FDMA signals (G1CA, G2CA), the PRN number is treated as FCN (frequency
#         channel number). [1]
# 
#     -p
#         Plot signal tracking status in an integrated window. The window shows
#         correlation envelope, correlation I-Q plot, correlation I/Q to time
#         plot and navigation data decoded. You easily find the signal tracking
#         situation. If multiple PRN number specified in -prn option, only the
#         signal with the first PRN number is plotted. [no plot]
#
#     -e
#         Plot correlation shape as an envelop (SQRT(I^2+Q^2)). [I*sign(IP)]
# 
#     -toff toff
#         Time offset from the start of digital IF data in s. [0.0]
# 
#     -f freq
#         Sampling frequency of digital IF data in MHz. [12.0]
#
#     -fi freq
#         IF frequency of digital IF data in MHz. The IF frequency is equal 0,
#         the IF data is treated as IQ-sampling without -IQ option (zero-IF).
#         [0.0]
#
#     -IQ
#         IQ-sampling even if the IF frequency is not equal 0.
#
#     -ti tint
#         Update interval of signal tracking status, plot and log in s. [0.05]
#
#     -ts tspan
#         Time span for correlation to time plot in s. [1.0]
#
#     -yl ylim
#         Y-axis limit of plots. [0.3]
#
#     -log file
#         Log file path to output signal tracking status. The log includes decoded
#         navigation data and code offset,  including navigation data decoded.
#
#     -q
#         Suppress showing signal tracking status.
#
#     [file]
#         File path of the input digital IF data. The format should be a series of
#         int8_t (signed byte) for real-sampling (I-sampling) or interleaved int8_t
#         for complex-sampling (IQ-sampling). PocketSDR and AP pocket_dump can be
#         used to capture such digital IF data. If the option omitted, the input
#         is taken from stdin.
#
if __name__ == '__main__':
    sig, prns, plot, env = 'L1CA', [1], False, False
    fs, fi, IQ, toff, tint, tspan = 12e6, 0.0, 1, 0.0, 0.05, 1.0
    ch = {}
    file, log_file, log_lvl, quiet = '', '', 4, 0
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-sig':
            i += 1
            sig = sys.argv[i]
        elif sys.argv[i] == '-prn':
            i += 1
            prns = parse_nums(sys.argv[i])
        elif sys.argv[i] == '-p':
            plot = True
        elif sys.argv[i] == '-e':
            env = True
        elif sys.argv[i] == '-toff':
            i += 1
            toff = float(sys.argv[i])
        elif sys.argv[i] == '-f':
            i += 1
            fs = float(sys.argv[i]) * 1e6
        elif sys.argv[i] == '-fi':
            i += 1
            fi = float(sys.argv[i]) * 1e6
        elif sys.argv[i] == '-IQ':
            IQ = 2
        elif sys.argv[i] == '-ti':
            i += 1
            tint = float(sys.argv[i])
        elif sys.argv[i] == '-ts':
            i += 1
            tspan = float(sys.argv[i])
        elif sys.argv[i] == '-yl':
            i += 1
            ylim = float(sys.argv[i])
        elif sys.argv[i] == '-log':
            i += 1
            log_file = sys.argv[i]
        elif sys.argv[i] == '-q':
            quiet = 1
        elif sys.argv[i][0] == '-':
            show_usage()
        else:
            file = sys.argv[i];
        i += 1
    
    T = sdr_code.code_cyc(sig) # code cycle
    if T <= 0.0:
        print('Invalid signal %s.' % (sig))
        exit()
    
    IQ = 1 if IQ == 1 and fi > 0.0 else 2
    if file == '':
        fp = None
    else:
        try:
            fp = open(file, 'rb')
            fp.seek(int(toff * fs * IQ))
        except:
            print('file open error: %s' % (file))
            exit()
    
    for i in range(len(prns)):
        flag = (plot and i == 0)
        ch[i] = sdr_ch.ch_new(sig, prns[i], fs, fi, add_corr=flag)
    
    if not quiet:
        print_head()
    
    if plot:
        fig, ax, p = init_plot(sig, prns[0], ch[0], env, file)
    
    if log_file != '':
        log_open(log_file)
        log_level(log_lvl)
    
    N = int(T * fs)
    buff = np.zeros(N * 2, dtype='complex64')
    ncol = 0
    tt = time.time()
    log(3, '$LOG,%.3f,%s,%d,START FILE=%s FS=%.3f FI=%.3f IQ=%d TOFF=%.3f' %
        (0.0, '', 0, file, fs * 1e-6, fi * 1e-6, IQ, toff))
    
    try:
        for i in range(1, 1000000000):
            time_rcv = toff + T * i # receiver time
            
            # read IF data
            data = read_data(fp, fs, IQ, T)
            if len(data) < N:
                break;
            buff[:N], buff[N:] = buff[N:], data
            if i == 1:
                continue
            
            # update receiver channel
            for j in range(len(prns)):
                sdr_ch.ch_update(ch[j], time_rcv, buff)
            
            if i % int(tint / T) != 0:
                continue
            
            # update receiver channel status
            if not quiet:
                ncol = update_stat(prns, ch, ncol)
            
            # update plots
            if plot:
                update_plot(fig, ax, p, ch[0], env, toff, tspan)
            
            # update log
            for j in range(len(prns)):
                if ch[j].state != 'LOCK':
                    continue
                log(3, '$STAT,%.3f,%s,%d,%d,%.1f,%.7f,%.1f,%.1f,%d,%d' %
                    (ch[j].time, ch[j].sig, ch[j].prn, ch[j].lock, ch[j].cn0,
                    ch[j].coff * 1e3, ch[j].fd, ch[j].adr, ch[j].nav.count[0],
                    ch[j].nav.count[1]))
        
    except KeyboardInterrupt:
        pass
        
    tt = time.time() - tt
    log(3, '$LOG,%.3f,%s,%d,END FILE=%s' % (tt, '', 0, file))
    if not quiet:
        print('  TIME(s) = %.3f' % (tt))
    
    if fp != None:
        fp.close()
    
    if log_file != '':
        log_close()
    
    if plot and plt.figure(window) == fig:
        plt.show()
