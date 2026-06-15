#!/usr/bin/env python3
#
#  Pocket SDR Python AP - GNSS SDR Receiver
#
#  Author:
#  T.TAKASU
#
#  History:
#  2024-06-29  1.0  ver.0.13
#  2025-03-19  1.1  ver.0.14
#  2026-05-02  1.2  ver.0.15
#  2026-06-01  1.3  ver.0.16
#
import sys, os, platform, time, re, shutil
from collections import deque
from math import *
from ctypes import *
import numpy as np
from numpy import ctypeslib
from tkinter import *
from tkinter import ttk
from tkinter import scrolledtext
from tkinter import filedialog
import tkinter.font as tkfont
import sdr_func, sdr_code, sdr_opt, sdr_rtk
import sdr_plot as plt

# constants --------------------------------------------------------------------
TITLE      = 'An Open Source GNSS SDR\n(Software Defined Receiver)'
AP_URL     = 'https://github.com/tomojitakasu/PocketSDR'
AP_DIR     = os.path.dirname(__file__)
COPYRIGHT  = 'Copyright (C) 2021-2026 T.Takasu\nAll rights reserved.'
OPTS_FILE  = AP_DIR + '/pocket_sdr.ini' # options file
CALIB_FILE = AP_DIR + '/array_calib.txt' # array calibration save file
HELP_LINK  = 'file://' + AP_DIR + '/../doc/pocket_sdr_help.pdf'
WIDTH      = 800             # root window width
HEIGHT     = 600             # root window height
TB_HEIGHT  = 25              # toolbar height
SB_HEIGHT  = 20              # status bar height
ROW_HEIGHT = 15              # table row height
BG_COLOR1  = '#F8F8F8'       # background color 1
BG_COLOR2  = BG_COLOR1       # background color 2
P1_COLOR   = '#003020'       # plot color 1
P2_COLOR   = '#888844'       # plot color 2
P3_COLOR   = '#BBBBBB'       # plot color 3
WARN_COLOR = '#FF4000'       # warning color
FONT_SIZE  = (9, 9)          # font size (sans, mono)
SDR_N_CORR = (6+101)         # number of correlators
SDR_N_HIST = 5000            # number of correlator history
SDR_N_PSD  = 2048            # number FFT points for PSD
MAX_RCVLOG = 2000            # max receiver logs
MAX_SOLLOG = 3600            # max solution logs
MAX_RFCH   = 8               # max number of RF CH
MAX_ARCH   = 8               # max number of array CH
GAIN_OVL_M = 100             # array gain overlay grid size (M x M)
UD_CYCLE1  = 50              # update cycle (ms) RF channels/Correlator pages
UD_CYCLE2  = 200             # update cycle (ms) other pages
UD_CYCLE3  = 1000            # update cycle (ms) receiver stopped
LOG_BUFF_SIZE = 262144       # receiver log buffer size
CLIGHT = 299792458.0
D2R = np.pi / 180
SYSTEMS = ('ALL', 'GPS', 'GLONASS', 'Galileo', 'QZSS', 'BeiDou', 'NavIC', 'SBAS')

# setup SoapySDR paths ---------------------------------------------------------
def setup_soapy_paths(soapy_dir, win32=False):
    bin_dir = os.path.join(soapy_dir, 'bin')
    mod_dir = os.path.join(soapy_dir, 'lib', 'SoapySDR', 'modules0.8')
    
    if win32:
        if os.path.isdir(bin_dir) and hasattr(os, 'add_dll_directory'):
            os.add_dll_directory(bin_dir)
    elif os.path.isdir(bin_dir):
        os.environ['PATH'] = bin_dir + os.pathsep + os.environ.get('PATH', '')
    
    if os.path.isdir(mod_dir):
        os.environ['SOAPY_SDR_PLUGIN_PATH'] = mod_dir

# platform dependent settings --------------------------------------------------
env = platform.platform()
if 'Windows' in env:
    #LIBSDR = AP_DIR + '/../lib/win32_msvc/libsdr.dll' # MSVC DLL
    LIBSDR = AP_DIR + '/../lib/win32/libsdr.so' # UCRT64 DLL
    FONT = ('Tahoma', 'Consolas')
    
    # setup SoapySDR paths (radioconda)
    soapy_dir = os.path.expandvars(r'%USERPROFILE%\radioconda\Library')
    setup_soapy_paths(soapy_dir, win32=True)

elif 'macOS' in env:
    LIBSDR = AP_DIR + '/../lib/macos/libsdr.so'
    #FONT = ('Arial Narrow', 'Monaco')
    FONT = ('Tahoma', 'Monaco')
    
    # setup SoapySDR paths (radioconda)
    soapy_dir = os.path.expanduser('~/radioconda')
    setup_soapy_paths(soapy_dir)
else: # Linux or Raspberry Pi OS
    LIBSDR = AP_DIR + '/../lib/linux/libsdr.so'
    FONT = ('DejaVu Sans', 'DejaVu Sans Mono')
    #FONT = ('Noto Sans', 'Noto Sans Mono')
    #FONT = ('Ubuntu', 'Ubuntu Mono')

# load external library
try:
    libsdr = cdll.LoadLibrary(LIBSDR)
except OSError as e:
    print('libsdr load error: ' + LIBSDR)
    print(str(e))
    exit(-1)

# global variables -------------------------------------------------------------
rcv_body = None
rcv_page = None
array_page = None
sol_log = deque(maxlen=MAX_SOLLOG)
rcv_log = deque(maxlen=MAX_RCVLOG)
rcv_log_filt = ''
rcv_log_buff = create_string_buffer(LOG_BUFF_SIZE)
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

# set state of all widgets in panel --------------------------------------------
def config_panel_state(p, state):
    for c in p.winfo_children():
        if c.winfo_class() == 'Frame':
            config_panel_state(c, state)
        else:
            c.configure(state=state)

# start receiver ---------------------------------------------------------------
def rcv_open(sys_opt, inp_opt, out_opt, sig_opt, array_opt):
    set_rcv_opts(sys_opt)
    set_log_mask(out_opt)
    libsdr.sdr_func_init.argtypes = (c_char_p,)
    libsdr.sdr_func_init(sys_opt.fftw_wisdom_path.get().encode())
    if inp_opt.inp.get() == 1:
        return rcv_open_file(sys_opt, inp_opt, out_opt, sig_opt, array_opt)
    elif inp_opt.type.get() == 'Pocket SDR FE':
        return rcv_open_dev(sys_opt, inp_opt, out_opt, sig_opt, array_opt)
    else:
        return rcv_open_sdev(sys_opt, inp_opt, out_opt, sig_opt, array_opt)

# build -LPF=... option string -------------------------------------------------
def lpf_opt_str(inp_opt):
    parts = []
    for i in range(len(inp_opt.lpf_bw)):
        bw = to_float(inp_opt.lpf_bw[i].get())
        if bw > 0.0:
            parts.append('%d:%.4f' % (i + 1, bw / 2.0))
    return '-LPF=' + ','.join(parts) if parts else ''

# build receiver option string -------------------------------------------------
def rcv_opt_str(sys_opt, inp_opt, sig_opt, array_opt):
    opt = ''
    opt += ' ' + sys_opt.rcv_options.get()
    opt += ' ' + inp_opt.dev_opt.get()
    opt += ' ' + lpf_opt_str(inp_opt)
    opt += ' ' + '-RFCH ' + sig_opt.sig_rfch.get()
    if sys_opt.acq_mode.get() == 'Fast-Search':
        opt += ' -FAST_SRCH'
    if out_opt.array_sep.get():
        opt += ' -ARRAY'
    narch = to_int(array_opt.no_array.get())
    if narch > 0:
        opt += ' -ARCH=%d' % (narch)
    return opt

# start receiver by Pocket SDR FE ----------------------------------------------
def rcv_open_dev(sys_opt, inp_opt, out_opt, sig_opt, array_opt):
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
        c_int32, c_int32, c_int32, c_char_p, POINTER(c_char_p), c_char_p]
    libsdr.sdr_rcv_open_dev.restype = c_void_p
    info = '(bus/port=%d/%d, conf=%s)' % (bus, port, conf_file)
    opt = rcv_opt_str(sys_opt, inp_opt, sig_opt, array_opt)
    return libsdr.sdr_rcv_open_dev(c_sigs, c_prns, len(sigs), bus, port,
        conf_file.encode(), c_paths, opt.encode()), info

