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
from tkinter import *
from tkinter import ttk
import sdr_func, sdr_opt, sdr_plot

# constants --------------------------------------------------------------------
BG_COLOR = 'white'
FG_COLOR = '#555555'
GR_COLOR = '#CCCCCC'
PLT_COLOR = 'red'
FONT = {'family': 'Tahoma', 'size': 10, 'weight': 'normal', 'color': FG_COLOR}
PLOT_FONT = (FONT['family'], FONT['size'] + 2, FONT['weight'])
D2R = pi / 180.0
TB_HEIGHT = 25
CONTOUR_M = 241

# antenna element positions ----------------------------------------------------
lam = 299792458.0 / 1.57542e9 # L1 wave length (m)
ANT_POS_1 = np.array([0.0, 0.0, 0]) # 1-element
ANT_POS_2 = lam / 2 * np.array([-1.5, 0.0, 0, # 4-element ULA
    -0.5, 0.0, 0,  0.5, 0.0, 0,  1.5, 0.0, 0])
ANT_POS_3 = lam / 2 * np.array([-0.5, -0.5, 0, # 4-element square
    -0.5, 0.5, 0,  0.5, -0.5, 0,  0.5, 0.5, 0])
ANT_POS_4 = lam / 2 * np.array([-1.5, -0.5, 0, # 8-element square
    -0.5, -0.5, 0,  0.5, -0.5, 0,  1.5, -0.5, 0,
    -1.5, 0.5, 0, -0.5, 0.5, 0,  0.5, 0.5, 0,  1.5, 0.5, 0])
ANT_POS_5 = lam / 2 * 1.3 * np.array([ # 8-element UCA
    sin(pi*0/8), cos(pi*0/8), 0, sin(pi*2/8), cos(pi*2/8), 0,
    sin(pi*4/8), cos(pi*4/8), 0, sin(pi*6/8), cos(pi*6/8), 0,
    sin(pi*8/8), cos(pi*8/8), 0, sin(pi*10/8), cos(pi*10/8), 0,
    sin(pi*12/8), cos(pi*12/8), 0, sin(pi*14/8), cos(pi*14/8), 0])
