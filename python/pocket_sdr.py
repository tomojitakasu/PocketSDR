#!/usr/bin/env python3
#
#  Pocket SDR Python AP - GNSS SDR Receiver
#
#  Author:
#  T.TAKASU
#
#  History:
#  2024-06-29  1.0  new
#
import os, platform, time, re
from math import *
from ctypes import *
import numpy as np
from numpy import ctypeslib
from tkinter import *
from tkinter import ttk
import sdr_func, sdr_code, sdr_opt
import sdr_plot as plt

# constants --------------------------------------------------------------------
AP_NAME    = 'Pocket SDR'    # AP name
VERSION    = 'ver.0.13'      # version
TITLE      = 'An Open-Source GNSS SDR\n(Software Defined Receiver)'
AP_URL     = 'https://github.com/tomojitakasu/PocketSDR'
AP_DIR     = os.path.dirname(__file__)
COPYRIGHT  = 'Copyright (c) 2021-2024, T.Takasu\nAll rights reserved.'
OPTS_FILE  = AP_DIR + '/pocket_sdr.ini' # options file
WIDTH      = 800             # root window width
HEIGHT     = 600             # root window height
TB_HEIGHT  = 25              # toolbar height
SB_HEIGHT  = 20              # status bar height
P1_COLOR   = '#003020'       # plot color 1
P2_COLOR   = '#888844'       # plot color 2
SDR_N_CORR = (4+81)          # number of correlators
SDR_N_HIST = 5000            # number of correlator history
SDR_N_PSD  = 2048            # number FFT points for PSD
MAX_RCVLOG = 2000            # max receiver logs
UD_CYCLE1  = 20              # update cycle (ms) RF channels/Correlator pages
UD_CYCLE2  = 100             # update cycle (ms) other pages
UD_CYCLE3  = 1000            # update cycle (ms) receiver stopped
SYSTEMS = ('ALL', 'GPS', 'GLONASS', 'Galileo', 'QZSS', 'BeiDou', 'NavIC', 'SBAS')

# platform depedent settings ---------------------------------------------------
env = platform.platform()
if 'Windows' in env:
    LIBSDR = AP_DIR + '/../lib/win32/libsdr.so'
    #FONT = ('Tahoma', 'Lucida Console')
    FONT = ('Tahoma', 'Consolas')
    FONT_SIZE = (9, 9)
    BG_COLOR1 = '#F8F8F8'
    BG_COLOR2 = BG_COLOR1
    ROW_HEIGHT = 15
elif 'macOS' in env:
    LIBSDR = AP_DIR + '/../lib/macos/libsdr.so'
    FONT = ('Arial Narrow', 'Monaco')
    FONT_SIZE = (12, 10)
    BG_COLOR1 = '#E5E5E5'
    BG_COLOR2 = '#ECECEC'
    ROW_HEIGHT = 14
else: # Linux or Raspberry Pi OS
    LIBSDR = AP_DIR + '/../lib/linux/libsdr.so'
    FONT = ('Noto Sans', 'Noto Sans Mono')
    FONT_SIZE = (9, 9)
    BG_COLOR1 = '#F8F8F8'
    BG_COLOR2 = BG_COLOR1
    ROW_HEIGHT = 15

# load external library
try:
    libsdr = cdll.LoadLibrary(LIBSDR)
except:
    print('libsdr load error: ' + LIBSDR)
    exit(-1)

# general object class ---------------------------------------------------------
class Obj: pass

# get font ---------------------------------------------------------------------
def get_font(add_size=0, weight='normal', mono=0):
    return (FONT[mono], FONT_SIZE[mono] + add_size, weight)

# convert string to integer or float -------------------------------------------
def to_int(str):
    try:
       return int(str)
    except:
       return -1

def to_float(str):
    try:
       return float(str)
    except:
       return 0.0

# start receiver ---------------------------------------------------------------
def rcv_open(sys_opt, inp_opt, out_opt, sig_opt):
    set_rcv_opts(sys_opt)
    if inp_opt.inp.get() == 0:
        return rcv_open_dev(sys_opt, inp_opt, out_opt, sig_opt)
    else:
        return rcv_open_file(sys_opt, inp_opt, out_opt, sig_opt)

# start receiver by device -----------------------------------------------------
def rcv_open_dev(sys_opt, inp_opt, out_opt, sig_opt):
    sigs, prns = get_sig_opt(sig_opt)
    s = inp_opt.dev.get().split(',')
    bus  = to_int(s[0]) if len(s) >= 1 else -1
    port = to_int(s[1]) if len(s) >= 2 else -1
    conf_file = inp_opt.conf_path.get() if inp_opt.conf_ena.get() else ''
    paths = [out_opt.path[i].get() if out_opt.path_ena[i].get() else ''
        for i in range(4)]
    c_sigs = (c_char_p * len(sigs))(*[s.encode() for s in sigs])
    c_prns = (c_int32 * len(sigs))(*prns)
    c_paths = (c_char_p * 4)(*[s.encode() for s in paths])
    libsdr.sdr_rcv_open_dev.argtypes = [POINTER(c_char_p), POINTER(c_int32),
        c_int32, c_int32, c_int32, c_char_p, POINTER(c_char_p)]
    libsdr.sdr_rcv_open_dev.restype = c_void_p
    return libsdr.sdr_rcv_open_dev(c_sigs, c_prns, len(sigs), bus, port,
        conf_file.encode(), c_paths)

# start receiver by file -------------------------------------------------------
def rcv_open_file(sys_opt, inp_opt, out_opt, sig_opt):
    sigs, prns = get_sig_opt(sig_opt)
    fmt = inp_opt.fmts.index(inp_opt.fmt.get())
    fs = to_float(inp_opt.fs.get()) * 1e6
    fo = [to_float(inp_opt.fo[i].get()) * 1e6 for i in range(4)]
    IQ = [1 if inp_opt.IQ[i].get() == 'I' else 2 for i in range(4)]
    toff = to_float(inp_opt.toff.get())
    path = inp_opt.str_path.get()
    paths = [out_opt.path[i].get() if out_opt.path_ena[i].get() else ''
        for i in range(4)]
    c_sigs = (c_char_p * len(sigs))(*[s.encode() for s in sigs])
    c_prns = (c_int32 * len(sigs))(*prns)
    c_fo = (c_double * 4)(*fo)
    c_IQ = (c_int32 * 4)(*IQ)
    c_paths = (c_char_p * 4)(*[s.encode() for s in paths])
    
    libsdr.sdr_func_init.argtypes = (c_char_p)
    libsdr.sdr_func_init(sys_opt.fftw_wisdom_path.get().encode())
    libsdr.sdr_rcv_open_file.argtypes = (POINTER(c_char_p), POINTER(c_int32),
        c_int32, c_int32, c_double, POINTER(c_double), POINTER(c_int32),
        c_double, c_char_p, POINTER(c_char_p))
    libsdr.sdr_rcv_open_file.restype = c_void_p
    return libsdr.sdr_rcv_open_file(c_sigs, c_prns, len(sigs), fmt, fs, c_fo,
        c_IQ, toff, path.encode(), c_paths)

# get signal options -----------------------------------------------------------
def get_sig_opt(opt):
    sigs, prns = [], []
    for i in range(len(opt.sys)):
        if not opt.sys_sel[i].get(): continue
        satno = opt.satno[i].get()
        for j in range(len(opt.sig[i])):
            if not opt.sig_sel[i][j].get(): continue
            add_sig(sigs, prns, i, satno, opt.sig[i][j])
    return sigs, prns