# start receiver by SoapySDR device --------------------------------------------
def rcv_open_sdev(sys_opt, inp_opt, out_opt, sig_opt, array_opt):
    sigs, prns = get_sig_opt(sig_opt)
    fmt = inp_opt.fmts.index(inp_opt.fmt.get()) + 1
    driver = re.findall('\\((.*)\\)', inp_opt.type.get())[0]
    rate = to_float(inp_opt.fs.get()) * 1e6
    freq = to_float(inp_opt.fo[0].get()) * 1e6
    paths = [out_opt.path[i].get() if out_opt.path_ena[i].get() else ''
        for i in range(4)]
    c_sigs = (c_char_p * len(sigs))(*[s.encode() for s in sigs])
    c_prns = (c_int32 * len(sigs))(*prns)
    c_paths = (c_char_p * 4)(*[s.encode() for s in paths])
    libsdr.sdr_rcv_open_sdev.argtypes = [POINTER(c_char_p), POINTER(c_int32),
        c_int32, c_char_p, c_int32, c_double, c_double, POINTER(c_char_p),
        c_char_p]
    libsdr.sdr_rcv_open_sdev.restype = c_void_p
    info = '(driver=%s, rate=%.3fMsps, freq=%.3fMHz)' % (
        driver, rate * 1e-6, freq * 1e-6)
    opt = rcv_opt_str(sys_opt, inp_opt, sig_opt, array_opt)
    return libsdr.sdr_rcv_open_sdev(c_sigs, c_prns, len(sigs), driver.encode(),
        fmt, rate, freq, c_paths, opt.encode()), info

# start receiver by file -------------------------------------------------------
def rcv_open_file(sys_opt, inp_opt, out_opt, sig_opt, array_opt):
    sigs, prns = get_sig_opt(sig_opt)
    fmt = inp_opt.fmts.index(inp_opt.fmt.get()) + 1
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
    libsdr.sdr_rcv_open_file.argtypes = (POINTER(c_char_p), POINTER(c_int32),
        c_int32, c_int32, c_double, POINTER(c_double), POINTER(c_int32),
        POINTER(c_int32), c_double, c_double, c_char_p, POINTER(c_char_p),
        c_char_p)
    libsdr.sdr_rcv_open_file.restype = c_void_p
    info = '(path=%s, toff=%s, tscale=%s)' % (inp_opt.str_path.get(),
        inp_opt.toff.get(), inp_opt.tscale.get())
    opt = rcv_opt_str(sys_opt, inp_opt, sig_opt, array_opt)
    return libsdr.sdr_rcv_open_file(c_sigs, c_prns, len(sigs), fmt, fs, c_fo,
        c_IQ, c_bits, toff, tscale, path.encode(), c_paths, opt.encode()), info

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
    libsdr.sdr_rcv_setopt('thres_pli'.encode(), float(sys_opt.thres_pli.get()))
    libsdr.sdr_rcv_setopt('lost_th'.encode()  , float(sys_opt.lost_th.get()))
    libsdr.sdr_rcv_setopt('bump_jump'.encode(), float(sys_opt.bump_jump.get() == 'ON'))
    libsdr.sdr_rcv_setopt('max_acq'.encode(), float(sys_opt.max_acq.get()))

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
    size = 2048
    buff = create_string_buffer(size)
    libsdr.sdr_rcv_rcv_stat.argtypes = (c_void_p, c_char_p, c_int32)
    libsdr.sdr_rcv_rcv_stat(rcv, buff, size)
    return buff.value.decode()

# get receiver stream status ---------------------------------------------------
def get_str_stat(rcv):
    stat = (c_int32 * 4)()
    libsdr.sdr_rcv_str_stat.argtypes = (c_void_p, POINTER(c_int32))
    libsdr.sdr_rcv_str_stat(rcv, stat)
    return stat

# get receiver channel status --------------------------------------------------
def get_ch_stat(rcv, sys, chno=0, min_lock=2.0, rfch=0, opt=0):
    size = 128 * 1500
    buff = create_string_buffer(size)
    libsdr.sdr_rcv_ch_stat.argtypes = (c_void_p, c_char_p, c_int32, c_double,
        c_int32, c_int32, c_char_p, c_int32)
    libsdr.sdr_rcv_ch_stat(rcv, sys.encode(), chno, min_lock, rfch, opt, buff,
        size)
    return buff.value.decode().splitlines()

# sat id to number -------------------------------------------------------------
def sat2no(sat):
    sys = 'GREJCIS'.find(sat[0])
    return sys * 100 + int(sat[1:]) if sys >= 0 else -1

# get signal status ------------------------------------------------------------
def get_sig_stat(rcv, sys, sort=0, rfch=0):
    stat = get_ch_stat(rcv, sys, rfch=rfch)[2:]
    sig_stat = []
    for i, s in enumerate(stat):
        ss = s.split()
        try:
            sig_stat.append([sat2no(ss[2]), -float(ss[6]) if sort else i, ss[2],
                ss[3], float(ss[6]), int(ss[4])])
        except:
            continue
    sig_stat = sorted(sig_stat)
    sat = [s[2] for s in sig_stat]
    sig = [s[3] for s in sig_stat]
    cn0 = [s[4] for s in sig_stat]
    prn = [s[5] for s in sig_stat]
    return sorted(set(sat), key=sat.index), sat, sig, cn0, prn

# get satellite status ---------------------------------------------------------
def get_sat_stat(rcv, sats):
    size = 1024
    libsdr.sdr_rcv_sat_stat.argtypes = (c_void_p, c_char_p, c_char_p, c_int32)
    libsdr.sdr_rcv_sat_stat.restype = c_int32
    buff = create_string_buffer(size)
    az, el, pvt, obs, eph, svh, fcn = [], [], [], [], [], [], []
    for sat in sats:
        libsdr.sdr_rcv_sat_stat(rcv, sat.encode(), buff, size)
        stat = buff.value.decode().split()
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
    stat = np.zeros(8, dtype='float64')
    libsdr.sdr_rcv_rfch_stat.argtypes = (c_void_p, c_int32,
        ctypeslib.ndpointer('float64'))
    if not libsdr.sdr_rcv_rfch_stat(rcv, ch, stat):
        return 0, 0, 24.0, 0.0, 0, 0, 0.0, 0
    return int(stat[0]), int(stat[1]), stat[2] / 1e6, stat[3] / 1e6, \
        int(stat[4]), int(stat[5]), stat[6], int(stat[7])
    # dev, fmt, fs (MHz), fo (MHz), IQ, bits, std-dev, rtoc

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
def set_sel_ch(rcv, ch, width):
    libsdr.sdr_rcv_sel_ch.argtypes = (c_void_p, c_int32, c_double)
    libsdr.sdr_rcv_sel_ch(rcv, ch, width)

# get correlator status ---------------------------------------------------------
def get_corr_stat(rcv, ch):
    stat = np.array([0, 24e6, 0, 0, 0, 0, 0], dtype='float64')
    pos = np.zeros(SDR_N_CORR, dtype='float64')
    pos[0:4] = [0, -40, 0, 40]
    C = np.zeros(SDR_N_CORR, dtype='complex64')
    P = np.zeros(SDR_N_CORR, dtype='float64')
    I = np.zeros(SDR_N_CORR, dtype='float64')
    libsdr.sdr_rcv_corr_stat.argtypes = (c_void_p, c_int32,
        ctypeslib.ndpointer('float64'), ctypeslib.ndpointer('float64'),
        ctypeslib.ndpointer('complex64'), ctypeslib.ndpointer('float64'),
        ctypeslib.ndpointer('float64'))
    n = libsdr.sdr_rcv_corr_stat(rcv, ch, stat, pos, C, P, I)
    # state, fs, lock, cn0, coff, fd, npos, pos, C, P, I
    return int(stat[0]), stat[1], stat[2], stat[3], stat[4], stat[5], \
        int(stat[6]) if n > 0 else 1, pos[:n] if n > 0 else pos[:4], \
        C[:n] if n > 0 else C[:4], P[:n] if n > 0 else P[:4], \
        I[:n] if n > 0 else I[:4]

# get correlator history -------------------------------------------------------
def get_corr_hist(rcv, ch, tspan):
    stat = np.array([0, 1e-3], dtype='float64')
    P = np.zeros(SDR_N_HIST, dtype='complex64')
    libsdr.sdr_rcv_corr_hist.argtypes = (c_void_p, c_int32, c_double,
        ctypeslib.ndpointer('float64'), ctypeslib.ndpointer('complex64'))
    n = libsdr.sdr_rcv_corr_hist(rcv, ch, tspan, stat, P)
    return stat[0], stat[1], P[:n] if n > 0 else P[:2] # time, T, P

# array calibration run control ------------------------------------------------
def array_run(rcv, run):
    libsdr.sdr_rcv_array_run.argtypes = (c_void_p, c_int32)
    return libsdr.sdr_rcv_array_run(rcv, run)

# set array calibration mode (0:both, 1:bias only, 2:rpy only) ----------------
def array_calib_mode(rcv, mode):
    libsdr.sdr_rcv_array_set_mode.argtypes = (c_void_p, c_int32)
    return libsdr.sdr_rcv_array_set_mode(rcv, mode)

# start array calibration ------------------------------------------------------
def array_calib_start(rcv):
    return array_run(rcv, 1)

# stop array calibration -------------------------------------------------------
def array_calib_stop(rcv):
    return array_run(rcv, 0)

# clear array calibration ------------------------------------------------------
def array_calib_clear(rcv):
    return array_run(rcv, 2)

