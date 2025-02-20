#!/usr/bin/env python3
#
#  Pocket SDR Python AP - GNSS Receiver Log Plot
#
#  Author:
#  T.TAKASU
#
#  History:
#  2022-02-11  1.0  new
#  2025-02-20  1.1  re-written for ver.0.14
#
import sys, re, time
import numpy as np
from datetime import datetime
import matplotlib as mpl
import matplotlib.pyplot as plt
import sdr_rtk, sdr_code

# global settings --------------------------------------------------------------
CLIGHT = 299792458.0 
D2R = np.pi / 180
BG_COLOR = 'white'   # background color
#FG_COLOR = '#555555' # foreground color
FG_COLOR = 'g' # foreground color
GR_COLOR = '#DDDDDD' # grid color
P1_COLOR = '#0000CC' # plot color 1
P2_COLOR = '#BBBBBB' # plot color 2
FONT_SIZE = 9
mpl.rcParams['toolbar'] = 'None';
mpl.rcParams['font.size'] = FONT_SIZE
mpl.rcParams['axes.edgecolor'] = FG_COLOR
mpl.rcParams['axes.facecolor'] = BG_COLOR
mpl.rcParams['axes.labelcolor'] = FG_COLOR
mpl.rcParams['grid.color'] = GR_COLOR
mpl.rcParams['xtick.color'] = FG_COLOR
mpl.rcParams['ytick.color'] = FG_COLOR
mpl.rcParams['text.color'] = FG_COLOR
mpl.rcParams['xtick.direction'] = 'in'
mpl.rcParams['ytick.direction'] = 'in'

# conversion tables ------------------------------------------------------------
SIGS = ('L1CA', 'L1S', 'L1CB', 'L1CP', 'L1CD', 'L2CM', 'L2CL', 'L5I', 'L5Q',
    'L5SI', 'L5SQ', 'L5SIV', 'L5SQV', 'L6D', 'L6E', 'G1CA', 'G2CA', 'G1OCD',
    'G1OCP', 'G2OCP', 'G3OCD', 'G3OCP', 'E1B', 'E1C', 'E5AI', 'E5AQ', 'E5BI',
    'E5BQ', 'E6B', 'E6C', 'B1I', 'B1CD', 'B1CP', 'B2I', 'B2AD', 'B2AP', 'B2BI',
    'B3I', 'I1SD', 'I1SP', 'I5S', 'ISS')
CODES = ('1C', '1Z', '1E', '1L', '1S', '2S', '2L', '5I', '5Q', '5D', '5P', '5D',
    '5P', '6S', '6E', '1C', '2C', '4A', '4B', '6B', '3I', '3Q', '1B', '1C',
    '5I', '5Q', '7I', '7Q', '6B', '6C', '2I', '1D', '1P', '7I', '5D', '5P',
    '7D', '6I', '1D', '1P', '5A', '9A')
TYPES = ('CN0', 'COFF', 'DOP', 'ADR', 'PR', 'CP', 'PR-CP', 'AZ', 'EL', 'POS-E',
    'POS-N', 'POS-U', 'POS-EN', 'RES')
UNITS = ('dB-Hz', 'ms', 'Hz', 'cycle', 'm', 'cycle', 'm', '\xb0', '\xb0', 'm',
    'm', 'm', 'm', 'm')

# show usage -------------------------------------------------------------------
def show_usage():
    print('Usage: pocket_plot.py [-sat sat] [-sig sig] [-type type[,type...]]')
    print('    [-tspan [ts],[te]] [-range rng[,rng]] [-style {1|2|3}] [-mark size]')
    print('    [-stats] file ...')
    exit()

# signal to signal code --------------------------------------------------------
def sig2code(sig):
    return CODES[SIGS.index(sig)] if sig in SIGS else ''

# string to time ---------------------------------------------------------------
def str2time(str):
    ep = [float(s) for s in re.split('[-/:_ ]', str)]
    return sdr_rtk.epoch2time(ep) if len(ep) >= 3 else sdr_rtk.GTIME()

# time to datetime -------------------------------------------------------------
def time2dtime(t0, x=0.0):
    ep = [int(e) for e in sdr_rtk.time2epoch(sdr_rtk.timeadd(t0, x))]
    return datetime(ep[0], ep[1], ep[2], ep[3], ep[4], ep[5])