# add signals ------------------------------------------------------------------
def add_sig(sigs, prns, i, satno, sig):
    s = satno.split('/')
    if len(s) < 1: return
    for prn in sdr_func.parse_nums(s[0]):
        if i == 3: # QZSS
            prn = qzss_no2prn(sig, prn)
        if i == 1 and sig != 'G1CA' and sig != 'G2CA':
            continue
        sat = sdr_code.sat_id(sig, prn)
        if sat[0] != 'GREJCIS'[i]: continue
        sigs.append(sig)
        prns.append(prn)
    if len(s) < 2: return
    for prn in sdr_func.parse_nums(s[1]):
        if i != 1 or sig == 'G1CA' or sig == 'G2CA':
            continue
        sat = sdr_code.sat_id(sig, prn)
        if sat[0] != 'R': continue
        sigs.append(sig)
        prns.append(prn)

# QZSS satellite number to prn -------------------------------------------------
def qzss_no2prn(sig, no):
    sat_L1B = (4, 5, 8, 9)
    sat_L5S = (2, 4, 5, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        7, 8)
    if sig == 'L1CA' or sig == 'L1CD' or sig == 'L1CP' or sig == 'L2CM' or \
       sig == 'L5I' or sig == 'L5Q' or sig == 'L6D':
        return 192 + no
    elif sig == 'L1S':
        return 182 + no
    elif sig == 'L6E':
        return 202 + no
    elif sig == 'L1CB' and no in sat_L1B:
        return 203 + sat_L1B.index(no)
    elif sig[:3] == 'L5S' and no in sat_L5S:
        return 184 + sat_L5S.index(no)
    return 0

# stop receiver ----------------------------------------------------------------
def rcv_close(rcv):
    libsdr.sdr_rcv_close.argtypes = (c_void_p,)
    libsdr.sdr_rcv_close(rcv)

# set receiver options ---------------------------------------------------------
def set_rcv_opts(sys_opt):
    libsdr.sdr_rcv_setopt.argtypes = (c_char_p, c_double)
    libsdr.sdr_rcv_setopt('epoch'.encode()    , float(sys_opt.epoch.get()))
    libsdr.sdr_rcv_setopt('lag_epoch'.encode(), float(sys_opt.lag_epoch.get()))
    libsdr.sdr_rcv_setopt('el_mask'.encode()  , float(sys_opt.el_mask.get()))
    libsdr.sdr_rcv_setopt('sp_corr'.encode()  , float(sys_opt.sp_corr.get()))
    libsdr.sdr_rcv_setopt('t_acq'.encode()    , float(sys_opt.t_acq.get()))
    libsdr.sdr_rcv_setopt('t_dll'.encode()    , float(sys_opt.t_dll.get()))
    libsdr.sdr_rcv_setopt('b_dll'.encode()    , float(sys_opt.b_dll.get()))
    libsdr.sdr_rcv_setopt('b_pll'.encode()    , float(sys_opt.b_pll.get()))
    libsdr.sdr_rcv_setopt('b_fll_w'.encode()  , float(sys_opt.b_fll_w.get()))
    libsdr.sdr_rcv_setopt('b_fll_n'.encode()  , float(sys_opt.b_fll_n.get()))
    libsdr.sdr_rcv_setopt('max_dop'.encode()  , float(sys_opt.max_dop.get()))
    libsdr.sdr_rcv_setopt('thres_cn0_l'.encode(), float(sys_opt.thres_cn0_l.get()))
    libsdr.sdr_rcv_setopt('thres_cn0_u'.encode(), float(sys_opt.thres_cn0_u.get()))

# get receiver status ----------------------------------------------------------
def get_rcv_stat(rcv):
    libsdr.sdr_rcv_rcv_stat.argtypes = (c_void_p,)
    libsdr.sdr_rcv_rcv_stat.restype = c_char_p
    return libsdr.sdr_rcv_rcv_stat(rcv).decode()

# get receiver channel status --------------------------------------------------
def get_ch_stat(rcv, sys, all=0):
    libsdr.sdr_rcv_ch_stat.argtypes = (c_void_p, c_char_p, c_int32)
    libsdr.sdr_rcv_ch_stat.restype = c_char_p
    return libsdr.sdr_rcv_ch_stat(rcv, sys.encode(), all).decode().splitlines()

# get signal status ------------------------------------------------------------
def get_sig_stat(rcv, sys):
    stat = get_ch_stat(rcv, sys)[2:]
    sig_stat = []
    for s in stat:
        ss = s.split()
        no = 'GREJCIS'.find(ss[2][0]) * 100 + int(ss[2][1:])
        sig_stat.append([no, -float(ss[6]), ss[2], ss[3], float(ss[6])])
    sig_stat = sorted(sig_stat)
    sats = [s[2] for s in sig_stat]
    sigs = [s[3] for s in sig_stat]
    cn0  = [s[4] for s in sig_stat]
    return sats, sigs, cn0

# get satellite status ---------------------------------------------------------
def get_sat_stat(rcv, sys):
    libsdr.sdr_rcv_sat_stat.argtypes = (c_void_p, c_char_p)
    libsdr.sdr_rcv_sat_stat.restype = c_char_p
    stat = libsdr.sdr_rcv_sat_stat(rcv, sys.encode()).decode().splitlines()
    sats = [s.split()[0] for s in stat]
    az  = [float(s.split()[1]) for s in stat]
    el  = [float(s.split()[2]) for s in stat]
    valid = [int(s.split()[3]) for s in stat]
    return sats, az, el, valid

# get RF channel status -------------------------------------------------------
def get_rfch_stat(rcv, ch):
    stat = np.zeros(5, dtype='float64')
    libsdr.sdr_rcv_rfch_stat.argtypes = (c_void_p, c_int32,
        ctypeslib.ndpointer('float64'))
    if not libsdr.sdr_rcv_rfch_stat(rcv, ch, stat):
        return 0, 0, 24.0, 0.0, 0
    return int(stat[0]), int(stat[1]), stat[2] / 1e6, stat[3] / 1e6, \
        int(stat[4]) # dev, fmt, fs (MHz), fo (MHz), IQ

# get RF channel PSD -----------------------------------------------------------
def get_rfch_psd(rcv, ch, tave):
    psd = np.zeros(SDR_N_PSD, dtype='float32')
    libsdr.sdr_rcv_rfch_psd.argtypes = (c_void_p, c_int32, c_double, c_int32,
        ctypeslib.ndpointer('float32'))
    n = libsdr.sdr_rcv_rfch_psd(rcv, ch, tave, SDR_N_PSD, psd)
    return psd[:n] if n > 0 else psd[:2]

# get RF channel histgram ------------------------------------------------------
def get_rfch_hist(rcv, ch, tave):
    val = np.zeros(256, dtype='int32')
    hist1 = np.zeros(256, dtype='float64')
    hist2 = np.zeros(256, dtype='float64')
    libsdr.sdr_rcv_rfch_hist.argtypes = (c_void_p, c_int32, c_double,
        ctypeslib.ndpointer('int32'), ctypeslib.ndpointer('float64'),
        ctypeslib.ndpointer('float64'))
    n = libsdr.sdr_rcv_rfch_hist(rcv, ch, tave, val, hist1, hist2)
    if n <= 0:
        return [], [], []
    else:
        return val[:n], hist1[:n], hist2[:n]