# get array calibration status -------------------------------------------------
def array_calib_stat(rcv):
    rpy = np.zeros(3, dtype='float64')
    bias = np.zeros(MAX_RFCH, dtype='float64')
    rms = c_double(0.0)
    nep = c_int32(0)
    libsdr.sdr_rcv_array_stat.argtypes = (c_void_p,
        ctypeslib.ndpointer('float64'), ctypeslib.ndpointer('float64'),
        POINTER(c_double), POINTER(c_int32))
    run = libsdr.sdr_rcv_array_stat(rcv, rpy, bias, byref(rms), byref(nep))
    if run < 0:
        return None
    return run, rpy, bias, rms.value, nep.value

# set array CH beam direction --------------------------------------------------
def array_set_beam(rcv, arch, az, el):
    libsdr.sdr_rcv_array_set_beam.argtypes = (c_void_p, c_int32, c_double,
        c_double)
    return libsdr.sdr_rcv_array_set_beam(rcv, arch, az, el)

# get array CH beam direction --------------------------------------------------
def array_get_beam(rcv, arch):
    az = c_double(0.0)
    el = c_double(0.0)
    libsdr.sdr_rcv_array_get_beam.argtypes = (c_void_p, c_int32,
        POINTER(c_double), POINTER(c_double))
    if not libsdr.sdr_rcv_array_get_beam(rcv, arch, byref(az), byref(el)):
        return None
    return az.value, el.value # (rad)

# set array element positions and enable flags ---------------------------------
def array_ant_pos(rcv, ant_pos, ena):
    n = len(ena)
    c_pos = (c_double * len(ant_pos))(*ant_pos)
    c_ena = (c_int32 * n)(*[int(x) for x in ena])
    libsdr.sdr_rcv_array_ant_pos.argtypes = (c_void_p, POINTER(c_double),
        POINTER(c_int32))
    return libsdr.sdr_rcv_array_ant_pos(rcv, c_pos, c_ena)

# save / load array calibration state to / from file -------------------------
def array_calib_save_file(rcv, file=CALIB_FILE):
    libsdr.sdr_rcv_array_save.argtypes = (c_void_p, c_char_p)
    return libsdr.sdr_rcv_array_save(rcv, file.encode())

def array_calib_load_file(rcv, file=CALIB_FILE):
    libsdr.sdr_rcv_array_load.argtypes = (c_void_p, c_char_p)
    return libsdr.sdr_rcv_array_load(rcv, file.encode())

# save / load array geometry to / from file ----------------------------------
def array_geom_save_file(file, ant_pos):
    n = len(ant_pos) // 3
    c_pos = (c_double * (n * 3))(*ant_pos)
    libsdr.sdr_array_geom_save.argtypes = (c_char_p, POINTER(c_double), c_int32)
    libsdr.sdr_array_geom_save.restype = c_int32
    return libsdr.sdr_array_geom_save(file.encode(), c_pos, n)

def array_geom_load_file(file, max_ant=MAX_RFCH):
    c_pos = (c_double * (max_ant * 3))()
    libsdr.sdr_array_geom_load.argtypes = (c_char_p, POINTER(c_double), c_int32)
    libsdr.sdr_array_geom_load.restype = c_int32
    n = libsdr.sdr_array_geom_load(file.encode(), c_pos, max_ant)
    return [(c_pos[i*3], c_pos[i*3+1], c_pos[i*3+2]) for i in range(n)]

# get PVT solution -------------------------------------------------------------
def get_rcv_pvt_sol(rcv):
    size = 128
    buff = create_string_buffer(size)
    libsdr.sdr_rcv_pvt_sol.argtypes = (c_void_p, c_char_p, c_int32)
    if not libsdr.sdr_rcv_pvt_sol(rcv, buff, size):
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
    global rcv_log, rcv_log_filt, rcv_log_buff, LOG_BUFF_SIZE
    libsdr.sdr_get_log.argtypes = (POINTER(c_char), c_int32)
    size = libsdr.sdr_get_log(rcv_log_buff, LOG_BUFF_SIZE)
    if size <= 0:
        return
    data = rcv_log_buff.raw[:size].decode(errors='ignore')
    for log in data.splitlines():
        if len(log) > 0 and filt_log(rcv_log_filt, log):
            rcv_log.append(log)

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
        btn = ttk.Button(bar.panel, text=label, width=5)
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

# set Receiver settings buttons state -----------------------------------------
def set_settings_btn_state(page, state):
    if page:
        page.label_opt.configure(state=state)
        page.btn_load.configure(state=state)
        page.btn_save.configure(state=state)

# generate selection box -------------------------------------------------------
def sel_box_new(parent, vals=[], val='', width=8):
    box = ttk.Combobox(parent, width=width, state='readonly', justify=CENTER,
        values=vals, height=min([len(vals), 32]), font=get_font())
    box.set(val)
    return box

# show Help dialog -------------------------------------------------------------
def help_dlg(root):
    dlg = sdr_opt.modal_dlg_new(root, 280, 220, 'About', nocancel=1)
    name, ver = get_name_ver()
    python_info = 'with Python ' + sys.version.split()[0]
    sdr_opt.link_label_new(dlg.panel, text=name + ' ver.' + ver,
        font=get_font(2, 'bold'), link=AP_URL).pack(pady=1)
    ttk.Label(dlg.panel, text=python_info, justify=CENTER).pack(pady=1)
    ttk.Label(dlg.panel, text=TITLE, justify=CENTER).pack(pady=1)
    sdr_opt.link_label_new(dlg.panel, text='HELP', font=get_font(2, 'bold'),
        link=HELP_LINK).pack(pady=1)
    ttk.Label(dlg.panel, text=COPYRIGHT, justify=CENTER).pack(pady=1)
    root.wait_window(dlg.win)

# generate Receiver page -------------------------------------------------------
def rcv_page_new(parent):
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.toolbar = tool_bar_new(p.panel)
    p.label_opt = ttk.Label(p.toolbar, text='Options')
    p.label_opt.pack(side=LEFT, padx=(8, 2))
    p.btn_load = ttk.Button(p.toolbar, width=8, text='Load',
        command=on_settings_load)
    p.btn_load.pack(side=LEFT)
    p.btn_save = ttk.Button(p.toolbar, width=8, text='Save',
        command=on_settings_save)
    p.btn_save.pack(side=LEFT)
    p.txt1 = ttk.Label(p.toolbar)
    p.txt1.pack(side=LEFT, fill=X, padx=4)
    p.ind = []
    for i in range(4):
        frm = Frame(p.toolbar, bg='lightgrey')
        frm.pack(side=RIGHT, padx=(1, 1 if i > 0 else 6), pady=(2, 0))
        ind = Frame(frm, width=6, height=10)
        ind.pack(fill=BOTH, padx=1, pady=1)
        p.ind.append(ind)
    ttk.Label(p.toolbar, text='Output').pack(side=RIGHT, padx=1)
    p.box1 = sel_box_new(p.toolbar, SYSTEMS, 'ALL', width=8)
    p.box1.pack(side=RIGHT, padx=(1, 4))
    ttk.Label(p.toolbar, text='System').pack(side=RIGHT, padx=1)
    p.box2 = sel_box_new(p.toolbar, ['ALL'] +
        [str(i + 1) for i in range(MAX_RFCH + MAX_ARCH)], 'ALL', 5)
    p.box2.pack(side=RIGHT, padx=(1, 4))
    ttk.Label(p.toolbar, text='RF CH').pack(side=RIGHT, padx=1)
    panel1 = Frame(p.panel)
    panel1.pack(expand=1, fill=BOTH)
    p.plt1 = plt.plot_new(panel1, 257, 257, margin=(18, 18, 18, 18),
        xlim=(-1, 1), ylim=(-1, 1), aspect=1, font=get_font(-1))
    p.plt1.c.pack(side=RIGHT, expand=1, fill=BOTH)
    p.plt1.gain_on = BooleanVar(value=False)
    btn = ttk.Checkbutton(p.plt1.c, style='w.TCheckbutton', takefocus=0,
        variable=p.plt1.gain_on,
        command=lambda: update_sky_plot(p.plt1, p.box1.get(), p.box2.get()))
    btn.place(relx=1.0, rely=0.0, anchor=NE, x=-8, y=12)
    p.stat = plt.plot_new(panel1, 543, 257, margin=(20, 15, 30, 30))
    p.stat.c.pack(side=LEFT, expand=1, fill=BOTH)
    p.plt2 = plt.plot_new(p.panel, 800, 245, title='Signal C/N0 (dB-Hz)', tick=10)
    p.plt2.c.pack(expand=1, fill=BOTH)
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_sys_select(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_sys_select(e, p))
    p.plt1.c.bind('<Button-1>', lambda e: on_skyplot_click(e, p))
    p.plt1.c.bind('<B1-Motion>', lambda e: on_skyplot_click(e, p))
    update_rcv_page(p)
    return p

