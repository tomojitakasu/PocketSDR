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
BG_COLOR = 'white'     # background color
FG_COLOR = '#555555'   # foreground color
GR_COLOR = '#DDDDDD'   # grid color
P1_COLOR = '#003020'   # plot color 1
P2_COLOR = '#BBBBBB'   # plot color 2
mpl.rcParams['toolbar'] = 'None';
mpl.rcParams['font.size'] = 9
mpl.rcParams['axes.edgecolor'] = FG_COLOR
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
    return sdr_rtk.epoch2time([float(s) for s in re.split('[/:\-_ ]', str)])

# time to datetime -------------------------------------------------------------
def time2dtime(t0, x):
    ep = [int(e) for e in sdr_rtk.time2epoch(sdr_rtk.timeadd(t0, x))]
    return datetime(ep[0], ep[1], ep[2], ep[3], ep[4], ep[5])

# plot type to unit ------------------------------------------------------------
def type2unit(type):
    return UNITS[TYPES.index(type)] if type in TYPES else ''

# lat/lon/hgt to enu -----------------------------------------------------------
def llh2enu(llh):
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

# plot breaked lines -----------------------------------------------------------
def plot_blines(ax, x, y, color, thres):
    i = 0
    for j in np.argwhere(np.abs(np.diff(x)) > thres):
        ax.plot(x[i:j[0]+1], y[i:j[0]+1], '-', lw=0.5, color=color)
    ax.plot(x[i:], y[i:], '-', lw=0.5, color=color)

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
    if sat != s[4] or sig != s[5]: return
    if not test_tspan(t, tspan): return
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
    if sat != s[2]: return
    t = sdr_rtk.timeadd(t0, float(s[1]))
    if not test_tspan(t, tspan): return
    if type == 'AZ':
        logs.append(float(s[6]))
    elif type == 'EL':
        logs.append(float(s[7]))
    elif type == 'RES':
        if not int(s[3]): return
        logs.append(float(s[8]))
    ts.append(sdr_rtk.timediff(t, t0))

# read receiver log files -----------------------------------------------------
def read_log(tspan, sat, sig, types, files):
    t0 = None
    ts, logs = [[] for _ in types], [[] for _ in types]
    for file in files:
        try:
            fp = open(file)
            print('reading ' + file)
        except:
            continue
        t = None
        for line in fp.readlines():
            if line.startswith('$TIME'):
                t, t0 = read_log_time(line, t0)
            if not t or not t0: continue
            for i, type in enumerate(types):
                if line.startswith('$CH'):
                    read_log_ch(line, t0, tspan, t, sat, sig, type, ts[i], logs[i])
                elif line.startswith('$OBS'):
                    read_log_obs(line, t0, tspan, sat, sig, type, ts[i], logs[i])
                elif line.startswith('$POS'):
                    read_log_pos(line, t0, tspan, type, ts[i], logs[i])
                elif line.startswith('$SAT'):
                    read_log_sat(line, t0, tspan, t, sat, type, ts[i], logs[i])
        fp.close()
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
    tl = [time2dtime(t0, x) for x in xl]
    if tspan[0].time: tl[0] = time2dtime(tspan[0], 0.0)
    if tspan[1].time: tl[1] = time2dtime(tspan[1], 0.0)
    return tl

# set plot range ---------------------------------------------------------------
def set_range(ax, range):
    s = range.split('/')
    if len(s) >= 2:
        ax.set_ylim([float(s[0]), float(s[1])])
    elif len(s) >= 1 and s[0] != '':
        ax.set_ylim([-float(s[0]), float(s[0])])

# plot sections as lines -------------------------------------------------------
def plot_sec(ax, xs, ts, ys, color, thres):
    i = 0
    for j in np.argwhere(np.abs(np.diff(xs)) > thres):
        ax.plot(ts[i:j[0]+1], ys[i:j[0]+1], '-', lw=0.5, color=color)
        i = j[0] + 1
    ax.plot(ts[i:], ys[i:], '-', lw=0.5, color=color)

# add statistics ---------------------------------------------------------------
def add_stats(ax, x, y, type, ys):
    if len(ys) >= 2:
        text = 'AVE=%.3f%s STD=%.3f%s' % (np.mean(ys), type2unit(type),
            np.std(ys), type2unit(type)) 
        ax.text(x, y, text, ha='right', va='top', transform=ax.transAxes)

