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
#  2022-01-20  1.2  improve performance
#                   add option -3d
#
import sys, math, time, datetime
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from sdr_func import *
import sdr_code, sdr_ch

# constants --------------------------------------------------------------------
CYC_SRCH = 10.0      # signal search cycle (s)
MAX_BUFF = 32        # max number of IF data buffer
NCORR_PLOT = 40      # numober of additional correlators for plot
ESC_UP  = '\033[%dF' # ANSI escape cursor up
ESC_COL = '\033[34m' # ANSI escape color blue
ESC_RES = '\033[0m'  # ANSI escape reset

# global variable --------------------------------------------------------------
Xp = np.zeros(1)
Yp = np.zeros(1) * np.nan
Zp = np.zeros(1) * np.nan
Xt = np.zeros(1)
Yt = np.zeros(1) * np.nan
Zt = np.zeros(1) * np.nan

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
rect1  = [0.080, 0.568, 0.560, 0.380]
rect2  = [0.652, 0.568, 0.280, 0.380]
rect3  = [0.080, 0.198, 0.840, 0.320]
rect4  = [0.080, 0.040, 0.840, 0.110]
rect5  = [-.430, 0.000, 1.800, 1.160]

# read IF data -----------------------------------------------------------------
def read_data(fp, N, IQ, buff, ix):
    if fp == None:
        raw = np.frombuffer(sys.stdin.buffer.read(N * IQ), dtype='int8')
    else:
        raw = np.frombuffer(fp.read(N * IQ), dtype='int8')
    
    if len(raw) < N * IQ:
        return False
    elif IQ == 1: # I
        buff[ix:ix+N] = np.array(raw, dtype='complex64')
    else: # IQ (Q sign inverted in MAX2771)
        buff[ix:ix+N] = np.array(raw[0::2] - raw[1::2] * 1j, dtype='complex64')
    return True

# print receiver channel status header -----------------------------------------
def print_head():
    print('%9s %5s %3s %5s %8s %4s %-12s %10s %7s %11s %4s %4s %4s %4s %3s' %
        ('TIME(s)', 'SIG', 'PRN', 'STATE', 'LOCK(s)', 'C/N0', '(dB-Hz)',
        'COFF(ms)', 'DOP(Hz)', 'ADR(cyc)', 'SYNC', '#NAV', '#ERR', '#LOL', 'NER'))

# receiver channel sync status -------------------------------------------------
def sync_stat(ch):
    return (('S' if ch.trk.sec_sync > 0 else '-') +
        ('B' if ch.nav.ssync > 0 else '-') +
        ('F' if ch.nav.fsync > 0 else '-') +
        ('R' if ch.nav.rev else '-'))

# update receiver channel status -----------------------------------------------
def update_stat(prns, ch, ncol):
    if ncol > 0: # cursor up
        print(ESC_UP % (ncol), end='')
    ncol = 0
    for i in range(len(prns)):
        print('%s%9.2f %5s %3d %5s %8.2f %4.1f %-13s%10.7f %7.1f %11.1f %s %4d %4d %4d %3d%s' %
            (ESC_COL if ch[i].state == 'LOCK' else '',
            ch[i].time, ch[i].sig, prns[i], ch[i].state, ch[i].lock * ch[i].T,
            ch[i].cn0, cn0_bar(ch[i].cn0), ch[i].coff * 1e3, ch[i].fd, ch[i].adr,
            sync_stat(ch[i]), ch[i].nav.count[0], ch[i].nav.count[1], ch[i].lost,
            ch[i].nav.nerr, ESC_RES if ch[i].state == 'LOCK' else ''))
        ncol += 1
    return ncol

# C/N0 bar ---------------------------------------------------------------------
def cn0_bar(cn0):
    return '|' * np.min([int((cn0 - 30.0) / 1.5), 13])