# system select callback -------------------------------------------------------
def on_sys_select(e, p):
    sys = p.box1.get()
    rfch = p.box2.get()
    update_sky_plot(p.plt1, sys, rfch)
    update_sig_plot(p.plt2, sys, rfch)

# get cursol az/el in skyplot --------------------------------------------------
def get_azel(e, p):
    xs, ys = plt.plot_scale(p)
    if xs <= 0 or ys <= 0: return -1, -1
    xc = p.m[0] + (p.c.winfo_width()  - p.m[0] - p.m[1]) / 2
    yc = p.m[2] + (p.c.winfo_height() - p.m[2] - p.m[3]) / 2
    x = (e.x - xc) / xs + (p.xl[0] + p.xl[1]) / 2
    y = (p.yl[0] + p.yl[1]) / 2 - (e.y - yc) / ys
    r = sqrt(x * x + y * y)
    if r > 1.0:
        return -1, -1
    el = (1.0 - r) * pi / 2
    az = atan2(x, y)
    if az < 0: az += 2 * pi
    return az, el

# skyplot click/drag callback --------------------------------------------------
def on_skyplot_click(e, p):
    rfch = p.box2.get()
    arch = to_int(rfch)
    az, el = get_azel(e, p.plt1)
    if not rcv_body or arch <= MAX_RFCH or az < 0.0:
        return
    array_set_beam(rcv_body, arch - 1, az, el)
    if array_page is not None:
        idx = arch - 1 - MAX_RFCH
        if 0 <= idx < MAX_ARCH:
            array_page.beam_az[idx].set('%.3f' % (az / D2R))
            array_page.beam_el[idx].set('%.3f' % (el / D2R))
    update_sky_plot(p.plt1, p.box1.get(), rfch)

# update Receiver page ---------------------------------------------------------
def update_rcv_page(p):
    sys = p.box1.get()
    rfch = p.box2.get()
    update_rcv_stat(p.stat)
    update_str_stat(p.ind)
    update_sky_plot(p.plt1, sys, rfch)
    update_sig_plot(p.plt2, sys, rfch)

# update receiver status panel -------------------------------------------------
def update_rcv_stat(p):
    labels = ('Receiver Time (s)', 'Input Source',
        'Fmt/# RF CH/# Array CH', 'LO Frequencies (MHz)', '', '', 'Sampling',
        'Sampling Rate (Msps)', '# BB CH Locked/All',
        'IF Data Rate (MB/s)', 'IF Data Buffer Usage (%)', 'Time (GPST)',
        'Solution Status', 'Latitude (\xb0)', 'Longitude (\xb0)',
        'Altitude (m)', 'Roll/Pitch/Yaw (\xb0)', '# Sats Used/All',
        'Solution Latency (s)', 'Output', '# PVT/# OBS/NAV Data',
        'IF Data Log (MB)')
    stat = get_rcv_stat(rcv_body).split()
    sol = get_rcv_pvt_sol(rcv_body).split()
    arr = array_calib_stat(rcv_body)
    rpy = arr[1] if arr else np.zeros(3)
    rpy_str = '%.2f/%.2f/%.2f' % (rpy[0]/D2R, rpy[1]/D2R, rpy[2]/D2R)
    val = stat[0:3] + [''] + stat[3:10] + [sol[0] + ' ' + sol[1], sol[6]] + \
        sol[2:5] + [rpy_str] + sol[5:6] + [stat[10], ''] + stat[11:]
    plt.plot_clear(p)
    xs, ys = plt.plot_scale(p)
    for i in range(len(labels)):
        x, y = 0.0 if i < 11 else 0.51, 0.5 + (5 - i % 11) * 20 / ys
        color = P1_COLOR
        if (labels[i] == 'IF Data Buffer Usage (%)' and float(val[i]) > 90.0) or \
            labels[i] == 'IF Data Log (MB)' and float(val[i]) > 0:
            color = WARN_COLOR
        plt.plot_text(p, x, y, labels[i], anchor=W, font=get_font(0, 'bold'),
            color=color)
        plt.plot_text(p, x + 0.48, y, val[i], anchor=E, color=color)

# update stream status ---------------------------------------------------------
def update_str_stat(p):
    col = ('#CC0000', BG_COLOR1, '#EE9900', '#006600', '#00CC00')
    stat = get_str_stat(rcv_body)
    for i, ind in enumerate(p):
        ind.configure(bg=col[stat[3-i]+1])

# read rpy (rad) from saved calibration file, or zeros if unavailable ---------
def read_calib_rpy_file(file=CALIB_FILE):
    try:
        with open(file) as f:
            for line in f:
                idx = line.find('#')
                if idx >= 0: line = line[:idx]
                vals = line.split()
                if len(vals) >= 3:
                    try:
                        return [float(v) * D2R for v in vals[:3]]
                    except ValueError:
                        pass
    except OSError:
        pass
    return [0.0, 0.0, 0.0]

# overlay array gain -----------------------------------------------------------
def draw_array_gain_overlay(p, arch):
    if rcv_body:
        stat = array_calib_stat(rcv_body)
        if not stat: return
        rpy = stat[1]
    else:
        rpy = read_calib_rpy_file()
    ant_pos, ena = array_read_opt()
    pos = np.asarray(ant_pos, dtype=float).reshape(-1, 3)
    ena_arr = np.asarray(ena, dtype=bool)[:len(pos)]
    if ena_arr.sum() < 1: return
    pos = pos[ena_arr]
    if rcv_body:
        beam = array_get_beam(rcv_body, arch - 1)
        if beam is None: return
        az_b, el_b = beam
    else:
        m = arch - MAX_RFCH - 1
        if not (0 <= m < MAX_ARCH) or array_page is None: return
        az_b = to_float(array_page.beam_az[m].get()) * D2R
        el_b = to_float(array_page.beam_el[m].get()) * D2R
    lam = CLIGHT / 1.57542e9
    g = np.linspace(-1.0, 1.0, GAIN_OVL_M)
    X, Y = np.meshgrid(g, g)
    R2 = X * X + Y * Y
    az = np.arctan2(X, Y)
    el = np.maximum(0.0, (1.0 - np.sqrt(np.minimum(R2, 1.0))) * np.pi / 2)

    cr, sr = cos(rpy[0]), sin(rpy[0])
    cp, sp = cos(rpy[1]), sin(rpy[1])
    cy, sy = cos(rpy[2]), sin(rpy[2])
    Rmat = np.array([ # body-to-ENU rotation R = Rz(yaw) Ry(pitch) Rx(roll)
        [cy*cp, cy*sp*sr - sy*cr, cy*sp*cr + sy*sr],
        [sy*cp, sy*sp*sr + cy*cr, sy*sp*cr - cy*sr],
        [-sp,   cp*sr,            cp*cr           ]])
    pos_enu = pos @ Rmat.T
    ce = np.cos(el)
    e_enu = np.stack([np.sin(az) * ce, np.cos(az) * ce, np.sin(el)], axis=-1)
    proj = e_enu @ pos_enu.T
    e_b = np.array([sin(az_b) * cos(el_b), cos(az_b) * cos(el_b), sin(el_b)])
    proj_b = e_b @ pos_enu.T
    resp = np.sum(np.exp(1j * (2.0 * np.pi / lam) * (proj - proj_b)), axis=-1)
    gain = 20.0 * np.log10(np.abs(resp) + 1e-30)
    gain_img = gain[::-1]
    actual_half = plt.plot_image(p, 0.0, 0.0, 1.0, gain_img, -30, 20, tag='gain')
    plt.plot_sky_mask(p, actual_half, n_arc=24, tag='gain')
    p.c.tag_lower('gain')

# update skyplot ---------------------------------------------------------------
def update_sky_plot(p, sys, rfch):
    sats, sat, sig, cn0, prn = get_sig_stat(rcv_body, sys, 1, 0)
    az, el, pvt, obs, eph, svh, fcn = get_sat_stat(rcv_body, sats)
    if to_int(rfch) > 0:
        e_sats, _, _, _, _ = get_sig_stat(rcv_body, sys, 1,
            rfch=(0 if rfch == 'ALL' else int(rfch)))
    else:
        e_sats = sats
    plt.plot_clear(p)
    plt.plot_sky(p, color=None)
    arch = to_int(rfch)
    gain_on = getattr(p, 'gain_on', None)
    if arch > MAX_RFCH and (gain_on is None or gain_on.get()):
        draw_array_gain_overlay(p, arch)
    xs, ys = plt.plot_scale(p)
    for i in range(len(sats) - 1, -1, -1):
        if el[i] <= 0.0:
            continue
        x = (90 - el[i]) / 90 * sin(az[i] * pi / 180)
        y = (90 - el[i]) / 90 * cos(az[i] * pi / 180)
        ena = pvt[i] and sats[i] in e_sats
        color1 = sat_color(sats[i]) if ena else None
        color2 = BG_COLOR1 if ena else plt.FG_COLOR
        plt.plot_circle(p, x, y, 12 / xs, fill=color1)
        plt.plot_text(p, x, y, sats[i], color=color2, font=get_font(-1))
    plt.plot_sky(p, gcolor=None)

    if rcv_body and arch > MAX_RFCH:
        draw_beam_mark(p, arch)
    