ANT_POS_6 = lam / 2 * np.array([0.0, 0.0, 0, # 7-element hexagonal
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

# antenna element gain ---------------------------------------------------------
ELE_GAIN_ON = False

def ant_ele_gain(el):
    if not ELE_GAIN_ON: return 0.0
    if   el >  pi / 2: el = pi - el
    elif el < -pi / 2: el = -pi - el
    return 1 - 2 ** ((90 - el / D2R) / 30) # (dB)

# steering vector a_i(az,el) = exp(2j pi pos_i . e(az,el) / lam) ---------------
def steering_vec(lam, pos, az, el):
    n = len(pos) // 3
    es = np.array((sin(az) * cos(el), cos(az) * cos(el), sin(el)))
    a = np.empty(n, dtype=complex)
    for i in range(n):
        a[i] = np.exp(2j * pi * np.dot(pos[i*3:i*3+3], es) / lam)
    return a

# complex RFCH weights (classic beamformer, MVDR or PI) ------------------------
def compute_weights(lam, pos, az_s, el_s, slider_amp, jammers, mode='MVDR'):
    n = len(pos) // 3
    if n == 0:
        return np.array([], dtype=complex)
    a_s = steering_vec(lam, pos, az_s, el_s)
    if not jammers:
        return np.array([slider_amp[i] * np.conj(a_s[i]) for i in range(n)])
    # element-gain-weighted covariance: low-el jammers get proportionally less
    # null-suppression weight than high-el ones
    J = 1e4
    R = np.eye(n, dtype=complex)
    for az, el in jammers:
        a_j = steering_vec(lam, pos, az, el)
        g_j = 10.0 ** (ant_ele_gain(el) / 20.0)
        R += J * (g_j * g_j) * np.outer(a_j, np.conj(a_j))
    g_s = 10.0 ** (ant_ele_gain(el_s) / 20.0)
    if mode == 'PI':  # reference element (CH1) weight stays at unity
        c = np.zeros(n, dtype=complex)
        c[0] = 1.0
    else:             # MVDR: signal direction array factor = N
        c = g_s * a_s
    Rinv_c = np.linalg.solve(R, c)
    w = Rinv_c / (np.conj(c) @ Rinv_c)
    scale = 1.0 if mode == 'PI' else n * g_s
    return np.conj(w) * scale

# antenna gain at single (az, el) ----------------------------------------------
def ant_gain(lam, pos, weight, a, e):
    n = len(weight)
    es = np.array((sin(a) * cos(e), cos(a) * cos(e), sin(e)))
    sig = 0j
    for i in range(n):
        phi = np.dot(pos[i*3:i*3+3], es) / lam
        sig += weight[i] * np.exp(2j * pi * phi)
    return 20.0 * log10(abs(sig) + 1e-30) + ant_ele_gain(e) # (dB)

# vectorized antenna gain over a square grid ----------------------------------
# samples at pixel centers ((2j+1)/M - 1)*half — matches plot_image's render
# convention so the heatmap aligns with X marks regardless of canvas size
def ant_gain_grid(lam, pos, weight, M, half=1.0):
    n = len(weight)
    pos_arr = np.asarray(pos, dtype=float).reshape(-1, 3)[:n]
    g = ((2 * np.arange(M, dtype=float) + 1) / M - 1) * half
    X, Y = np.meshgrid(g, g)
    R = np.sqrt(X * X + Y * Y)
    az = np.arctan2(X, Y)
    el = np.maximum(0.0, (1.0 - np.minimum(R, 1.0)) * pi / 2)
    ce = np.cos(el)
    es = np.stack([np.sin(az) * ce, np.cos(az) * ce, np.sin(el)], axis=-1)
    if n == 0:
        return np.full((M, M), -100.0)
    phi = (es @ pos_arr.T) / lam
    sig = np.sum(weight[None, None, :] * np.exp(2j * pi * phi), axis=-1)
    gain = 20.0 * np.log10(np.abs(sig) + 1e-30)
    if ELE_GAIN_ON:
        gain = gain + (1.0 - 2.0 ** ((90.0 - el / D2R) / 30.0))
    return gain

# X marker on tk canvas (with white halo) -------------------------------------
def plot_x_mark(plt_obj, x, y, color, halo=BG_COLOR, size_px=11):
    xs, _ = sdr_plot.plot_scale(plt_obj)
    if xs <= 0: return
    d = (size_px / 2.0) / xs
    sdr_plot.plot_poly(plt_obj, [x - d, x + d], [y - d, y + d], color=halo, width=8)
    sdr_plot.plot_poly(plt_obj, [x - d, x + d], [y + d, y - d], color=halo, width=8)
    sdr_plot.plot_poly(plt_obj, [x - d, x + d], [y - d, y + d], color=color, width=4)
    sdr_plot.plot_poly(plt_obj, [x - d, x + d], [y + d, y - d], color=color, width=4)

# arrow with head triangle on tk canvas ---------------------------------------
def plot_arrow(plt_obj, x0, y0, x1, y1, color, width=2, head_px=12):
    sdr_plot.plot_poly(plt_obj, [x0, x1], [y0, y1], color=color, width=width)
    xs, _ = sdr_plot.plot_scale(plt_obj)
    if xs <= 0: return
    h = head_px / xs
    dx, dy = x1 - x0, y1 - y0
    L = sqrt(dx * dx + dy * dy)
    if L < 1e-9: return
    ux, uy = dx / L, dy / L
    px, py = -uy, ux
    bx, by = x1 - h * ux, y1 - h * uy
    sdr_plot.plot_poly(plt_obj,
        [x1, bx + 0.45 * h * px, bx - 0.45 * h * px, x1],
        [y1, by + 0.45 * h * py, by - 0.45 * h * py, y1],
        color=color, width=width, fill=1)

# horizontal colorbar in the bottom margin of a tk plot -----------------------
def draw_colorbar_tk(plt_obj, vmin, vmax, lut=None, tag=''):
    if lut is None: lut = sdr_plot.jet_lut()
    w = plt_obj.c.winfo_width()
    h = plt_obj.c.winfo_height()
    if w <= 1 or h <= 1: return
    bar_h = 12
    bar_y = h - 40
    bar_x0 = plt_obj.m[0] + 30
    bar_x1 = w - plt_obj.m[1] - 30
    if bar_x1 - bar_x0 < 30: return
    n = len(lut)
    width = bar_x1 - bar_x0
    for i, c in enumerate(lut):
        x0 = bar_x0 + width * i / n
        x1 = bar_x0 + width * (i + 1) / n
        plt_obj.c.create_rectangle(x0, bar_y, x1 + 1, bar_y + bar_h,
            outline='', fill=c, tag=tag)
    plt_obj.c.create_rectangle(bar_x0, bar_y, bar_x1, bar_y + bar_h,
        outline=FG_COLOR, tag=tag)
    rng = vmax - vmin
    v0 = ceil(vmin / 10.0) * 10
    for v in np.arange(v0, vmax + 0.5, 10):
        x = bar_x0 + width * (v - vmin) / rng
        plt_obj.c.create_line(x, bar_y + bar_h, x, bar_y + bar_h + 3,
            fill=FG_COLOR, tag=tag)
        plt_obj.c.create_text(x, bar_y + bar_h + 4, text='%.0f' % v,
            fill=FG_COLOR, anchor=N, font=PLOT_FONT, tag=tag)
    plt_obj.c.create_text(bar_x1 + 6, bar_y + bar_h / 2.0, text='(dB)',
        fill=FG_COLOR, anchor=W, font=PLOT_FONT, tag=tag)

# tk canvas pixel -> data coordinate (skyplot) --------------------------------
def sky_pixel_to_data(plt_obj, xp, yp):
    xs, ys = sdr_plot.plot_scale(plt_obj)
    if xs <= 0 or ys <= 0: return None, None
    xc = plt_obj.m[0] + (plt_obj.c.winfo_width () - plt_obj.m[0] - plt_obj.m[1]) / 2
    yc = plt_obj.m[2] + (plt_obj.c.winfo_height() - plt_obj.m[2] - plt_obj.m[3]) / 2
    x = (xp - xc) / xs + (plt_obj.xl[0] + plt_obj.xl[1]) / 2
    y = (plt_obj.yl[0] + plt_obj.yl[1]) / 2 - (yp - yc) / ys
    return x, y

# plot antenna gain heatmap on the skyplot ------------------------------------
def plot_sky_skyplot(plt_obj, lam, pos, weight, az_s, el_s, vmin, vmax,
    jammers, show_beam=True):
    sdr_plot.plot_clear(plt_obj)
    # match the compute grid to plot_image's actual rendered extent so
    # heatmap pixels align with X marks (avoids horizontal stretching when
    # plot_image's z-rounding gives actual_half > 1). The 1e-9 shrink keeps
    # plot_image's internal `ceil(2*half*xs/M)` from rounding 2.0+1ulp up to z+1
    xs, _ = sdr_plot.plot_scale(plt_obj)
    if xs <= 0: return
    z = max(1, int(np.ceil(2.0 * xs / CONTOUR_M)))
    actual_half = CONTOUR_M * z / (2.0 * xs)
    gain = ant_gain_grid(lam, pos, weight, CONTOUR_M, half=actual_half)
    sdr_plot.plot_image(plt_obj, 0.0, 0.0, actual_half * (1 - 1e-9),
        gain[::-1], vmin, vmax)
    # large outer half so the corner polygons clip past the canvas regardless
    # of size or image zoom-rounding overflow
    sdr_plot.plot_sky_mask(plt_obj, 100.0, color=BG_COLOR, n_arc=24)
    for az_g in np.arange(0, 360, 30):
        x, y = sin(az_g * D2R), cos(az_g * D2R)
        sdr_plot.plot_poly(plt_obj, [0, x], [0, y], color=GR_COLOR)
        text = ('%.0f' % az_g) if az_g % 90 else 'NESW'[az_g // 90]
        sdr_plot.plot_text(plt_obj, x * 1.03, y * 1.03, text, color=FG_COLOR,
            anchor=S, angle=-az_g)
    for el_g in np.arange(0, 90, 15):
        sdr_plot.plot_circle(plt_obj, 0, 0, (90 - el_g) / 90,
            color=FG_COLOR if el_g < 5 else GR_COLOR)
    if show_beam:
        d = 1.0 - el_s / pi * 2
        xb = d * sin(az_s)
        yb = d * cos(az_s)
        if d > 0.1:
            xa = (d - 0.1) / d * xb
            ya = (d - 0.1) / d * yb
            plot_arrow(plt_obj, 0, 0, xa, ya, 'white', width=6, head_px=12)
            plot_arrow(plt_obj, 0, 0, xa, ya, PLT_COLOR, width=2, head_px=12)
        sdr_plot.plot_dots(plt_obj, [0], [0], color=PLT_COLOR, fill=PLT_COLOR,
            size=10)
        plot_x_mark(plt_obj, xb, yb, PLT_COLOR, halo=BG_COLOR, size_px=22)
    for az_j, el_j in jammers:
        dj = 1.0 - el_j / pi * 2
        plot_x_mark(plt_obj, dj * sin(az_j), dj * cos(az_j), 'black',
            halo=BG_COLOR, size_px=11)
    draw_colorbar_tk(plt_obj, vmin, vmax)

# vectorized antenna gain along a 1-D angle array (cross-section) -------------
def ant_gain_1d(lam, pos, weight, az, e_arr):
    n = len(weight)
    pos_arr = np.asarray(pos, dtype=float).reshape(-1, 3)[:n]
    if n == 0:
        return np.full(len(e_arr), -100.0)
    ce = np.cos(e_arr)
    es = np.stack([sin(az) * ce, cos(az) * ce, np.sin(e_arr)], axis=-1)
    phi = (es @ pos_arr.T) / lam
    sig = np.sum(weight[None, :] * np.exp(2j * pi * phi), axis=-1)
    g = 20.0 * np.log10(np.abs(sig) + 1e-30)
    if ELE_GAIN_ON:
        ee = np.where(e_arr >  pi / 2, pi - e_arr, e_arr)
        ee = np.where(ee   < -pi / 2, -pi - ee, ee)
        g = g + (1.0 - 2.0 ** ((90.0 - ee / D2R) / 30.0))
    return g

# plot antenna gain cross-section ---------------------------------------------
def plot_gain_tk(plt_obj, lam, pos, weight, az, el, vmin, vmax):
    sdr_plot.plot_clear(plt_obj)
    rng = vmax - vmin
    if rng <= 0: return
    e_arr = np.arange(-180.0, 180.0, 1.0) * D2R
    g = ant_gain_1d(lam, pos, weight, az, e_arr)
    mag = np.maximum(0.0, (g - vmin) / rng)
    x_curve = mag * np.cos(e_arr)
    y_curve = mag * np.sin(e_arr)
    for ag in np.arange(0, 360, 30):
        sdr_plot.plot_poly(plt_obj, [0, sin(ag * D2R)], [0, cos(ag * D2R)],
            color=GR_COLOR)
    n_circ = max(1, int(round(rng / 10)))
    for k in range(1, n_circ + 1):
        sdr_plot.plot_circle(plt_obj, 0, 0, k / n_circ,
            color=FG_COLOR if k == n_circ else GR_COLOR)
    sdr_plot.plot_poly(plt_obj, [-1.05, 1.05], [0, 0], color=FG_COLOR)
    sdr_plot.plot_poly(plt_obj, x_curve, y_curve, color=PLT_COLOR, width=2)
    sdr_plot.plot_dots(plt_obj, [0], [0], color=PLT_COLOR, fill=PLT_COLOR, size=10)
    sdr_plot.plot_dots(plt_obj, [cos(el)], [sin(el)], color=PLT_COLOR,
        fill=PLT_COLOR, size=10)
    plot_arrow(plt_obj, 0, 0, 0.9 * cos(el), 0.9 * sin(el), PLT_COLOR,
        width=2, head_px=12)
    #for db in np.arange(vmin, vmax + 0.1, 10):
    for db in np.arange(vmin, vmax + 0.1, 20):
        frac = (db - vmin) / rng
        sdr_plot.plot_text(plt_obj, frac, -0.05, '%.0f' % db, color=FG_COLOR,
            anchor=N)
        if frac > 0.001:
            sdr_plot.plot_text(plt_obj, -frac, -0.05, '%.0f' % db,
                color=FG_COLOR, anchor=N)
    for e_g in range(0, 91, 30):
        sdr_plot.plot_text(plt_obj, 1.08 * cos(e_g * D2R),
            1.08 * sin(e_g * D2R), str(e_g), color=FG_COLOR, anchor=CENTER,
            angle=e_g - 90)

# plot antenna element positions ----------------------------------------------
def plot_pos_tk(plt_obj, lam, pos):
    sdr_plot.plot_clear(plt_obj)
    n = len(pos) // 3
    for v in lam * np.arange(-2.0, 2.1, 0.5):
        sdr_plot.plot_poly(plt_obj, [-lam, lam], [v, v], color=GR_COLOR)
        sdr_plot.plot_poly(plt_obj, [v, v], [-lam, lam], color=GR_COLOR)
    r = 15 # px
    for i in range(n):
        xp, yp = sdr_plot.plot_pos(plt_obj, pos[i * 3], pos[i * 3 + 1])
        plt_obj.c.create_oval(xp - r, yp - r, xp + r, yp + r,
            outline=PLT_COLOR, fill=BG_COLOR, width=2)
        plt_obj.c.create_text(xp, yp, text=str(i + 1), fill=PLT_COLOR,
            font=PLOT_FONT)
    w = plt_obj.c.winfo_width()
    h = plt_obj.c.winfo_height()
    if w > 1 and h > 1:
        plt_obj.c.create_text(w / 2, h - 6, text='Antenna Element Positions',
            fill=FG_COLOR, anchor=S, font=PLOT_FONT)

# dispatch antenna array plot -------------------------------------------------
def plot_array(p, type, weight, az_s, el_s, ant_pos, jammers, show_beam=True):
    bins = np.arange(-40, 20.05, 0.1)
    if type == 'SKY':
        plot_sky_skyplot(p.plt, lam, ant_pos, weight, az_s, el_s, bins[0],
            bins[-1], jammers, show_beam)
    elif type == 'GAIN':
        plot_gain_tk(p.plt, lam, ant_pos, weight, az_s, el_s, bins[0],
            bins[-1])
    elif type == 'POS':
        plot_pos_tk(p.plt, lam, ant_pos)

# generate skyplot tk-canvas plot ---------------------------------------------
def plot_new_sky(parent, width, height):
    p = Obj()
    p.c = Frame(parent, width=width, height=height, bg=BG_COLOR)
    p.c.pack_propagate(False)
    p.plt = sdr_plot.plot_new(p.c, width=width, height=height,
        xlim=(-1.05, 1.05), ylim=(-1.05, 1.05),
        margin=(40, 40, 10, 40), aspect=1, tick=0, font=PLOT_FONT)
    p.plt.c.pack(fill=BOTH, expand=1)
    return p

# generate gain cross-section tk-canvas plot ----------------------------------
def plot_new_gain(parent, width, height):
    p = Obj()
    p.c = Frame(parent, width=width, height=height, bg=BG_COLOR)
    p.c.pack_propagate(False)
    # symmetric T/B margin + ylim centered on y=0.5 keeps the half-circle vertically centered
    p.plt = sdr_plot.plot_new(p.c, width=width, height=height,
        xlim=(-1.1, 1.1), ylim=(-1.1, 1.1),
        margin=(10, 10, 10, 10), aspect=1, tick=0, font=PLOT_FONT)
    p.plt.c.pack(fill=BOTH, expand=1)
    return p

# generate antenna position tk-canvas plot ------------------------------------
def plot_new_pos(parent, width, height):
    p = Obj()
    p.c = Frame(parent, width=width, height=height, bg=BG_COLOR)
    p.c.pack_propagate(False)
    p.plt = sdr_plot.plot_new(p.c, width=width, height=height,
        xlim=(-lam - 1e-3, lam + 1e-3),
        ylim=(-lam - 1e-3, lam + 1e-3),
        margin=(10, 10, 10, 24), aspect=1, tick=0, font=PLOT_FONT)
    p.plt.c.pack(fill=BOTH, expand=1)
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
    p.plt1 = plot_new_sky(p.panel, 320, 200)
    p.gain_lbl = ttk.Label(p.plt1.plt.c, text='', background=BG_COLOR,
        foreground=FG_COLOR,
        font=(FONT['family'], FONT['size'] + 4, FONT['weight']))
    p.gain_lbl.place(relx=1.0, x=-15, y=10, anchor='ne')
    p.plt1.c.pack(side=LEFT, expand=1, fill=BOTH, padx=2, pady=2)
    p.panel1 = Frame(p.panel)
    p.panel1.pack(side=LEFT, expand=1, fill=BOTH)
    p.plt2 = plot_new_gain(p.panel1, 100, 100)
    p.plt2.c.pack(expand=1, fill=BOTH, padx=2, pady=2)
    p.plt3 = plot_new_pos(p.panel1, 100, 100)
    p.plt3.c.pack(expand=1, fill=BOTH, padx=2, pady=2)
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_pos_select(e, p))
    p.box2.bind('<<ComboboxSelected>>', lambda e: on_jam_cnt_change(e, p))
    p.box3.bind('<<ComboboxSelected>>', lambda e: update_plt(p))
    p.box4.bind('<<ComboboxSelected>>', lambda e: on_ele_gain_change(e, p))
    p.plt1.plt.c.bind('<Configure>',       lambda e: schedule_update(p))
    p.plt2.plt.c.bind('<Configure>',       lambda e: schedule_update(p))
    p.plt3.plt.c.bind('<Configure>',       lambda e: schedule_update(p))
    p.plt1.plt.c.bind('<Button-1>',        lambda e: on_sky_press  (e, p))
    p.plt1.plt.c.bind('<B1-Motion>',       lambda e: on_sky_drag   (e, p))
    p.plt1.plt.c.bind('<ButtonRelease-1>', lambda e: on_sky_release(e, p))
    p.plt1.plt.c.bind('<Motion>',          lambda e: on_sky_motion (e, p))
    p.plt1.plt.c.bind('<Leave>',           lambda e: on_sky_leave  (e, p))
    p.sky_drag = None        # None | ('beam',) | ('jammer', k)
    p.jammers = []           # list of [az_rad, el_rad]
    p.weight_last = np.array([], dtype=complex)
    p.ant_pos = ANT_POSS[0]
    return p

# antenna position select callback --------------------------------------------
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

# coalesce per-canvas <Configure> events into one redraw per idle cycle -------
def schedule_update(p):
    if getattr(p, '_upd_pending', False): return
    p._upd_pending = True
    def run():
        p._upd_pending = False
        update_plt(p)
    p.parent.after_idle(run)

# weight change callback ------------------------------------------------------
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
def set_sky_pos_xy(p, x, y):
    if p.sky_drag is None: return
    r = sqrt(x * x + y * y)
    if r > 1.0: return
    az = atan2(x, y)
    if az < 0: az += 2 * pi
    el = (1.0 - r) * pi / 2
    if p.sky_drag[0] == 'beam':
        p.azel[0].set(az / D2R)
        p.azel[1].set(el / D2R)
        p.txt1.configure(text='%.0f\xb0' % (el / D2R))
        p.txt2.configure(text='%.0f\xb0' % (az / D2R))
    else:
        p.jammers[p.sky_drag[1]] = [az, el]
    update_plt(p)

# skyplot mouse press callback ------------------------------------------------
def on_sky_press(e, p):
    x, y = sky_pixel_to_data(p.plt1.plt, e.x, e.y)
    if x is None or sqrt(x * x + y * y) > 1.0: return
    k = find_jammer(p, x, y)
    p.sky_drag = ('jammer', k) if k >= 0 else ('beam',)
    set_sky_pos_xy(p, x, y)

# skyplot mouse drag callback -------------------------------------------------
def on_sky_drag(e, p):
    if p.sky_drag is None: return
    x, y = sky_pixel_to_data(p.plt1.plt, e.x, e.y)
    if x is None: return
    set_sky_pos_xy(p, x, y)

# skyplot mouse release callback ----------------------------------------------
def on_sky_release(e, p):
    p.sky_drag = None

# skyplot mouse motion callback -----------------------------------------------
def on_sky_motion(e, p):
    if p.sky_drag is not None: return
    update_cursor_gain(e, p)

# skyplot mouse leave callback ------------------------------------------------
def on_sky_leave(e, p):
    if p.gain_lbl.cget('text'):
        p.gain_lbl.configure(text='')

# cursor hover -> show gain at cursor point -----------------------------------
def update_cursor_gain(e, p):
    text = ''
    x, y = sky_pixel_to_data(p.plt1.plt, e.x, e.y)
    if x is not None and len(p.weight_last) > 0:
        r = sqrt(x * x + y * y)
        if r <= 1.0:
            az = atan2(x, y)
            el = (1.0 - r) * pi / 2
            gain = ant_gain(lam, p.ant_pos, p.weight_last, az, el)
            text = '%.1f dB' % gain
    if p.gain_lbl.cget('text') != text:
        p.gain_lbl.configure(text=text)

# update complex weight labels ------------------------------------------------
def update_weight_text(p, weight):
    n = len(weight)
    for i in range(8):
        if i < n:
            w = weight[i]
            p.w_txt[i].configure(text='%+.2f%+.2fj' % (w.real, w.imag))
        else:
            p.w_txt[i].configure(text='---')

# update plots ----------------------------------------------------------------
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

# root window close callback --------------------------------------------------
def on_root_close():
    exit()

# set styles ------------------------------------------------------------------
def set_styles():
    style = ttk.Style()
    style.configure('TLabel', font=get_font(), background=BG_COLOR)
    style.configure('TScale', background=BG_COLOR)
    style.configure('TCombobox', font=get_font(), background=BG_COLOR)

# main ------------------------------------------------------------------------
def main():
    root = Tk()
    root.geometry('%dx%d' % (800, 600))
    root.title('ANTENNA ARRAY SIMULATION')
    root.protocol("WM_DELETE_WINDOW", on_root_close)
    set_styles()
    p = array_page_new(root)
    root.mainloop()

if __name__ == '__main__':
    main()