# initialize plot --------------------------------------------------------------
def init_plot(sig, prn, ch, env, p3d, file):
    fig = plt.figure(window, figsize=size, facecolor=bc)
    ax0 = fig.add_axes(rect0)
    ax0.axis('off')
    ax0.set_title('SIG = %s, PRN = %3d, FILE = %s' % (sig, prn, file),
        fontsize=10)
    pos = np.array(ch.trk.pos) / ch.fs
    if p3d:
        ax1, p1 = plot_corr_3d(fig, rect5, env, pos)
        ax2 = ax3 = ax4 = p2 = p3 = p4 = None
        text = 'SQRT(I^2+Q^2)' if env else 'I * sign(IP)'
        ax0.text(0.97, 0.85, text, color=fc, ha='right', va='top')
        p1 = (ax0.text(0.03, 0.95, '', ha='left', va='top'),
              ax0.text(0.03, 0.05, '', ha='left', va='bottom'),
              ax0.text(0.97, 0.10, '', color=fc, ha='right', va='bottom'))
    else:
        Tc = ch.T / sdr_code.code_len(sig)
        ax1, p1 = plot_corr_env (fig, rect1, env, pos, pos / Tc)
        ax2, p2 = plot_corr_IQ  (fig, rect2)
        ax3, p3 = plot_corr_time(fig, rect3)
        ax4, p4 = plot_nav_data (fig, rect4)
    return fig, (ax0, ax1, ax2, ax3, ax4), ([], p1, p2, p3, p4)

# update plot -----------------------------------------------------------------
def update_plot(fig, ax, p, ch, env, p3d, toff, tspan):
    if p3d:
        update_corr_3d(ax[1], p[1], ch, env, toff, tspan)
    else:
        update_corr_env (ax[1], p[1], ch, env)
        update_corr_IQ  (ax[2], p[2], ch, tspan)
        update_corr_time(ax[3], p[3], ch, toff, tspan)
        update_nav_data (ax[4], p[4], ch)
    plt.pause(1e-3)

# plot correlation envelope ---------------------------------------------------
def plot_corr_env(fig, rect, env, pos, chip):
    ax = fig.add_axes(rect)
    p0 = ax.plot([], [], '-', color=gc, lw=0.4)
    p1 = ax.plot([], [], '.', color=fc, ms=2)
    p2 = ax.plot([], [], '.', color=fc, ms=10)
    xl = [pos[4] * 1e3, pos[-1] * 1e3]
    if env:
        yl = [0, ylim]
        text = 'SQRT(I^2+Q^2)'
    else:
        yl = [-ylim / 3.0, ylim]
        text = 'I * sign(IP)'
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.plot(0.0, 0.0, '.', color=lc, ms=6)
    ax.grid(True, lw=0.4)
    set_axcolor(ax, lc)
    ax.text(0.03, 0.95, text, ha='left', va='top', transform=ax.transAxes)
    ax.text(0.97, 0.05, '(ms)', ha='right', va='bottom', transform=ax.transAxes)
    p3 = ax.text(0.97, 0.95, '', color=fc, ha='right', va='top',
        transform=ax.transAxes)
    ax2 = ax.twiny()
    ax2.set_xticks(range(-100, 101))
    ax2.xaxis.set_ticklabels([])
    ax2.xaxis.set_tick_params(direction='in', bottom=True, top=False)
    xl = [chip[4], chip[-1]]
    ax2.set_xlim(xl)
    ax2.plot([0, 0], yl, '-', lw=0.4, color=lc)
    ax2.plot(xl, [0, 0], '-', lw=0.4, color=lc)
    ax2.plot(0, 0, '.', ms=6, color=lc)
    return ax, (p0, p1, p2, p3)

# update correlation envelope --------------------------------------------------
def update_corr_env(ax, p, ch, env):
    Tc = ch.T / sdr_code.code_len(sig)
    x0 = ch.coff * 1e3
    x = x0 + np.array(ch.trk.pos) / ch.fs * 1e3
    if env:
        y = np.abs(ch.trk.C.real)
    else:
        y = ch.trk.C.real * np.sign(ch.trk.C[0].real)
    p[0][0].set_data(x[4:], y[4:])
    p[1][0].set_data(x[4:], y[4:])
    p[2][0].set_data(x[:3], y[:3])
    p[3].set_text('E=%6.3f P=%6.3f L=%6.3f' % (y[1], y[0], y[2]))
    ax.set_xlim(x[4], x[-1])