# draw beam direction mark ------------------------------------------------------
def draw_beam_mark(p, arch):
    beam = array_get_beam(rcv_body, arch - 1)
    if beam is not None:
        az, el = beam
        az /= D2R
        el /= D2R
        if 0 <= el <= 90:
            xs, ys = plt.plot_scale(p)
            d = 10 / xs
            x = (90 - el) / 90 * sin(az * D2R)
            y = (90 - el) / 90 * cos(az * D2R)
            xs1, ys1 = [x - d, x + d], [y - d, y + d]
            xs2, ys2 = [x + d, x - d], [y - d, y + d]
            plt.plot_poly(p, xs1, ys1, color='white', width=6)
            plt.plot_poly(p, xs2, ys2, color='white', width=6)
            plt.plot_poly(p, xs1, ys1, color='red', width=2)
            plt.plot_poly(p, xs2, ys2, color='red', width=2)

# update signal plot -----------------------------------------------------------
def update_sig_plot(p, sys, rfch):
    sats, sat, sig, cn0, prn = get_sig_stat(rcv_body, sys, 1,
        rfch=(0 if rfch == 'ALL' else int(rfch)))
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
    chs = ['ALL'] + [str(i + 1) for i in range(MAX_RFCH + MAX_ARCH)] + \
        ['1-4', '5-8', '9-12', '13-16']
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
    if ch.find('-') >= 0 or val == '-':
        return
    set_rfch_gain(rcv_body, int(ch), 0 if val == 'Auto' else int(val) + 1)

# IF Filter select callback ----------------------------------------------------
def on_filt_select(e, p):
    ch = p.box1.get()
    val1 = p.box4.get()
    val2 = p.box5.get()
    dev, fmt, fs, fo, IQ, bits, std, rtoc = get_rfch_stat(rcv_body, int(ch))
    if ch.find('-') >= 0 or val1 == '-' or val2 == '-' or IQ != 2:
        return
    set_rfch_filt(rcv_body, int(ch), float(val2), 0.0, val1 == '3rd')

# update RF Channels page ------------------------------------------------------
def update_rfch_page(p):
    ch = p.box1.get()
    tave = float(p.box2.get())
    pos = ch.find('-')
    if ch == 'ALL':
        p.panel2.pack_forget()
        p.panel3.pack_forget()
        p.panel1.pack(expand=1, fill=BOTH)
        update_freq_plot(p.plt1)
        p.box3.set('-')
        p.box4.set('-')
        p.box5.set('-')
    elif pos >= 0:
        p.panel1.pack_forget()
        p.panel2.pack_forget()
        p.panel3.pack(side=LEFT, expand=1, fill=BOTH)
        rfch = to_int(ch[:pos])
        for i in range(4):
            p.plt3[i].c.place(relx=i % 2 * 0.5, rely=i // 2 * 0.5, relwidth=0.5,
                relheight=0.5)
            update_psd_plot(p.plt3[i], rfch + i, tave)
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
    dev, fmt, fs, fo, IQ, bits, std, rtoc = get_rfch_stat(rcv_body, 1)
    p.txt1.configure(text='F_S: %.6f MHz' % (fs))

# update PSD plot --------------------------------------------------------------
def update_psd_plot(p, ch, tave):
    dev, fmt, fs, fo, IQ, bits, std, rtoc = get_rfch_stat(rcv_body, ch)
    psd = get_rfch_psd(rcv_body, ch, tave)
    f = np.linspace(fo, fo + fs / 2, len(psd)) if IQ == 1 else \
        np.linspace(fo - fs / 2, fo + fs / 2, len(psd))
    plt.plot_clear(p)
    plt.plot_xlim(p, [f[0], f[-1]])
    plt.plot_ylim(p, [-85, -45])
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, [fo, fo], p.yl, color=plt.GR_COLOR)
    plt.plot_poly(p, f, psd, color=P1_COLOR)
    plot_filt(p, ch, fo, fs, rtoc)
    plt.plot_axis(p, gcolor=None)
    xs, ys = plt.plot_scale(p)
    plt.plot_poly(p, [fo, fo], [p.yl[0], p.yl[0] + 6 / ys], color=plt.FG_COLOR)
    plt.plot_poly(p, [fo, fo], [p.yl[1], p.yl[1] - 6 / ys], color=plt.FG_COLOR)
    plot_mark(p, fo, p.yl[0] + 16 / ys, color=plt.FG_COLOR)
    plot_sig_freq(p, p.xl, 16)
    plt.plot_text(p, p.xl[0] + 10 / xs, p.yl[1] - 8 / ys, 'CH%d' % (ch),
        font=get_font(1, 'bold'), anchor=NW)
    x = fo - 12 / xs if fo > p.xl[0] else fo + 12 / xs
    align = E if fo > p.xl[0] else W
    plt.plot_text(p, x, p.yl[0] + 16 / ys, '%.6f MHz' % (fo), anchor=align)
    text = '%s (%d bits, Std %.1f)' % ('I' if IQ == 1 else 'IQ', bits, std)
    plt.plot_text(p, p.xl[1] - 10 / xs, p.yl[0] + 16 / ys, text, anchor=E)

# update signal frequency plot -------------------------------------------------
def update_freq_plot(p):
    for i in range(2):
        plt.plot_clear(p[i])
        plt.plot_axis(p[i])
        for ch in range(1, 9):
            dev, fmt, fs, fo, IQ, bits, std, rtoc = get_rfch_stat(rcv_body, ch)
            xs, ys = plt.plot_scale(p[i])
            xl = [fo, fo + fs / 2] if IQ == 1 else [fo - fs / 2, fo + fs / 2]
            yl = [p[i].yl[0] + 5 / ys, p[i].yl[1] - 20 / ys]
            plt.plot_rect(p[i], xl[0], yl[0], xl[1], yl[1], fill='#F8F8F8')
        for ch in range(1, 9):
            dev, fmt, fs, fo, IQ, bits, std, rtoc = get_rfch_stat(rcv_body, ch)
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
def plot_filt(p, ch, fo, fs, rtoc):
    bw, freq, order = get_rfch_filt(rcv_body, ch)
    if bw < 0: return
    xs, ys = plt.plot_scale(p)
    # rtoc shifts the displayed fo by +fs/4; the analog filter centered at
    # chip IF "freq" sits at (chip_LO + freq) = (fo - fs/4 + freq) in display.
    fc = fo - fs * 0.25 + freq if rtoc else fo + freq
    x1, x2 = fc - bw * 0.5, fc + bw * 0.5
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
    dev, fmt, fs, fo, IQ, bits, std, rtoc = get_rfch_stat(rcv_body, ch)
    val, hist1, hist2 = get_rfch_hist(rcv_body, ch, tave)
    plot_hist(p1, bits, val, hist1)
    plot_hist(p2, bits, val, hist2)

