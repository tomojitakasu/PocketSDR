#!/usr/bin/env python3
#
#  Pocket SDR Python AP - GNSS Signal Tracking Log Plot
#
#  Author:
#  T.TAKASU
#
#  History:
#  2022-02-11  1.0  new
#
import sys, re
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import sdr_rtk

mpl.rcParams['toolbar'] = 'None';
mpl.rcParams['font.size'] = 9

# show usage -------------------------------------------------------------------
def show_usage():
    print('Usage: pocket_plot.py [-sig sig] [-prn prn] [-type type] [-atype type]')
    print('       [-ts time] [-te time] [-nav file] [-pos lat,lon,hgt] file')
    exit()

# time string to time ----------------------------------------------------------
def str2time(str):
    return sdr_rtk.epoch2time([float(s) for s in re.split('[/:-_ ]', str)])

# satellite elevations ---------------------------------------------------------
def sat_els(ts, te, sat, pos, nav):
    rr = sdr_rtk.pos2ecef(pos)
    els = []
    span = sdr_rtk.timediff(te, ts)
    for t in np.arange(0.0, span + 30.0, 30.0):
        time = sdr_rtk.timeadd(ts, t)
        rs, dts, var, svh = sdr_rtk.satpos(time, time, sat, nav)
        r, e = sdr_rtk.geodist(rs, rr)
        az, el = sdr_rtk.satazel(pos, e)
        els.append([time.time, el * sdr_rtk.R2D])
    return np.array(els)

# read tracking log -----------------------------------------------------------
def read_log(ts, te, sig, prn, type, file):
    time = sdr_rtk.GTIME()
    log = []
    fp = open(file)
    for line in fp.readlines():
        s = line.split(',')
        if len(s) < 1:
            continue
        elif s[0] == '$TIME':
            utc = sdr_rtk.epoch2time([float(s) for s in s[1:]])
            time = sdr_rtk.utc2gpst(utc)
            continue
        
        if ((ts.time != 0 and sdr_rtk.timediff(time, ts) <  0.0) or
            (te.time != 0 and sdr_rtk.timediff(time, te) >= 0.0)):
            continue
        
        if s[0] == '$CH':
            if s[2] != sig or int(s[3]) != prn:
                continue
            elif type == 'LOCK':
                log.append([time.time, float(s[4])])
            elif type == 'C/N0':
                log.append([time.time, float(s[5])])
            elif type == 'COFF':
                log.append([time.time, float(s[6])])
            elif type == 'DOP':
                log.append([time.time, float(s[7])])
            elif type == 'ADR':
                log.append([time.time, float(s[8])])
        elif s[0] == '$L6FRM' and type == 'L6FRM':
            if s[2] == sig and int(s[3]) == prn:
                log.append([time.time, 1])
    fp.close()
    return np.array(log)

# plot log --------------------------------------------------------------------
def plot_log(fig, rect, type, log, els, msg):
    color = ('darkblue', 'dimgray', 'blue')
    ax = fig.add_axes(rect)
    t0 = np.floor(log.T[0][0] / 86400) * 86400
    time = (log.T[0] - t0) / 3600
    ax.plot(time, log.T[1], '.', color=color[0], ms=0.1)
    ax.grid(True, lw=0.4)
    ax.set_xticks(np.arange(0, 24 * 7, 2))
    ax.set_xlim(time[0], time[-1])
    ax.set_xlabel('Hour (GPST)')
    if type == 'LOCK':
        ax.set_ylabel('LOCK TIME (s)')
    elif type == 'C/N0':
        ax.set_ylabel('C/N0 (dB-Hz)')
        ax.set_ylim(20.0, 65.0)
    elif type == 'COFF':
        ax.set_ylabel('Code Offset (ms)')
        ax.set_ylim(0.0, 10.0)
    elif type == 'DOP':
        ax.set_ylabel('Doppler Frequency (Hz)')
        ax.set_ylim(-3000.0, 3000.0)
    elif type == 'ADR':
        ax.set_ylabel('Accumlated Delta Range (cyc)')
    
    if len(els) > 0:
        ax3 = ax.twinx()
        time = (els.T[0] - t0) / 3600
        ax3.plot(time, els.T[1], '.', color=color[1], ms=0.1)
        ax3.set_ylim(0, 90.0)
        ax3.set_ylabel('Elevation Angle (deg)', color=color[1])
        plt.setp(ax3.get_yticklabels(), color=color[1])

    if len(msg) > 0:
        ax2 = ax.twinx()
        ax2.axis('off')
        time = (msg.T[0] - t0) / 3600
        ax2.plot(time, msg.T[1], 'o', color=color[2], ms=2)
        ax2.set_ylim(0, 20)
        n = len(time)
        N = (time[-1] - time[0]) * 3600 + 1
        rate = n * 100.0 / N
        ax2.text(0.97, 0.96, '# L6FRM = %d / %d (%.1f %%)' % (n, N, rate),
            ha='right', va='top', c=color[2], transform=ax2.transAxes)
    
