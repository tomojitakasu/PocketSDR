#!/usr/bin/env python3
#
#  Pocket SDR Python AP - GNSS Receiver Log Plot
#
#  Author:
#  T.TAKASU
#
#  History:
#  2022-02-11  1.0  new
#  2025-03-09  1.1  re-written for ver.0.14
#
import sys, re, time
from math import *
import numpy as np
from datetime import datetime
import matplotlib as mpl
import matplotlib.pyplot as plt
import sdr_func, sdr_rtk, sdr_code

# global settings --------------------------------------------------------------
PLOT_SIZE = [8, 6]
PLOT_RECT = [0.08, 0.04, 0.89, 0.92]
FONT_SIZE = 9
BG_COLOR = 'white'   # background color
FG_COLOR = '#555555' # foreground color
GR_COLOR = '#DDDDDD' # grid color
CLIGHT = 299792458.0 
D2R = np.pi / 180

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

# color table ------------------------------------------------------------------
cmap1 = mpl.colormaps['tab10']
cmap2 = mpl.colormaps['tab20']
COLORS = [cmap1(i) for i in range(10)] + [cmap2(i) for i in range(20)]

# C/N0 color table (compatible to RTKLIB) --------------------------------------
cols = ('#808080', '#FF0000', '#0000FF', '#FF00FF', '#FFAA00', '#008000')
CN0_COLORS = []
for cn in range(20, 60):
    d = np.clip((cn - 22.5) / 5, 0, 4.999)
    col1 = np.array(mpl.colors.to_rgba(cols[int(d)]))
    col2 = np.array(mpl.colors.to_rgba(cols[int(d)+1]))
    color = mpl.colors.to_hex((1 - d + int(d)) * col1 + (d - int(d)) * col2)
    CN0_COLORS.append(color)

# code to signal table ---------------------------------------------------------
CODES = ('1C', '1Z', '1E', '1L', '1S', '2S', '2L', '5I', '5Q', '5D', '5P', '5D',
    '5P', '6S', '6E', '1C', '2C', '4A', '4B', '6B', '3I', '3Q', '1B', '1C',
    '5I', '5Q', '7I', '7Q', '6B', '6C', '2I', '1D', '1P', '7I', '5D', '5P',
    '7D', '6I', '1D', '1P', '5A', '9A')
SIGS = ('L1CA', 'L1S', 'L1CB', 'L1CP', 'L1CD', 'L2CM', 'L2CL', 'L5I', 'L5Q',
    'L5SI', 'L5SQ', 'L5SIV', 'L5SQV', 'L6D', 'L6E', 'G1CA', 'G2CA', 'G1OCD',
    'G1OCP', 'G2OCP', 'G3OCD', 'G3OCP', 'E1B', 'E1C', 'E5AI', 'E5AQ', 'E5BI',
    'E5BQ', 'E6B', 'E6C', 'B1I', 'B1CD', 'B1CP', 'B2I', 'B2AD', 'B2AP', 'B2BI',
    'B3I', 'I1SD', 'I1SP', 'I5S', 'ISS')

# plot type to unit table ------------------------------------------------------
TYPES = ('CN0', 'COFF', 'DOP', 'ADR', 'PR', 'CP', 'PR-CP', 'AZ', 'EL', 'POS',
    'POS-E', 'POS-N', 'POS-U', 'POS-H', 'RES', 'RCLK', 'ERR_PHAS', 'ERR_CODE',
    'NFEC')
UNITS = ('dB-Hz', 'ms', 'Hz', 'cycle', 'm', 'cycle', 'm', '\xb0', '\xb0', 'm',
    'm', 'm', 'm', 'm', 'm', 'ms', 'cycle', 'us', 'bits')

# show usage -------------------------------------------------------------------
def show_usage():
    print('Usage: pocket_plot.py [-type type[,type...]] [-sat sat[,...]] [-sig sig[,...]]')
    print('    [-tspan [ts],[te]] [-tint ti] [-range rng[,...]] [-style {-|.|.-|...}]')
    print('    [-mark size] [-stats] [-legend] [-opt option] file ...')
    exit()

# string to time ---------------------------------------------------------------
def str2time(str):
    if str == '': return sdr_rtk.GTIME()
    ep = [float(s) for s in re.split('[-/:_ ]', str)]
    return sdr_rtk.epoch2time(ep) if len(ep) >= 3 else sdr_rtk.GTIME()

# time to datetime -------------------------------------------------------------
def time2dtime(time):
    ep = [int(e) for e in sdr_rtk.time2epoch(time)]
    return datetime(ep[0], ep[1], ep[2], ep[3], ep[4], ep[5])