# plot type to unit ------------------------------------------------------------
def type2unit(type):
    return UNITS[TYPES.index(type)] if type in TYPES else ''

# lat/lon/hgt to enu -----------------------------------------------------------
def llh2enu(llh):
    if len(llh) <= 0: return np.zeros([3, 0]), np.zeros(3)
    xyz = [sdr_rtk.pos2ecef([p[0] * D2R, p[1] * D2R, p[2]]) for p in llh]
    ref = np.mean(xyz, axis=0)
    pos = sdr_rtk.ecef2pos(ref)
    return np.transpose([sdr_rtk.ecef2enu(pos, r - ref) for r in xyz]), pos

# test time span ---------------------------------------------------------------
def test_tspan(t, tspan):
    if tspan[0].time and sdr_rtk.timediff(t, tspan[0]) < 0.0: return 0
    if tspan[1].time and sdr_rtk.timediff(t, tspan[1]) > 0.0: return 0
    return 1

# remove offset ----------------------------------------------------------------
def rm_off(ys, thres):
    # detect large gap and separate sections
    ys_off = []
    i = 0
    for j in np.argwhere(np.abs(np.diff(ys)) > thres):
        ys_off.extend(ys[i:j[0]+1] - np.mean(ys[i:j[0]+1]))
        i = j[0] + 1
    ys_off.extend(ys[i:] - np.mean(ys[i:]))
    return ys_off

# read log $TIME ---------------------------------------------------------------
def read_log_time(line, t0):
    # $TIME,time,year,month,day,hour,min,sec,timesys
    s = line.split(',')
    t = sdr_rtk.epoch2time([float(v) for v in s[2:8]])
    if s[8] == 'UTC':
        t = sdr_rtk.utc2gpst(t)
    return t, t if not t0 else t0

# read log $CH -----------------------------------------------------------------
def read_log_ch(line, t0, tspan, t, sat, sig, type, ts, logs):
    # $CH,time,ch,rfch,sat,sig,prn,lock,cn0,coff,dop,adr,ssync,bsync,
    # fsync,rev,week,tow,towv,nnav,nerr,nlol,nfec
    if not t or not type in ('LOCK', 'CN0', 'COFF', 'DOP', 'ADR'): return
    s = line.split(',')
    if sat != s[4] or sig != s[5] or not test_tspan(t, tspan): return
    if type == 'LOCK':
        logs.append(float(s[7]))
    elif type == 'CN0':
        logs.append(float(s[8]))
    elif type == 'COFF':
        logs.append(float(s[9]))
    elif type == 'DOP':
        logs.append(float(s[10]))
    elif type == 'ADR':
        logs.append(float(s[11]))
    ts.append(sdr_rtk.timediff(t, t0))

# read log $OBS ---------------------------------------------------------------
def read_log_obs(line, t0, tspan, sat, sig, type, ts, logs):
    # $OBS,time,year,month,day,hour,min,sec,sat,code,cn0,pr,cp,dop,lli
    if not type in ('PR', 'CP', 'PR-CP', 'LLI'): return
    s = line.split(',')
    if sat != s[8] or sig2code(sig) != s[9]: return
    t = sdr_rtk.epoch2time([float(v) for v in s[2:8]])
    if not test_tspan(t, tspan): return
    if type == 'PR':
        logs.append(float(s[11]))
    elif type == 'CP':
        logs.append(float(s[12]))
    elif type == 'PR-CP':
        lam = CLIGHT / sdr_code.sig_freq(sig)
        logs.append(float(s[11]) - lam * float(s[12]))
    elif type == 'LLI':
        logs.append(float(s[14]))
    ts.append(sdr_rtk.timediff(t, t0))

# read log $POS ---------------------------------------------------------------
def read_log_pos(line, t0, tspan, type, ts, logs):
    # $POS,time,year,month,day,hour,min,sec,lat,lon,hgt,Q,ns,stdn,stde,stdu
    if not type in ('POS', 'POSH', 'NSAT'): return
    s = line.split(',')
    t = sdr_rtk.epoch2time([float(v) for v in s[2:8]])
    if not test_tspan(t, tspan): return
    if type == 'POS' or type == 'POSH':
        logs.append([float(s[8]), float(s[9]), float(s[10])])
    elif type == 'NSAT':
        logs.append(float(s[12]))
    ts.append(sdr_rtk.timediff(t, t0))