# get RF channel LNA gain -------------------------------------------------------
def get_rfch_gain(rcv, ch):
    libsdr.sdr_rcv_get_gain.argtypes = (c_void_p, c_int32)
    return libsdr.sdr_rcv_get_gain(rcv, ch - 1)

# set RF channel LNA gain -------------------------------------------------------
def set_rfch_gain(rcv, ch, gain):
    libsdr.sdr_rcv_set_gain.argtypes = (c_void_p, c_int32, c_int32)
    return libsdr.sdr_rcv_set_gain(rcv, ch - 1, gain)

# select receiver channel -------------------------------------------------------
def set_sel_ch(rcv, ch):
    libsdr.sdr_rcv_sel_ch.argtypes = (c_void_p, c_int32)
    libsdr.sdr_rcv_sel_ch(rcv, ch)

# get correlator status ---------------------------------------------------------
def get_corr_stat(rcv, ch):
    stat = np.array([0, 24e6, 0, 0, 0, 0], dtype='float64')
    pos = np.zeros(SDR_N_CORR, dtype='int32')
    pos[4:7] = [-40, 0, 40]
    C = np.zeros(SDR_N_CORR, dtype='complex64')
    libsdr.sdr_rcv_corr_stat.argtypes = (c_void_p, c_int32,
        ctypeslib.ndpointer('float64'), ctypeslib.ndpointer('int32'),
        ctypeslib.ndpointer('complex64'))
    n = libsdr.sdr_rcv_corr_stat(rcv, ch, stat, pos, C)
    # state, fs, lock, cn0, coff, fd, pos, C
    return int(stat[0]), stat[1], stat[2], stat[3], stat[4], stat[5], \
        pos[:n] if n > 0 else pos[:7], C[:n] if n > 0 else C[:7]

# get correlator history --------------------------------------------------------
def get_corr_hist(rcv, ch, tspan):
    stat = np.array([0, 1e-3], dtype='float64')
    P = np.zeros(SDR_N_HIST, dtype='complex64')
    libsdr.sdr_rcv_corr_hist.argtypes = (c_void_p, c_int32, c_double,
        ctypeslib.ndpointer('float64'), ctypeslib.ndpointer('complex64'))
    n = libsdr.sdr_rcv_corr_hist(rcv, ch, tspan, stat, P)
    return stat[0], stat[1], P[:n] if n > 0 else P[:2] # time, T, P

# get satellite color ----------------------------------------------------------
def sat_color(sat):
    colors = ('#006600', '#EE9900', '#CC00CC', '#0000AA', '#CC0000', '#007777',
        '#777777')
    for i in range(len(colors)):
        if sat[0] == 'GREJCIS'[i]:
            return colors[i]
    return BG_COLOR1

# update receiver log ----------------------------------------------------------
def update_rcv_log():
    global rcv_log
    buff_size = 32768
    buff = create_string_buffer(buff_size)
    libsdr.sdr_get_log.argtypes = (POINTER(c_char), c_int32)
    size = libsdr.sdr_get_log(buff, buff_size)
    for log in buff.value.decode().splitlines():
        if len(log) > 0:
            rcv_log.append(log)
    if len(rcv_log) > MAX_RCVLOG * 5:
        rcv_log = rcv_log[-MAX_RCVLOG * 5:]

# generate button bar ----------------------------------------------------------
def btn_bar_new(parent, labels, callbacks):
    bar = Obj()
    bar.panel = Frame(parent)
    for label in labels:
        btn = ttk.Button(bar.panel, text=label)
        btn.bind('<ButtonRelease-1>', lambda e: on_btn_bar_push(e, bar))
        btn.pack(side=LEFT, expand=1, fill=X)
    bar.callbacks = callbacks
    bar.panel.pack(fill=X, padx=1)
    return bar

# button bar callback ----------------------------------------------------------
def on_btn_bar_push(e, bar):
    for i, btn in enumerate(bar.panel.winfo_children()):
        if btn == e.widget:
            bar.callbacks[i](bar)
            root.focus_set()
            return

# generate status bar ----------------------------------------------------------
def status_bar_new(parent):
    bar = Obj()
    panel = Frame(parent, height=SB_HEIGHT)
    panel.pack(side=BOTTOM, fill=X, pady=(0, 4))
    bar.msg1 = ttk.Label(panel, anchor=W, background='white',
        foreground='darkblue', padding=(4, 0))
    bar.msg2 = ttk.Label(panel, width=20, anchor=CENTER, background='white',
        foreground='darkblue', padding=(4, 0))
    bar.msg1.pack(side=LEFT, expand=1, fill=X, padx=2)
    bar.msg2.pack(side=RIGHT, padx=2)
    return bar

# show Help dialog -------------------------------------------------------------
def help_dlg(root):
    dlg = sdr_opt.modal_dlg_new(root, 280, 160, 'About', nocancel=1)
    sdr_opt.link_label_new(dlg.panel, text=AP_NAME + ' ' + VERSION,
        font=get_font(2, 'bold'), link=AP_URL).pack(pady=(4, 0))
    ttk.Label(dlg.panel, text=TITLE, font=get_font(1),
        justify=CENTER).pack(pady=2)
    ttk.Label(dlg.panel, text=COPYRIGHT, justify=CENTER).pack()
    root.wait_window(dlg.win)

# generate Receiver page -------------------------------------------------------
def rcv_page_new(parent):
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent, bg=BG_COLOR1)
    p.toolbar = Frame(p.panel, height=TB_HEIGHT, bg=BG_COLOR1)
    p.toolbar.pack_propagate(0)
    p.toolbar.pack(fill=X)
    p.txt1 = ttk.Label(p.toolbar)
    p.txt1.pack(side=LEFT, fill=X, padx=4)
    p.box1 = ttk.Combobox(p.toolbar, width=9, state='readonly', justify=CENTER,
        values=SYSTEMS, font=get_font())
    p.box1.set('ALL')
    p.box1.pack(side=RIGHT, padx=(4, 20))
    ttk.Label(p.toolbar, text='System').pack(side=RIGHT)
    panel1 = Frame(p.panel, bg=BG_COLOR1)
    panel1.pack(expand=1, fill=X)
    p.plt1 = plt.plot_new(panel1, 257, 257, (-1.2, 1.2), (-1.2, 1.2),
        (0, 0, 0, 0), font=get_font(-1))
    p.plt1.c.pack(side=RIGHT)
    p.stat = plt.plot_new(panel1, 543, 257, margin=(25, 20, 30, 30),
        font=get_font())
    p.stat.c.pack(side=LEFT, expand=1, fill=X)
    p.plt2 = plt.plot_new(p.panel, 800, 245, title='Signal C/N0 (dB-Hz)',
        font=get_font(), tick=2)
    p.plt2.c.pack(expand=1, fill=BOTH)
    p.panel.bind('<Expose>', lambda e: on_rcv_panel_expose(e, p))
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_sys_select(e, p))
    return p

# receiver panel expose callback -----------------------------------------------
def on_rcv_panel_expose(e, p):
    update_rcv_page(p)

# system select callback -------------------------------------------------------
def on_sys_select(e, p):
    sys = p.box1.get()
    update_sky_plot(p.plt1, sys)
    update_sig_plot(p.plt2, sys)

# update Receiver page ---------------------------------------------------------
def update_rcv_page(p):
    sys = p.box1.get()
    update_rcv_stat(p.stat)
    update_sky_plot(p.plt1, sys)
    update_sig_plot(p.plt2, sys)