# plot type to unit ------------------------------------------------------------
def type2unit(type):
    return UNITS[TYPES.index(type)] if type in TYPES else ''

# position to enu --------------------------------------------------------------
def pos2enu(pos):
    if len(pos) <= 0:
        return np.zeros([3, 0]), np.zeros(3)
    ecef = [sdr_rtk.pos2ecef([p[0] * D2R, p[1] * D2R, p[2]]) for p in pos]
    ecef_ref = np.mean(ecef, axis=0)
    pos_ref = sdr_rtk.ecef2pos(ecef_ref)
    enu = [sdr_rtk.ecef2enu(pos_ref, e - ecef_ref) for e in ecef]
    return np.transpose(enu), pos_ref

# code to signal ---------------------------------------------------------------
def code2sig(sat, code):
    sig = 'LGELBIL'['GREJCIS'.index(sat[0])]
    for i in range(len(CODES)):
        if CODES[i] == code and SIGS[i][0] == sig:
            return SIGS[i]
    return ''

# extend satellite list --------------------------------------------------------
def extend_sats(sats):
    sys, max_no = 'GREJCIS', (32, 27, 36, 10, 63, 14, 58)
    sats_ext = []
    for s in sats:
        if s in sys:
            i = sys.find(s)
            sats_ext.extend(['%s%02d' % (s, no + 1) for no in range(max_no[i])])
        else:
            sats_ext.append(s)
    return sats_ext

# get satellite color ----------------------------------------------------------
def sat_color(sat):
    colors = ('#006600', '#EE9900', '#CC00CC', '#0000AA', '#CC0000', '#007777',
        '#777777')
    i = 'GREJCIS'.find(sat[0])
    return colors[i] if i >= 0 else 'grey'

# get C/N0 color --------------------------------------------------------------
def cn0_color(cn0):
    if cn0 >= 60: return CN0_COLORS[-1]
    if cn0 <= 20: return CN0_COLORS[0]
    return CN0_COLORS[round(cn0) - 20]

# test time in time span -------------------------------------------------------
def test_time(time, tspan):
    if tspan[0].time and sdr_rtk.timediff(time, tspan[0]) < 0.0: return 0
    if tspan[1].time and sdr_rtk.timediff(time, tspan[1]) > 0.0: return 0
    if tspan[2] > 0 and round(time.time) % int(tspan[2]) != 0: return 0
    return 1

# get option -------------------------------------------------------------------
def get_opt(opt, opt_str):
    vals = []
    for o in opt.split():
        i = o.find(opt_str)
        if i >= 0 and len(o) > len(opt_str):
            vals = [s for s in o[i+len(opt_str):].split(',')]
    return vals    

# read log $TIME ---------------------------------------------------------------
def read_log_time(t0, line):
    # $TIME,time,year,month,day,hour,min,sec,timesys
    s = line.split(',')
    time = sdr_rtk.epoch2time([float(v) for v in s[2:8]])
    if s[8] == 'UTC':
        time = sdr_rtk.utc2gpst(time)
    return sdr_rtk.timeadd(time, -float(s[1]));

# read log $POS ---------------------------------------------------------------
def read_log_pos(t0, line, type, tspan, ts, logs, opt):
    types = ('POS', 'POS-E', 'POS-N', 'POS-U', 'POS-H', 'NSAT', 'RCLK')
    if not type in types:
        return t0
    # $POS,time,year,month,day,hour,min,sec,lat,lon,hgt,Q,ns,stdn,stde,stdu,dtr
    s = line.split(',')
    time = sdr_rtk.epoch2time([float(v) for v in s[2:8]])
    t0 = sdr_rtk.timeadd(time, -float(s[1]))
    
    if test_time(time, tspan):
        if type in ('POS', 'POS-E', 'POS-N', 'POS-U', 'POS-H'):
            logs.append([float(s[8]), float(s[9]), float(s[10])])
        elif type == 'NSAT':
            logs.append(['NSAT', float(s[12])])
        elif type == 'RCLK':
            logs.append(['RCLK', float(s[16]) * 1e3])
        ts.append(time)
    return t0