# plot log ---------------------------------------------------------------------
def plot_log(fig, rect, opts, tspan, t0, types, xs, ys, refs, range, color):
    n = len(types)
    box = [rect[0], 0, rect[2], (rect[3] - 0.02 * (n - 1)) / n]
    
    # set time span
    tl = set_tspan(tspan, t0, types, xs)
    
    for i, type in enumerate(types):
        box[1] = rect[1] + (box[3] + 0.02) * (n - 1 - i)
        ax = fig.add_axes(box)
        ax.grid(True, lw=0.5)
        set_range(ax, range[i] if i < len(range) else '')
        mc = color[i] if i < len(color) else P1_COLOR
        lc = P2_COLOR if opts[0] & 2 else mc
        if type == 'POS-EN':
            if opts[0] & 1: ax.plot(xs[i], ys[i], '-', color=lc, lw=0.5)
            if opts[0] & 2: ax.plot(xs[i], ys[i], '.', color=mc, ms=opts[1])
            ax.set_aspect('equal', adjustable='datalim')
        else:
            ts = [time2dtime(t0, t) for t in xs[i]]
            if opts[0] & 1: plot_sec(ax, xs[i], ts, ys[i], lc, 10.0)
            if opts[0] & 2: ax.plot(ts, ys[i], '.', color=mc, ms=opts[1])
            ax.set_xlim(tl)
            ax.xaxis.set_major_formatter(mpl.dates.DateFormatter('%H:%M'))
        x, y = 0.015, 1.0 - 0.025 * len(types)
        ax.text(x, y, refs[i], ha='left', va='top', transform=ax.transAxes)
        if opts[2]:
            add_stats(ax, 1.0 - x, y, type, ys[i])
        if i < len(types) - 1:
            ax.xaxis.set_ticklabels([])
        ax.set_ylabel(type + ' (' + type2unit(type) + ')')
    
#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     pocket_plot.py [-sat sat] [-sig sig] [-type type[,type...]]
#         [-tspan [ts],[te]] [-range rng[,rng]] [-style {1|2|3}] [-mark size]
#         [-stats] file ...
# 
#   Description
# 
#     Plot GNSS receiver log written by pocket_trk or pocket_sdr.py.
# 
#   Options ([]: default)
#  
#     -sat sat
#         GNSS Satellite ID (G01, R01, ...) to be plotted.
# 
#     -sig sig
#         GNSS signal type ID (L1CA, L2CM, ...) to be plotted.
# 
#     -type type[,type...]
#         Log type(s) to be plotted as follows.
#           LOCK  : signal lock time
#           CN0   : signal C/N0
#           COFF  : code offset
#           DOP   : Doppler frequency
#           ADR   : accumlated delta range
#           PR    : pseudorange
#           CP    : carrier-phase
#           PR-CP : psudorange - carrier-phase
#           LLI   : loss-of-lock indicator
#           POS   : positioning solution
#           POSH  : positioning solution horizontal plot
#           NSAT  : number of used satellites for solution
#
#     -tspan [ts],[te]
#         Set start and end time as format "y/m/d_h:m:s" (GPST). [all]
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
#         Set mark color(s).
#
#     -mark size
#         Set mark size in pixels.
#
#     -stats
#         Show statistics in plots.
#
#     file ...
#         GNSS receiver log written by pocket_trk or pocket_sdr.py.
#
if __name__ == '__main__':
    ti = 'Pocket SDR - GNSS RECEIVER LOG'
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
            if ts[0] != '': tspan[0] = str2time(ts[0])
            if ts[1] != '': tspan[1] = str2time(ts[1])
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
    ti += ': %s' % (files[0]) + (' ...' if len(files) > 1 else '')
    fig = plt.figure(ti, figsize=size)
    
    # set title
    ax0 = fig.add_axes(rect)
    ax0.axis('off')
    ti = 'RECEIVER LOG'
    ti += ' SAT=%s' % (sat) if sat != '' else ''
    ti += ' SIG=%s' % (sig) if sig != '' else ''
    ax0.set_title(ti, fontsize=9, fontweight='bold')
    
    # plot log
    plot_log(fig, rect, opts, tspan, t0, types, xs, ys, refs, range, color)
    
    plt.show()