# plot histogram ---------------------------------------------------------------
def plot_hist(p, bits, val, hist):
    bits = bits if bits <= 4 else 4
    xl = (5, 3, 5, 9, 9)
    yl = (0.4, 0.4, 0.4, 0.3, 0.25)
    xs, ys = plt.plot_scale(p)
    plt.plot_clear(p)
    plt.plot_axis(p, fcolor=None, tcolor=None)
    w = 6 / xs * 5 / xl[bits]
    for i in range(len(val)):
        plt.plot_rect(p, val[i] - w, 0, val[i] + w, hist[i], fill=P1_COLOR)
    plt.plot_xlim(p, [-xl[bits], xl[bits]])
    plt.plot_ylim(p, [0, yl[bits]])
    plt.plot_axis(p, gcolor=None)
    if len(val) > 0 and np.sum(hist) > 0:
        ave = np.sum(val * hist) / np.sum(hist)
        std = sqrt(np.sum((val - ave) ** 2 * hist) / np.sum(hist))
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
    p.box1 = sel_box_new(p.toolbar, ['ALL'] +
        [str(i + 1) for i in range(MAX_RFCH + MAX_ARCH)], 'ALL', 5)
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
    p.tbl1.tag_configure('idle', foreground=P2_COLOR)
    p.tbl1.tag_configure('srch', foreground='blue')
    p.tbl1_cols = ()
    p.tbl1_last_width = 0
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
    stat = get_ch_stat(rcv_body, sys, chno=(-1 if state == 'ALL' else 0),
        rfch=(0 if rfch == 'ALL' else int(rfch)))
    w = (40, 22, 36, 52, 32, 68, 38, 70, 82, 62, 84, 44, 48, 36, 34, 32)
    a = 'ecccceeweeeceeee'
    buff, srch, lock = stat[0][72:82], stat[0][82:92], stat[0][92:]
    p.txt1.configure(text=buff)
    p.txt2.configure(text=srch)
    p.txt3.configure(text=lock)
    buff_use = int(re.split('[:%]', buff)[1])
    srch_ch = int(re.split('[:]', srch)[1])
    p.txt1.configure(foreground='green' if buff_use < 90 else WARN_COLOR)
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
    ttk.Label(p.toolbar, text='BB CH').pack(side=LEFT, padx=(8, 1))
    p.btn1 = sdr_opt.custom_btn_new(p.toolbar, label=' < ')
    p.btn1.panel.pack(side=LEFT, padx=1, pady=1)
    p.box1 = sel_box_new(p.toolbar, ['1'], '1', 4)
    p.box1.pack(side=LEFT)
    p.btn2 = sdr_opt.custom_btn_new(p.toolbar, label=' > ')
    p.btn2.panel.pack(side=LEFT, padx=1, pady=1)
    p.txt1 = ttk.Label(p.toolbar, foreground=P1_COLOR)
    p.txt1.pack(side=LEFT, expand=1, fill=X, padx=6)
    p.box3 = sel_box_new(p.toolbar, ['0.1', '0.15', '0.2', '0.3', '0.4', '0.6',
        '0.8', '1.0', '1.5', '2'], '0.4', 3)
    p.box3.pack(side=RIGHT, padx=(1, 4))
    p.box2 = sel_box_new(p.toolbar, ['0.1', '0.2', '0.5', '1', '2', '5', '10'],
        '1', 3)
    p.box2.pack(side=RIGHT, padx=1)
    p.box5 = sel_box_new(p.toolbar, ['0.2', '0.3', '0.5', '1', '1.5', '2', '3',
        '5', '10', '20'], '5', 3)
    p.box5.pack(side=RIGHT, padx=1)
    p.box4 = sel_box_new(p.toolbar, ['I', 'Q', 'IQ', 'AveI', 'AveIQ'], 'I', 5)
    p.box4.pack(side=RIGHT, padx=1)
    ttk.Label(p.toolbar, text='IQ/W(μs)/T(s)/Range').pack(side=RIGHT, padx=2)
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
    p.box5.bind('<<ComboboxSelected>>', lambda e: on_corr_ch_select(e, p))
    update_corr_page(p)
    update_corr_ch_sel(p)
    return p

# correlator channel down callback ---------------------------------------------
def on_corr_ch_down(e, p):
    chs, ch = p.box1['values'], p.box1.get()
    w = float(p.box5.get()) * 1e-6
    i = chs.index(ch) if ch in chs else -1
    if i - 1 >= 0:
        ch = chs[i-1]
        p.box1.set(ch)
        set_sel_ch(rcv_body, int(ch), w)
        update_corr_page(p)

# correlator channel up callback -----------------------------------------------
def on_corr_ch_up(e, p):
    chs, ch = p.box1['values'], p.box1.get()
    w = float(p.box5.get()) * 1e-6
    i = chs.index(ch) if ch in chs else -1
    if i >= 0 and i + 1 < len(chs):
        ch = chs[i+1]
        p.box1.set(ch)
        set_sel_ch(rcv_body, int(ch), w)
        update_corr_page(p)

# correlator channel select callback -------------------------------------------
def on_corr_ch_select(e, p):
    ch = p.box1.get()
    w = float(p.box5.get()) * 1e-6
    set_sel_ch(rcv_body, int(ch), w)
    update_corr_page(p)

# update Correlator page -------------------------------------------------------
def update_corr_page(p):
    update_corr_ch_sel(p)
    ch = int(p.box1.get())
    w = float(p.box5.get()) * 1e-6
    rng = [float(p.box2.get()), float(p.box3.get())]
    type = p.box4.get()
    state, fs, lock, cn0, coff, fd, npos, pos, C, aveC, aveI = get_corr_stat(rcv_body, ch)
    tt, T, P = get_corr_hist(rcv_body, ch, rng[0])
    update_corr_plot1(p.plt1, coff, fs, npos, pos, w, C, aveC, aveI, type, rng[1])
    update_corr_plot2(p.plt2, P, C, rng[1])
    update_corr_plot3(p.plt3, tt, T, P, rng)
    update_corr_text(p, ch, tt)
    
# update correlator channel selection ------------------------------------------
def update_corr_ch_sel(p):
    chs = [s.split()[0] for s in get_ch_stat(rcv_body, 'ALL')[2:]]
    w = float(p.box5.get()) * 1e-6
    p.box1.configure(values=chs, height=min([len(chs), 32]))
    if len(chs) > 0 and not p.box1.get() in chs:
        p.box1.set(chs[0])
        set_sel_ch(rcv_body, int(chs[0]), w)
    else:
        set_sel_ch(rcv_body, int(p.box1.get()), w)

# update correlator text -------------------------------------------------------
def update_corr_text(p, ch, time):
    s = get_ch_stat(rcv_body, 'ALL', chno=int(ch), opt=1)[2:]
    if not s: return
    ss = s[0].split()
    text = 'RF CH: %s SAT: %s SIG: %s PRN: %s LOCK: %s s' % (ss[1], ss[2],
        ss[3], ss[4], ss[5])
    p.txt1.configure(text=text)
    xs, ys = plt.plot_scale(p.plt3)
    text = ('C/N0: %s dB-Hz  COFF: %s ms  DOP: %s Hz  ADR: %s cyc  SYNC: %s' +
        '  #NAV: %s') % (ss[6], ss[8], ss[9], ss[10], ss[11], ss[12])
    plt.plot_text(p.plt3, p.plt3.xl[0] + 12 / xs, p.plt3.yl[1] - 15 / ys,
        text, anchor=W)
    text = ('ERR_P: %7s cyc  ERR_C: %7s m  NAV: %2s-%2s-%2s  WEEK: %4s  TOW: %6s') % (
        ss[16], ss[17], ss[19], ss[20], ss[21], ss[22],
        ss[23] if int(ss[24]) else '------')
    plt.plot_text(p.plt3, p.plt3.xl[0] + 12 / xs, p.plt3.yl[0] + 15 / ys,
        text, anchor=W)

# update correlator plot 1 -----------------------------------------------------
def update_corr_plot1(p, coff, fs, npos, pos, w, C, aveC, aveI, type, rng):
    x = [coff + pos[i] / fs * 1e3 for i in range(len(pos))]
    D = C * np.sign(C[0])
    if type == 'I':
        ti, yl = 'I * sign(IP)', [-rng * 0.3, rng]
    elif type == 'Q':
        ti, yl = 'Q', [-rng, rng]
    elif type == 'IQ':
        ti, yl = '\u221A (I\xb2+Q\xb2)', [0, rng]
    elif type == 'AveI':
        ti, yl = '\u03A3 (I * sign(IP)) / N', [-rng * 0.3, rng]
    else:
        ti, yl = '\u221A (\u03A3 (I\xb2+Q\xb2) / N)', [0, rng]
    plt.plot_clear(p)
    #plt.plot_xlim(p, [x[npos], x[-1]])
    plt.plot_xlim(p, [x[0] - w * 5e2, x[0] + w * 5e2])
    plt.plot_ylim(p, yl)
    xs, ys = plt.plot_scale(p)
    p.title = ti
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, [coff, coff], p.yl, color='grey')
    plt.plot_poly(p, p.xl, [0, 0], color='grey')
    plt.plot_dots(p, [coff], [0], color=plt.FG_COLOR, size=3)
    y = D.real if type == 'I' else D.imag if type == 'Q' \
        else abs(D) if type == 'IQ' else aveI if type == 'AveI' \
        else np.sqrt(aveC)
    plt.plot_poly(p, x[npos:], y[npos:], color=P2_COLOR)
    plt.plot_dots(p, x[npos:], y[npos:], color=P2_COLOR, fill=P1_COLOR, size=3)
    plt.plot_dots(p, x[:npos], y[:npos], color=P1_COLOR, fill=P1_COLOR, size=9)
    plt.plot_axis(p, gcolor=None)
    plt.plot_text(p, p.xl[1] - 18 / xs, p.yl[0] + 10 / ys, 'COFF (ms)',
        anchor=SE)
    plot_scale(p, x[0] + w * 4.5e2 - 20 / xs, yl[1] - 20 / ys, w * 1e2, '%.2f ns' % (w * 1e5))
    if type == 'AveIQ' or type == 'AveI':
        plt.plot_text(p, p.xl[1] - 18 / xs, p.yl[1] - 32 / ys,
            'Integ = %s s' % (sys_opt.t_dll.get()), anchor=NE)

# plot scale -------------------------------------------------------------------
def plot_scale(p, x, y, w, label, color=plt.FG_COLOR):
    xs, ys = plt.plot_scale(p)
    d = 6 / ys
    plt.plot_poly(p, [x - w / 2, x + w / 2], [y, y], color=color)
    plt.plot_poly(p, [x - w / 2, x - w / 2], [y - d, y + d], color=color)
    plt.plot_poly(p, [x + w / 2, x + w / 2], [y - d, y + d], color=color)
    plt.plot_text(p, x - w / 2 - 8 / xs, y, label, anchor=E, color=color)