# read log $SAT ---------------------------------------------------------------
def read_log_sat(line, t0, tspan, t, sat, type, ts, logs):
    # $SAT,time,sat,pvt,obs,cn0,az,el,res
    if not t or not type in ('AZ', 'EL', 'RES'): return
    s = line.split(',')
    if sat != s[2] or not test_tspan(t, tspan): return
    if type == 'AZ':
        logs.append(float(s[6]))
    elif type == 'EL':
        logs.append(float(s[7]))
    elif type == 'RES':
        if not int(s[3]): return
        logs.append(float(s[8]))
    ts.append(sdr_rtk.timediff(t, t0))

# read receiver log -----------------------------------------------------------
def read_log(tspan, sat, sig, types, files):
    t, t0 = None, None
    ts, logs = [[] for _ in types], [[] for _ in types]
    for file in files:
        try:
            with open(file) as fp:
                print('reading ' + file)
                for line in fp:
                    if line.startswith('$TIME'):
                        t, t0 = read_log_time(line, t0)
                    if not t0: continue
                    for i, type in enumerate(types):
                        if line.startswith('$CH'):
                            read_log_ch(line, t0, tspan, t, sat, sig, type, ts[i], logs[i])
                        elif line.startswith('$OBS'):
                            read_log_obs(line, t0, tspan, sat, sig, type, ts[i], logs[i])
                        elif line.startswith('$POS'):
                            read_log_pos(line, t0, tspan, type, ts[i], logs[i])
                        elif line.startswith('$SAT'):
                            read_log_sat(line, t0, tspan, t, sat, type, ts[i], logs[i])
        except:
            print('no file ' + file)
    return t0 if t0 else sdr_rtk.epoch2time([2000, 1, 1]), ts, logs

# transform receiver log -------------------------------------------------------
def trans_log(types, ts, logs):
    types_p, xs, ys, refs = [], [], [], []
    for i, type in enumerate(types):
        if type == 'POS' or type == 'POSH':
            enu, pos = llh2enu(logs[i])
            pos[:2] /= D2R
            ref = 'REF=%.8f\xb0, %.8f\xb0, %.3fm' % (pos[0], pos[1], pos[2])
            if type == 'POS':
                types_p.extend(['POS-E', 'POS-N', 'POS-U'])
                xs.extend([ts[i], ts[i], ts[i]])
                ys.extend(enu)
                refs.extend([ref, '', ''])
            else:
                types_p.append('POS-EN')
                xs.append(enu[0])
                ys.append(enu[1])
                refs.append(ref)
        else:
            types_p.append(types[i])
            xs.append(ts[i])
            ys.append(logs[i] if type != 'PR-CP' else rm_off(logs[i], 10))
            refs.append('')
    return types_p, xs, ys, refs

# set time span ----------------------------------------------------------------
def set_tspan(tspan, t0, types, xs):
    xl = [1e6, -1e6]
    for i, type in enumerate(types):
        if type == 'POS-EN' or len(xs[i]) <= 0: continue
        if xs[i][ 0] < xl[0]: xl[0] = xs[i][ 0]
        if xs[i][-1] > xl[1]: xl[1] = xs[i][-1]
    if xl[0] > xl[1]: xl = [0, 86400]
    ts = [sdr_rtk.timeadd(t0, t) for t in xl]
    if tspan[0].time: ts[0] = tspan[0]
    if tspan[1].time: ts[1] = tspan[1]
    return ts

# set plot range ---------------------------------------------------------------
def set_range(ax, type, range):
    s = range.split('/')
    if len(s) >= 2:
        ax.set_ylim([float(s[0]), float(s[1])])
    elif len(s) >= 1 and s[0] != '':
        ax.set_ylim([-float(s[0]), float(s[0])])
    elif type == 'CN0':
        ax.set_ylim([20, 60])
    elif type == 'LLI':
        ax.set_ylim([0, 3])
    elif type == 'AZ':
        ax.set_ylim([0, 360])
    elif type == 'EL':
        ax.set_ylim([0, 90])
    elif type in ('POS-E', 'POS-N', 'POS-U', 'POS-EN', 'RES'):
        ax.set_ylim([-10, 10])
    elif type == 'NSAT':
        ax.set_ylim([0, 60])