# update receiver status panel -------------------------------------------------
def update_rcv_stat(p):
    labels = ('Receiver Time (s)', 'Input Source', 'IF Data Format',
        '# of RF CHs', 'LO Freqs (MHz)', '', 'Sampling Types',
        'Sampling Rate (Msps)', '# of BB CHs Locked/Total',
        'IF Data Rate (MB/s)', 'IF Data Buffer Usage (%)', 'Time (GPST)',
        'Solution Status', 'Latitude (\xb0)', 'Longitude (\xb0)',
        'Altitude (m)', 'Systems Tracked', '# of Sats Used/Tracked', 'Output',
        '# of PVT Solutions', '# of OBS/NAV Data', 'IF Data Log (MB)')
    value = get_rcv_stat(rcv_body).split(',')
    plt.plot_clear(p)
    for i in range(len(labels)):
        x, y = 0.0 if i < 11 else 0.51, 1.0 - (i % 11) / 10
        plt.plot_text(p, x, y, labels[i], anchor=W,
            font=get_font(1, 'bold'), color=P1_COLOR)
        plt.plot_text(p, x + 0.48, y, value[i], anchor=E, font=get_font(1),
            color=P1_COLOR)

# update skyplot ---------------------------------------------------------------
def update_sky_plot(p, sys):
    sats, az, el, valid = get_sat_stat(rcv_body, sys)
    plt.plot_clear(p)
    plt.plot_sky(p, color=None)
    xs, ys = plt.plot_scale(p)
    for i in range(len(sats) - 1, -1, -1):
        x = (90 - el[i]) / 90 * sin(az[i] * pi / 180)
        y = (90 - el[i]) / 90 * cos(az[i] * pi / 180)
        color1 = sat_color(sats[i]) if valid[i] and rcv_body else BG_COLOR1
        color2 = BG_COLOR1 if valid[i] and rcv_body else plt.FG_COLOR
        plt.plot_circle(p, x, y, 12 / xs, fill=color1)
        plt.plot_text(p, x, y, sats[i], color=color2, font=get_font(-1))
    plt.plot_sky(p, gcolor=None)

# update signal plot -----------------------------------------------------------
def update_sig_plot(p, sys):
    sats, sigs, cn0 = get_sig_stat(rcv_body, sys)
    sat_set = sorted(set(sats), key=sats.index)
    sig_set = sorted(set(sigs))
    plt.plot_clear(p)
    plt.plot_xlim(p, [-0.7, len(sat_set) - 0.3])
    plt.plot_ylim(p, [20, 55])
    xs, ys = plt.plot_scale(p)
    font = get_font(1)
    plt.plot_axis(p)
    if sys == 'ALL':
        for i, s in enumerate('GREJCIS'):
            x, y = p.xl[1] + (i * 9 - 70) / xs, p.yl[1] - 16 / ys
            plt.plot_text(p, x, y, s, color=sat_color(s), font=font)
    else:
        txt = 'Signals: '
        for sig in sig_set:
            txt += ' ' + sig + ','
        x, y = p.xl[1] - 12 / xs, p.yl[1] - 16 / ys
        plt.plot_text(p, x, y, txt[:-1], font=font, anchor=E)
    for x, sat in enumerate(sat_set):
        text = sat[1:] if sys == 'ALL' else sat
        plt.plot_text(p, x, p.yl[0] - 2 / ys, text, color=sat_color(sat),
            font=font, anchor=N)
        color = sat_color(sat) if rcv_body else BG_COLOR1
        idx = [i for i, s in enumerate(sats) if s == sat]
        if sys != 'ALL':
            for i in range(len(sig_set)):
                xi = x + (8 * i - (len(sig_set) - 1) * 4) / xs
                plt.plot_rect(p, xi - 3 / xs, 20, xi + 3 / xs, p.yl[0] + 0.5,
                    fill=BG_COLOR1)
        for i in idx:
            xi = x
            if sys != 'ALL':
                j = sig_set.index(sigs[i])
                xi += (8 * j - (len(sig_set) - 1) * 4) / xs
            plt.plot_rect(p, xi - 3 / xs, 20, xi + 3 / xs, cn0[i], fill=color)

# generate RF Channels page ----------------------------------------------------
def rfch_page_new(parent):
    ti = ['Power Spectral Density (dB/Hz)', 'Histgram I', 'Histgram Q']
    labels = ['Frequency (MHz)', 'Quantized Value']
    margin = (35, 25, 25, 40)
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent, bg=BG_COLOR1)
    p.toolbar = Frame(p.panel, height=TB_HEIGHT, bg=BG_COLOR1)
    p.toolbar.pack_propagate(0)
    p.toolbar.pack(fill=X)
    ttk.Label(p.toolbar, text='RF CH').pack(side=LEFT, padx=(10, 4))
    p.box1 = ttk.Combobox(p.toolbar, width=4, state='readonly', justify=CENTER,
        values=['1', '2', '3', '4'], font=get_font())
    p.box1.set('1')
    p.box1.pack(side=LEFT)
    p.box2 = ttk.Combobox(p.toolbar, width=5, state='readonly', justify=CENTER,
        values=['0.1', '0.03', '0.01', '0.003', '0.001'], font=get_font())
    p.box2.set('0.01')
    p.box2.pack(side=RIGHT, padx=(2, 10))
    ttk.Label(p.toolbar, text='Aver (s)').pack(side=RIGHT)
    p.btn1 = sdr_opt.custom_btn_new(p.toolbar, label=' > ')
    p.btn1.panel.pack(side=RIGHT, padx=(0, 8), pady=1)
    p.txt2 = ttk.Label(p.toolbar, width=3, text='0', anchor=CENTER,
        background='white', relief=FLAT)
    p.txt2.pack(side=RIGHT)
    p.btn2 = sdr_opt.custom_btn_new(p.toolbar, label=' < ')
    p.btn2.panel.pack(side=RIGHT, padx=(2, 0), pady=1)
    ttk.Label(p.toolbar, text='LNA Gain (0:Auto)').pack(side=RIGHT)
    p.txt1 = ttk.Label(p.toolbar, font=get_font(1), foreground=P1_COLOR)
    p.txt1.pack(side=LEFT, expand=1, fill=X, padx=10)
    p.plt1 = plt.plot_new(p.panel, 520, 490, (0, 1), (-80, -40), margin,
        font=get_font(), title=ti[0], xlabel=labels[0])
    p.plt1.c.pack(side=LEFT, expand=1, fill=BOTH)
    panel1 = Frame(p.panel)
    panel1.pack(side=RIGHT, expand=1, fill=BOTH)
    p.plt2 = plt.plot_new(panel1, 280, 245, (-5, 5), (0, 0.5), margin,
        font=get_font(), title=ti[1], xlabel=labels[1])
    p.plt3 = plt.plot_new(panel1, 280, 245, (-5, 5), (0, 0.5), margin,
        font=get_font(), title=ti[2], xlabel=labels[1])
    p.plt2.c.pack(expand=1, fill=BOTH)
    p.plt3.c.pack(expand=1, fill=BOTH)
    p.panel.bind('<Expose>', lambda e: on_rfch_page_expose(e, p))
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_rfch_select(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_rfch_select(e, p))
    p.btn1.btn.bind('<Button-1>', lambda e: on_updown_push(e, p))
    p.btn2.btn.bind('<Button-1>', lambda e: on_updown_push(e, p))
    return p

# RF Channels page expose callback ---------------------------------------------
def on_rfch_page_expose(e, p):
    update_rfch_page(p)