#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     pocket_plot.py [-sig sig] [-prn prn] [-type type] [-atype type]
#         [-ts time] [-te time] [-nav file] [-pos lat,lon,hgt] file
# 
#   Description
# 
#     Plot GNSS signal tracking log written by pocket_trk.py.
# 
#   Options ([]: default)
#  
#     -sig sig
#         GNSS signal type ID (L1CA, L2CM, ...). [L6D]
# 
#     -prn prn
#         PRN numbers of the GNSS signal. [194]
# 
#     -type type
#         Log type to be plotted as follows. [C/N0]
#
#           LOCK : signal lock time
#           C/N0 : signal C/N0
#           COFF : code offset
#           DOP  : Doppler frequency
#           ADR  : accumlated delta range
#
#     -atype type
#         Additional log type to be plotted as follows. []
#
#           L6FRM : Valid L6 Frame decoded.
# 
#     -ts time
#         Start time in GPST as format as YYYYMMDDHHmmss. [all]
#
#     -te time
#         End time in GPST as format as YYYYMMDDHHmmss. [all]
#
#     -nav file
#         RINEX NAV file path to plot satellite elevation angle. []
#
#     -pos lat,lon,hgt
#         Receiver latitude (deg), longitude (deg) and height (m)
#
#     file
#         GNSS signal tracking log written by pocket_trk.py.
#
if __name__ == '__main__':
    window = 'Pocket SDR - GNSS SIGNAL TRACKING LOG'
    ts = sdr_rtk.GTIME()
    te = sdr_rtk.GTIME()
    sig, prn, type, atype = 'L6D', 194, 'C/N0', ''
    pos = [35.6 * sdr_rtk.D2R, 139.6 * sdr_rtk.D2R, 0.0]
    size = (9, 6)
    rect0 = [0.08, 0.090, 0.84, 0.85]
    rect1 = [0.08, 0.089, 0.84, 0.85]
    file, nfile = '', ''
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-sig':
            i += 1
            sig = sys.argv[i]
        elif sys.argv[i] == '-prn':
            i += 1
            prn = int(sys.argv[i])
        elif sys.argv[i] == '-type':
            i += 1
            type = sys.argv[i]
        elif sys.argv[i] == '-atype':
            i += 1
            atype = sys.argv[i]
        elif sys.argv[i] == '-ts':
            i += 1
            ts = str2time(sys.argv[i])
        elif sys.argv[i] == '-te':
            i += 1
            te = str2time(sys.argv[i])
        elif sys.argv[i] == '-nav':
            i += 1
            nfile = sys.argv[i]
        elif sys.argv[i] == '-pos':
            i += 1
            pos = [float(s) for s in sys.argv[i].split(',')]
            pos[0] *= sdr_rtk.D2R
            pos[1] *= sdr_rtk.D2R
        elif sys.argv[i][0] == '-':
            show_usage()
        else:
            file = sys.argv[i]
        i += 1
    
    if file == '':
        print('Specify input file.')
        exit()
    
    if nfile:
        sat = sdr_rtk.satno(sdr_rtk.SYS_QZS, prn)
        obs, nav = sdr_rtk.readrnx(nfile)
        els = sat_els(ts, te, sat, pos, nav)
        sdr_rtk.navfree(nav)
    else:
        els = []
    
    log = read_log(ts, te, sig, prn, type, file)
    msg = read_log(ts, te, sig, prn, atype, file)
    
    fig = plt.figure(window, figsize=size)
    ax0 = fig.add_axes(rect0)
    ax0.axis('off')
    ax0.set_title('SIG = %s, PRN = %d, TYPE = %s, FILE = %s' %
        (sig, prn, type, file), fontsize=10)
    
    plot_log(fig, rect1, type, log, els, msg)
    plt.show()
