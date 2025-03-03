#!/usr/bin/env python3
#
#  Pocket SDR Python AP - GNSS SDR Receiver
#
#  Author:
#  T.TAKASU
#
#  History:
#  2024-06-29  1.0  ver.0.13
#  2024-12-30  1.1  ver.0.14
#
import os, platform, time, re
from math import *
from ctypes import *
import numpy as np
from numpy import ctypeslib
from tkinter import *
from tkinter import ttk
from tkinter import scrolledtext
import tkinter.font as tkfont
import sdr_func, sdr_code, sdr_opt, sdr_rtk
import sdr_plot as plt

# constants --------------------------------------------------------------------
TITLE      = 'An Open-Source GNSS SDR\n(Software Defined Receiver)'
AP_URL     = 'https://github.com/tomojitakasu/PocketSDR'
AP_DIR     = os.path.dirname(__file__)
COPYRIGHT  = 'Copyright (c) 2021-2025, T.Takasu\nAll rights reserved.'
OPTS_FILE  = AP_DIR + '/pocket_sdr.ini' # options file
WIDTH      = 800             # root window width
HEIGHT     = 600             # root window height
TB_HEIGHT  = 25              # toolbar height
SB_HEIGHT  = 20              # status bar height
P1_COLOR   = '#003020'       # plot color 1
P2_COLOR   = '#888844'       # plot color 2
P3_COLOR   = '#BBBBBB'       # plot color 3
SDR_N_CORR = (6+81)          # number of correlators
SDR_N_HIST = 5000            # number of correlator history
SDR_N_PSD  = 2048            # number FFT points for PSD
MAX_RCVLOG = 2000            # max receiver logs
MAX_SOLLOG = 3600            # max solution logs
UD_CYCLE1  = 50              # update cycle (ms) RF channels/Correlator pages
UD_CYCLE2  = 200             # update cycle (ms) other pages
UD_CYCLE3  = 1000            # update cycle (ms) receiver stopped
D2R = np.pi / 180
SYSTEMS = ('ALL', 'GPS', 'GLONASS', 'Galileo', 'QZSS', 'BeiDou', 'NavIC', 'SBAS')

# platform dependent settings --------------------------------------------------
env = platform.platform()
if 'Windows' in env:
    LIBSDR = AP_DIR + '/../lib/win32/libsdr.so'
    FONT = ('Tahoma', 'Consolas')
    FONT_SIZE = (9, 9)
    BG_COLOR1 = '#F8F8F8'
    BG_COLOR2 = BG_COLOR1
    ROW_HEIGHT = 15
elif 'macOS' in env:
    LIBSDR = AP_DIR + '/../lib/macos/libsdr.so'
    #FONT = ('Arial Narrow', 'Monaco')
    FONT = ('Tahoma', 'Monaco')
    FONT_SIZE = (13, 12)
    BG_COLOR1 = '#E5E5E5'
    BG_COLOR2 = '#ECECEC'
    ROW_HEIGHT = 14
else: # Linux or Raspberry Pi OS
    LIBSDR = AP_DIR + '/../lib/linux/libsdr.so'
    FONT = ('DejaVu Sans', 'DejaVu Sans Mono')
    #FONT = ('Noto Sans', 'Noto Sans Mono')
    #FONT = ('Ubuntu', 'Ubuntu Mono')
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

# global variables -------------------------------------------------------------
rcv_body = None
sol_log, rcv_log, rcv_log_filt = [], [], ''
root_resize = 0

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

# convert string to time -------------------------------------------------------
def str2time(str):
    ep = [float(s) for s in re.split('[-/:_ ]', str)]
    return sdr_rtk.epoch2time(ep) if len(ep) >= 3 else sdr_rtk.GTIME()

# start receiver ---------------------------------------------------------------
def rcv_open(sys_opt, inp_opt, out_opt, sig_opt):
    set_rcv_opts(sys_opt)
    set_log_mask(out_opt)
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
    opt = ''
    opt += ' -RFCH ' + sig_opt.sig_rfch.get()
    c_sigs = (c_char_p * len(sigs))(*[s.encode() for s in sigs])
    c_prns = (c_int32 * len(sigs))(*prns)
    c_paths = (c_char_p * 4)(*[s.encode() for s in paths])
    libsdr.sdr_rcv_open_dev.argtypes = [POINTER(c_char_p), POINTER(c_int32),
        c_int32, c_int32, c_int32, c_char_p, POINTER(c_char_p), c_char_p]
    libsdr.sdr_rcv_open_dev.restype = c_void_p
    return libsdr.sdr_rcv_open_dev(c_sigs, c_prns, len(sigs), bus, port,
        conf_file.encode(), c_paths, opt.encode())

# start receiver by file -------------------------------------------------------
def rcv_open_file(sys_opt, inp_opt, out_opt, sig_opt):
    sigs, prns = get_sig_opt(sig_opt)
    fmt = inp_opt.fmts.index(inp_opt.fmt.get())
    fs = to_float(inp_opt.fs.get()) * 1e6
    fo = [to_float(inp_opt.fo[i].get()) * 1e6 for i in range(8)]
    IQ = [1 if inp_opt.IQ[i].get() == 'I' else 2 for i in range(8)]
    bits = [to_int(inp_opt.bits[i].get()) for i in range(8)]
    toff = to_float(inp_opt.toff.get())
    tscale = to_float(inp_opt.tscale.get())
    path = inp_opt.str_path.get()
    paths = [out_opt.path[i].get() if out_opt.path_ena[i].get() else ''
        for i in range(4)]
    c_sigs = (c_char_p * len(sigs))(*[s.encode() for s in sigs])
    c_prns = (c_int32 * len(sigs))(*prns)
    c_fo = (c_double * 8)(*fo)
    c_IQ = (c_int32 * 8)(*IQ)
    c_bits = (c_int32 * 8)(*bits)
    c_paths = (c_char_p * 4)(*[s.encode() for s in paths])
    libsdr.sdr_func_init.argtypes = (c_char_p,)
    libsdr.sdr_func_init(sys_opt.fftw_wisdom_path.get().encode())
    libsdr.sdr_rcv_open_file.argtypes = (POINTER(c_char_p), POINTER(c_int32),
        c_int32, c_int32, c_double, POINTER(c_double), POINTER(c_int32),
        POINTER(c_int32), c_double, c_double, c_char_p, POINTER(c_char_p),
        c_char_p)
    libsdr.sdr_rcv_open_file.restype = c_void_p
    return libsdr.sdr_rcv_open_file(c_sigs, c_prns, len(sigs), fmt, fs, c_fo,
        c_IQ, c_bits, toff, tscale, path.encode(), c_paths,
        sig_opt.sig_rfch.get().encode())

# get signal options -----------------------------------------------------------
def get_sig_opt(opt):
    sigs, prns = [], []
    for i in range(16):
        for j in range(len(opt.sys)):
            if len(opt.sig[j]) <= i: continue
            if not opt.sys_sel[j].get() or not opt.sig_sel[j][i].get(): continue
            satno = opt.satno[j].get()
            add_sig(sigs, prns, j, satno, opt.sig[j][i])
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
    if sig == 'L1CA' or sig == 'L1CD' or sig == 'L1CP' or sig == 'L2CM' or \
       sig == 'L5I' or sig == 'L5Q' or sig == 'L6D':
        return 192 + no
    elif sig == 'L1S' and no <= 7:
        return 182 + no
    elif sig == 'L6E':
        return 202 + no
    elif sig == 'L1CB' and no in (4, 5, 8, 9, 10):
        return 199 + no if no <= 5 else 197 + no if no <= 9 else 202
    elif sig in ('L5SI', 'L5SQ') and no in (2, 3, 4, 7):
        return 182 + no
    elif sig in ('L5SIV', 'L5SQV') and no in (4, 8, 9):
        return 186 if no == 4 else 197 + no
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
    libsdr.sdr_rcv_setopt('bump_jump'.encode(), float(sys_opt.bump_jump.get() == 'ON'))

# set log mask -----------------------------------------------------------------
def set_log_mask(out_opt):
    mask = [sel.get() for sel in out_opt.log_sel]
    log_mask = (c_int32 * len(mask))(*mask)
    libsdr.sdr_log_mask.argtypes = (POINTER(c_int32), c_int32)
    libsdr.sdr_log_mask(log_mask, len(log_mask))

# get library name and version -------------------------------------------------
def get_name_ver():
    libsdr.sdr_get_name.restype = c_char_p
    libsdr.sdr_get_ver.restype = c_char_p
    return libsdr.sdr_get_name().decode(), libsdr.sdr_get_ver().decode()

# get receiver status ----------------------------------------------------------
def get_rcv_stat(rcv):
    libsdr.sdr_rcv_rcv_stat.argtypes = (c_void_p,)
    libsdr.sdr_rcv_rcv_stat.restype = c_char_p
    return libsdr.sdr_rcv_rcv_stat(rcv).decode()