# read log $OBS ---------------------------------------------------------------
def read_log_obs(t0, line, type, sats, sigs, tspan, ts, logs, opt):
    types = ('PR', 'CP', 'PR-CP', 'LLI')
    if not type in types:
        return t0
    # $OBS,time,year,month,day,hour,min,sec,sat,code,cn0,pr,cp,dop,lli,fcn
    s = line.split(',')
    time = sdr_rtk.epoch2time([float(v) for v in s[2:8]])
    t0 = sdr_rtk.timeadd(time, -float(s[1]))
    sig = code2sig(s[8], s[9])
    
    cn0 = get_opt(opt, 'MIN_CN0=')
    if len(cn0) > 0 and float(s[10]) < float(cn0[0]):
        return t0
    
    if ('ALL' in sats or s[8] in sats) and sig in sigs and \
        test_time(time, tspan):
        id = s[8] + '-' + sig
        if type == 'PR':
            logs.append([id, float(s[11])])
        elif type == 'CP':
            logs.append([id, float(s[12])])
        elif type == 'PR-CP':
            freq = sdr_code.sig_freq(sig)
            lam = CLIGHT / sdr_func.shift_freq(sig, float(s[15]), freq)
            logs.append([id, float(s[11]) - lam * float(s[12])])
        elif type == 'LLI':
            logs.append([id, float(s[14])])
        ts.append(time)
    return t0

# read log $SAT ---------------------------------------------------------------
def read_log_sat(t0, line, type, sats, tspan, ts, logs, opt):
    types = ('AZ', 'EL', 'RES', 'SKY')
    if not type in types:
        return t0
    # $SAT,time,year,month,day,hour,min,sec,sat,pvt,obs,cn0,az,el,res
    s = line.split(',')
    time = sdr_rtk.epoch2time([float(v) for v in s[2:8]])
    t0 = sdr_rtk.timeadd(time, -float(s[1]))
    
    cn0 = get_opt(opt, 'MIN_CN0=')
    if len(cn0) > 0 and float(s[11]) < float(cn0[0]):
        return t0
    el = get_opt(opt, 'MIN_EL=')
    if len(el) > 0 and float(s[13]) < float(el[0]):
        return t0
    
    if ('ALL' in sats or s[8] in sats) and test_time(time, tspan) and \
        float(s[13]) > 0.0:
        id = s[8]
        if type == 'AZ':
            logs.append([id, float(s[12])])
        elif type == 'EL':
            logs.append([id, float(s[13])])
        elif type == 'RES':
            if not int(s[9]): return
            logs.append([id, float(s[14])])
        elif type == 'SKY':
            logs.append([id, float(s[12]), float(s[13]), float(s[11])])
        ts.append(time)
    return t0

# read log $CH -----------------------------------------------------------------
def read_log_ch(t0, line, type, sats, sigs, tspan, ts, logs, opt):
    types = ('TRK', 'LOCK', 'CN0', 'COFF', 'DOP', 'ADR', 'SSYNC', 'BSYNC',
        'FSYNC', 'ERR_PHAS', 'ERR_CODE', 'NFEC')
    if not t0 or not type in types:
        return t0
    # $CH,time,ch,rfch,sat,sig,prn,lock,cn0,coff,dop,adr,ssync,bsync,fsync,
    #   rev,srev,err_phas,err_code,tow_v,tow,week,type,nnav,nerr,nlol,nfec
    s = line.split(',')
    time = sdr_rtk.timeadd(t0, float(s[1]))
    
    rfch = get_opt(opt, 'RFCH=')
    if len(rfch) > 0 and not s[3] in rfch:
        return t0
    cn0 = get_opt(opt, 'MIN_CN0=')
    if len(cn0) > 0 and float(s[8]) < float(cn0[0]):
        return t0
    lock = get_opt(opt, 'MIN_LOCK=')
    if len(lock) > 0 and float(s[7]) < float(lock[0]):
        return t0
    
    if ('ALL' in sats or s[4] in sats) and ('ALL' in sigs or s[5] in sigs) and \
        test_time(time, tspan) and not s[4][:2] in ('R-' , 'R+'):
        id = s[4] + '-' + s[5] + '/' + s[3]
        if type == 'TRK':
            logs.append([id, float(s[8])])
        elif type == 'LOCK':
            logs.append([id, float(s[7])])
        elif type == 'CN0':
            logs.append([id, float(s[8])])
        elif type == 'COFF':
            logs.append([id, float(s[9])])
        elif type == 'DOP':
            logs.append([id, float(s[10])])
        elif type == 'ADR':
            if not int(s[13]) or not int(s[14]): return t0
            adr = float(s[11])
            if int(s[15]): adr -= 0.5
            logs.append([id, adr])
        elif type == 'SSYNC':
            logs.append([id, int(s[12]) * (-1 if int(s[16]) else 1)])
        elif type == 'BSYNC':
            logs.append([id, int(s[13])])
        elif type == 'FSYNC':
            logs.append([id, int(s[14]) * (-1 if int(s[15]) else 1)])
        elif type == 'ERR_PHAS':
            logs.append([id, float(s[17])])
        elif type == 'ERR_CODE':
            logs.append([id, float(s[18])])
        elif type == 'NFEC':
            logs.append([id, int(s[25])])
        ts.append(time)
    return t0