# update correlator plot 2 -----------------------------------------------------
def update_corr_plot2(p, P, C, rng):
    plt.plot_clear(p)
    plt.plot_xlim(p, [-rng, rng])
    plt.plot_ylim(p, [-rng, rng])
    plt.plot_axis(p, fcolor=None, tcolor=None)
    plt.plot_poly(p, [0, 0], p.yl, color='grey')
    plt.plot_poly(p, p.xl, [0, 0], color='grey')
    plt.plot_dots(p, [0], [0], color=plt.FG_COLOR, size=3)
    plt.plot_dots(p, P.real, P.imag, color=P2_COLOR, size=1)
    plt.plot_dots(p, [C[0].real], [C[0].imag], color=BG_COLOR1, size=11)
    plt.plot_dots(p, [C[0].real], [C[0].imag], color=P1_COLOR, fill=P1_COLOR, size=9)
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
    p.tbl1.tag_configure('unhealthy', foreground=WARN_COLOR)
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
    sol_log.clear()
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

# generate Array page ----------------------------------------------------------
def array_page_new(parent):
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.toolbar = tool_bar_new(p.panel)
    p.txt_stat = Label(p.toolbar, font=get_font(), bg=BG_COLOR1)
    p.txt_stat.pack(side=LEFT, padx=10)
    p.btn_save = ttk.Button(p.toolbar, width=7, text='Save')
    p.btn_save.pack(side=RIGHT, padx=(0, 10))
    p.btn_load = ttk.Button(p.toolbar, width=7, text='Load')
    p.btn_load.pack(side=RIGHT)
    p.btn_clear = ttk.Button(p.toolbar, width=7, text='Clear')
    p.btn_clear.pack(side=RIGHT)
    p.btn_calib = ttk.Button(p.toolbar, width=7, text='Start')
    p.btn_calib.pack(side=RIGHT)
    p.mode_var = StringVar(value='Both')
    p.mode_box = ttk.Combobox(p.toolbar, width=5, state='readonly',
        values=['Both', 'Delay', 'Att'], textvariable=p.mode_var,
        justify=CENTER, font=get_font())
    p.mode_box.pack(side=RIGHT)
    ttk.Label(p.toolbar, text='Calibration').pack(side=RIGHT, padx=2)
    panel1 = Frame(p.panel, bg=BG_COLOR1)
    panel1.pack(expand=1, fill=BOTH)
    panel2 = Frame(panel1, relief=SOLID, bd=1, bg=BG_COLOR1)
    panel2.pack(fill=X, padx=10, pady=4)
    ttk.Label(panel2, text='RF CH DELAY', font=get_font(1, 'bold')).pack(pady=4)
    p.txt_bias = Label(panel2, font=get_font(), bg=BG_COLOR1)
    p.txt_bias.pack(padx=10, pady=(6, 16))
    panel3 = Frame(panel1, relief=SOLID, bd=1, bg=BG_COLOR1)
    panel3.pack(fill=X, padx=10, pady=4)
    ttk.Label(panel3, text='ARRAY ATTITUDE', font=get_font(1, 'bold')).pack(pady=4)
    p.txt_att = Label(panel3, font=get_font(), bg=BG_COLOR1)
    p.txt_att.pack(padx=10, pady=(6, 16))
    panel4 = Frame(panel1, relief=SOLID, bd=1, bg=BG_COLOR1)
    panel4.pack(expand=1, fill=BOTH, padx=10, pady=4)
    ttk.Label(panel4, text='ARRAY CH BEAM DIRECTION', font=get_font(1, 'bold')).pack(pady=4)
    p.beam_az = [StringVar() for i in range(MAX_ARCH)]
    p.beam_el = [StringVar() for i in range(MAX_ARCH)]
    p.beam_panel = []
    for i in range(MAX_ARCH):
        p.beam_az[i].set('0.000')
        p.beam_el[i].set('90.000')
        p.beam_panel.append(gen_beam_dirs(panel4, p, i))
    p.btn_calib.bind('<Button-1>', lambda e: on_array_calib_toggle(e, p))
    p.btn_clear.bind('<Button-1>', lambda e: on_array_calib_clear(e, p))
    p.btn_load.bind('<Button-1>', lambda e: on_array_calib_load(e, p))
    p.btn_save.bind('<Button-1>', lambda e: on_array_calib_save(e, p))
    p.mode_box.bind('<<ComboboxSelected>>', lambda e: on_array_mode_change(e, p))
    return p

# array calibration mode lookup ------------------------------------------------
ARRAY_MODES = {'Both': 0, 'Delay': 1, 'Att': 2}

# Array page mode change callback ----------------------------------------------
def on_array_mode_change(e, p):
    if not rcv_body: return
    array_calib_mode(rcv_body, ARRAY_MODES.get(p.mode_var.get(), 0))

# generate beam directions -----------------------------------------------------
def gen_beam_dirs(parent, p, i):
    panel = Frame(parent, height=25, width=380, bg=BG_COLOR1)
    panel.pack_propagate(0)
    panel.pack(pady=1)
    btn = ttk.Button(panel, width=6, text='Update')
    btn.pack(side=RIGHT, padx=1)
    ttk.Label(panel, text='\xb0').pack(side=RIGHT, padx=(0, 4))
    el = ttk.Entry(panel, width=10, justify='right', textvariable=p.beam_el[i])
    el.pack(side=RIGHT, padx=1)
    ttk.Label(panel, text='\xb0  EL').pack(side=RIGHT)
    az = ttk.Entry(panel, width=10, justify='right', textvariable=p.beam_az[i])
    az.pack(side=RIGHT, padx=2)
    ttk.Label(panel, text='  AZ').pack(side=RIGHT)
    ttk.Label(panel, text='  CH%-2d' % (i + 9), font=get_font(0, 'bold')).pack(side=LEFT, padx=2)
    btn.bind('<Button-1>', lambda e: on_beam_update(e, p, i))
    return panel

# update Array page ------------------------------------------------------------
def update_array_page(p):
    global array_opt
    narch = to_int(array_opt.no_array.get())
    for i in range(MAX_ARCH):
        config_panel_state(p.beam_panel[i], NORMAL if i < narch else DISABLED)
    stat = array_calib_stat(rcv_body)
    if not stat: return
    run, rpy, bias, rms, nep = stat
    text1 = 'CALIB: %s  EPOCHS: %3d  RMS: %6.4f m' % (
        'RUN' if run else 'STOP', nep, rms)
    text2 = 'ROLL:%8.3f\xb0  PITCH:%8.3f\xb0  YAW:%8.3f\xb0' % (rpy[0] / D2R,
        rpy[1] / D2R, rpy[2] / D2R)
    text3 = ''
    for i in range(MAX_RFCH):
        text3 += ' CH%d:%7.4f m ' % (i + 1, bias[i])
        text3 += '\n\n' if i % 4 == 3 and i < MAX_RFCH - 1 else ''
    p.txt_stat.configure(text=text1, fg=WARN_COLOR if run else 'black')
    p.txt_att.configure(text=text2, fg=WARN_COLOR if run else 'black')
    p.txt_bias.configure(text=text3, fg=WARN_COLOR if run else 'black')

# Array page clear button callback ---------------------------------------------
def on_array_calib_clear(e, p):
    if not rcv_body: return
    array_calib_clear(rcv_body)
    try:
        if os.path.isfile(CALIB_FILE):
            os.remove(CALIB_FILE)
    except OSError:
        pass
    p.btn_calib.configure(text='Start')

# read antenna geometry and enable mask from array_opt -------------------------
def array_read_opt():
    global array_opt
    ant_pos = []
    ena = []
    for i in range(MAX_RFCH):
        x = to_float(array_opt.posx[i].get())
        y = to_float(array_opt.posy[i].get())
        z = to_float(array_opt.posz[i].get())
        ant_pos.extend([x, y, z])
        ena.append(1 if array_opt.ena_ele[i].get() else 0)
    return ant_pos, ena

# Array page calibrate toggle callback -----------------------------------------
def on_array_calib_toggle(e, p):
    if not rcv_body:
        return
    if p.btn_calib['text'] == 'Start':
        ant_pos, ena = array_read_opt()
        if sum(ena) < 2:
            return
        array_ant_pos(rcv_body, ant_pos, ena)
        array_calib_mode(rcv_body, ARRAY_MODES.get(p.mode_var.get(), 0))
        if not array_calib_start(rcv_body):
            return
        p.btn_calib.configure(text='Stop')
    else:
        array_calib_stop(rcv_body)
        p.btn_calib.configure(text='Start')

# Array page calibration load button callback ---------------------------------
def on_array_calib_load(e, p):
    file = filedialog.askopenfilename(parent=p.panel,
        title='Load Array Calibration', initialfile=CALIB_FILE,
        filetypes=[('Calibration', '*.txt'), ('All', '*.*')])
    if not file: return
    if rcv_body:
        ok = array_calib_load_file(rcv_body, file)
    else:
        try:
            shutil.copyfile(file, CALIB_FILE)
            ok = True
        except OSError:
            ok = False
    if not ok:
        status_bar_show('Calibration load error. ' + file)