# get receiver stream status ---------------------------------------------------
def get_str_stat(rcv):
    stat = (c_int32 * 4)()
    libsdr.sdr_rcv_str_stat.argtypes = (c_void_p, POINTER(c_int32))
    libsdr.sdr_rcv_str_stat(rcv, stat)
    return stat

# get receiver channel status --------------------------------------------------
def get_ch_stat(rcv, sys, all=0, min_lock=2.0, rfch=0):
    libsdr.sdr_rcv_ch_stat.argtypes = (c_void_p, c_char_p, c_int32, c_double,
        c_int32)
    libsdr.sdr_rcv_ch_stat.restype = c_char_p
    stat = libsdr.sdr_rcv_ch_stat(rcv, sys.encode(), all, min_lock, rfch)
    return stat.decode().splitlines()

# get signal status ------------------------------------------------------------
def get_sig_stat(rcv, sys, sort=0):
    stat = get_ch_stat(rcv, sys)[2:]
    sig_stat = []
    for i, s in enumerate(stat):
        ss = s.split()
        no = 'GREJCIS'.find(ss[2][0]) * 100 + int(ss[2][1:])
        sig_stat.append([no, -float(ss[6]) if sort else i, ss[2], ss[3],
            float(ss[6]), int(ss[4])])
    sig_stat = sorted(sig_stat)
    sat = [s[2] for s in sig_stat]
    sig = [s[3] for s in sig_stat]
    cn0  = [s[4] for s in sig_stat]
    prn  = [s[5] for s in sig_stat]
    return sorted(set(sat), key=sat.index), sat, sig, cn0, prn

# get satellite status ---------------------------------------------------------
def get_sat_stat(rcv, sats):
    libsdr.sdr_rcv_sat_stat.argtypes = (c_void_p, c_char_p)
    libsdr.sdr_rcv_sat_stat.restype = c_char_p
    az, el, pvt, obs, eph, svh, fcn = [], [], [], [], [], [], []
    for sat in sats:
        stat = libsdr.sdr_rcv_sat_stat(rcv, sat.encode()).decode().split()
        az.append(float(stat[1]) if len(stat) >=2 else 0.0)
        el.append(float(stat[2]) if len(stat) >=3 else 0.0)
        pvt.append(int(stat[3]) if len(stat) >=4 else 0)
        obs.append(int(stat[4]) if len(stat) >=5 else 0)
        eph.append(int(stat[5]) if len(stat) >=6 else 0)
        svh.append(int(stat[6]) if len(stat) >=7 else 0)
        fcn.append(int(stat[7]) if len(stat) >=8 else 0)
    return az, el, pvt, obs, eph, svh, fcn

# get RF channel status -------------------------------------------------------
def get_rfch_stat(rcv, ch):
    stat = np.zeros(6, dtype='float64')
    libsdr.sdr_rcv_rfch_stat.argtypes = (c_void_p, c_int32,
        ctypeslib.ndpointer('float64'))
    if not libsdr.sdr_rcv_rfch_stat(rcv, ch, stat):
        return 0, 0, 24.0, 0.0, 0, 0
    return int(stat[0]), int(stat[1]), stat[2] / 1e6, stat[3] / 1e6, \
        int(stat[4]), int(stat[5]) # dev, fmt, fs (MHz), fo (MHz), IQ, bits

# get RF channel PSD -----------------------------------------------------------
def get_rfch_psd(rcv, ch, tave):
    psd = np.zeros(SDR_N_PSD, dtype='float32')
    libsdr.sdr_rcv_rfch_psd.argtypes = (c_void_p, c_int32, c_double, c_int32,
        ctypeslib.ndpointer('float32'))
    n = libsdr.sdr_rcv_rfch_psd(rcv, ch, tave, SDR_N_PSD, psd)
    return psd[:n] if n > 0 else psd[:2]

# get RF channel histogram -----------------------------------------------------
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

# get RF channel IF Filter ------------------------------------------------------
def get_rfch_filt(rcv, ch):
    bw, freq, order = c_double(-1), c_double(), c_int32()
    libsdr.sdr_rcv_get_filt.argtypes = (c_void_p, c_int32, POINTER(c_double),
        POINTER(c_double), POINTER(c_int32))
    libsdr.sdr_rcv_get_filt(rcv, ch - 1, byref(bw), byref(freq), byref(order))
    return bw.value, freq.value, order.value

# set RF channel IF Filter ------------------------------------------------------
def set_rfch_filt(rcv, ch, bw, freq, order):
    libsdr.sdr_rcv_set_filt.argtypes = (c_void_p, c_int32, c_double, c_double,
        c_int32)
    return libsdr.sdr_rcv_set_filt(rcv, ch - 1, bw, freq, order)

# select receiver channel -------------------------------------------------------
def set_sel_ch(rcv, ch):
    libsdr.sdr_rcv_sel_ch.argtypes = (c_void_p, c_int32)
    libsdr.sdr_rcv_sel_ch(rcv, ch)

# get correlator status ---------------------------------------------------------
def get_corr_stat(rcv, ch):
    stat = np.array([0, 24e6, 0, 0, 0, 0, 0], dtype='float64')
    pos = np.zeros(SDR_N_CORR, dtype='float64')
    pos[0:4] = [0, -40, 0, 40]
    C = np.zeros(SDR_N_CORR, dtype='complex64')
    P = np.zeros(SDR_N_CORR, dtype='float64')
    libsdr.sdr_rcv_corr_stat.argtypes = (c_void_p, c_int32,
        ctypeslib.ndpointer('float64'), ctypeslib.ndpointer('float64'),
        ctypeslib.ndpointer('complex64'), ctypeslib.ndpointer('float64'))
    n = libsdr.sdr_rcv_corr_stat(rcv, ch, stat, pos, C, P)
    # state, fs, lock, cn0, coff, fd, pos, C, P
    return int(stat[0]), stat[1], stat[2], stat[3], stat[4], stat[5], \
        int(stat[6]) if n > 0 else 1, pos[:n] if n > 0 else pos[:4], \
        C[:n] if n > 0 else C[:4], P[:n] if n > 0 else P[:4]

# get correlator history --------------------------------------------------------
def get_corr_hist(rcv, ch, tspan):
    stat = np.array([0, 1e-3], dtype='float64')
    P = np.zeros(SDR_N_HIST, dtype='complex64')
    libsdr.sdr_rcv_corr_hist.argtypes = (c_void_p, c_int32, c_double,
        ctypeslib.ndpointer('float64'), ctypeslib.ndpointer('complex64'))
    n = libsdr.sdr_rcv_corr_hist(rcv, ch, tspan, stat, P)
    return stat[0], stat[1], P[:n] if n > 0 else P[:2] # time, T, P

# get PVT solution -------------------------------------------------------------
def get_rcv_pvt_sol(rcv):
    buff = create_string_buffer(128)
    libsdr.sdr_rcv_pvt_sol.argtypes = (c_void_p, c_char_p)
    if not libsdr.sdr_rcv_pvt_sol(rcv, buff):
        return '1970-01-01 00:00:00.0 0.00000000 0.00000000 0.000 0/0 ---'
    return buff.value.decode() 

# get satellite color ----------------------------------------------------------
def sat_color(sat, sel=0):
    colors = (('#006600', '#EE9900', '#CC00CC', '#0000AA', '#CC0000', '#007777',
        '#777777'), ('#88AA88', '#F8CC88', '#EE88EE', '#8888CC', '#E08888',
        '#88BBBB', '#BBBBBB'))
    for i in range(len(colors[0])):
        if sat[0] == 'GREJCIS'[i]:
            return colors[sel][i]
    return BG_COLOR1

# update receiver log ---------------------------------------------------------
def update_rcv_log():
    global rcv_log, rcv_log_filt
    buff_size = 262144
    buff = create_string_buffer(buff_size)
    libsdr.sdr_get_log.argtypes = (POINTER(c_char), c_int32)
    size = libsdr.sdr_get_log(buff, buff_size)
    for log in buff.value.decode().splitlines():
        if len(log) > 0 and filt_log(rcv_log_filt, log):
            rcv_log.append(log)
    if len(rcv_log) > MAX_RCVLOG:
        rcv_log = rcv_log[-MAX_RCVLOG:]
    return 

# update solution log ----------------------------------------------------------
def update_sol_log():
    global sol_log
    sol = get_rcv_pvt_sol(rcv_body).split()
    if len(sol) < 7 or sol[6] != 'FIX': return
    time = str2time(sol[0] + ' ' + sol[1])
    if len(sol_log) > 0 and sdr_rtk.timediff(time, sol_log[-1][0]) < 1e-3:
        return
    pos = [float(s) for s in sol[2:5]]
    pos[0] *= D2R
    pos[1] *= D2R
    nsat = [int(s) for s in sol[5].split('/')]
    sol_log.append([time, pos, nsat])
    if len(sol_log) > MAX_SOLLOG:
        sol_log = sol_log[-MAX_SOLLOG:]