# RF Channels page select callback ---------------------------------------------
def on_rfch_select(e, p):
    ch = int(p.box1.get())
    tave = float(p.box2.get())
    update_psd_plot(p.plt1, p.txt1, ch, tave)
    update_hist_plot(p.plt2, p.plt3, ch, tave)

# updown button push callback ---------------------------------------------------
def on_updown_push(e, p):
    ch = int(p.box1.get())
    val = int(p.txt2['text'])
    if e.widget['text'] == ' < ' and val > 0:
        p.txt2['text'] = str(val - 1)
        set_rfch_gain(rcv_body, ch, val - 1)
    elif e.widget['text'] == ' > ' and val < 64:
        p.txt2['text'] = str(val + 1)
        set_rfch_gain(rcv_body, ch, val + 1)

# update RF Channels page ------------------------------------------------------
def update_rfch_page(p):
    ch = int(p.box1.get())
    tave = float(p.box2.get())
    update_psd_plot(p.plt1, p.txt1, ch, tave)
    update_hist_plot(p.plt2, p.plt3, ch, tave)
    p.txt2.configure(text='%d' % get_rfch_gain(rcv_body, ch))

# update PSD plot --------------------------------------------------------------
def update_psd_plot(p, txt, ch, tave):
    dev, fmt, fs, fo, IQ = get_rfch_stat(rcv_body, ch)
    psd = get_rfch_psd(rcv_body, ch, tave)
    f = np.linspace(fo, fo + fs * 0.5, len(psd)) if IQ == 1 else \
        np.linspace(fo - fs * 0.5, fo + fs * 0.5, len(psd))
    dev_str = ('-------', 'IF Data', 'RF Frontend')
    fmt_str = ('-------', 'INT8', 'INT8X2', 'RAW8', 'RAW16')
    IQ_str = ('---', 'I', 'IQ')
    txt.configure(text='INPUT: %s  FORMAT: %s  F_S: %.3f MHz (%s)' % (
        dev_str[dev], fmt_str[fmt], fs, IQ_str[IQ]))
    plt.plot_clear(p)
    plt.plot_xlim(p, [f[0], f[-1]])
    plt.plot_ylim(p, [-80, -45])
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, [fo, fo], p.yl, color=plt.GR_COLOR)
    plt.plot_poly(p, f, psd, color=P1_COLOR)
    plt.plot_axis(p, gcolor=None)
    xs, ys = plt.plot_scale(p)
    plt.plot_poly(p, [fo, fo], [p.yl[0], p.yl[0] + 6 / ys], color=plt.FG_COLOR)
    plt.plot_poly(p, [fo, fo], [p.yl[1], p.yl[1] - 6 / ys], color=plt.FG_COLOR)
    plot_mark(p, fo, p.yl[0] + 16 / ys, color=plt.FG_COLOR)
    plt.plot_text(p, fo + 12 / xs, p.yl[0] + 16 / ys, 'F_LO: %.3f MHz' % (fo),
        anchor=W)
    plot_sig_freq(p)

# plot signal frequency marks --------------------------------------------------
def plot_sig_freq(p):
    global sig_opt
    xs, ys = plt.plot_scale(p)
    y = p.yl[1] - 2
    for i in range(len(sig_opt.sys)):
        if not sig_opt.sys_sel[i].get(): continue
        color = color=sat_color('GREJCIS'[i])
        for j, sig in enumerate(sig_opt.sig[i]):
            if not sig_opt.sig_sel[i][j].get(): continue
            freq = sdr_code.sig_freq(sig) / 1e6
            if freq < p.xl[0] or freq > p.xl[1]: continue
            plt.plot_poly(p, [freq, freq], p.yl, color=plt.GR_COLOR)
            plot_mark(p, freq, y, color=color)
            xi = freq + (-7 / xs if i % 2 else 7 / xs)
            plt.plot_text(p, xi, y, sig, anchor=E if i % 2 else W, color=color)
            y -= 9 / ys

# plot mark --------------------------------------------------------------------
def plot_mark(p, x, y, color):
    xs, ys = plt.plot_scale(p)
    xi = [x, x - 4 / xs, x + 4 / xs, x]
    yi = [y - 3 / ys, y + 3 / ys, y + 3 / ys, y - 3 / ys]
    plt.plot_poly(p, xi, yi, color=color)

# update histgrams plot --------------------------------------------------------
def update_hist_plot(p1, p2, ch, tave):
    val, hist1, hist2 = get_rfch_hist(rcv_body, ch, tave)
    plot_hist(p1, val, hist1)
    plot_hist(p2, val, hist2)

# plot histgram ----------------------------------------------------------------
def plot_hist(p, val, hist):
    xs, ys = plt.plot_scale(p)
    plt.plot_clear(p)
    plt.plot_axis(p, fcolor=None, tcolor=None)
    for i in range(len(val)):
        plt.plot_rect(p, val[i] - 6 / xs, 0, val[i] + 6 / xs, hist[i],
            fill=P1_COLOR)
    plt.plot_axis(p, gcolor=None)

# generate BB Channels page ----------------------------------------------------
def bbch_page_new(parent):
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent, bg=BG_COLOR1)
    p.toolbar = Frame(p.panel, height=TB_HEIGHT, bg=BG_COLOR1)
    p.toolbar.pack_propagate(0)
    p.toolbar.pack(fill=X)
    ttk.Label(p.toolbar, text='CH').pack(side=LEFT, padx=(10, 4))
    p.box2 = ttk.Combobox(p.toolbar, width=9, state='readonly', justify=CENTER,
        values=('LOCK', 'ALL'), font=get_font())
    p.box2.set('LOCK')
    p.box2.pack(side=LEFT)
    ttk.Label(p.toolbar, text='System').pack(side=LEFT, padx=(8, 4))
    p.box1 = ttk.Combobox(p.toolbar, width=9, state='readonly', justify=CENTER,
        values=SYSTEMS, font=get_font())
    p.box1.set('ALL')
    p.box1.pack(side=LEFT)
    p.txt1 = ttk.Label(p.toolbar, font=get_font(1), width=12)
    p.txt2 = ttk.Label(p.toolbar, font=get_font(1), width=9, anchor=E)
    p.txt3 = ttk.Label(p.toolbar, font=get_font(1), width=13, anchor=E)
    p.txt3.pack(side=RIGHT, padx=(2, 15))
    p.txt2.pack(side=RIGHT, padx=2)
    p.txt1.pack(side=RIGHT, padx=2)
    p.tbl1 = ttk.Treeview(p.panel, show=('headings'))
    p.tbl1.pack(expand=1, fill=BOTH)
    p.scl1 = ttk.Scrollbar(p.tbl1, orient=VERTICAL, command=p.tbl1.yview)
    p.scl1.pack(side=RIGHT, fill=Y)
    p.tbl1.configure(yscrollcommand=p.scl1.set)
    p.tbl1.pack(expand=1, fill=BOTH)
    p.panel.bind('<Expose>', lambda e: on_bbch_page_expose(e, p))
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_bbch_sys_select(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_bbch_sys_select(e, p))
    p.tbl1.bind('<<TreeviewSelect>>', lambda e: on_bbch_ch_select(e, p))
    return p

# BB Channels page expose callback ---------------------------------------------
def on_bbch_page_expose(e, p):
    update_bbch_page(p)

# BB Channels page system select callback --------------------------------------
def on_bbch_sys_select(e, p):
    update_bbch_page(p)