# Array page calibration save button callback ---------------------------------
def on_array_calib_save(e, p):
    file = filedialog.asksaveasfilename(parent=p.panel,
        title='Save Array Calibration', initialfile=CALIB_FILE,
        defaultextension='.txt',
        filetypes=[('Calibration', '*.txt'), ('All', '*.*')])
    if not file: return
    if rcv_body:
        ok = array_calib_save_file(rcv_body, file)
    elif os.path.isfile(CALIB_FILE):
        try:
            shutil.copyfile(CALIB_FILE, file)
            ok = True
        except OSError:
            ok = False
    else:
        ok = False
    if not ok:
        status_bar_show('Calibration save error. ' + file)

# Array page beam update callback ----------------------------------------------
def on_beam_update(e, p, i):
    if not rcv_body: return
    az = to_float(p.beam_az[i].get()) * D2R
    el = to_float(p.beam_el[i].get()) * D2R
    array_set_beam(rcv_body, i + MAX_ARCH, az, el)

# generate Log page ------------------------------------------------------------
def log_page_new(parent):
    filts = ('', '$TIME', '$POS', '$ATT', '$OBS', '$NAV', '$SAT', '$CH', '$EPH',
        '$ALM', '$LOG')
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
        rcv_body, info = rcv_open(sys_opt, inp_opt, out_opt, sig_opt, array_opt)
        if rcv_body == None:
            status_bar_show('Receiver start error. ' + info)
            return
        # apply enable mask and antenna geometry from array_opt
        ant_pos, ena = array_read_opt()
        array_ant_pos(rcv_body, ant_pos, ena)
        array_calib_load_file(rcv_body)

        status_bar_show('Receiver started. ' + info)
        for i, btn in enumerate(bar.panel.winfo_children()):
            btn.configure(state=NORMAL if i in (1, 7) else DISABLED)
        set_settings_btn_state(rcv_page, DISABLED)

# Stop button push callback ----------------------------------------------------
def on_btn_stop_push(bar):
    global rcv_body
    if rcv_body:
        # save current array calibration
        array_calib_save_file(rcv_body)
        
        rcv_close(rcv_body)
        rcv_body = None
        for i, btn in enumerate(bar.panel.winfo_children()):
            btn.configure(state=DISABLED if i in (1,) else NORMAL)
        set_settings_btn_state(rcv_page, NORMAL)
        status_bar_show('Receiver stopped.')

# Load settings button push callback -------------------------------------------
def on_settings_load():
    if rcv_body:
        return
    file = filedialog.askopenfilename(parent=root, title='Load Settings',
        initialdir=AP_DIR, initialfile='pocket_sdr_settings.ini',
        filetypes=(('INI files', '*.ini'), ('All files', '*.*')))
    if file == '':
        return
    sdr_opt.load_opts(file, inp_opt, out_opt, sig_opt, sys_opt, array_opt)
    status_bar_show('Settings loaded. ' + file)

# Save settings button push callback -------------------------------------------
def on_settings_save():
    if rcv_body:
        return
    file = filedialog.asksaveasfilename(parent=root, title='Save Settings',
        initialdir=AP_DIR, initialfile='pocket_sdr_settings.ini',
        defaultextension='.ini',
        filetypes=(('INI files', '*.ini'), ('All files', '*.*')))
    if file == '':
        return
    sdr_opt.save_opts(file, inp_opt, out_opt, sig_opt, sys_opt, array_opt)
    status_bar_show('Settings saved. ' + file)

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

# Array button push callback ---------------------------------------------------
def on_btn_array_push(bar):
    global array_opt
    if not rcv_body:
        array_opt = sdr_opt.array_opt_dlg(root, array_opt,
            geom_load=array_geom_load_file, geom_save=array_geom_save_file)

# Help button push callback ----------------------------------------------------
def on_btn_help_push(bar):
    help_dlg(root)

# Exit button push callback ----------------------------------------------------
def on_btn_exit_push(bar):
    if not rcv_body:
        sdr_opt.save_opts(OPTS_FILE, inp_opt, out_opt, sig_opt, sys_opt,
            array_opt)
        exit()

# root Window close callback ---------------------------------------------------
def on_root_close():
    if rcv_body:
        rcv_close(rcv_body)
    exit()

# pages change callback --------------------------------------------------------
def on_pages_change(e, pages):
    i = e.widget.index('current')
    update_page(i, pages[i])

# pages interval timer callback ------------------------------------------------
def on_pages_timer(note, pages):
    tt = time.time()
    update_rcv_log()
    update_sol_log()
    ti = pages_update(note, pages)
    if not rcv_body: ti = UD_CYCLE3
    ts = (int)((time.time() - tt) * 1e3)
    delay = ti - ts if ti > ts else ti
    note.after(max(10, delay), lambda: on_pages_timer(note, pages))

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
    funcs = (update_rcv_page, update_rfch_page, update_bbch_page,
        update_corr_page, update_sats_page, update_sol_page, update_array_page,
        update_log_page)
    funcs[i](page)

# update pages -----------------------------------------------------------------
def pages_update(note, pages):
    global root_resize
    i = note.index('current')
    if not root_resize:
        update_page(i, pages[i])
        if i != 3:
            set_sel_ch(rcv_body, 0, 3e-6)
    text = 'Time: ' + get_rcv_stat(rcv_body).split()[0] + ' s'
    stat_bar.msg2.configure(text=text)
    return UD_CYCLE1 if i in (1, 3) else UD_CYCLE2 # update interval (ms)

# set styles -------------------------------------------------------------------
def set_styles():
    style = ttk.Style()
    if not 'Windows' in env:
        style.theme_use('clam')
    style.configure('TButton', font=get_font(1), background=BG_COLOR1)
    style.configure('TButton', padding=(0, 0))
    style.map('TButton', background=[(DISABLED, BG_COLOR1)])
    style.configure('TRadiobutton', font=get_font(), background=BG_COLOR1)
    style.configure('TLabel', font=get_font(), background=BG_COLOR1)
    style.map('TLabel', background=[(DISABLED, BG_COLOR1)])
    style.configure('TEntry', font=get_font(), background='white')
    style.configure('TCheckbutton', font=get_font(), background=BG_COLOR1)
    style.configure('w.TCheckbutton', font=get_font(), background='white')
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
def main():
    global sdr_opt, inp_opt, out_opt, sig_opt, sys_opt, array_opt
    global root, stat_bar

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
    array_opt = sdr_opt.array_opt_new()
    sdr_opt.load_opts(OPTS_FILE, inp_opt, out_opt, sig_opt, sys_opt, array_opt)
    
    # generate button bar
    labels = ('Start', 'Stop', 'Input ...', 'Output ...', 'Signal ...',
        'System ...', 'Array ...', 'Help ...', 'Exit')
    callbacks = (on_btn_start_push, on_btn_stop_push, on_btn_input_push,
        on_btn_output_push, on_btn_signal_push, on_btn_system_push,
        on_btn_array_push, on_btn_help_push, on_btn_exit_push)
    btn_bar = btn_bar_new(root, labels, callbacks)
    btn_bar.panel.winfo_children()[1].configure(state=DISABLED)
    
    # generate status bar
    stat_bar = status_bar_new(root)
    
    # generate receiver pages
    labels = ('Receiver', 'RF CH', 'BB CH', 'Correlator', 'Satellites',
        'Solution', 'Array', 'Log')
    note = ttk.Notebook(root, padding=0)
    note.pack(fill=BOTH)
    global rcv_page, array_page
    pages = (rcv_page_new(note), rfch_page_new(note),  bbch_page_new(note),
        corr_page_new(note), sats_page_new(note), sol_page_new(note),
        array_page_new(note), log_page_new(note))
    rcv_page = pages[0]  # used by settings load/save state control
    array_page = pages[6]  # used by skyplot click handler
    for i, page in enumerate(pages):
        note.add(page.panel, text=labels[i])
    note.after(100, lambda: on_pages_timer(note, pages))
    note.bind('<<NotebookTabChanged>>', lambda e: on_pages_change(e, pages))
    
    # main loop of Tk
    root.mainloop()

#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     pocket_sdr.py
# 
#   Description
# 
#     A GNSS SDR receiver with GUI (graphical user interface). To start the
#     receiver, push Start button. To stop the receiver, push Stop button.
#     To configure the receiver options, puth Input..., Output..., Signal...
#     or System... button and input settings and push OK on the options dialog. 
#     By pushing tab Receiver, RF CH, BB CH, Correlator, Satellites, Solution
#     or Log, the contents of the receiver internal status can be switched.
#     To exit the receiver, puth Exit button. In this case, the receiver options
#     are saved as pocket_sdr.ini file in the same directory of the program.
#     The detaild instruction for the program, refer doc/pocket_sdr_py.pdf.
#
#   Options
#     
#     No option.
#
if __name__ == '__main__':
    main()
