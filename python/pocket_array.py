#!/usr/bin/env python3
#
#  Pocket SDR AP - Antenna Array Simulation
#
#  Author:
#  T.TAKASU
#
#  History:
#  2025-03-06  1.0  new
#
import sys, time
from math import *
import numpy as np
from tkinter import *
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from tkinter import *
from tkinter import ttk
import sdr_func, sdr_opt

# constans ---------------------------------------------------------------------
BG_COLOR = 'white'     # background color
FG_COLOR = '#555555'   # foreground color
GR_COLOR = '#CCCCCC'   # grid color
PLT_COLOR = 'red'      # plot color
FONT = {'family': 'Tahoma', 'size': 10, 'weight': 'normal', 'color': FG_COLOR}
D2R = pi / 180.0
TB_HEIGHT  = 25              # toolbar height

# antenna element positions ----------------------------------------------------
lam = 299792458.0 / 1.57542e9 # L1 wave length (m)
ANT_POS_1 = np.array([0.0, 0.0, 0]) # 1-element ant
ANT_POS_2 = lam / 2 * np.array([-1.5, 0.0, 0, # 4-element ULA (uniform linear)
    -0.5, 0.0, 0,  0.5, 0.0, 0,  1.5, 0.0, 0])
ANT_POS_3 = lam / 2 * np.array([-0.5, -0.5, 0, # 4-element square grid
    -0.5, 0.5, 0,  0.5, -0.5, 0,  0.5, 0.5, 0])
ANT_POS_4 = lam / 2 * np.array([-1.5, -0.5, 0, # 8-element square grid
    -0.5, -0.5, 0,  0.5, -0.5, 0,  1.5, -0.5, 0,
    -1.5, 0.5, 0, -0.5, 0.5, 0,  0.5, 0.5, 0,  1.5, 0.5, 0])
ANT_POS_5 = lam / 2 * 1.3 * np.array([ # 8-element UCA (uniform circular)
    sin(pi*0/8), cos(pi*0/8), 0, sin(pi*2/8), cos(pi*2/8), 0,
    sin(pi*4/8), cos(pi*4/8), 0, sin(pi*6/8), cos(pi*6/8), 0,
    sin(pi*8/8), cos(pi*8/8), 0, sin(pi*10/8), cos(pi*10/8), 0,
    sin(pi*12/8), cos(pi*12/8), 0, sin(pi*14/8), cos(pi*14/8), 0])
ANT_POS_6 = lam / 2 * np.array([0.0, 0.0, 0, # 7-element hexagnal
    sin(pi*0/6), cos(pi*0/6), 0, sin(pi*2/6), cos(pi*2/6), 0,
    sin(pi*4/6), cos(pi*4/6), 0, sin(pi*6/6), cos(pi*6/6), 0,
    sin(pi*8/6), cos(pi*8/6), 0, sin(pi*10/6), cos(pi*10/6), 0])
ANT_POSS = (ANT_POS_1, ANT_POS_2, ANT_POS_3, ANT_POS_4, ANT_POS_5, ANT_POS_6)

# general object class ---------------------------------------------------------
class Obj: pass

# get font ---------------------------------------------------------------------
def get_font(add_size=0, weight='normal', mono=0):
    return (FONT['family'], FONT['size'] + add_size, FONT['weight'])

# set font ---------------------------------------------------------------------
def set_font(font):
    global FONT
    FONT['family'] = font[0]
    FONT['size'] = font[1]
    FONT['weight'] = font[2]

# generate tool bar ------------------------------------------------------------
def tool_bar_new(parent, height=TB_HEIGHT):
    toolbar = Frame(parent, height=height, bg=BG_COLOR)
    toolbar.pack_propagate(0)
    toolbar.pack(fill=X)
    return toolbar

# generate selection box -------------------------------------------------------
def sel_box_new(parent, vals=[], val='', width=8):
    box = ttk.Combobox(parent, width=width, state='readonly', justify=CENTER,
        values=vals, height=min([len(vals), 32]), font=get_font())
    box.set(val)
    return box