# filter log -------------------------------------------------------------------
def filt_log(filt, log):
    if filt == '': return 1
    for s in filt.split():
        stat = 0
        for ss in s.split('|'):
            if ss in log: stat = 1
        if not stat: return 0
    return 1

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

# generate tool bar ------------------------------------------------------------
def tool_bar_new(parent):
    toolbar = Frame(parent, height=TB_HEIGHT, bg=BG_COLOR1)
    toolbar.pack_propagate(0)
    toolbar.pack(fill=X)
    return toolbar

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

# show status bar --------------------------------------------------------------
def status_bar_show(text):
    stat_bar.msg1.configure(text=text)
    stat_bar.msg1.update()

# generate selection box -------------------------------------------------------
def sel_box_new(parent, vals=[], val='', width=8):
    box = ttk.Combobox(parent, width=width, state='readonly', justify=CENTER,
        values=vals, height=min([len(vals), 32]), font=get_font())
    box.set(val)
    return box

# show Help dialog -------------------------------------------------------------
def help_dlg(root):
    dlg = sdr_opt.modal_dlg_new(root, 280, 180, 'About', nocancel=1)
    name, ver = get_name_ver()
    sdr_opt.link_label_new(dlg.panel, text=name + ' ver.' + ver,
        font=get_font(2, 'bold'), link=AP_URL).pack(pady=(4, 0))
    ttk.Label(dlg.panel, text=TITLE, justify=CENTER).pack(pady=2)
    ttk.Label(dlg.panel, text=COPYRIGHT, justify=CENTER).pack()
    root.wait_window(dlg.win)

# generate Receiver page -------------------------------------------------------
def rcv_page_new(parent):
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.toolbar = tool_bar_new(p.panel)
    p.txt1 = ttk.Label(p.toolbar)
    p.txt1.pack(side=LEFT, fill=X, padx=4)
    p.ind = []
    for i in range(4):
        frm = Frame(p.toolbar, bg='lightgrey')
        frm.pack(side=RIGHT, padx=(1, 1 if i > 0 else 6), pady=(2, 0))
        ind = Frame(frm, width=6, height=10)
        ind.pack(fill=BOTH, padx=1, pady=1)
        p.ind.append(ind)
    ttk.Label(p.toolbar, text='Output').pack(side=RIGHT, padx=(8, 4))
    p.box1 = sel_box_new(p.toolbar, SYSTEMS, 'ALL', width=8)
    p.box1.pack(side=RIGHT, padx=4)
    ttk.Label(p.toolbar, text='System').pack(side=RIGHT)
    panel1 = Frame(p.panel)
    panel1.pack(expand=1, fill=BOTH)
    p.plt1 = plt.plot_new(panel1, 257, 257, margin=(25, 25, 25, 25),
        xlim=(-1, 1), ylim=(-1, 1), aspect=1, font=get_font(-1))
    p.plt1.c.pack(side=RIGHT, expand=1, fill=BOTH)
    p.stat = plt.plot_new(panel1, 543, 257, margin=(20, 15, 30, 30))
    p.stat.c.pack(side=LEFT, expand=1, fill=BOTH)
    p.plt2 = plt.plot_new(p.panel, 800, 245, title='Signal C/N0 (dB-Hz)', tick=10)
    p.plt2.c.pack(expand=1, fill=BOTH)
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_sys_select(e, p))
    update_rcv_page(p)
    return p

# system select callback -------------------------------------------------------
def on_sys_select(e, p):
    sys = p.box1.get()
    update_sky_plot(p.plt1, sys)
    update_sig_plot(p.plt2, sys)

# update Receiver page ---------------------------------------------------------
def update_rcv_page(p):
    sys = p.box1.get()
    update_rcv_stat(p.stat)
    update_str_stat(p.ind)
    update_sky_plot(p.plt1, sys)
    update_sig_plot(p.plt2, sys)

# update receiver status panel -------------------------------------------------
def update_rcv_stat(p):
    labels = ('Receiver Time (s)', 'Input Source', 'IF Data Fmt/# RF CH',
        'LO Frequencies (MHz)', '', '', 'Sampling',
        'Sampling Rate (Msps)', '# BB CH Locked/All',
        'IF Data Rate (MB/s)', 'IF Data Buffer Usage (%)', 'Time (GPST)',
        'Solution Status', 'Latitude (\xb0)', 'Longitude (\xb0)',
        'Altitude (m)', '# Sats Used/All', 'Solution Latency (s)',
        'Output', '# PVT Solutions', '# OBS/NAV Data', 'IF Data Log (MB)')
    stat = get_rcv_stat(rcv_body).split()
    sol = get_rcv_pvt_sol(rcv_body).split()
    val = stat[0:3] + [''] + stat[3:10] + [sol[0] + ' ' + sol[1], sol[6]] + \
        sol[2:6] + [stat[10], ''] + stat[11:]
    plt.plot_clear(p)
    xs, ys = plt.plot_scale(p)
    for i in range(len(labels)):
        x, y = 0.0 if i < 11 else 0.51, 0.5 + (5 - i % 11) * 20 / ys
        plt.plot_text(p, x, y, labels[i], anchor=W, font=get_font(0, 'bold'),
            color=P1_COLOR)
        plt.plot_text(p, x + 0.48, y, val[i], anchor=E, color=P1_COLOR)

# update stream status ---------------------------------------------------------
def update_str_stat(p):
    col = ('#CC0000', BG_COLOR1, '#EE9900', '#006600', '#00CC00')
    stat = get_str_stat(rcv_body)
    for i, ind in enumerate(p):
        ind.configure(bg=col[stat[3-i]+1])

# update skyplot ---------------------------------------------------------------
def update_sky_plot(p, sys):
    sats, sat, sig, cn0, prn = get_sig_stat(rcv_body, sys, 1)
    az, el, pvt, obs, eph, svh, fcn = get_sat_stat(rcv_body, sats)
    plt.plot_clear(p)
    plt.plot_sky(p, color=None)
    xs, ys = plt.plot_scale(p)
    for i in range(len(sats) - 1, -1, -1):
        if el[i] <= 0.0:
            continue
        x = (90 - el[i]) / 90 * sin(az[i] * pi / 180)
        y = (90 - el[i]) / 90 * cos(az[i] * pi / 180)
        color1 = sat_color(sats[i]) if pvt[i] else BG_COLOR1
        color2 = BG_COLOR1 if pvt[i] and rcv_body else plt.FG_COLOR
        plt.plot_circle(p, x, y, 12 / xs, fill=color1)
        plt.plot_text(p, x, y, sats[i], color=color2, font=get_font(-1))
    plt.plot_sky(p, gcolor=None)

# update signal plot -----------------------------------------------------------
def update_sig_plot(p, sys):
    sats, sat, sig, cn0, prn = get_sig_stat(rcv_body, sys, 1)
    az, el, pvt, obs, eph, svh, fcn = get_sat_stat(rcv_body, sats)
    sigs = sorted(set(sig))
    plt.plot_clear(p)
    plt.plot_xlim(p, [-0.7, len(sats) - 0.3])
    plt.plot_ylim(p, [20, 55])
    xs, ys = plt.plot_scale(p)
    plt.plot_axis(p, tcolor=None)
    if sys == 'ALL':
        for i, s in enumerate('GREJCIS'):
            x, y = p.xl[1] + (i * 9 - 70) / xs, p.yl[1] - 16 / ys
            plt.plot_text(p, x, y, s, color=sat_color(s))
    else:
        txt = 'Signals: '
        for s in sigs:
            txt += ' ' + s + ','
        x, y = p.xl[1] - 12 / xs, p.yl[1] - 16 / ys
        plt.plot_text(p, x, y, txt[:-1], anchor=E)
    for x, s in enumerate(sats):
        color = sat_color(s, sel=not pvt[x]) if rcv_body else BG_COLOR1
        idx = [i for i, ss in enumerate(sat) if ss == s]
        if sys != 'ALL':
            for i in range(len(sigs)):
                xi = x + (8 * i - (len(sigs) - 1) * 4) / xs
                plt.plot_rect(p, xi - 3 / xs, 20, xi + 3 / xs, p.yl[0] + 0.5,
                    fill=BG_COLOR1)
        for i in idx:
            xi = x
            if sys != 'ALL':
                j = sigs.index(sig[i])
                xi += (8 * j - (len(sigs) - 1) * 4) / xs
            plt.plot_rect(p, xi - 3 / xs, 20, xi + 3 / xs, cn0[i], fill=color)
    plt.plot_axis(p, gcolor=None)
    for x, s in enumerate(sats):
        text = s[1:] if sys == 'ALL' else s
        plt.plot_text(p, x, p.yl[0] - 2 / ys, text, color=sat_color(s), anchor=N)
    text = '#Sats: %d/%d' % (pvt.count(1), len(sats))
    plt.plot_text(p, -0.7 + 12 / xs, p.yl[1] - 16 / ys, text, anchor=W)