# plot line sections -----------------------------------------------------------
def plot_sec(ax, xs, ts, ys, color, lw, thres):
    dx, dy = np.abs(np.diff(xs)), np.abs(np.diff(ys))
    i = 0
    #for j in np.argwhere(dx > thres[0] and dy > thres[1]):
    for j in np.argwhere(dx > thres[0]):
        ax.plot(ts[i:j[0]+1], ys[i:j[0]+1], '-', lw=lw, color=color)
        i = j[0] + 1
    ax.plot(ts[i:], ys[i:], '-', lw=lw, color=color)

# add statistics ---------------------------------------------------------------
def add_stats(ax, x, y, type, ys):
    if len(ys) >= 2:
        text = 'AVE=%.3f%s STD=%.3f%s' % (np.mean(ys), type2unit(type),
            np.std(ys), type2unit(type)) 
        ax.text(x, y, text, ha='right', va='top', transform=ax.transAxes)

# add title --------------------------------------------------------------------
def add_title(fig, rect, sat, sig, tspan):
    ax = fig.add_axes(rect)
    ax.axis('off')
    ti = 'RECEIVER LOG (%s-%s' % (sdr_rtk.time2str(tspan[0], 0),
        sdr_rtk.time2str(tspan[1], 0))
    ti += ', SAT=%s' % (sat) if sat != '' else ''
    ti += ', SIG=%s' % (sig) if sig != '' else ''
    ti += ')'
    ax.set_title(ti, fontsize=FONT_SIZE)
    
# plot log ---------------------------------------------------------------------
def plot_log(fig, rect, opts, tspan, t0, types, xs, ys, refs, range, color):
    n, margin = len(types), 0.02
    box = [rect[0], 0, rect[2], (rect[3] - margin * (n - 1)) / n]
    
    # set time span
    tspan = set_tspan(tspan, t0, types, xs)
    
    for i, type in enumerate(types):
        box[1] = rect[1] + (box[3] + margin) * (n - 1 - i)
        ax = fig.add_axes(box)
        ax.grid(True, lw=0.5)
        set_range(ax, type, range[i] if i < len(range) else '')
        mc = color[i] if i < len(color) else P1_COLOR
        lc = P2_COLOR if opts[0] & 2 else mc
        if type == 'POS-EN':
            if opts[0] & 1: ax.plot(xs[i], ys[i], '-', color=lc, lw=0.5)
            if opts[0] & 2: ax.plot(xs[i], ys[i], '.', color=mc, ms=opts[1])
            ax.set_aspect('equal', adjustable='datalim')
        else:
            ts = [time2dtime(t0, t) for t in xs[i]]
            if opts[0] & 1: plot_sec(ax, xs[i], ts, ys[i], lc, 0.5, [10, 0])
            if opts[0] & 2: ax.plot(ts, ys[i], '.', color=mc, ms=opts[1])
            ax.set_xlim([time2dtime(t) for t in tspan])
            ax.xaxis.set_major_formatter(mpl.dates.DateFormatter('%H:%M'))
        x, y = 0.015, 1.0 - 0.025 * len(types)
        ax.text(x, y, refs[i], ha='left', va='top', transform=ax.transAxes)
        if opts[2]:
            add_stats(ax, 1.0 - x, y, type, ys[i])
        if i < len(types) - 1:
            ax.xaxis.set_ticklabels([])
        ax.set_ylabel(type + ' (' + type2unit(type) + ')')
    return tspan