# BB Channels page channel select callback -------------------------------------
def on_bbch_ch_select(e, p):
    ch = e.widget.focus()
    page = p.parent.winfo_children()[3]
    box = page.winfo_children()[0].winfo_children()[2]
    if ch != '': box.set(ch)

# update BB Channels page ------------------------------------------------------
def update_bbch_page(p):
    sys = p.box1.get()
    ch = p.box2.get()
    stat = get_ch_stat(rcv_body, sys, all=(ch == 'ALL'))
    w = (36, 24, 36, 52, 32, 66, 40, 70, 80, 64, 84, 44, 44, 36, 36, 36)
    a = 'ecccceeweeeceeee'
    buff, srch, lock = stat[0][72:83], stat[0][84:92], stat[0][93:]
    p.txt1.configure(text=buff)
    p.txt2.configure(text=srch)
    p.txt3.configure(text=lock)
    buff_use = int(re.split('[:%]', buff)[1])
    srch_ch = int(re.split('[:]', srch)[1])
    p.txt1.configure(foreground='green' if buff_use < 90 else 'red')
    for c in p.tbl1.get_children():
       p.tbl1.delete(c)
    cols = stat[1].split()
    p.tbl1.configure(columns=cols)
    for i in range(len(cols)):
        p.tbl1.heading(cols[i], text=cols[i])
        p.tbl1.column(cols[i], width=w[i], anchor=a[i], stretch=0)
    for s in stat[2:]:
        vals = s.split()
        tag = 'idle' if float(vals[5]) == 0.0 else ''
        tag = 'srch' if int(vals[0]) == srch_ch else tag
        p.tbl1.insert('', END, iid=vals[0], values=vals, tags=tag)
    p.tbl1.tag_configure('idle', foreground=P2_COLOR)
    p.tbl1.tag_configure('srch', foreground='blue')

# generate Correlator page -----------------------------------------------------
def corr_page_new(parent):
    ti = ('I * sign(IP)', 'IP-QP', 'Time (s) - IP/QP')
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent, bg=BG_COLOR1)
    p.toolbar = Frame(p.panel, height=TB_HEIGHT, bg=BG_COLOR1)
    p.toolbar.pack_propagate(0)
    p.toolbar.pack(fill=X)
    ttk.Label(p.toolbar, text='BB CH').pack(side=LEFT, padx=(10, 2))
    p.btn1 = sdr_opt.custom_btn_new(p.toolbar, label=' < ')
    p.btn1.panel.pack(side=LEFT, padx=2, pady=1)
    p.box1 = ttk.Combobox(p.toolbar, width=4, height=32, state='readonly',
        justify=CENTER, font=get_font())
    p.box1.set('1')
    p.box1.pack(side=LEFT)
    p.btn2 = sdr_opt.custom_btn_new(p.toolbar, label=' > ')
    p.btn2.panel.pack(side=LEFT, padx=2, pady=1)
    p.txt1 = ttk.Label(p.toolbar, font=get_font(1), foreground=P1_COLOR)
    p.txt1.pack(side=LEFT, expand=1, fill=X, padx=10)
    p.box3 = ttk.Combobox(p.toolbar, width=3, state='readonly', justify=CENTER,
        values=('0.1', '0.2', '0.3', '0.4', '0.6', '0.8', '1.0', '1.5', '2'), font=get_font())
    p.box3.set('0.4')
    p.box3.pack(side=RIGHT, padx=(1, 10))
    p.box2 = ttk.Combobox(p.toolbar, width=3, state='readonly', justify=CENTER,
        values=('0.1', '0.2', '0.5', '1', '2', '5', '10'), font=get_font())
    p.box2.set('1')
    p.box2.pack(side=RIGHT, padx=1)
    p.box4 = ttk.Combobox(p.toolbar, width=3, state='readonly', justify=CENTER,
        values=('I', 'IQ'), font=get_font())
    p.box4.set('I')
    p.box4.pack(side=RIGHT, padx=1)
    ttk.Label(p.toolbar, text='IQ/Span(s)/Range').pack(side=RIGHT, padx=2)
    p.plt3 = plt.plot_new(p.panel, 800, 245, [0, 1], [-0.6, 0.6],
        font=get_font(), title=ti[2])
    p.plt3.c.pack(side=BOTTOM, expand=1, fill=BOTH)
    panel1 = Frame(p.panel, bg=BG_COLOR1)
    panel1.pack(expand=1, fill=BOTH)
    p.plt2 = plt.plot_new(panel1, 255, 245, [-0.6, 0.6], [-0.6, 0.6],
        font=get_font(), aspect=1, title=ti[1])
    p.plt2.c.pack(side=RIGHT, expand=1, fill=BOTH)
    p.plt1 = plt.plot_new(panel1, 545, 245, [0, 1], [-0.2, 0.6],
        font=get_font(), title=ti[0])
    p.plt1.c.pack(side=LEFT, expand=1, fill=BOTH)
    p.panel.bind('<Expose>', lambda e: on_corr_page_expose(e, p))
    p.btn1.btn.bind('<Button-1>', lambda e: on_corr_ch_down(e, p))
    p.btn2.btn.bind('<Button-1>', lambda e: on_corr_ch_up(e, p))
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_corr_ch_select(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_corr_ch_select(e, p))
    p.box3.bind('<<ComboboxSelected>>', lambda e: on_corr_ch_select(e, p))
    p.box4.bind('<<ComboboxSelected>>', lambda e: on_corr_ch_select(e, p))
    return p

# Correlator page expose callback ----------------------------------------------
def on_corr_page_expose(e, p):
    update_corr_page(p)
    update_corr_ch_sel(p)

# correlator channel down callback ---------------------------------------------
def on_corr_ch_down(e, p):
    chs, ch = p.box1['values'], p.box1.get()
    i = chs.index(ch) if ch in chs else -1
    if i - 1 >= 0:
        ch = chs[i-1]
        p.box1.set(ch)
        set_sel_ch(rcv_body, int(ch))
        update_corr_page(p)

# correlator channel up callback -----------------------------------------------
def on_corr_ch_up(e, p):
    chs, ch = p.box1['values'], p.box1.get()
    i = chs.index(ch) if ch in chs else -1
    if i >= 0 and i + 1 < len(chs):
        ch = chs[i+1]
        p.box1.set(ch)
        set_sel_ch(rcv_body, int(ch))
        update_corr_page(p)

# correlator channel select callback -------------------------------------------
def on_corr_ch_select(e, p):
    ch = p.box1.get()
    set_sel_ch(rcv_body, int(ch))
    update_corr_page(p)

# update Correlator page -------------------------------------------------------
def update_corr_page(p):
    update_corr_ch_sel(p)
    ch = int(p.box1.get())
    rng = [float(p.box2.get()), float(p.box3.get())]
    type = p.box4.get()
    state, fs, lock, cn0, coff, fd, pos, C = get_corr_stat(rcv_body, ch)
    tt, T, P = get_corr_hist(rcv_body, ch, rng[0])
    update_corr_plot1(p.plt1, coff, fs, pos, C, type, rng[1])
    update_corr_plot2(p.plt2, P, rng[1])
    update_corr_plot3(p.plt3, tt, T, P, rng)
    update_corr_text(p, ch, tt)
    