# generate RF Channels page ----------------------------------------------------
def rfch_page_new(parent):
    ti = ['Power Spectral Density (dB/Hz)', 'Histogram I', 'Histogram Q']
    labels = ['Frequency (MHz)', 'Quantized Value']
    chs = ['ALL'] + [str(i + 1) for i in range(8)] + ['1-4', '5-8']
    margin = (35, 25, 25, 40)
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.toolbar = tool_bar_new(p.panel)
    ttk.Label(p.toolbar, text='RF CH').pack(side=LEFT, padx=(8, 4))
    p.box1 = sel_box_new(p.toolbar, chs, '1', 5)
    p.box1.pack(side=LEFT)
    p.box2 = sel_box_new(p.toolbar, ['0.1', '0.03', '0.01', '0.003', '0.001'],
        '0.01', 5)
    p.box2.pack(side=RIGHT, padx=(2, 4))
    ttk.Label(p.toolbar, text='Ave (s)').pack(side=RIGHT)
    vals = ['-', 'Auto'];
    for g in range(64):
        vals.append(str(g))
    p.box3 = sel_box_new(p.toolbar, vals, '', 5)
    p.box3.pack(side=RIGHT, padx=(2, 4))
    ttk.Label(p.toolbar, text='LNA Gain').pack(side=RIGHT)
    p.box4 = sel_box_new(p.toolbar, ['3rd', '5th'], '', 3)
    p.box4.pack(side=RIGHT, padx=(2, 4))
    p.box5 = sel_box_new(p.toolbar, ['2.5', '4.2', '8.7', '16.4', '23.4', '36.0'],
        '', 4)
    p.box5.pack(side=RIGHT, padx=2)
    ttk.Label(p.toolbar, text='Filter BW (MHz)').pack(side=RIGHT)
    sdr_opt.link_label_new(p.toolbar, text='?', link=sdr_opt.BAND_LINK).pack(
        side=LEFT, padx=6)
    p.txt1 = ttk.Label(p.toolbar, foreground=P1_COLOR)
    p.txt1.pack(side=LEFT, expand=1, fill=X, padx=10)
    p.panel1 = Frame(p.panel)
    tis = ('GNSS Signal Band L1 (MHz)', 'GNSS Signal Band L2/L5/L6 (MHz)')
    freq = ((1510, 1650), (1160, 1300))
    p.plt1 = []
    for i in range(2):
        p.plt1.append(plt.plot_new(p.panel1, 200, 200, freq[i],
            margin=(25, 25, 25, 25), tick=5, title=tis[i]))
        p.plt1[-1].c.pack(expand=1, fill=BOTH)
    p.panel2 = Frame(p.panel)
    p.plt2 = []
    p.plt2.append(plt.plot_new(p.panel2, 200, 200, (0, 1), (-80, -40), margin,
        title=ti[0], xlabel=labels[0]))
    for i in range(2):
        p.plt2.append(plt.plot_new(p.panel2, 200, 200, (-5, 5), (0, 0.5),
            margin, title=ti[1+i], xlabel=labels[1]))
    p.panel3 = Frame(p.panel)
    p.plt3 = []
    for i in range(4):
        p.plt3.append(plt.plot_new(p.panel3, 200, 200, (0, 1), (-80, -40),
            margin, title=ti[0], xlabel=labels[0]))
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_rfch_select(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_rfch_select(e, p))
    p.box3.bind('<<ComboboxSelected>>', lambda e: on_gain_select(e, p))
    p.box4.bind('<<ComboboxSelected>>', lambda e: on_filt_select(e, p))
    p.box5.bind('<<ComboboxSelected>>', lambda e: on_filt_select(e, p))
    update_rfch_page(p)
    return p

# RF Channels page select callback ---------------------------------------------
def on_rfch_select(e, p):
    update_rfch_page(p)

# LNA Gain select callback -----------------------------------------------------
def on_gain_select(e, p):
    ch = p.box1.get()
    val = p.box3.get()
    if ch == '1-4' or ch == '5-8' or val == '-':
        return
    set_rfch_gain(rcv_body, int(ch), 0 if val == 'Auto' else int(val) + 1)

# IF Filter select callback ----------------------------------------------------
def on_filt_select(e, p):
    ch = p.box1.get()
    val1 = p.box4.get()
    val2 = p.box5.get()
    dev, fmt, fs, fo, IQ, bits = get_rfch_stat(rcv_body, int(ch))
    if ch == '1-4' or ch == '5-8' or val1 == '-' or val2 == '-' or IQ != 2:
        return
    set_rfch_filt(rcv_body, int(ch), float(val2), 0.0, val1 == '3rd')

# update RF Channels page ------------------------------------------------------
def update_rfch_page(p):
    ch = p.box1.get()
    tave = float(p.box2.get())
    if ch == 'ALL':
        p.panel2.pack_forget()
        p.panel3.pack_forget()
        p.panel1.pack(expand=1, fill=BOTH)
        update_freq_plot(p.plt1)
        p.box3.set('-')
        p.box4.set('-')
        p.box5.set('-')
    elif ch == '1-4' or ch == '5-8':
        p.panel1.pack_forget()
        p.panel2.pack_forget()
        p.panel3.pack(side=LEFT, expand=1, fill=BOTH)
        for i in range(4):
            p.plt3[i].c.place(relx=i % 2 * 0.5, rely=i // 2 * 0.5, relwidth=0.5,
                relheight=0.5)
            update_psd_plot(p.plt3[i], i + 1 if ch == '1-4' else i + 5, tave)
        p.box3.set('-')
        p.box4.set('-')
        p.box5.set('-')
    else:
        p.panel1.pack_forget()
        p.panel3.pack_forget()
        p.panel2.pack(side=LEFT, expand=1, fill=BOTH)
        p.plt2[0].c.place(relx=0, rely=0, relwidth=0.65, relheight=1)
        p.plt2[1].c.place(relx=0.65, rely=0, relwidth=0.35, relheight=0.5)
        p.plt2[2].c.place(relx=0.65, rely=0.5, relwidth=0.35, relheight=0.5)
        update_psd_plot(p.plt2[0], int(ch), tave)
        update_hist_plot(p.plt2[1], p.plt2[2], int(ch), tave)
        g = get_rfch_gain(rcv_body, int(ch))
        p.box3.set('-' if g < 0 else 'Auto' if g == 0 else '%d' % (g - 1))
        bw, freq, order = get_rfch_filt(rcv_body, int(ch))
        p.box4.set('-' if bw < 0 else '3rd' if order else '5th')
        p.box5.set('-' if bw < 0 else '%.1f' % (bw))
    dev, fmt, fs, fo, IQ, bits = get_rfch_stat(rcv_body, 1)
    p.txt1.configure(text='F_S: %.6f MHz' % (fs))

# update PSD plot --------------------------------------------------------------
def update_psd_plot(p, ch, tave):
    dev, fmt, fs, fo, IQ, bits = get_rfch_stat(rcv_body, ch)
    psd = get_rfch_psd(rcv_body, ch, tave)
    f = np.linspace(fo, fo + fs / 2, len(psd)) if IQ == 1 else \
        np.linspace(fo - fs / 2, fo + fs / 2, len(psd))
    plt.plot_clear(p)
    plt.plot_xlim(p, [f[0], f[-1]])
    plt.plot_ylim(p, [-80, -45])
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, [fo, fo], p.yl, color=plt.GR_COLOR)
    plt.plot_poly(p, f, psd, color=P1_COLOR)
    plot_filt(p, ch, fo)
    plt.plot_axis(p, gcolor=None)
    xs, ys = plt.plot_scale(p)
    plt.plot_poly(p, [fo, fo], [p.yl[0], p.yl[0] + 6 / ys], color=plt.FG_COLOR)
    plt.plot_poly(p, [fo, fo], [p.yl[1], p.yl[1] - 6 / ys], color=plt.FG_COLOR)
    plot_mark(p, fo, p.yl[0] + 16 / ys, color=plt.FG_COLOR)
    plot_sig_freq(p, p.xl, 16)
    plt.plot_text(p, p.xl[0] + 10 / xs, p.yl[1] - 8 / ys, 'CH%d' % (ch),
        font=get_font(1, 'bold'), anchor=NW)
    plt.plot_text(p, fo + 12 / xs, p.yl[0] + 16 / ys, '%.6f MHz' % (fo),
        anchor=W)
    plt.plot_text(p, p.xl[1] - 10 / xs, p.yl[0] + 16 / ys,
        '%s (%d bits)' % ('I' if IQ == 1 else 'IQ', bits), anchor=E)