# read receiver log ------------------------------------------------------------
def read_log(t0, file, types, sats, sigs, tspan, ts, logs, opt):
    with open(file) as fp:
        print('reading ' + file)
        for line in fp:
            if line.startswith('$TIME'):
                t0 = read_log_time(t0, line)
            for i, type in enumerate(types):
                if line.startswith('$POS'):
                    t0 = read_log_pos(t0, line, type, tspan, ts[i], logs[i], opt)
                elif line.startswith('$OBS'):
                    t0 = read_log_obs(t0, line, type, sats, sigs, tspan, ts[i],
                        logs[i], opt)
                elif line.startswith('$SAT'):
                    t0 = read_log_sat(t0, line, type, sats, tspan, ts[i],
                        logs[i], opt)
                elif line.startswith('$CH'):
                    t0 = read_log_ch(t0, line, type, sats, sigs, tspan, ts[i],
                        logs[i], opt)
    return t0

# read receiver logs -----------------------------------------------------------
def read_logs(files, types, sats, sigs, tspan, opt):
    t0 = None
    ts, logs = [[] for t in types], [[] for t in types]
    sats = extend_sats(sats)
    for file in files:
        try:
            t0 = read_log(t0, file, types, sats, sigs, tspan, ts, logs, opt)
        
        except (FileNotFoundError, PermissionError):
            print('no file: ' + file)
    return ts, logs

# update tspan -----------------------------------------------------------------
def update_tspan(ts, tspan):
    t0, t1 = None, None
    for i in range(len(ts)):
        if len(ts[i]) > 0:
            if not t0 or sdr_rtk.timediff(ts[i][ 0], t0) < 0: t0 = ts[i][ 0]
            if not t1 or sdr_rtk.timediff(ts[i][-1], t1) > 0: t1 = ts[i][-1]
    if tspan[0].time == 0:
        tspan[0] = t0 if t0 else sdr_rtk.epoch2time([2000, 1, 1, 0])
    if tspan[1].time == 0:
        tspan[1] = t1 if t1 else sdr_rtk.epoch2time([2000, 1, 1, 1])

# remove offset ----------------------------------------------------------------
def rm_off(ys, thres):
    ys_off = []
    if len(ys) > 0:
        i = 0
        for j in np.argwhere(np.abs(np.diff(ys)) > thres):
            ys_off.extend(ys[i:j[0]+1] - np.mean(ys[i:j[0]+1]))
            i = j[0] + 1
        ys_off.extend(ys[i:] - np.mean(ys[i:]))
    return ys_off

# separate log -----------------------------------------------------------------
def sep_log(type, ts, log):
    ids, xs, ys, zs = [], [], [], []
    if len(log) > 0:
        log = list(map(list, zip(*log))) # transpose
        for id in sorted(set(log[0])):
            ids.append(id)
            idx = [i for i, x in enumerate(log[0]) if x == id]
            if type == 'SKY':
                xs.append(np.array([log[1][i] for i in idx]))
                ys.append(np.array([log[2][i] for i in idx]))
                zs.append(np.array([log[3][i] for i in idx]))
            else:
                xs.append(np.array([time2dtime(ts[i]) for i in idx]))
                ys.append(np.array([log[1][i] for i in idx]))
    return ids, xs, ys, zs

# make difference of logs ------------------------------------------------------
def diff_log(diff, ids, xs, ys):
    ids_d, xs_d, ys_d, ref = [], [], [], ''
    if len(ids) > 0 and len(diff) >= 2:
        for id in sorted(set([s.split('/')[0] for s in ids])):
            id_ch = id + '/' + diff[0]
            if not id_ch in ids: continue
            i = ids.index(id_ch)
            for ch in diff[1:]:
                id_ch = id + '/' + ch
                if not id_ch in ids: continue
                j = ids.index(id_ch)
                x, k, l = np.intersect1d(xs[j], xs[i], return_indices=True)
                ids_d.append(id + ' (RFCH ' + ch + '-' + diff[0] + ')')
                xs_d.append(x)
                ys_d.append(ys[j][k] - ys[i][l])
    return ids_d, xs_d, ys_d

# sort key ---------------------------------------------------------------------
def sort_key(key):
    order = ('G', 'R', 'E', 'J', 'C', 'I', 'S')
    return order.index(key[0]) if key[0] in order else len(order)

# sort receiver log ------------------------------------------------------------
def sort_log(ids, xs, ys, zs):
    idx_ids = list(enumerate(ids))
    idx_ids = sorted(idx_ids, key=lambda x: sort_key(x[1]))
    ids = [id for i, id in idx_ids]
    xs = [xs[i] for i, id in idx_ids]
    ys = [ys[i] for i, id in idx_ids]
    if len(zs) > 0:
        zs = [zs[i] for i, id in idx_ids]
    return ids, xs, ys, zs
    