# update correlator channel selection ------------------------------------------
def update_corr_ch_sel(p):
    chs = [s.split()[0] for s in get_ch_stat(rcv_body, 'ALL')[2:]]
    p.box1.configure(values=chs)
    if len(chs) > 0 and not p.box1.get() in chs:
        p.box1.set(chs[0])
        set_sel_ch(rcv_body, int(chs[0]))
    else:
        set_sel_ch(rcv_body, int(p.box1.get()))

# update correlator text -------------------------------------------------------
def update_corr_text(p, ch, time):
    for s in get_ch_stat(rcv_body, 'ALL')[2:]:
        ss = s.split()
        if int(ss[0]) != ch: continue
        text = 'SAT: %s  SIG: %s  PRN: %s  TIME: %.2f s  LOCK: %s s' % (ss[2],
            ss[3], ss[4], time, ss[5])
        p.txt1.configure(text=text)
        xs, ys = plt.plot_scale(p.plt3)
        text = ('C/N0: %s dB-Hz  COFF: %s ms  DOP: %s Hz  ADR: %s cyc  SYNC: %s' +
            '  #NAV: %s') % (ss[6], ss[8], ss[9], ss[10], ss[11], ss[12])
        plt.plot_text(p.plt3, p.plt3.xl[0] + 12 / xs, p.plt3.yl[1] - 15 / ys,
            text, anchor=W)
        return

# update correlator plot 1 -----------------------------------------------------
def update_corr_plot1(p, coff, fs, pos, C, type, rng):
    x = [coff + pos[i] / fs * 1e3 for i in range(len(pos))]
    C *= np.sign(C[0])
    plt.plot_clear(p)
    plt.plot_xlim(p, [x[4], x[-1]])
    plt.plot_ylim(p, [-rng * 0.3 if type == 'I' else 0, rng])
    xs, ys = plt.plot_scale(p)
    p.title = 'I * sign(IP)' if type == 'I' else 'sqrt(I\xb2+Q\xb2)'
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, [coff, coff], p.yl, color='grey')
    plt.plot_poly(p, p.xl, [0, 0], color='grey')
    plt.plot_dots(p, [coff], [0], color=plt.FG_COLOR, size=5)
    y = C.real if type == 'I' else abs(C)
    plt.plot_poly(p, x[4:], y[4:], color=P2_COLOR)
    plt.plot_dots(p, x[4:], y[4:], color=P2_COLOR, fill=P1_COLOR, size=3)
    plt.plot_dots(p, x[:3], y[:3], color=P1_COLOR, fill=P1_COLOR, size=9)
    plt.plot_axis(p, gcolor=None)
    plt.plot_text(p, p.xl[1] - 18 / xs, p.yl[0] + 10 / ys, 'COFF (ms)',
        anchor=SE)

# update correlator plot 2 -----------------------------------------------------
def update_corr_plot2(p, P, rng):
    plt.plot_clear(p)
    plt.plot_xlim(p, [-rng, rng])
    plt.plot_ylim(p, [-rng, rng])
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, [0, 0], p.yl, color='grey')
    plt.plot_poly(p, p.xl, [0, 0], color='grey')
    plt.plot_dots(p, [0], [0], color=plt.FG_COLOR, size=5)
    plt.plot_dots(p, P.real, P.imag, color=P2_COLOR, size=1)
    plt.plot_dots(p, P[-1:].real, P[-1:].imag, color=BG_COLOR1, size=11)
    plt.plot_dots(p, P[-1:].real, P[-1:].imag, color=P1_COLOR, fill=P1_COLOR,
        size=9)
    plt.plot_axis(p, gcolor=None)
    
# update correlator plot 3 -----------------------------------------------------
def update_corr_plot3(p, tt, T, P, rng):
    t = [tt + (i - len(P) + 1) * T for i in range(len(P))]
    plt.plot_clear(p)
    plt.plot_xlim(p, [t[-1] - rng[0], t[-1]])
    plt.plot_ylim(p, [-rng[1], rng[1]])
    xs, ys = plt.plot_scale(p)
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, t, P.imag, color=P2_COLOR)
    plt.plot_poly(p, t, P.real, color=P1_COLOR)
    plt.plot_axis(p, gcolor=None)
    plt.plot_dots(p, t[-1:], P[-1:].imag, color=P2_COLOR, fill=P2_COLOR, size=9)
    plt.plot_dots(p, t[-1:], P[-1:].real, color=BG_COLOR1, fill=BG_COLOR1, size=11)
    plt.plot_dots(p, t[-1:], P[-1:].real, color=P1_COLOR, fill=P1_COLOR, size=9)
    plt.plot_text(p, p.xl[1] - 70 / xs, p.yl[1] - 15 / ys, '--- IP', P1_COLOR)
    plt.plot_text(p, p.xl[1] - 35 / xs, p.yl[1] - 15 / ys, '--- QP', P2_COLOR)

# generate Log page ------------------------------------------------------------
def log_page_new(parent):
    filts = ('', '$TIME', '$LOG', '$CH', '$POS', '$OBS', '$LNAV', '$CNAV',
        '$CNV2', '$GSTR', '$INAV', '$FNAV', '$L6FRM', '$BCNV', '$IRNV', '$SBAS')
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent, bg=BG_COLOR1)
    p.toolbar = Frame(p.panel, height=TB_HEIGHT, bg=BG_COLOR1)
    p.toolbar.pack_propagate(0)
    p.toolbar.pack(fill=X)
    ttk.Label(p.toolbar, text='Filter').pack(side=LEFT, padx=(6, 4))
    p.box1 = ttk.Combobox(p.toolbar, width=12, height=len(filts), values=filts, font=get_font())
    p.box1.pack(side=LEFT)
    p.btn1 = ttk.Button(p.toolbar, width=8, text='Pause')
    p.btn2 = ttk.Button(p.toolbar, width=8, text='Clear')
    p.btn2.pack(side=RIGHT, padx=(0, 18))
    p.btn1.pack(side=RIGHT)
    p.txt1 = ttk.Label(p.toolbar)
    p.txt1.pack(side=LEFT, expand=1, fill=X)
    panel = Frame(p.panel)
    panel.pack(expand=1, fill=X)
    p.cvs1 = Canvas(panel, height=15 * MAX_RCVLOG, bg='white')
    p.scl = ttk.Scrollbar(panel, orient=VERTICAL, command=p.cvs1.yview)
    p.scl.pack(side=RIGHT, fill=Y)
    p.cvs1.pack(expand=1, fill=BOTH)
    p.cvs1.config(yscrollcommand=p.scl.set)
    p.panel.bind('<Expose>', lambda e: on_log_page_expose(e, p))
    p.btn1.bind('<Button-1>', lambda e: on_log_pause_push(e, p))
    p.btn2.bind('<Button-1>', lambda e: on_log_clear_push(e, p))
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_log_filt_select(e, p))
    return p

# Log page expose callback -----------------------------------------------------
def on_log_page_expose(e, p):
    update_log_page(p)

# Log page button pause push callback ------------------------------------------
def on_log_pause_push(e, p):
    p.btn1.configure(text='Pause' if p.btn1['text'] == 'Resume' else 'Resume')

# Log page button clear push callback ------------------------------------------
def on_log_clear_push(e, p):
    global rcv_log
    rcv_log = []
    show_log_page(p)

# Log page filter select callback ----------------------------------------------
def on_log_filt_select(e, p):
    show_log_page(p)

# update Log page --------------------------------------------------------------
def update_log_page(p):
    if p.btn1['text'] != 'Resume':
        show_log_page(p)