# update signal frequency plot -------------------------------------------------
def update_freq_plot(p):
    for i in range(2):
        plt.plot_clear(p[i])
        plt.plot_axis(p[i])
        for ch in range(1, 9):
            dev, fmt, fs, fo, IQ, bits = get_rfch_stat(rcv_body, ch)
            xs, ys = plt.plot_scale(p[i])
            xl = [fo, fo + fs / 2] if IQ == 1 else [fo - fs / 2, fo + fs / 2]
            yl = [p[i].yl[0] + 5 / ys, p[i].yl[1] - 20 / ys]
            plt.plot_rect(p[i], xl[0], yl[0], xl[1], yl[1], color=plt.FG_COLOR)
            plt.plot_text(p[i], xl[0], yl[1], 'CH%d' % (ch),
                font=get_font(1, 'bold'), anchor=SW)
            plt.plot_text(p[i], fo, yl[0] + 6 / ys, '%.3f' % (fo), anchor=S)
            plt.plot_poly(p[i], [fo, fo], [yl[0], yl[0] + 6 / ys], color=plt.FG_COLOR)
            plt.plot_poly(p[i], [fo, fo], [yl[1], yl[1] - 6 / ys], color=plt.FG_COLOR)
            plot_sig_freq(p[i], xl, 35)
        plt.plot_axis(p[i], fcolor=plt.GR_COLOR, gcolor=None)
    for i, sys in enumerate(SYSTEMS[1:]):
        xs, ys = plt.plot_scale(p[0])
        color = sat_color('GREJCIS'[i])
        x, y = p[0].xl[1] - 80 / xs, p[0].yl[1] - (25 + 15 * i) / ys
        plot_mark(p[0], x, y, color)
        plt.plot_text(p[0], x + 10 / xs, y, sys, color=color, anchor=W)

# plot filter ------------------------------------------------------------------
def plot_filt(p, ch, fo):
    bw, freq, order = get_rfch_filt(rcv_body, ch)
    if bw < 0: return
    xs, ys = plt.plot_scale(p)
    x1, x2 = fo + freq - bw * 0.5, fo + freq + bw * 0.5
    y1, y2 = p.yl[0] + 15 / ys, p.yl[0] + 15 / ys + 3
    dx = bw * (0.15 if order else 0.1)
    plt.plot_poly(p, [x1, x1 + dx, x2 - dx, x2], [y1, y2, y2, y1],
        color=P2_COLOR)

# plot signal frequency marks --------------------------------------------------
def plot_sig_freq(p, xl, yp):
    global sig_opt
    xs, ys = plt.plot_scale(p)
    y = p.yl[1] - yp / ys
    for i in range(len(sig_opt.sys)):
        if not sig_opt.sys_sel[i].get(): continue
        color = color=sat_color('GREJCIS'[i])
        for j, sig in enumerate(sig_opt.sig[i]):
            if not sig_opt.sig_sel[i][j].get(): continue
            freq = sdr_code.sig_freq(sig) / 1e6
            if freq < xl[0] or freq > xl[1]: continue
            if sig in ('G1CA', 'G2CA'):
                for fcn in range(-7, 7):
                    f = sdr_func.shift_freq(sig, fcn, freq * 1e6) / 1e6
                    plot_mark(p, f, y, color=color)
                freq = sdr_func.shift_freq(sig, -7, freq * 1e6) / 1e6
            else:
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

# update histograms plot -------------------------------------------------------
def update_hist_plot(p1, p2, ch, tave):
    dev, fmt, fs, fo, IQ, bits = get_rfch_stat(rcv_body, ch)
    val, hist1, hist2 = get_rfch_hist(rcv_body, ch, tave)
    plot_hist(p1, bits, val, hist1)
    plot_hist(p2, bits, val, hist2)

# plot histogram ---------------------------------------------------------------
def plot_hist(p, bits, val, hist):
    xl = (5, 3, 5, 9, 17, 33, 65, 129, 257)
    yl = (0.4, 0.4, 0.4, 0.3, 0.2, 0.15, 0.1, 0.1, 0.1)
    xs, ys = plt.plot_scale(p)
    plt.plot_clear(p)
    plt.plot_axis(p, fcolor=None, tcolor=None)
    w = 6 / xs * 5 / xl[bits]
    for i in range(len(val)):
        plt.plot_rect(p, val[i] - w, 0, val[i] + w, hist[i], fill=P1_COLOR)
    plt.plot_xlim(p, [-xl[bits], xl[bits]])
    plt.plot_ylim(p, [0, yl[bits]])
    plt.plot_axis(p, gcolor=None)
    if len(val) > 0:
        ave = np.sum(val * hist) / len(val)
        std = sqrt(np.sum((val - ave) ** 2 * hist) / len(val))
        x = xl[bits] - 12 / xs
        plt.plot_text(p, x, yl[bits] - 10 / ys, 'Ave: %.2f' % (ave), anchor=NE)
        plt.plot_text(p, x, yl[bits] - 26 / ys, 'Std: %.2f' % (std), anchor=NE)

# generate BB Channels page ----------------------------------------------------
def bbch_page_new(parent):
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.toolbar = tool_bar_new(p.panel)
    ttk.Label(p.toolbar, text='RF CH').pack(side=LEFT, padx=(8, 4))
    p.box1 = sel_box_new(p.toolbar, ['ALL'] + [str(i + 1) for i in range(8)],
        'ALL', 5)
    p.box1.pack(side=LEFT)
    ttk.Label(p.toolbar, text='System').pack(side=LEFT, padx=(8, 4))
    p.box2 = sel_box_new(p.toolbar, SYSTEMS, 'ALL', 8)
    p.box2.pack(side=LEFT)
    ttk.Label(p.toolbar, text='State').pack(side=LEFT, padx=(8, 4))
    p.box3 = sel_box_new(p.toolbar, ['ALL', 'LOCK'], 'LOCK', 6)
    p.box3.pack(side=LEFT)
    sdr_opt.link_label_new(p.toolbar, text='?', link=sdr_opt.SIG_LINK).pack(
        side=LEFT, padx=6)
    p.txt1 = ttk.Label(p.toolbar, width=12)
    p.txt2 = ttk.Label(p.toolbar, width=10, anchor=E)
    p.txt3 = ttk.Label(p.toolbar, width=14, anchor=E)
    p.txt3.pack(side=RIGHT, padx=(2, 15))
    p.txt2.pack(side=RIGHT, padx=2)
    p.txt1.pack(side=RIGHT, padx=2)
    p.tbl1 = ttk.Treeview(p.panel, show=('headings'))
    p.tbl1.pack(expand=1, fill=BOTH)
    p.scl1 = ttk.Scrollbar(p.tbl1, orient=VERTICAL, command=p.tbl1.yview)
    p.scl1.pack(side=RIGHT, fill=Y)
    p.tbl1.configure(yscrollcommand=p.scl1.set)
    p.tbl1.pack(expand=1, fill=BOTH)
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_bbch_sys_select(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_bbch_sys_select(e, p))
    p.tbl1.bind('<<TreeviewSelect>>', lambda e: on_bbch_ch_select(e, p))
    update_bbch_page(p)
    return p

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
    rfch = p.box1.get()
    sys = p.box2.get()
    state = p.box3.get()
    stat = get_ch_stat(rcv_body, sys, all=(state == 'ALL'),
        rfch=(0 if rfch == 'ALL' else int(rfch)))
    w = (40, 22, 36, 52, 32, 68, 38, 70, 82, 62, 84, 44, 48, 36, 34, 32)
    a = 'ecccceeweeeceeee'
    buff, srch, lock = stat[0][72:82], stat[0][82:92], stat[0][92:]
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
    ws = (p.tbl1.winfo_width() - 8) / sum(w)
    for i in range(len(cols)):
        p.tbl1.heading(cols[i], text=cols[i])
        p.tbl1.column(cols[i], width=int(ws * w[i]), anchor=a[i], stretch=0)
    for s in stat[2:]:
        vals = s.split()
        vals[7] = bar_cn0(float(vals[6]), int(ws * w[7]))
        tag = 'idle' if float(vals[5]) == 0.0 else ''
        tag = 'srch' if int(vals[0]) == srch_ch else tag
        p.tbl1.insert('', END, iid=vals[0], values=vals, tags=tag)
    p.tbl1.tag_configure('idle', foreground=P2_COLOR)
    p.tbl1.tag_configure('srch', foreground='blue')

# C/N0 bar ---------------------------------------------------------------------
def bar_cn0(cn0, width):
    bar_max = (width - 6) // tkfont.Font(font=get_font()).measure('|')
    return '|' * int(np.clip(bar_max * (cn0 - 30.0) / 20.0, 1, bar_max))