# transform receiver log to x-y values -----------------------------------------
def trans_log(type, ts, log, opt):
    ids, xs, ys, zs, ref, diff = [], [], [], [], '', []
    
    if type in ('POS', 'POS-E', 'POS-N', 'POS-U', 'POS-H'):
        types = ('POS-E', 'POS-N', 'POS-U')
        enu, pos = pos2enu(log)
        if type == 'POS':
            for i, id in enumerate(types):
                ids.append(id)
                xs.append([time2dtime(t) for t in ts])
                ys.append(enu[i])
        elif type in types:
            ids.append(type)
            xs.append([time2dtime(t) for t in ts])
            ys.append(enu[types.index(type)])
        else:
            ids.append('POS-H')
            xs.append(enu[0])
            ys.append(enu[1])
        ref = 'REF=%.8f\xb0 %.8f\xb0 %.3fm' % (pos[0] / D2R, pos[1] / D2R, pos[2])
    else:
        ids, xs, ys, zs = sep_log(type, ts, log)
    
    diff = get_opt(opt, 'RFCH_DIFF=')
    if len(diff) >= 2 and type in ('COFF', 'DOP', 'ADR'):
        ids, xs, ys = diff_log(diff, ids, xs, ys)
        if type == 'ADR':
            for i in range(len(ys)):
                ys[i] -= np.floor(ys[i] + 0.5) # -0.5 <= y < 0.5
    
    if 'RM_OFF' in opt:
        for i in range(len(ys)):
            ys[i] = rm_off(ys[i], 10)
    
    if not type in ('POS', 'POS-E', 'POS-N', 'POS-U', 'POS-H', 'NSAT', 'RCLK'):
        ids, xs, ys, zs = sort_log(ids, xs, ys, zs)
    
    return ids, xs, ys, zs, ref

# plot line sections -----------------------------------------------------------
def plot_sec(ax, xs, ys, style, color, lw, ms):
    i = 0
    dx = np.abs([d.total_seconds() for d in np.diff(xs)])
    for j in np.argwhere(dx > 30):
        ax.plot(xs[i:j[0]+1], ys[i:j[0]+1], style, color=color, lw=lw, ms=ms)
        i = j[0] + 1
    ax.plot(xs[i:], ys[i:], style, color=color, lw=lw, ms=ms)

