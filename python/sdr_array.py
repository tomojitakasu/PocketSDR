#
#  Pocket SDR Python Library - Antenna Array Simulation
#
#  Author:
#  T.TAKASU
#
#  History:
#  2025-11-30  1.0  new
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

# general object class ---------------------------------------------------------
class Obj: pass

# get font ---------------------------------------------------------------------
def get_font(add_size=0, weight='normal', mono=0):
    return (FONT['family'], FONT['size'] + add_size, FONT['weight'])

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
def ant_ele_gain(el):
    if   el >  pi / 2: el = pi - el
    elif el < -pi / 2: el = -pi - el
    return 1 - 2 ** ((90 - el / D2R) / 30) # (dB)
    
# antenna gain ----------------------------------------------------------------
def ant_gain(lam, pos, az, el, a, e, weight):
    es0 = np.array((sin(az) * cos(el), cos(az) * cos(el), sin(el)))
    es1 = np.array((sin(a) * cos(e), cos(a) * cos(e), sin(e)))
    n = len(pos) // 2
    sig = 0.0
    for i in range(n):
        z = 0.0
        phi = np.dot(np.hstack([pos[i*2:i*2+2], z]), es1 - es0) / lam
        sig += weight[i] * np.exp(2j * pi * phi)
    return 10.0 * log10(abs(sig)) + ant_ele_gain(e) # (dB)

# plot antenna gain contour in skyplot -----------------------------------------
def plot_sky(ax, lam, pos, az, el, weight, bins):
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
            z[j][i] = ant_gain(lam, pos, az, el, a, e, weight)
    cont = ax.contourf(x, y, z, bins, extend='both', cmap='jet')
    draw_mask(ax)
    draw_sky(ax, d_el=15, label=1)
    d = 1.0 - el / pi * 2
    x = d * sin(az)
    y = d * cos(az)
    if d > 0.1:
        xa = (d - 0.1) / d * x
        ya = (d - 0.1) / d * y
        ax.arrow(0, 0, xa, ya, lw=2, head_width=0.06, head_length=0.11,
            ec=BG_COLOR, fc=BG_COLOR)
        ax.arrow(0, 0, xa, ya, head_width=0.05, head_length=0.1, ec=PLT_COLOR,
            fc=PLT_COLOR)
    ax.plot(0, 0, '.', color=BG_COLOR, ms=14)
    ax.plot(0, 0, '.', color=PLT_COLOR, ms=10)
    ax.plot(x, y, '.', color=BG_COLOR, ms=14)
    ax.plot(x, y, '.', color=PLT_COLOR, ms=10)
    return cont

# add color bar ----------------------------------------------------------------
def add_colorbar(fig, rect, cont, bins):
    cax = fig.add_axes(rect)
    bar = fig.colorbar(cont, ax=cax, aspect=40, pad=0.05, orientation='horizontal')
    bar.set_ticks(np.arange(bins[0], bins[-1] + 5, 5))
    bar.ax.tick_params(color=FG_COLOR, labelcolor=FG_COLOR,
        labelfontfamily=FONT['family'])
    bar.ax.set_xlabel('Antenna Array Gain (dB)', fontdict=FONT)
    cax.remove()
    return bar
    
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

# set styles -------------------------------------------------------------------
def set_styles():
    style = ttk.Style()
    style.configure('TLabel', font=get_font(), background=BG_COLOR)
    style.configure('TScale', background=BG_COLOR)
    style.configure('TCombobox', font=get_font(), background=BG_COLOR)