# generate Correlator page -----------------------------------------------------
def corr_page_new(parent):
    ti = ('I * sign(IP)', 'IP-QP', 'Time (s) - IP/QP')
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.toolbar = tool_bar_new(p.panel)
    ttk.Label(p.toolbar, text='BB CH').pack(side=LEFT, padx=(8, 4))
    p.btn1 = sdr_opt.custom_btn_new(p.toolbar, label=' < ')
    p.btn1.panel.pack(side=LEFT, padx=2, pady=1)
    p.box1 = sel_box_new(p.toolbar, ['1'], '1', 4)
    p.box1.pack(side=LEFT)
    p.btn2 = sdr_opt.custom_btn_new(p.toolbar, label=' > ')
    p.btn2.panel.pack(side=LEFT, padx=2, pady=1)
    p.txt1 = ttk.Label(p.toolbar, foreground=P1_COLOR)
    p.txt1.pack(side=LEFT, expand=1, fill=X, padx=10)
    p.box3 = sel_box_new(p.toolbar, ['0.1', '0.2', '0.3', '0.4', '0.6', '0.8',
        '1.0', '1.5', '2'], '0.4', 3)
    p.box3.pack(side=RIGHT, padx=(1, 10))
    p.box2 = sel_box_new(p.toolbar, ['0.1', '0.2', '0.5', '1', '2', '5', '10'],
        '1', 3)
    p.box2.pack(side=RIGHT, padx=1)
    p.box4 = sel_box_new(p.toolbar, ['I', 'IQ', 'AveIQ'], 'I', 5)
    p.box4.pack(side=RIGHT, padx=1)
    ttk.Label(p.toolbar, text='IQ/Span(s)/Range').pack(side=RIGHT, padx=2)
    p.plt3 = plt.plot_new(p.panel, 800, 245, [0, 1], [-0.6, 0.6], title=ti[2])
    p.plt3.c.pack(side=BOTTOM, expand=1, fill=BOTH)
    panel1 = Frame(p.panel)
    panel1.pack(expand=1, fill=BOTH)
    p.plt2 = plt.plot_new(panel1, 255, 245, [-0.6, 0.6], [-0.6, 0.6], aspect=1,
        title=ti[1])
    p.plt2.c.pack(side=RIGHT, expand=1, fill=BOTH)
    p.plt1 = plt.plot_new(panel1, 545, 245, [0, 1], [-0.2, 0.6], title=ti[0])
    p.plt1.c.pack(side=LEFT, expand=1, fill=BOTH)
    p.btn1.btn.bind('<Button-1>', lambda e: on_corr_ch_down(e, p))
    p.btn2.btn.bind('<Button-1>', lambda e: on_corr_ch_up(e, p))
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_corr_ch_select(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_corr_ch_select(e, p))
    p.box3.bind('<<ComboboxSelected>>', lambda e: on_corr_ch_select(e, p))
    p.box4.bind('<<ComboboxSelected>>', lambda e: on_corr_ch_select(e, p))
    update_corr_page(p)
    update_corr_ch_sel(p)
    return p

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
    state, fs, lock, cn0, coff, fd, npos, pos, C, aveC = get_corr_stat(rcv_body, ch)
    tt, T, P = get_corr_hist(rcv_body, ch, rng[0])
    update_corr_plot1(p.plt1, coff, fs, npos, pos, C, aveC, type, rng[1])
    update_corr_plot2(p.plt2, P, rng[1])
    update_corr_plot3(p.plt3, tt, T, P, rng)
    update_corr_text(p, ch, tt)
    
# update correlator channel selection ------------------------------------------
def update_corr_ch_sel(p):
    chs = [s.split()[0] for s in get_ch_stat(rcv_body, 'ALL', 0, 0.0)[2:]]
    p.box1.configure(values=chs, height=min([len(chs), 32]))
    if len(chs) > 0 and not p.box1.get() in chs:
        p.box1.set(chs[0])
        set_sel_ch(rcv_body, int(chs[0]))
    else:
        set_sel_ch(rcv_body, int(p.box1.get()))

# update correlator text -------------------------------------------------------
def update_corr_text(p, ch, time):
    for s in get_ch_stat(rcv_body, 'ALL', 0, 0.0)[2:]:
        ss = s.split()
        if int(ss[0]) != ch: continue
        text = 'SAT: %s  SIG: %s  PRN: %s  LOCK: %s s' % (ss[2], ss[3], ss[4],
            ss[5])
        p.txt1.configure(text=text)
        xs, ys = plt.plot_scale(p.plt3)
        text = ('C/N0: %s dB-Hz  COFF: %s ms  DOP: %s Hz  ADR: %s cyc  SYNC: %s' +
            '  #NAV: %s') % (ss[6], ss[8], ss[9], ss[10], ss[11], ss[12])
        plt.plot_text(p.plt3, p.plt3.xl[0] + 12 / xs, p.plt3.yl[1] - 15 / ys,
            text, anchor=W)
        return

# update correlator plot 1 -----------------------------------------------------
def update_corr_plot1(p, coff, fs, npos, pos, C, aveC, type, rng):
    x = [coff + pos[i] / fs * 1e3 for i in range(len(pos))]
    C *= np.sign(C[0])
    plt.plot_clear(p)
    plt.plot_xlim(p, [x[npos], x[-1]])
    plt.plot_ylim(p, [-rng * 0.3 if type == 'I' else 0, rng])
    xs, ys = plt.plot_scale(p)
    p.title = 'I * sign(IP)' if type == 'I' else '\u221A (I\xb2+Q\xb2)' \
        if type == 'IQ' else '\u221A \u03A3 (I\xb2+Q\xb2)'
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, [coff, coff], p.yl, color='grey')
    plt.plot_poly(p, p.xl, [0, 0], color='grey')
    plt.plot_dots(p, [coff], [0], color=plt.FG_COLOR, size=3)
    y = C.real if type == 'I' else abs(C) if type == 'IQ' else np.sqrt(aveC)
    plt.plot_poly(p, x[npos:], y[npos:], color=P2_COLOR)
    plt.plot_dots(p, x[npos:], y[npos:], color=P2_COLOR, fill=P1_COLOR, size=3)
    plt.plot_dots(p, x[:npos], y[:npos], color=P1_COLOR, fill=P1_COLOR, size=9)
    plt.plot_axis(p, gcolor=None)
    plt.plot_text(p, p.xl[1] - 18 / xs, p.yl[0] + 10 / ys, 'COFF (ms)',
        anchor=SE)
    if type == 'AveIQ':
        plt.plot_text(p, p.xl[1] - 18 / xs, p.yl[1] - 10 / ys,
            'Integ = %s s' % (sys_opt.t_dll.get()), anchor=NE)

# update correlator plot 2 -----------------------------------------------------
def update_corr_plot2(p, P, rng):
    plt.plot_clear(p)
    plt.plot_xlim(p, [-rng, rng])
    plt.plot_ylim(p, [-rng, rng])
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, [0, 0], p.yl, color='grey')
    plt.plot_poly(p, p.xl, [0, 0], color='grey')
    plt.plot_dots(p, [0], [0], color=plt.FG_COLOR, size=3)
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

# generate Satellites page -----------------------------------------------------
def sats_page_new(parent):
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.toolbar = tool_bar_new(p.panel)
    ttk.Label(p.toolbar, text='System').pack(side=LEFT, padx=(8, 4))
    p.box1 = sel_box_new(p.toolbar, SYSTEMS, 'ALL', 8)
    p.box1.pack(side=LEFT)
    p.txt1 = ttk.Label(p.toolbar, foreground=P1_COLOR)
    p.txt1.pack(side=RIGHT, padx=10)
    p.tbl1 = ttk.Treeview(p.panel, show=('headings'))
    p.tbl1.pack(expand=1, fill=BOTH)
    p.scl1 = ttk.Scrollbar(p.tbl1, orient=VERTICAL, command=p.tbl1.yview)
    p.scl1.pack(side=RIGHT, fill=Y)
    p.scl2 = ttk.Scrollbar(p.tbl1, orient=HORIZONTAL, command=p.tbl1.xview)
    p.scl2.pack(side=BOTTOM, fill=X)
    p.tbl1.configure(yscrollcommand=p.scl1.set, xscrollcommand=p.scl2.set)
    p.tbl1.pack(expand=1, fill=BOTH)
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_sats_sys_select(e, p))
    update_sats_page(p)
    return p

# Satellites page system select callback ---------------------------------------
def on_sats_sys_select(e, p):
    update_sats_page(p)