# plot skyplot -----------------------------------------------------------------
def plot_sky(ax):
    ax.axis('off')
    for az in range(0, 360, 30):
        x, y = sin(az * D2R), cos(az * D2R)
        ax.plot([0, x], [0, y], '-', color=GR_COLOR, lw=0.5)
        text = str(az) if az % 90 else 'NESW'[az // 90]
        ax.text(x * 1.03, y * 1.03, text, ha='center', va='center',
            rotation=-az)
    for el in range(0, 90, 15):
        x = [sin(az * D2R) * (1 - el / 90) for az in range(0, 363, 3)]
        y = [cos(az * D2R) * (1 - el / 90) for az in range(0, 363, 3)]
        color = FG_COLOR if el == 0 else GR_COLOR
        ax.plot(x, y, '-', color=color, lw=0.8 if el == 0 else 0.5)
        if el > 0:
            ax.text(0, 1 - el / 90, str(el), ha='center', va='center')

# plot log type ----------------------------------------------------------------
def plot_log_type(ax, type, ids, xs, ys, zs, color, opts):
    lw = 0.8 if opts[0] == '-' else 0.2
    if type == 'TRK':
        ax.grid(True, lw=0.5)
        for i, id in enumerate(ids):
            y = len(ids) - i
            if color == 'cn0':
                for j in range(len(xs[i])):
                    ax.plot(xs[i][j], y, '.', color=cn0_color(ys[i][j]),
                        lw=lw, ms=opts[1])
            else:
                col = sat_color(id) if color == 'sys' else color if color != '' \
                    else COLORS[i % 30]
                ax.plot(xs[i], np.full(len(xs[i]), y), opts[0], color=col, lw=lw,
                    ms=opts[1])
    elif type == 'SKY':
        for i, id in enumerate(ids):
            x = [sin(xs[i][j] * D2R) * (1 - ys[i][j] / 90) for j in range(len(xs[i]))]
            y = [cos(xs[i][j] * D2R) * (1 - ys[i][j] / 90) for j in range(len(xs[i]))]
            if color == 'cn0':
                for j in range(len(x)):
                    ax.plot(x[j], y[j], '.', color=cn0_color(zs[i][j]), ms=opts[1])
            else:
                col = sat_color(id) if color == 'sys' else color if color != '' \
                    else COLORS[i % 30]
                ax.plot(x, y, opts[0], color=col, lw=lw, ms=opts[1])
            if 'PLOT_SAT=S' in opts[4]:
                col = cn0_color(zs[i][0]) if color == 'cn0' else col
                ax.plot(x[0], y[0], 'o', mec=FG_COLOR, mfc=col, ms=22)
                ax.text(x[0], y[0], id, color=BG_COLOR, ha='center', va='center')
            elif 'PLOT_SAT=E' in opts[4]:
                col = cn0_color(zs[i][-1]) if color == 'cn0' else col
                ax.plot(x[-1], y[-1], 'o', mec=FG_COLOR, mfc=col, ms=22)
                ax.text(x[-1], y[-1], id, color=BG_COLOR, ha='center', va='center')
            elif 'PLOT_SAT=L' in opts[4]:
                xl = x[0] if y[0] < y[-1] else x[-1]
                yl = y[0] if y[0] < y[-1] else y[-1]
                ax.text(xl, yl - 0.04, id, color=FG_COLOR, ha='center', va='center')
        plot_sky(ax)
    else:
        ax.grid(True, lw=0.5)
        for i, id in enumerate(ids):
            col = sat_color(id) if color == 'sys' else color if color != '' \
                else COLORS[i % 30]
            if type in ('POS-H',):
                ax.plot(xs[i], ys[i], opts[0], color=col, lw=lw, ms=opts[1])
            else:
                plot_sec(ax, xs[i], ys[i], opts[0], col, lw, opts[1])

# set plot y-range -------------------------------------------------------------
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
    elif type in ('SSYNC', 'BSYNC', 'FSYNC'):
        ax.set_ylim([-0.2, 1.2])
    elif type == 'AZ':
        ax.set_ylim([0, 360])
    elif type == 'EL':
        ax.set_ylim([0, 90])
    elif type in ('POS', 'POS-E', 'POS-N', 'POS-U', 'POS-H', 'RES'):
        ax.set_ylim([-10, 10])
    elif type == 'NSAT':
        ax.set_ylim([0, 60])
    if type == 'POS-H':
        ax.set_xlim(ax.get_ylim())

# add statistics ---------------------------------------------------------------
def add_stats(ax, x, y, type, xs, ys):
    n, sumx, sumy, sumx_s, sumy_s = 0, 0, 0, 0, 0
    for i in range(len(xs)):
        n += len(xs[i])
        sumy += np.sum(ys[i])
        if type == 'POS-H':
            sumx += np.sum(xs[i])
    if n > 0:
        avey = sumy / n
        avex = sumx / n
        for i in range(len(xs)):
            sumy_s += np.sum((ys[i] - avey) ** 2)
            if type == 'POS-H':
                sumx_s += np.sum((xs[i] - avex) ** 2)
        stdx = np.sqrt(sumx_s / n)
        stdy = np.sqrt(sumy_s / n)
        if type == 'POS-H':
            text = 'AVE=%.3f,%.3f STD=%.3f,%.3f %s' % (avex, avey, stdx, stdy,
                type2unit(type)) 
        else:
            text = 'AVE=%.3f STD=%.3f %s' % (avey, stdy, type2unit(type)) 
        ax.text(x, y, text, ha='left', va='top', transform=ax.transAxes)

# plot logs --------------------------------------------------------------------
def plot_log(fig, rect, types, ts, logs, tspan, ranges, colors, opts):
    for i, type in enumerate(types):
        h = (rect[3] - 0.02 * (len(types) - 1)) / len(types)
        w = rect[2] - (0.015 if type == 'TRK' else 0)
        x = rect[0] + (0.015 if type == 'TRK' else 0)
        y = rect[1] + (h + 0.02) * (len(types) - 1 - i)
        ax = fig.add_axes([x, y, w, h])
        
        # transform receiver log to x, y-values
        ids, xs, ys, zs, ref = trans_log(type, ts[i], logs[i], opts[4])
        
        # plot log type
        color = colors[i] if i < len(colors) else ''
        plot_log_type(ax, type, ids, xs, ys, zs, color, opts)
        
        # set plot x-range
        if type in ('POS-H', 'SKY'):
            ax.set_aspect('equal')
        else:
            ax.set_xlim([time2dtime(t) for t in tspan[:2]])
            
        # set plot y-range
        if type == 'TRK':
            ax.set_ylim([0, len(ids) + 1])
            xl = ax.get_xlim()
            for j, id in enumerate(ids):
                text = id.split('/')[0] + ' '
                ax.text(xl[0], len(ids) - j, text, ha='right', va='center')
            ax.set_yticklabels([])
            ax.set_yticks(range(len(ids)))
        
        elif type == 'SKY':
            ax.set_xlim([-1.4, 1.4])
            ax.set_ylim([-1.1, 1.1])
        else:
            set_range(ax, type, ranges[i] if i < len(ranges) else '')
        
        # add reference and stats
        y = 1.0 - 0.025 * len(types)
        ax.text(0.985, y, ref, ha='right', va='top', transform=ax.transAxes)
        if opts[2] and not type in ('TRK', 'SKY'):
            add_stats(ax, 0.015, y, type, xs, ys)
        
        # add legend
        if opts[3] and not type in ('TRK',):
            labels = [id.split('/')[0] for id in ids]
            ax.legend(labels, loc='upper right')
        
        # add legend for C/N0 colors
        if 'cn0' in colors and i == 0:
            ax.text(0.86, 1.008, 'C/N0:', ha='center', va='bottom',
                transform=ax.transAxes)
            for j, cn in enumerate((25, 30, 35, 40, 45)):
                ax.text(0.9 + j * 0.023, 1.008, str(cn), color=cn0_color(cn),
                    ha='center', va='bottom', transform=ax.transAxes)
        
        # set time tick format
        if not type in ('POS-H',):
            dt = sdr_rtk.timediff(tspan[1], tspan[0]) 
            fmt = '%H:%M' if dt > 300 else '%H:%M:%S'
            ax.xaxis.set_major_formatter(mpl.dates.DateFormatter(fmt))
        
        # add y-label
        if not type in ('TRK', 'SKY'):
            ax.set_ylabel(type + ' (' + type2unit(type) + ')')

        # hide x-tick labels
        if i < len(types) - 1:
            ax.xaxis.set_ticklabels([])
        
# add title --------------------------------------------------------------------
def add_title(fig, rect, sats, sigs, tspan, opt):
    ax = fig.add_axes(rect)
    ax.axis('off')
    ti = 'TIME: ' + sdr_rtk.time2str(tspan[0], 0) + ' - ' + \
        sdr_rtk.time2str(tspan[1], 0)[11:] + ' GPST'
    ax.set_title(ti, fontsize=FONT_SIZE)

#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     pocket_plot.py [-type type[,type...]] [-sat sat[,...]] [-sig sig[,...]]
#         [-tspan [ts],[te]] [-tint ti] [-range rng[,...]] [-style {-|.|.-|...}]
#         [-mark size] [-stats] [-legend] [-opt option [-opt ...]] file ...
# 
#   Description
# 
#     Plot GNSS receiver log(s) written by pocket_trk or pocket_sdr.py.
#
#     Example:
#
#     pocket_plot.py -type CN0,EL -sat G01,G04,J -sig L1CA,L2CM \
#         -tspan 2025/1/1,2025/1/2 -tint 30 -range 0/60,0/100 -style . -mark 3 \
#         test_20250101*.log 
#
#   Options ([]: default)
#  
#     -type type[,type...]
#         Plot type(s) of receiver log as follows.
#           TRK   : signal tracking status
#           SKY   : satellite positions in skyplot
#           LOCK  : signal lock time
#           CN0   : signal C/N0
#           COFF  : code offset
#           DOP   : Doppler frequency
#           ADR   : accumlated delta range
#           SSYNC : secondary code sync status (0:no-sync,1:normal-sync,-1:reverse-sync)
#           BSYNC : bit sync status (0:no-sync, 1:sync)
#           FSYNC : subframe/message sync status (0:no-sync,1:normal-sync,-1:reverse-sync)
#           ERR_PHAS: phase error in PLL
#           ERR_CODE: code error in DLL
#           NFEC  : number of bit errors corrected
#           PR    : pseudorange
#           CP    : carrier-phase
#           PR-CP : psudorange - carrier-phase
#           LLI   : loss-of-lock indicator
#           AZ    : satellite azimuth angle
#           EL    : satellite elevation angle
#           RES   : residuals for position solution
#           POS   : position solution east, north, up
#           POS-E : position solution east
#           POS-N : position solution north
#           POS-U : position solution up
#           POS-H : position solution horizontal
#           NSAT  : number of used satellites for solution
#           RCLK  : receiver clock bias
#
#     -sat sat[,...]
#         GNSS satellite IDs (G01, R01, ...), satellite system IDs (G, R, ...)
#         or "ALL" to be plotted. It is required for plot type: TRK, SKY, LOCK,
#         CN0, COFF, DOP, ADR, SSYNC, BSYNC, FSYNC, ERR_PHAS, ERR_CODE, NFEC,
#         PR, CP, PR-CP, LLI, AZ, EL, RES.
# 
#     -sig sig[,...]
#         GNSS signal type IDs (L1CA, L2CM, ...) or "ALL" to be plotted. If
#         omitted, default signals are selected. [default]
# 
#     -tspan [ts],[te]
#         Plot start time ts and end time te in GPST. The format for ts or te
#         should be "y/m/d_h:m:s", where "_h:m:s" can be omitted. [auto]
#
#     -tint ti
#         Time interval ti for plot in seconds. [all]
#
#     -range rng[,...]
#         Y-axis ranges as format "ymax" for range [-ymax...ymax] or "ymin/ymax"
#         for range [ymin...ymax]. Multiple ranges correspond to multiple plot
#         types. With NULL, the range is automatically configured by data
#         values. [auto]
#
#     -color color[,color...]
#         Mark and line color(s). Multiple colors correspond to multiple plot
#         types. With NULL, the color is automatically selected. "sys" or
#         "cn0" can be allowed for system or C/N0 colors in several plot
#         types. [auto]
#
#     -style {-|.|.-|...}
#         Plot style as same as by matplotlib plot. [.-]
#
#     -mark size
#         Mark size in pixels. [2]
#
#     -stats
#         Show statistics in plots. [no]
#
#     -legend
#         Show legends in plots. [no]
#
#     -opt option [-opt ...]
#         Special options as string. Multiple options should be separated by
#         spaces. ['']
#
#         MIN_CN0=cn0   : Minimum C/N0 (dB-Hz)
#         MIN_EL=el     : Minimum elevation angle (deg)
#         MIN_LOCK=lock : Mininum lock time (s)
#         PLOT_SAT={S|E|L}: Plot satellite positions in plot type 'SKY'
#             (S: mark at start, E: mark at end, L: only label)
#         RFCH=ch[,...] : Select specified RFCH(s)
#         RFCH_DIFF=ch,ch[,...]:
#             Make difference of RFCHs referenced by first RFCH
#
#     file ...
#         GNSS receiver log file(s) written by pocket_trk or pocket_sdr.py.
#
if __name__ == '__main__':
    sats, sigs, types, ranges, colors, files = [], [], [], [], [], []
    tspan = [sdr_rtk.GTIME(), sdr_rtk.GTIME(), 0]
    opts = ['.-', 2, 0, 0, ''] # style, mark, stats, legend, opt
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-sat':
            i += 1
            sats = sys.argv[i].split(',')
        elif sys.argv[i] == '-sig':
            i += 1
            sigs = sys.argv[i].split(',')
        elif sys.argv[i] == '-type':
            i += 1
            types = sys.argv[i].split(',')
        elif sys.argv[i] == '-tspan':
            i += 1
            ts = sys.argv[i].split(',')
            if len(ts) >= 1: tspan[0] = str2time(ts[0])
            if len(ts) >= 2: tspan[1] = str2time(ts[1])
        elif sys.argv[i] == '-tint':
            i += 1
            tspan[2] = float(sys.argv[i])
        elif sys.argv[i] == '-range':
            i += 1
            ranges = sys.argv[i].split(',')
        elif sys.argv[i] == '-color':
            i += 1
            colors = sys.argv[i].split(',')
        elif sys.argv[i] == '-style':
            i += 1
            opts[0] = sys.argv[i]
        elif sys.argv[i] == '-mark':
            i += 1
            opts[1] = float(sys.argv[i])
        elif sys.argv[i] == '-stats':
            opts[2] = 1
        elif sys.argv[i] == '-legend':
            opts[3] = 1
        elif sys.argv[i] == '-opt':
            i += 1
            opts[4] += ' ' + sys.argv[i]
        elif sys.argv[i][0] == '-':
            show_usage()
        else:
            files.append(sys.argv[i])
        i += 1
    
    if len(files) <= 0 or len(types) <= 0:
        print('Specify input file(s) or plot type(s).')
        exit()
    
    # set default signals
    sigs_ = sigs if len(sigs) else ['L1CA', 'G1CA', 'E1B', 'B1I', 'I5S']
    
    # read receiver logs
    ts, logs = read_logs(files, types, sats, sigs_, tspan, opts[4])
    
    # update tspan
    update_tspan(ts, tspan)
    
    # generate window
    ti = 'Pocket SDR - Receiver Log'
    ti += ': %s' % (files[0]) + (' ...' if len(files) > 1 else '')
    fig = plt.figure(ti, figsize=PLOT_SIZE, facecolor=BG_COLOR)
    
    # plot logs
    plot_log(fig, PLOT_RECT, types, ts, logs, tspan, ranges, colors, opts)

    # add title
    add_title(fig, PLOT_RECT, sats, sigs, tspan, opts[4])
    
    plt.show()