# show Log page ----------------------------------------------------------------
def show_log_page(p):
    global rcv_log
    filt = p.box1.get()
    p.cvs1.delete('all')
    text = [s for s in rcv_log if filt_log(filt, s)]
    if len(text) >= MAX_RCVLOG:
        text = text[-MAX_RCVLOG:]
    txt = p.cvs1.create_text(4, 0, text='\n'.join(text), anchor=NW,
        font=get_font(mono=1), fill=P1_COLOR)
    p.cvs1.configure(scrollregion=p.cvs1.bbox(txt))
    p.cvs1.yview(MOVETO, 1.0)

# filter log -------------------------------------------------------------------
def filt_log(filt, log):
    if filt == '': return 1
    for s in filt.split():
        if not s in log: return 0
    return 1

# Start button push callback ---------------------------------------------------
def on_btn_start_push(bar):
    global rcv_body
    if not rcv_body:
        rcv_body = rcv_open(sys_opt, inp_opt, out_opt, sig_opt)
        if rcv_body == None:
            stat_bar.msg1.configure(text='Receiver start error.')
            return
        if inp_opt.inp.get() == 0:
            info = ' (bus/port=%s, conf=%s)' % (inp_opt.dev.get(), \
                inp_opt.conf_path.get() if inp_opt.conf_ena.get() else '')
        else:
            info = ' (path=%s)' % (inp_opt.str_path.get())
        stat_bar.msg1.configure(text='Receiver started.' + info)
        for i, btn in enumerate(bar.panel.winfo_children()):
            btn.configure(state=NORMAL if i in (1, 6) else DISABLED)

# Stop button push callback ----------------------------------------------------
def on_btn_stop_push(bar):
    global rcv_body
    if rcv_body:
        rcv_close(rcv_body)
        rcv_body = None
        for i, btn in enumerate(bar.panel.winfo_children()):
            btn.configure(state=DISABLED if i in (1,) else NORMAL)
        stat_bar.msg1.configure(text='Receiver stopped.')

# Input button push callback ---------------------------------------------------
def on_btn_input_push(bar):
    global inp_opt
    if not rcv_body:
        inp_opt = sdr_opt.inp_opt_dlg(root, inp_opt)

# Output button push callback --------------------------------------------------
def on_btn_output_push(bar):
    global out_opt
    if not rcv_body:
        out_opt = sdr_opt.out_opt_dlg(root, out_opt)

# Signal button push callback --------------------------------------------------
def on_btn_signal_push(bar):
    global sig_opt
    if not rcv_body:
        sig_opt = sdr_opt.sig_opt_dlg(root, sig_opt)

# System button push callback --------------------------------------------------
def on_btn_system_push(bar):
    global sys_opt
    if not rcv_body:
        sys_opt = sdr_opt.sys_opt_dlg(root, sys_opt)

# Help button push callback ----------------------------------------------------
def on_btn_help_push(bar):
    help_dlg(root)

# Exit button push callback ----------------------------------------------------
def on_btn_exit_push(bar):
    if not rcv_body:
        sdr_opt.save_opts(OPTS_FILE, inp_opt, out_opt, sig_opt, sys_opt)
        exit()

# root Window close callback ---------------------------------------------------
def on_root_close():
    if rcv_body:
        rcv_close(rcv_body)
    exit()

# pages interval timer callback ------------------------------------------------
def on_pages_timer(note, pages):
    tt = time.time()
    update_rcv_log()
    ti = pages_update(note, pages)
    if not rcv_body: ti = UD_CYCLE3
    ts = (int)((time.time() - tt) * 1e3)
    note.after(ti - ts if ti > ts else 1, lambda: on_pages_timer(note, pages))

# update pages -----------------------------------------------------------------
def pages_update(note, pages):
    i = note.index('current')
    if i == 0:
        update_rcv_page(pages[0])
    elif i == 1:
        update_rfch_page(pages[1])
    elif i == 2:
        update_bbch_page(pages[2])
    elif i == 3:
        update_corr_page(pages[3])
    elif i == 4:
        update_log_page(pages[4])
    if i != 3:
        set_sel_ch(rcv_body, 0)
    return UD_CYCLE1 if i in (1, 3) else UD_CYCLE2 # update interval (ms)

# set styles -------------------------------------------------------------------
def set_styles():
    style = ttk.Style()
    style.configure('TButton', font=get_font(1), background=BG_COLOR1)
    style.map('TButton', background=[(DISABLED, BG_COLOR1)])
    style.configure('TRadiobutton', font=get_font(), background=BG_COLOR1)
    style.configure('TLabel', font=get_font(), background=BG_COLOR1)
    style.map('TLabel', background=[(DISABLED, BG_COLOR1)])
    style.configure('TEntry', font=get_font(), background='white')
    style.configure('TCheckbutton', font=get_font(), background=BG_COLOR1)
    style.map('TCheckbutton', background=[(DISABLED, BG_COLOR1)])
    style.configure('Treeview', font=get_font(), rowheight=ROW_HEIGHT,
        foreground=P1_COLOR)
    style.configure('Treeview.Heading', font=get_font())
    style.configure('TNotebook', background=BG_COLOR1)
    style.configure('TNotebook.Tab', font=get_font(1), padding=(25, 2))
    style.map('TNotebook.Tab', background=[('selected', BG_COLOR1)])
    style.configure('TCombobox', font=get_font(), background=BG_COLOR1)
    style.configure('link.TLabel', font=get_font(), foreground='blue')

# main -------------------------------------------------------------------------
if __name__ == '__main__':
    
    # generate root window
    root = Tk()
    root.geometry('%dx%d' % (WIDTH, HEIGHT))
    root.minsize(WIDTH * 3 // 4, HEIGHT * 3 // 4)
    root.title(AP_NAME + ' ' + VERSION)
    root.protocol("WM_DELETE_WINDOW", on_root_close)
    
    # set styles
    set_styles()
    
    # load options
    sdr_opt.set_bgcolor(BG_COLOR2)
    sdr_opt.set_font(get_font())
    inp_opt = sdr_opt.inp_opt_new()
    out_opt = sdr_opt.out_opt_new()
    sig_opt = sdr_opt.sig_opt_new()
    sys_opt = sdr_opt.sys_opt_new()
    sdr_opt.load_opts(OPTS_FILE, inp_opt, out_opt, sig_opt, sys_opt)
    
    # SDR receiver
    rcv_body = None
    rcv_log = []
    
    # generate button bar
    labels = ('Start', 'Stop', 'Input ...', 'Output ...', 'Signal ...',
        'System ...', 'Help ...', 'Exit')
    callbacks = (on_btn_start_push, on_btn_stop_push, on_btn_input_push,
        on_btn_output_push, on_btn_signal_push, on_btn_system_push,
        on_btn_help_push, on_btn_exit_push)
    btn_bar = btn_bar_new(root, labels, callbacks)
    btn_bar.panel.winfo_children()[1].configure(state=DISABLED)
    
    # generate status bar
    stat_bar = status_bar_new(root)
    
    # generate receiver pages
    labels = ('Receiver', 'RF Channels', 'BB Channels', 'Correlators', 'Log')
    note = ttk.Notebook(root, padding=0)
    note.pack(fill=BOTH)
    pages = (rcv_page_new(note), rfch_page_new(note),  bbch_page_new(note),
        corr_page_new(note), log_page_new(note))
    for i in range(len(pages)):
        note.add(pages[i].panel, text=labels[i])
    note.after(100, lambda: on_pages_timer(note, pages))
    
    # main loop of Tk
    root.mainloop()