# update Satellites page -------------------------------------------------------
def update_sats_page(p):
    cols = ('SAT', 'FCN', 'PVT', 'OBS', 'EPH', 'SVH', 'AZ(\xb0)', 'EL(\xb0)',
        'SIG1', 'C/N0', 'SIG2', 'C/N0', 'SIG3', 'C/N0', 'SIG4', 'C/N0', 'SIG5',
        'C/N0', 'SIG6', 'C/N0', 'SIG7', 'C/N0', 'SIG8', 'C/N0', 'SIG9', 'C/N0',
        'SIGA', 'C/N0', 'SIGB', 'C/N0', 'SIGC', 'C/N0')
    sys = p.box1.get()
    sats, sat, sig, cn0, prn = get_sig_stat(rcv_body, sys)
    az, el, pvt, obs, eph, svh, fcn = get_sat_stat(rcv_body, sats)
    w = (32, 28, 30, 30, 30, 32, 46, 42, 46, 39, 46, 39, 46, 39, 46, 39, 46,
        39, 46, 39, 46, 39, 46, 39, 46, 39, 46, 39, 46, 39, 46, 39)
    a = 'cccccceecccccccccccccccccccccccc'
    for c in p.tbl1.get_children():
       p.tbl1.delete(c)
    p.tbl1.configure(columns=cols)
    for i in range(len(cols)):
        p.tbl1.heading(i, text=cols[i])
        p.tbl1.column(i, width=w[i], anchor=a[i], stretch=0)
    for i, s in enumerate(sats):
        vals = [s, '%+d' % (fcn[i]) if s[0] == 'R' else '-',
            'OK' if pvt[i] else '-', 'OK' if obs[i] else '-',
            'OK' if eph[i] else '-', '%02X' % svh[i] if eph[i] else '-',
            '%.1f' % (az[i]), '%.1f' % (el[i])]
        for j, ss in enumerate(sat):
            if ss == s and len(vals) <= len(cols):
                vals.append(sig[j])
                vals.append('%.1f' % (cn0[j]))
        while len(vals) <= len(cols):
            vals.append('-')
        tag = '' if pvt[i] else 'unhealthy' if svh[i] else 'no_pvt'
        p.tbl1.insert('', END, iid=s, values=vals, tags=tag)
    p.tbl1.tag_configure('no_pvt', foreground=P2_COLOR)
    p.tbl1.tag_configure('unhealthy', foreground='orange')
    text = '# Sats Used/Sats Tracked/Signals:  %2d/%2d/%2d' % (pvt.count(1),
        len(sats), len(sat))
    p.txt1.configure(text=text)

# generate Solution page -------------------------------------------------------
def sol_page_new(parent):
    ranges = ['0.1', '0.2', '0.5', '1', '2', '5', '10', '20', '50']
    tspans = ['60', '120', '300', '900', '1800', '3600']
    w = (135, 105, 105, 80)
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.toolbar = tool_bar_new(p.panel)
    p.box1 = sel_box_new(p.toolbar, ranges, '10', width=4)
    p.box1.pack(side=RIGHT, padx=(0, 15))
    ttk.Label(p.toolbar, text='Range(m)').pack(side=RIGHT, padx=(6, 2))
    p.box2 = sel_box_new(p.toolbar, tspans, '300', width=4)
    p.box2.pack(side=RIGHT)
    ttk.Label(p.toolbar, text='Span(s)').pack(side=RIGHT, padx=(6, 2))
    p.btn1 = ttk.Button(p.toolbar, width=7, text='Clear')
    p.btn1.pack(side=RIGHT, padx=(0, 2))
    p.btn2 = ttk.Button(p.toolbar, width=7, text='Ref Pos')
    p.btn2.pack(side=RIGHT, padx=1)
    ttk.Label(p.toolbar, text='Type').pack(side=LEFT, padx=(8, 4))
    p.box3 = sel_box_new(p.toolbar, ['Pos ENU', 'Pos Horiz'], 'Pos ENU', width=8)
    p.box3.pack(side=LEFT)
    p.panel1 = Frame(p.panel)
    p.plt = []
    for i in range(4):
        panel = Frame(p.panel1)
        panel.pack(expand=1, fill=BOTH, padx=0, pady=0)
        p.plt.append(plt.plot_new(panel, 200, 100 if i in (1, 2) else 115,
            tick=11 if i < 3 else 15, margin=(35, 28, 23 if i < 1 else 8,
            8 if i < 3 else 23), taxis=1))
        p.plt[-1].c.pack(expand=1, fill=BOTH)
    p.panel2 = Frame(p.panel)
    p.plt.append(plt.plot_new(p.panel2, 200, 200, margin=(45, 25, 25, 35),
        xlabel='Pos E (m)', ylabel='Pos N (m)'))
    p.plt[-1].c.pack(expand=1, fill=BOTH)
    p.btn1.bind('<Button-1>', lambda e: on_sol_clear_push(e, p))
    p.btn2.bind('<Button-1>', lambda e: on_sol_ref_push(e, p))
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_sol_change(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_sol_change(e, p))
    p.box3.bind('<<ComboboxSelected>>', lambda e: on_sol_change(e, p))
    p.ref = []
    update_sol_page(p)
    return p

# solution plot change callback ------------------------------------------------
def on_sol_change(e, p):
    update_sol_page(p)

# solution clear button push callback ------------------------------------------
def on_sol_clear_push(e, p):
    global sol_log
    p.ref = []
    sol_log = []
    update_sol_page(p)

# solution ref button push callback --------------------------------------------
def on_sol_ref_push(e, p):
    label = ('Latitude (\xb0)', 'Longitude (\xb0)', 'Altitude (m)')
    dlg = sdr_opt.modal_dlg_new(root, 260, 150, 'Ref Pos')
    pos = sdr_rtk.ecef2pos(p.ref) if len(p.ref) else [0, 0, 0] 
    var = [StringVar() for i in range(3)]
    for i in range(3):
        var[i].set('%.8f' % (pos[i] / D2R) if i < 2 else '%.3f' % (pos[i]))
        sdr_opt.inp_panel_new(dlg.panel, label[i], var[i], width=14, pwidth=180)
    root.wait_window(dlg.win)
    pos = [to_float(var[i].get()) * (D2R if i < 2 else 1) for i in range(3)]
    if dlg.ok and np.any(pos):
        p.ref = sdr_rtk.pos2ecef(pos)
        update_sol_page(p)

# update Solution page ---------------------------------------------------------
def update_sol_page(p):
    global sol_log
    time, enu, nsat = [], [[], [], []], [[], []]
    if len(sol_log) > 0:
        time = timeval([log[0] for log in sol_log])
        enu, p.ref = pos2enu([log[1] for log in sol_log], p.ref)
        nsat = np.transpose([log[2] for log in sol_log])
    ymax = float(p.box1.get())
    tspan = float(p.box2.get())
    if p.box3.get() == 'Pos ENU':
        p.panel2.pack_forget()
        p.panel1.pack(expand=1, fill=BOTH)
        plot_pos_enu(p, time, enu, nsat, ymax, tspan)
        show_sol(p.plt[0])
    else:
        p.panel1.pack_forget()
        p.panel2.pack(expand=1, fill=BOTH)
        plot_pos_hori(p, time, enu, ymax, tspan)
        show_sol(p.plt[4])

# time value -------------------------------------------------------------------
def timeval(time):
    if len(time) <= 0: return []
    ep = sdr_rtk.time2epoch(time[0])
    ep[3:] = 0
    t0 = sdr_rtk.epoch2time(ep)
    return np.array([sdr_rtk.timediff(t, t0) for t in time])

# show solution ----------------------------------------------------------------
def show_sol(ax):
    sol = get_rcv_pvt_sol(rcv_body).split()
    if len(sol) >= 7:
        tstr = sol[0].replace('/', '-')
        text = '%s %s GPST  %12s\xb0 %13s\xb0 %9sm  %6s  %4s' % (tstr, sol[1],
            sol[2], sol[3], sol[4], sol[5], sol[6])
        xs, ys = plt.plot_scale(ax)
        x, y = (ax.xl[0] + ax.xl[1]) / 2, ax.yl[1] + 4 / ys
        color = P1_COLOR if sol[6] == 'FIX' else P2_COLOR
        plt.plot_text(ax, x, y, text, anchor=S, color=color)

# plot ENU position and NSAT ---------------------------------------------------
def plot_pos_enu(p, time, enu, nsat, ymax, tspan):
    label = ['Pos E (m)', 'Pos N (m)', 'Pos U (m)', '# Sats']
    t = 0 if len(time) <= 0 else time[-1]
    xl = [t - tspan, t]
    for i in range(4):
        yl = [-ymax, ymax] if i < 3 else [0, 80]
        ax = p.plt[i]
        plt.plot_clear(ax)
        if len(time) > 0:
            yl = [enu[i][-1] + yl[0], enu[i][-1] + yl[1]] if i < 3 else yl
        plt.plot_xlim(ax, xl)
        plt.plot_ylim(ax, yl)
        plt.plot_axis(ax)
        plt.plot_poly(ax, xl, [0, 0], P3_COLOR)
        if len(time) > 0:
            j = np.min(np.where(time >= xl[0]))
            if i < 3:
                plt.plot_poly(ax, time[j:], enu[i][j:], plt.GR_COLOR)
                plt.plot_dots(ax, time[j:], enu[i][j:], P1_COLOR, size=2)
            else:
                plt.plot_poly(ax, time[j:], nsat[1][j:], P2_COLOR)
                plt.plot_poly(ax, time[j:], nsat[0][j:], P1_COLOR)
        plt.plot_axis(ax, gcolor=None)
        if len(time) > 0:
            if i < 3:
                plt.plot_dots(ax, time[-1:], enu[i][-1:], P1_COLOR, size=9)
            else:
                plt.plot_dots(ax, time[-1:], nsat[1][-1:], P2_COLOR, size=9)
                plt.plot_dots(ax, time[-1:], nsat[0][-1:], P1_COLOR, size=9)
        xs, ys = plt.plot_scale(ax)
        plt.plot_text(ax, ax.xl[0] + 10 / xs, ax.yl[1] - 8 / ys, label[i],
            font=get_font(), anchor=NW, color=P1_COLOR)