# plot correlation 3D ---------------------------------------------------------
def plot_corr_3d(fig, rect, env, pos):
    ax = fig.add_axes(rect, projection='3d', facecolor='None')
    yl = [pos[4] * 1e3, pos[-1] * 1e3]
    zl = [ylim * 0.01, ylim]
    ax.set_ylim(yl)
    ax.set_zlim(zl)
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Code Offset (ms)')
    ax.set_zlabel('Correlation')
    ax.set_box_aspect((3, 2.5, 0.6))
    ax.xaxis.pane.set_visible(False)
    ax.yaxis.pane.set_visible(False)
    ax.zaxis.pane.set_visible(False)
    ax.grid(False)
    ax.view_init(35, -50)
    return ax, ()

# update correlation 3D --------------------------------------------------------
def update_corr_3d(ax, p, ch, env, toff, tspan):
    global Xp, Yp, Zp, Xt, Yt, Zt
    
    for line in ax.lines:
        line.remove()
    N = int(tspan / ch.T)
    time = ch.time + np.arange(-N+1, 1) * ch.T
    t0 = toff if ch.lock < N else ch.time - N * ch.T
    xl = [t0, t0 + N * ch.T]
    Tc = ch.T / sdr_code.code_len(ch.sig)
    x = np.full(len(ch.trk.pos), ch.time)
    y0 = 0.0
    #y0 = ch.coff * 1e3
    y = y0 + np.array(ch.trk.pos) / ch.fs * 1e3
    if env:
        z = np.abs(ch.trk.C)
    else:
        z = ch.trk.C.real * np.sign(ch.trk.C[0].real)
    ix = np.max(np.array(np.where(Xp <= xl[0])))
    Xp = np.hstack([Xp[ix:], x[4:], np.nan])
    Yp = np.hstack([Yp[ix:], y[4:], np.nan])
    Zp = np.hstack([Zp[ix:], z[4:], np.nan])
    ix = np.where(Xt >= xl[0])
    Xt = np.hstack([Xt[ix], ch.time])
    Yt = np.hstack([Yt[ix], y[0]])
    Zt = np.hstack([Zt[ix], z[0]])
    y1 = Yt[len(Yt)//2]
    yl = [y1 + ch.trk.pos[4] / ch.fs * 1.3e3, y1 + ch.trk.pos[-1] / ch.fs * 1.3e3]
    ax.plot(Xt, Yt, np.zeros(len(Zt)), '.', color=gc, ms=2)
    ax.plot(Xt, Yt, Zt, '.', color=fc, ms=4)
    ax.plot(Xp, Yp, Zp, '-', color=fc, lw=0.4)
    ax.plot(x[4:], y[4:], z[4:], '-', color=fc, lw=0.8)
    ax.plot(x[:3], y[:3], z[:3], '.', color=fc, ms=10)
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.set_xbound(xl)
    ax.set_ybound(yl)
    p[0].set_text('COFF=%10.7f ms DOP=%8.1f Hz ADR=%10.1f cyc C/N0=%5.1f dB-Hz' %
        (ch.coff * 1e3, ch.fd, ch.adr, ch.cn0))
    p[1].set_text('SYNC=%s #NAV=%4d #ERR=%2d #LOL=%2d NER=%2d SEQ=%6d' %
        (sync_stat(ch), ch.nav.count[0], ch.nav.count[1], ch.lost, ch.nav.nerr,
        ch.nav.seq))
    p[2].set_text('E=%6.3f P=%6.3f L=%6.3f' % (z[1], z[0], z[2]))

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
    ax.yaxis.set_ticklabels([])
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
    p[5].set_text(('COFF=%10.7f ms DOP=%8.1f Hz ADR=%10.1f cyc ' +
        'C/N0=%5.1f dB-Hz') % (ch.coff * 1e3, ch.fd, ch.adr, ch.cn0))
    p[6].set_text('SYNC=%s #NAV=%4d #ERR=%2d #LOL=%2d NER=%2d SEQ=%6d' % (
         sync_stat(ch), ch.nav.count[0], ch.nav.count[1], ch.lost, ch.nav.nerr,
         ch.nav.seq))
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

# set axes colors --------------------------------------------------------------
def set_axcolor(ax, color):
    ax.tick_params(color=color)
    plt.setp(ax.get_xticklabels(), color=color)
    plt.setp(ax.get_yticklabels(), color=color)

# show usage -------------------------------------------------------------------
def show_usage():
    print('Usage: pocket_trk.py [-sig sig] [-prn prn[,...]] [-p] [-e] [-toff toff] [-f freq]')
    print('           [-fi freq] [-IQ] [-ti tint] [-ts tspan] [-yl ylim] [-log path]')
    print('           [-out path] [-q] [file]')
    exit()

#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     pocket_trk.py [-sig sig] [-prn prn[,...]] [-p] [-e] [-toff toff] [-f freq]
#         [-fi freq] [-IQ] [-ti tint] [-ts tspan] [-yl ylim] [-log path]
#         [-out path] [-q] [file]
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
#         GNSS signal type ID (L1CA, L2CM, ...). Refer pocket_acq.py manual for
#         details. [L1CA]
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
#     -3d
#         3D Plot of correlation shapes. [no]
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
#     -log path
#         Log stream path to write signal tracking status. The log includes
#         decoded navigation data and code offset, including navigation data
#         decoded. The stream path should be one of the followings.
#         (1) local file  file path without ':'. The file path can be contain
#             time keywords (%Y, %m, %d, %h, %M) as same as RTKLIB stream.
#         (2) TCP server  :port
#         (3) TCP client  address:port
#
#     -out path
#         Output stream path to write special messages. Currently only UBX-RXM-
#         QZSSL6 message is supported as a special message,
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
    sig, prns, plot, env, p3d = 'L1CA', [1], False, False, False
    fs, fi, IQ, toff, tint, tspan = 12e6, 0.0, 1, 0.0, 0.1, 1.0
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
        elif sys.argv[i] == '-3d':
            p3d = True
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
        ncorr = NCORR_PLOT if plot and i == 0 else 0
        ch[i] = sdr_ch.ch_new(sig, prns[i], fs, fi, add_corr=ncorr)
        ch[i].state = 'SRCH'
    
    if not quiet:
        print_head()
    
    if plot:
        fig, ax, p = init_plot(sig, prns[0], ch[0], env, p3d, file)
    
    if log_file != '':
        log_open(log_file)
        log_level(log_lvl)
    
    N = int(T * fs)
    buff = np.zeros(N * (MAX_BUFF + 1), dtype='complex64')
    ncol = 0
    ix = 0
    tt = time.time()
    log(3, '$LOG,%.3f,%s,%d,START FILE=%s FS=%.3f FI=%.3f IQ=%d TOFF=%.3f' %
        (0.0, '', 0, file, fs * 1e-6, fi * 1e-6, IQ, toff))
    
    try:
        for i in range(0, 1000000000):
            time_rcv = toff + T * (i - 1) # receiver time
            
            # read IF data to buffer
            if not read_data(fp, N, IQ, buff, N * (i % MAX_BUFF)):
                break;
            
            if i == 0:
                continue
            elif i % MAX_BUFF == 0:
                buff[-N:] = buff[:N]
            
            # update receiver channel
            for j in range(len(ch)):
                sdr_ch.ch_update(ch[j], time_rcv, buff, N * ((i - 1) % MAX_BUFF))
            
            # update receiver channel state
            if i % int(CYC_SRCH / T) == 0:
                for j in range(len(ch)):
                    ix = (ix + 1) % len(ch)
                    if ch[ix].state == 'IDLE':
                        ch[ix].state = 'SRCH'
                        break
            
            if (i - 1) % int(tint / T) != 0:
                continue
            
            # update receiver channel status
            if not quiet:
                ncol = update_stat(prns, ch, ncol)
            
            # update plots
            if plot:
                ax[0].set_title('SIG = %s, PRN = %3d, FILE = %s, T = %7.2f s' %
                    (ch[0].sig, ch[0].prn, file, ch[0].time), fontsize=10)
                update_plot(fig, ax, p, ch[0], env, p3d, toff, tspan)
            
            # update log
            t = datetime.datetime.now(datetime.timezone.utc)
            log(3, '$TIME,%d,%d,%d,%d,%d,%.6f' % (t.year, t.month, t.day, t.hour,
                t.minute, t.second + t.microsecond * 1e-6))
            
            for j in range(len(prns)):
                if ch[j].state != 'LOCK':
                    continue
                log(3, '$CH,%.3f,%s,%d,%d,%.1f,%.9f,%.3f,%.3f,%d,%d' %
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