#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     pocket_plot.py [-type type[,type...]] [-sat sat] [-sig sig]
#         [-tspan [ts],[te]] [-range rng[,rng]] [-style {1|2|3}] [-mark size]
#         [-stats] file ...
# 
#   Description
# 
#     Plot GNSS receiver log written by pocket_trk or pocket_sdr.py.
#
#     Example:
#     pocket_plot.py -type CN0,EL -sat G01 -sig L1CA -tspan 2025/1/1,2025/1/2 \
#         -range 0/60 -style 2 -mark 3 test_20250101*.log 
#
#   Options ([]: default)
#  
#     -type type[,type...]
#         Plot type(s) of receiver log as follows.
#           LOCK  : signal lock time
#           CN0   : signal C/N0
#           COFF  : code offset
#           DOP   : Doppler frequency
#           ADR   : accumlated delta range
#           PR    : pseudorange
#           CP    : carrier-phase
#           PR-CP : psudorange - carrier-phase
#           LLI   : loss-of-lock indicator
#           AZ    : satellite azimuth angle
#           EL    : satellite elevation angle
#           RES   : residuals for position solution
#           POS   : position solution
#           POSH  : position solution (horizontal plot)
#           NSAT  : number of used satellites for position
#
#     -sat sat
#         GNSS Satellite ID (G01, R01, ...) to be plotted. It is for plot types:
#         LOCK, CN0, COFF, DOP, ADR, PR, CP, PR-CP, LLI, AZ, EL, RES.
# 
#     -sig sig
#         GNSS signal type ID (L1CA, L2CM, ...) to be plotted. It is for plot
#         types: LOCK, CN0, COFF, DOP, ADR, PR, CP, PR-CP, LLI
# 
#     -tspan [ts],[te]
#         Set plot start time ts and end time te (GPST). The format for ts or te
#         should be "y/m/d_h:m:s", where "_h:m:s" can be omitted. [auto]
#
#     -range rng[,rng...]
#         Set y-axis ranges as format "ymax" for range [-ymax...ymax] or
#         "ymin/ymax" for range [ymin...ymax]. Multiple ranges corresponds
#         to multiple plot types. With NULL, the range is automatically
#         set by data values. [auto]
#
#     -style {1|2|3}
#         Set plot style (1:line,2:mark,3:line+mark). [3]
#
#     -color color[,color...]
#         Set mark color(s). [blue]
#
#     -mark size
#         Set mark size in pixels. [1.5]
#
#     -stats
#         Show statistics in plots. [no]
#
#     file ...
#         GNSS receiver log file(s) written by pocket_trk or pocket_sdr.py.
#
if __name__ == '__main__':
    sat, sig, types = '', '', []
    tspan = [sdr_rtk.GTIME(), sdr_rtk.GTIME()]
    range, color = [], []
    size = [8, 6]
    rect = [0.08, 0.04, 0.89, 0.92]
    opts = [3, 1.5, 0] # style, mark size, show stats
    files = []
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-sat':
            i += 1
            sat = sys.argv[i]
        elif sys.argv[i] == '-sig':
            i += 1
            sig = sys.argv[i]
        elif sys.argv[i] == '-type':
            i += 1
            types = sys.argv[i].split(',')
        elif sys.argv[i] == '-tspan':
            i += 1
            ts = sys.argv[i].split(',')
            if len(ts) >= 1: tspan[0] = str2time(ts[0])
            if len(ts) >= 2: tspan[1] = str2time(ts[1])
        elif sys.argv[i] == '-range':
            i += 1
            range = sys.argv[i].split(',')
        elif sys.argv[i] == '-color':
            i += 1
            color = sys.argv[i].split(',')
        elif sys.argv[i] == '-style':
            i += 1
            opts[0] = int(sys.argv[i])
        elif sys.argv[i] == '-mark':
            i += 1
            opts[1] = float(sys.argv[i])
        elif sys.argv[i] == '-stats':
            opts[2] = 1
        elif sys.argv[i][0] == '-':
            show_usage()
        else:
            files.append(sys.argv[i])
        i += 1
    
    if len(files) <= 0 or len(types) <= 0:
        print('Specify input file(s) or plot type(s).')
        exit()
    
    # read receiver log files
    t0, ts, logs = read_log(tspan, sat, sig, types, files)
    
    # transform receiver log
    types, xs, ys, refs = trans_log(types, ts, logs)
    
    # generate window
    ti = 'Pocket SDR - RECEIVER LOG'
    ti += ': %s' % (files[0]) + (' ...' if len(files) > 1 else '')
    fig = plt.figure(ti, figsize=size, facecolor=BG_COLOR)
    
    # plot log
    tspan = plot_log(fig, rect, opts, tspan, t0, types, xs, ys, refs, range,
        color)
    
    # add title
    add_title(fig, rect, sat, sig, tspan)
    
    plt.show()