# plot horizontal position -----------------------------------------------------
def plot_pos_hori(p, time, enu, ymax, tspan):
    yl = [-ymax, ymax]
    w, h = p.panel2.winfo_width(), p.panel2.winfo_height()
    xl = [yl[0] * w / h, yl[1] * w / h]
    ax = p.plt[4]
    plt.plot_clear(ax)
    if len(time) > 0:
        xl = [enu[0][-1] + xl[0], enu[0][-1] + xl[1]]
        yl = [enu[1][-1] + yl[0], enu[1][-1] + yl[1]]
    plt.plot_xlim(ax, xl)
    plt.plot_ylim(ax, yl)
    plt.plot_axis(ax)
    plt.plot_poly(ax, xl, [0, 0], P3_COLOR)
    plt.plot_poly(ax, [0, 0], yl, P3_COLOR)
    plt.plot_dots(ax, [0], [0], P3_COLOR)
    if len(time) > 0:
        j = np.min(np.where(time >= time[-1] - tspan))
        plt.plot_poly(ax, enu[0][j:], enu[1][j:], plt.GR_COLOR)
        plt.plot_dots(ax, enu[0][j:], enu[1][j:], P2_COLOR, size=2)
        plt.plot_dots(ax, enu[0][-1:], enu[1][-1:], plt.BG_COLOR, size=11)
        plt.plot_dots(ax, enu[0][-1:], enu[1][-1:], P1_COLOR, size=9)
    plt.plot_axis(ax, gcolor=None)

# position to enu --------------------------------------------------------------
def pos2enu(log, ref):
    ecef = [sdr_rtk.pos2ecef(p) for p in log]
    if len(ref) <= 0:
        ref = ecef[0]
    pos = sdr_rtk.ecef2pos(ref)
    return np.transpose([sdr_rtk.ecef2enu(pos, r - ref) for r in ecef]), ref

# generate Log page ------------------------------------------------------------
def log_page_new(parent):
    filts = ('', '$TIME', '$CH', '$NAV', '$OBS', '$POS', '$SAT', '$EPH', '$LOG')
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.toolbar = tool_bar_new(p.panel)
    ttk.Label(p.toolbar, text='Filter').pack(side=LEFT, padx=(8, 4))
    p.box1 = sel_box_new(p.toolbar, filts, '', 7)
    p.box1.pack(side=LEFT)
    p.box2 = ttk.Entry(p.toolbar, width=16, font=get_font())
    p.box2.pack(side=LEFT, padx=2)
    p.btn1 = ttk.Button(p.toolbar, width=8, text='Pause')
    p.btn2 = ttk.Button(p.toolbar, width=8, text='Clear')
    p.btn2.pack(side=RIGHT, padx=(0, 18))
    p.btn1.pack(side=RIGHT)
    p.txt1 = ttk.Label(p.toolbar)
    p.txt1.pack(side=LEFT, expand=1, fill=X)
    panel = Frame(p.panel)
    panel.pack(expand=1, fill=X)
    p.stxt = scrolledtext.ScrolledText(panel, width=600, height=200, wrap=NONE,
        font=get_font(mono=1), padx=4, pady=2)
    p.stxt.pack(expand=1, fill=BOTH)
    p.btn1.bind('<Button-1>', lambda e: on_log_pause_push(e, p))
    p.btn2.bind('<Button-1>', lambda e: on_log_clear_push(e, p))
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_log_filt_change(e, p))
    p.box2.bind('<KeyRelease>', lambda e: on_log_filt_change(e, p))
    update_log_page(p)
    return p

# Log page button pause push callback ------------------------------------------
def on_log_pause_push(e, p):
    p.btn1.configure(text='Pause' if p.btn1['text'] == 'Resume' else 'Resume')

# Log page button clear push callback ------------------------------------------
def on_log_clear_push(e, p):
    global rcv_log
    rcv_log = []
    show_log_page(p)

# Log page filter change callback ----------------------------------------------
def on_log_filt_change(e, p):
    global rcv_log_filt
    rcv_log_filt = p.box1.get() + ' ' + p.box2.get()

# update Log page --------------------------------------------------------------
def update_log_page(p):
    if p.btn1['text'] != 'Resume':
        show_log_page(p)

# show Log page ----------------------------------------------------------------
def show_log_page(p):
    global rcv_log
    p.stxt.delete('1.0', END)
    p.stxt.insert(END, '\n'.join(rcv_log))
    p.stxt.see(END)
    p.stxt.xview_moveto(0.0) # prevent x-scroll

# Start button push callback ---------------------------------------------------
def on_btn_start_push(bar):
    global rcv_body
    if not rcv_body:
        status_bar_show('')
        if inp_opt.inp.get() == 0:
            info = ' (bus/port=%s, conf=%s)' % (inp_opt.dev.get(), \
                inp_opt.conf_path.get() if inp_opt.conf_ena.get() else '')
        else:
            info = ' (path=%s, toff=%s, tscale=%s)' % (inp_opt.str_path.get(),
                inp_opt.toff.get(), inp_opt.tscale.get())
        rcv_body = rcv_open(sys_opt, inp_opt, out_opt, sig_opt)
        if rcv_body == None:
            status_bar_show('Receiver start error.' + info)
            return
        status_bar_show('Receiver started.' + info)
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
        status_bar_show('Receiver stopped.')

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
    update_sol_log()
    ti = pages_update(note, pages)
    if not rcv_body: ti = UD_CYCLE3
    ts = (int)((time.time() - tt) * 1e3)
    note.after(ti - ts if ti > ts else 1, lambda: on_pages_timer(note, pages))

# root resize callback ---------------------------------------------------------
def on_root_resize(e):
    global root_resize
    if root_resize == 0:
        root_resize = 1
        root.after(100, on_root_end_resize)

# root resize end callback -----------------------------------------------------
def on_root_end_resize():
    global root_resize
    root_resize = 0

# update page ------------------------------------------------------------------
def update_page(i, page):
    global root_resize
    if root_resize:
        pass
    elif i == 0:
        update_rcv_page(page)
    elif i == 1:
        update_rfch_page(page)
    elif i == 2:
        update_bbch_page(page)
    elif i == 3:
        update_corr_page(page)
    elif i == 4:
        update_sats_page(page)
    elif i == 5:
        update_sol_page(page)
    elif i == 6:
        update_log_page(page)

# update pages -----------------------------------------------------------------
def pages_update(note, pages):
    i = note.index('current')
    update_page(i, pages[i])
    if i != 3:
        set_sel_ch(rcv_body, 0)
    text = 'Time: ' + get_rcv_stat(rcv_body).split()[0] + ' s'
    stat_bar.msg2.configure(text=text)
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
    style.configure('TNotebook.Tab', font=get_font(1), padding=(20, 2))
    style.map('TNotebook.Tab', background=[('selected', BG_COLOR1)])
    style.configure('TCombobox', font=get_font(), background=BG_COLOR1)
    style.configure('link.TLabel', font=get_font(1), foreground='blue')

# main -------------------------------------------------------------------------
if __name__ == '__main__':
    
    # generate root window
    root = Tk()
    root.geometry('%dx%d' % (WIDTH, HEIGHT))
    root.minsize(WIDTH * 3 // 4, HEIGHT * 3 // 4)
    name, ver = get_name_ver()
    root.title(name + ' ver.' + ver)
    root.protocol("WM_DELETE_WINDOW", on_root_close)
    root.bind("<Configure>", on_root_resize)
    
    # set styles
    set_styles()
    
    # load options
    plt.set_font(get_font())
    sdr_opt.set_bgcolor(BG_COLOR2)
    sdr_opt.set_font(get_font())
    inp_opt = sdr_opt.inp_opt_new()
    out_opt = sdr_opt.out_opt_new()
    sig_opt = sdr_opt.sig_opt_new()
    sys_opt = sdr_opt.sys_opt_new()
    sdr_opt.load_opts(OPTS_FILE, inp_opt, out_opt, sig_opt, sys_opt)
    
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
    labels = ('Receiver', 'RF CH', 'BB CH', 'Correlator', 'Satellites',
        'Solution', 'Log')
    note = ttk.Notebook(root, padding=0)
    note.pack(fill=BOTH)
    pages = (rcv_page_new(note), rfch_page_new(note),  bbch_page_new(note),
        corr_page_new(note), sats_page_new(note), sol_page_new(note),
        log_page_new(note))
    for i, page in enumerate(pages):
        note.add(page.panel, text=labels[i])
    note.after(100, lambda: on_pages_timer(note, pages))
    
    # main loop of Tk
    root.mainloop()