# draw skyplot mask ------------------------------------------------------------
def draw_mask(ax):
    p1 = [(sin(az * D2R), cos(az * D2R)) for az in np.arange(0, 363, 3)]
    p2 = ((0.0, 1.1), (-1.1, 1.1), (-1.1, -1.1), (1.1, -1.1), (1.1, 1.1), (0.0, 1.1))
    ax.add_patch(plt.Polygon(np.vstack([p1, p2]), facecolor=BG_COLOR))

# draw skyplot -----------------------------------------------------------------
def draw_sky(ax, d_az=30, d_el=30, label=0):
    for az in np.arange(0, 360, d_az):
        x = sin(az * D2R)
        y = cos(az * D2R)
        ax.plot([0, x], [0, y], '-', color=GR_COLOR, lw=0.8, alpha=0.5)
        if label and az % 30 == 0:
            text = ('%.0f' % (az)) if az % 90 else 'NESW'[az//90]
            ax.text(x * 1.05, y * 1.05, text, ha='center', va='center',
                rotation=-az, fontdict=FONT)
    for el in np.arange(0, 90, d_el):
        x = [(90 - el) / 90 * sin(az * D2R) for az in np.arange(0, 363, 3)]
        y = [(90 - el) / 90 * cos(az * D2R) for az in np.arange(0, 363, 3)]
        if el == 0:
            ax.plot(x, y, '-', color=FG_COLOR, lw=0.8)
        else:
            ax.plot(x, y, '-', color=GR_COLOR, lw=0.8, alpha=0.5)

# antenna element gain ---------------------------------------------------------
ELE_GAIN_ON = False     # apply elevation-dependent element gain

def ant_ele_gain(el):
    if not ELE_GAIN_ON: return 0.0
    if   el >  pi / 2: el = pi - el
    elif el < -pi / 2: el = -pi - el
    return 1 - 2 ** ((90 - el / D2R) / 30) # (dB)
    
# steering vector a_i(az,el) = exp(2j pi pos_i . e(az,el) / lam) -------------
def steering_vec(lam, pos, az, el):
    n = len(pos) // 3
    es = np.array((sin(az) * cos(el), cos(az) * cos(el), sin(el)))
    a = np.empty(n, dtype=complex)
    for i in range(n):
        a[i] = np.exp(2j * pi * np.dot(pos[i*3:i*3+3], es) / lam)
    return a

# complex RFCH weights (classic beamformer, MVDR or PI) ----------------------
#   jammers: list of (az,el) in radians. empty -> classic.
#   slider_amp: per-channel amplitude (used only when no jammers)
#   mode: 'MVDR' (signal direction unity) or 'PI' (reference antenna unity)
#   returns complex weight w such that sig(e) = sum_i w_i * exp(2j pi pos_i.e/lam)
def compute_weights(lam, pos, az_s, el_s, slider_amp, jammers, mode='MVDR'):
    n = len(pos) // 3
    if n == 0:
        return np.array([], dtype=complex)
    a_s = steering_vec(lam, pos, az_s, el_s)
    if not jammers:
        return np.array([slider_amp[i] * np.conj(a_s[i]) for i in range(n)])
    # element-gain-weighted covariance: R = I + J sum_k g_v(el_k)^2 a_k a_k^H
    # (received jammer power scales by element pattern, so low-el jammers get
    #  proportionally less null-suppression weight)
    J = 1e4
    R = np.eye(n, dtype=complex)
    for az, el in jammers:
        a_j = steering_vec(lam, pos, az, el)
        g_j = 10.0 ** (ant_ele_gain(el) / 20.0) # voltage gain
        R += J * (g_j * g_j) * np.outer(a_j, np.conj(a_j))
    # constraint c: MVDR -> effective signal steering (g_s a_s), PI -> ref element
    g_s = 10.0 ** (ant_ele_gain(el_s) / 20.0)
    if mode == 'PI':
        c = np.zeros(n, dtype=complex)
        c[0] = 1.0
    else:
        c = g_s * a_s
    # w = R^-1 c / (c^H R^-1 c)
    Rinv_c = np.linalg.solve(R, c)
    w = Rinv_c / (np.conj(c) @ Rinv_c)
    # MVDR: scale = N * g_s so signal-dir array factor = N (matches classic);
    #       total sky gain at signal dir = 20 log10(N) + ant_ele_gain(el_s).
    # PI  : no scaling, so the reference element weight (CH1) stays at unity.
    scale = 1.0 if mode == 'PI' else n * g_s
    return np.conj(w) * scale

# antenna gain ----------------------------------------------------------------
#   weight: complex ndarray (N), sig(e) = sum_i weight_i * exp(2j pi pos_i.e/lam)
def ant_gain(lam, pos, weight, a, e):
    n = len(weight)
    es = np.array((sin(a) * cos(e), cos(a) * cos(e), sin(e)))
    sig = 0j
    for i in range(n):
        phi = np.dot(pos[i*3:i*3+3], es) / lam
        sig += weight[i] * np.exp(2j * pi * phi)
    return 20.0 * log10(abs(sig) + 1e-30) + ant_ele_gain(e) # (dB)

# plot antenna gain contour in skyplot -----------------------------------------
def plot_sky(ax, lam, pos, weight, az, el, bins, jammers, show_beam=True):
    ax.axis('off')
    ax.set_aspect('equal')
    ax.set_xlim(-1.05, 1.05)
    ax.set_ylim(-1.09, 1.01)
    dx = 0.025
    x = np.arange(-1.0, 1.0 + dx, dx)
    y = np.arange(-1.0, 1.0 + dx, dx)
    z = np.zeros([len(y), len(x)])
    for i in range(len(x)):
        for j in range(len(y)):
            a = atan2(x[i], y[j])
            e = (1.0 - sqrt(x[i]**2 + y[j]**2)) * pi / 2.0
            z[j][i] = ant_gain(lam, pos, weight, a, e)
    cont = ax.contourf(x, y, z, bins, extend='both', cmap='jet')
    draw_mask(ax)
    draw_sky(ax, d_el=15, label=1)
    if show_beam:
        d = 1.0 - el / pi * 2
        xb = d * sin(az)
        yb = d * cos(az)
        if d > 0.1:
            xa = (d - 0.1) / d * xb
            ya = (d - 0.1) / d * yb
            ax.arrow(0, 0, xa, ya, lw=2, head_width=0.06, head_length=0.11,
                ec=BG_COLOR, fc=BG_COLOR)
            ax.arrow(0, 0, xa, ya, head_width=0.05, head_length=0.1, ec=PLT_COLOR,
                fc=PLT_COLOR)
        ax.plot(0, 0, '.', color=BG_COLOR, ms=14)
        ax.plot(0, 0, '.', color=PLT_COLOR, ms=10)
        ax.plot(xb, yb, 'X', color=BG_COLOR, ms=15, mew=3)
        ax.plot(xb, yb, 'X', color=PLT_COLOR, ms=11, mew=1.5,
            markeredgecolor=BG_COLOR)
    for az_j, el_j in jammers:
        dj = 1.0 - el_j / pi * 2
        xj = dj * sin(az_j)
        yj = dj * cos(az_j)
        ax.plot(xj, yj, 'X', color=BG_COLOR, ms=15, mew=3)
        ax.plot(xj, yj, 'X', color='black', ms=11, mew=1.5,
            markeredgecolor=BG_COLOR)
    return cont

# plot antenna gain plot in cross-section plot ---------------------------------
def plot_gain(ax, lam, pos, weight, az, el, bins):
    ax.axis('off')
    ax.set_aspect('equal')
    ax.set_xlim(-1.05, 1.05)
    ax.set_ylim(-0.15, 1.05)
    e = np.arange(-180.0, 180.0, 1.0) * D2R
    x = np.zeros(len(e))
    y = np.zeros(len(e))
    rng = bins[-1] - bins[0]
    for i in range(len(e)):
        p = ant_gain(lam, pos, weight, az, e[i])
        x[i] = np.max([0.0, (p - bins[0]) / rng]) * cos(e[i])
        y[i] = np.max([0.0, (p - bins[0]) / rng]) * sin(e[i])
    ax.plot(x, y, color=PLT_COLOR, lw=0.8)
    draw_sky(ax, d_el=90 / (rng / 5))
    for p in np.arange(bins[0], bins[-1], 10):
        ax.text((p - bins[0]) / rng, -0.15, '%.0f' % (p), ha='center', va='top', fontdict=FONT)
        ax.text((bins[0] - p) / rng, -0.15, '%.0f' % (p), ha='center', va='top', fontdict=FONT)
    for e in range(0, 91, 30):
        ax.text(1.07 * cos(e * D2R), 1.07 * sin(e * D2R), str(e), ha='center',
            va='center', rotation=e-90, fontdict=FONT)
    ax.plot([-1.05, 1.05], [0, 0], color=FG_COLOR, lw=0.8)
    ax.plot(0, 0, '.', color=PLT_COLOR, ms=10)
    ax.plot(cos(el), sin(el), '.', color=PLT_COLOR, ms=10)
    ax.arrow(0, 0, 0.9 * cos(el), 0.9 * sin(el), head_width=0.05,
        head_length=0.1, ec=PLT_COLOR, fc=PLT_COLOR)

# plot antenna element positions -----------------------------------------------
def plot_pos(ax, lam, pos):
    ax.set_aspect('equal')
    ax.set_xlim(-lam - 1e-3, lam + 1e-3)
    ax.set_ylim(-lam - 1e-3, lam + 1e-3)
    ax.axis('off')
    for x in lam * np.arange(-2, 2.1, 0.5):
        ax.plot([-lam, lam], [x, x], color=GR_COLOR, lw=0.8);
        ax.plot([x, x], [-lam, lam], color=GR_COLOR, lw=0.8);
    for i in range(len(pos) // 3):
        x = pos[i*3]
        y = pos[i*3+1]
        ax.plot(x, y, 'o', color=PLT_COLOR, markerfacecolor=BG_COLOR, ms=20, lw=0.8)
        ax.text(x, y, '%d' % (i + 1), ha='center', color=PLT_COLOR, va='center',
            fontdict=FONT)
    ax.text(0, -0.16, 'Antenna Element Positions', ha='center', va='top',
        fontdict=FONT)

# add color bar ----------------------------------------------------------------
def add_colorbar(fig, rect, cont, bins):
    cax = fig.add_axes(rect)
    bar = fig.colorbar(cont, cax=cax, orientation='horizontal')
    bar.set_ticks(np.arange(bins[0], bins[-1] + 0.5, 10))
    bar.ax.tick_params(color=FG_COLOR, labelcolor=FG_COLOR,
        labelfontfamily=FONT['family'])
    bar.lbl = fig.text(rect[0] + rect[2] + 0.03, rect[1] + rect[3] / 2,
        '(dB)', ha='left', va='center', fontdict=FONT)
    return bar
    
# plot antenna array -----------------------------------------------------------
def plot_array(p, type, weight, az_s, el_s, ant_pos, jammers, show_beam=True):
    bins = np.arange(-40, 20.05, 0.1)
    p.ax.cla()
    if p.bar != None:
        p.bar.ax.remove()
        p.bar.lbl.remove()
    if type == 'POS':
        plot_pos(p.ax, lam, ant_pos)
    elif type == 'GAIN':
        plot_gain(p.ax, lam, ant_pos, weight, az_s, el_s, bins)
    elif type == 'SKY':
        cont = plot_sky(p.ax, lam, ant_pos, weight, az_s, el_s, bins, jammers,
            show_beam)
        p.bar = add_colorbar(p.fig, [0.14, 0.08, 0.72, 0.025], cont, bins)
    p.canvas.draw()
    p.canvas.get_tk_widget().pack(fill=BOTH, expand=1)

# generate array plot -----------------------------------------------------------
def plot_new(parent, width, height):
    p = Obj()
    p.c = Frame(parent, width=width, height=height, bg=BG_COLOR)
    p.c.pack_propagate(False)
    p.fig = plt.figure()
    p.ax = p.fig.add_axes([0.08, 0.05, 0.84, 0.9])
    p.bar = None
    p.canvas = FigureCanvasTkAgg(p.fig, master=p.c)
    return p

# generate Array page ----------------------------------------------------------
def array_page_new(parent):
    p = Obj()
    p.parent = parent
    p.panel = Frame(parent)
    p.panel.pack(fill=BOTH, expand=1)
    p.azel = [DoubleVar() for i in range(2)]
    p.weight = [DoubleVar(value=1) for i in range(16)]
    p.toolbar = tool_bar_new(p.panel)
    p.txt1 = ttk.Label(p.toolbar, text='0\xb0', width=3, anchor=E)
    p.txt1.pack(side=RIGHT, padx=(1, 6))
    ttk.Label(p.toolbar, text='EL').pack(side=RIGHT, padx=(6, 1))
    p.txt2 = ttk.Label(p.toolbar, text='0\xb0', width=4, anchor=E)
    p.txt2.pack(side=RIGHT, padx=1)
    ttk.Label(p.toolbar, text='BEAM DIR  AZ').pack(side=RIGHT, padx=(6, 1))
    ttk.Label(p.toolbar, text='ANT POS').pack(side=LEFT, padx=(10, 4))
    p.box1 = sel_box_new(p.toolbar, vals=[str(i + 1) for i in range(len(ANT_POSS))],
        val='1', width=3)
    p.box1.pack(side=LEFT)
    ttk.Label(p.toolbar, text='JAMMER CNT').pack(side=LEFT, padx=(10, 4))
    p.box2 = sel_box_new(p.toolbar, vals=[str(i) for i in range(8)], val='0', width=3)
    p.box2.pack(side=LEFT)
    ttk.Label(p.toolbar, text='MODE').pack(side=LEFT, padx=(10, 4))
    p.box3 = sel_box_new(p.toolbar, vals=['MVDR', 'PI'], val='MVDR', width=5)
    p.box3.pack(side=LEFT)
    ttk.Label(p.toolbar, text='ELE GAIN').pack(side=LEFT, padx=(10, 4))
    p.box4 = sel_box_new(p.toolbar, vals=['ON', 'OFF'],
        val='ON' if ELE_GAIN_ON else 'OFF', width=4)
    p.box4.pack(side=LEFT)
    p.toolbar2 = tool_bar_new(p.panel, height=48)
    ttk.Label(p.toolbar2, text='RFCH\nWEIGHT', justify=CENTER).pack(side=LEFT, padx=(10, 4))
    p.w_txt = [None] * 8
    for i in range(8):
        idx = 7 - i
        frm = Frame(p.toolbar2, bg=BG_COLOR, width=85, height=46)
        frm.pack_propagate(False)
        frm.pack(side=RIGHT, padx=(1, 4))
        row = Frame(frm, bg=BG_COLOR)
        row.pack(side=TOP)
        ttk.Label(row, text=str(idx + 1) + ':').pack(side=LEFT)
        Scale(row, variable=p.weight[idx], to=1, orient='horizontal',
            length=55, sliderlength=20, showvalue=False, resolution=0.05,
            bg=BG_COLOR,
            command=lambda e: on_weight_change(e, p)).pack(side=LEFT)
        p.w_txt[idx] = ttk.Label(frm, text='', anchor=CENTER)
        p.w_txt[idx].pack(side=TOP, fill=X)
    p.plt1 = plot_new(p.panel, 320, 200)
    p.plt1.ax.set_position([0.08, 0.14, 0.84, 0.82])
    p.gain_txt = p.plt1.fig.text(0.91, 0.95, '', ha='right', va='top',
        fontdict={**FONT, 'size': FONT['size'] + 4})
    p.plt1.c.pack(side=LEFT, expand=1, fill=BOTH, padx=2, pady=2)
    p.panel1 = Frame(p.panel)
    p.panel1.pack(side=LEFT, expand=1, fill=BOTH)
    p.plt2 = plot_new(p.panel1, 100, 100)
    p.plt2.c.pack(expand=1, fill=BOTH, padx=2, pady=2)
    p.plt3 = plot_new(p.panel1, 100, 100)
    p.plt3.c.pack(expand=1, fill=BOTH, padx=2, pady=2)
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_pos_select(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_jam_cnt_change(e, p))
    p.box3.bind('<<ComboboxSelected>>', lambda e: update_plt(p))
    p.box4.bind('<<ComboboxSelected>>', lambda e: on_ele_gain_change(e, p))
    p.panel.bind("<Configure>", lambda e: on_plt_configure(e, p))
    p.plt1.fig.canvas.mpl_connect('button_press_event',   lambda e: on_sky_press  (e, p))
    p.plt1.fig.canvas.mpl_connect('motion_notify_event',  lambda e: on_sky_motion (e, p))
    p.plt1.fig.canvas.mpl_connect('button_release_event', lambda e: on_sky_release(e, p))
    p.plt1.fig.canvas.mpl_connect('axes_leave_event',     lambda e: on_sky_leave  (e, p))
    p.sky_drag = None        # None | ('beam',) | ('jammer', k)
    p.jammers = []           # list of [az_rad, el_rad]
    p.weight_last = np.array([], dtype=complex)  # cached weights for cursor gain
    p.ant_pos = ANT_POSS[0]
    return p

# antenna position select callback ---------------------------------------------
def on_pos_select(e, p):
    p.ant_pos = ANT_POSS[int(p.box1.get())-1]
    update_plt(p)

# element gain ON/OFF change callback -----------------------------------------
def on_ele_gain_change(e, p):
    global ELE_GAIN_ON
    ELE_GAIN_ON = (p.box4.get() == 'ON')
    update_plt(p)

# jammer count change callback ------------------------------------------------
def on_jam_cnt_change(e, p):
    n = int(p.box2.get())
    while len(p.jammers) < n:
        k = len(p.jammers)
        p.jammers.append([k * 360.0 / max(n, 1) * D2R, 30.0 * D2R])
    p.jammers = p.jammers[:n]
    update_plt(p)

# plots configure callback -----------------------------------------------------
def on_plt_configure(e, p):
    update_plt(p)

# weight change callback -------------------------------------------------------
def on_weight_change(e, p):
    update_plt(p)

# nearest jammer index within pixel threshold --------------------------------
def find_jammer(p, xdata, ydata, thresh=0.08):
    for k, (az, el) in enumerate(p.jammers):
        d = 1.0 - el / pi * 2
        dx = d * sin(az) - xdata
        dy = d * cos(az) - ydata
        if sqrt(dx * dx + dy * dy) < thresh:
            return k
    return -1

# skyplot click/drag -> set beam AZ/EL or jammer AZ/EL ------------------------
def set_sky_pos_from_event(e, p):
    if p.sky_drag is None:
        return
    if e.inaxes != p.plt1.ax or e.xdata is None or e.ydata is None:
        return
    x, y = e.xdata, e.ydata
    r = sqrt(x * x + y * y)
    if r > 1.0:
        return
    az = atan2(x, y)
    if az < 0: az += 2 * pi
    el = (1.0 - r) * pi / 2
    if p.sky_drag[0] == 'beam':
        p.azel[0].set(az / D2R)
        p.azel[1].set(el / D2R)
        p.txt1.configure(text='%.0f\xb0' % (el / D2R))
        p.txt2.configure(text='%.0f\xb0' % (az / D2R))
    else:  # 'jammer'
        p.jammers[p.sky_drag[1]] = [az, el]
    update_plt(p)

def on_sky_press(e, p):
    if e.button != 1 or e.inaxes != p.plt1.ax:
        return
    if e.xdata is None or e.ydata is None:
        return
    k = find_jammer(p, e.xdata, e.ydata)
    if k >= 0:
        p.sky_drag = ('jammer', k)
    else:
        p.sky_drag = ('beam',)
    set_sky_pos_from_event(e, p)

def on_sky_motion(e, p):
    if p.sky_drag is not None:
        set_sky_pos_from_event(e, p)
    else:
        update_cursor_gain(e, p)

def on_sky_release(e, p):
    p.sky_drag = None

def on_sky_leave(e, p):
    if p.gain_txt.get_text():
        p.gain_txt.set_text('')
        p.plt1.fig.canvas.draw_idle()

# cursor hover -> show gain at cursor point -----------------------------------
def update_cursor_gain(e, p):
    text = ''
    if (e.inaxes == p.plt1.ax and e.xdata is not None and e.ydata is not None
            and len(p.weight_last) > 0):
        x, y = e.xdata, e.ydata
        r = sqrt(x * x + y * y)
        if r <= 1.0:
            az = atan2(x, y)
            el = (1.0 - r) * pi / 2
            gain = ant_gain(lam, p.ant_pos, p.weight_last, az, el)
            text = '%.1f dB' % gain
    if p.gain_txt.get_text() != text:
        p.gain_txt.set_text(text)
        p.plt1.fig.canvas.draw_idle()

# update complex weight labels ------------------------------------------------
def update_weight_text(p, weight):
    n = len(weight)
    for i in range(8):
        if i < n:
            w = weight[i]
            p.w_txt[i].configure(text='%+.2f%+.2fj' % (w.real, w.imag))
        else:
            p.w_txt[i].configure(text='---')

# update plots -----------------------------------------------------------------
def update_plt(p):
    az_s = p.azel[0].get() * D2R
    el_s = p.azel[1].get() * D2R
    slider_amp = [p.weight[i].get() for i in range(8)]
    mode = p.box3.get()
    show_beam = not (mode == 'PI' and p.jammers)
    weight = compute_weights(lam, p.ant_pos, az_s, el_s, slider_amp, p.jammers,
        mode)
    p.weight_last = weight
    plot_array(p.plt1, 'SKY',  weight, az_s, el_s, p.ant_pos, p.jammers, show_beam)
    plot_array(p.plt2, 'GAIN', weight, az_s, el_s, p.ant_pos, p.jammers, show_beam)
    plot_array(p.plt3, 'POS',  weight, az_s, el_s, p.ant_pos, p.jammers, show_beam)
    update_weight_text(p, weight)

# root Window close callback ---------------------------------------------------
def on_root_close():
    exit()

# set styles -------------------------------------------------------------------
def set_styles():
    style = ttk.Style()
    style.configure('TLabel', font=get_font(), background=BG_COLOR)
    style.configure('TScale', background=BG_COLOR)
    style.configure('TCombobox', font=get_font(), background=BG_COLOR)

# main -------------------------------------------------------------------------
if __name__ == '__main__':
    
    # generate root window
    root = Tk()
    root.geometry('%dx%d' % (800, 600))
    root.title('ANTENNA ARRAY SIMULATION')
    root.protocol("WM_DELETE_WINDOW", on_root_close)
    
    # set styles
    set_styles()
   
    # generate array pages
    p = array_page_new(root)
    
    # main loop of Tk
    root.mainloop()
