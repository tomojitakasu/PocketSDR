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
FONT = {'family': 'Tahoma', 'size': 10, 'weight': 'normal', 'color': FG_COLOR}
D2R = pi / 180.0
TB_HEIGHT  = 25              # toolbar height

# antenna element positions ----------------------------------------------------
lam = 299792458.0 / 1.57542e9 # L1 wave length (m)
ANT_POS_1 = lam / 2 * np.array([0.0, 0.0, # 7-element array (1,0,6)
    sin(pi*0/6), cos(pi*0/6), sin(pi*2/6), cos(pi*2/6),
    sin(pi*4/6), cos(pi*4/6), sin(pi*6/6), cos(pi*6/6),
    sin(pi*8/6), cos(pi*8/6), sin(pi*10/6), cos(pi*10/6)])
ANT_POS_2 = lam / 3 * np.array([ # 8-element array (0,8,0)
    sin(pi*0/8), cos(pi*0/8), sin(pi*2/8), cos(pi*2/8),
    sin(pi*4/8), cos(pi*4/8), sin(pi*6/8), cos(pi*6/8),
    sin(pi*8/8), cos(pi*8/8), sin(pi*10/8), cos(pi*10/8),
    sin(pi*12/8), cos(pi*12/8), sin(pi*14/8), cos(pi*14/8)])
ANT_POS_3 = lam / 2 * np.array([0.0, 0.0, # 8-element array (1,0,7)
    sin(pi*0/7), cos(pi*0/7), sin(pi*2/7), cos(pi*2/7),
    sin(pi*4/7), cos(pi*4/7), sin(pi*6/7), cos(pi*6/7),
    sin(pi*8/7), cos(pi*8/7), sin(pi*10/7), cos(pi*10/7),
    sin(pi*12/7), cos(pi*12/7)])
ANT_POS_4 = lam / 2 * np.array([ # 8-element array (0,0,8)
    sin(pi*0/8), cos(pi*0/8), sin(pi*2/8), cos(pi*2/8),
    sin(pi*4/8), cos(pi*4/8), sin(pi*6/8), cos(pi*6/8),
    sin(pi*8/8), cos(pi*8/8), sin(pi*10/8), cos(pi*10/8),
    sin(pi*12/8), cos(pi*12/8), sin(pi*14/8), cos(pi*14/8)])
ANT_POS_5 = lam / 2 * np.array([0.0, 0.0, # 14-element array (1,0,13)
    sin(pi*0/13), cos(pi*0/13), sin(pi*2/13), cos(pi*2/13),
    sin(pi*4/13), cos(pi*4/13), sin(pi*6/13), cos(pi*6/13),
    sin(pi*8/13), cos(pi*8/13), sin(pi*10/13), cos(pi*10/13),
    sin(pi*12/13), cos(pi*12/13), sin(pi*14/13), cos(pi*14/13),
    sin(pi*16/13), cos(pi*16/13), sin(pi*18/13), cos(pi*18/13),
    sin(pi*20/13), cos(pi*20/13), sin(pi*22/13), cos(pi*22/13),
    sin(pi*24/13), cos(pi*24/13)])
ANT_POS_6 = lam / 2 * np.array([0.0, 0.0, # 11-element array (1,0,10)
    sin(pi*0/10), cos(pi*0/10), sin(pi*2/10), cos(pi*2/10),
    sin(pi*4/10), cos(pi*4/10), sin(pi*6/10), cos(pi*6/10),
    sin(pi*8/10), cos(pi*8/10), sin(pi*10/10), cos(pi*10/10),
    sin(pi*12/10), cos(pi*12/10), sin(pi*14/10), cos(pi*14/10),
    sin(pi*16/10), cos(pi*16/10), sin(pi*18/10), cos(pi*18/10)])
ANT_POS_7 = lam / 2 * np.array([0.0, 0.0, # 14-element array (1,4,9)
    sin(pi*0/9), cos(pi*0/9), sin(pi*2/9), cos(pi*2/9),
    sin(pi*4/9), cos(pi*4/9), sin(pi*6/9), cos(pi*6/9),
    sin(pi*8/9), cos(pi*8/9), sin(pi*10/9), cos(pi*10/9),
    sin(pi*12/9), cos(pi*12/9), sin(pi*14/9), cos(pi*14/9),
    sin(pi*16/9), cos(pi*16/9),
    sin(pi*0/4)/2, cos(pi*0/4)/2, sin(pi*2/4)/2, cos(pi*2/4)/2,
    sin(pi*4/4)/2, cos(pi*4/4)/2, sin(pi*6/4)/2, cos(pi*6/4)/2])
ANT_POS_8 = lam / 2 * np.array([0.0, 0.0, # 9-element array (1,0,8)
    sin(pi*0/8), cos(pi*0/8), sin(pi*2/8), cos(pi*2/8),
    sin(pi*4/8), cos(pi*4/8), sin(pi*6/8), cos(pi*6/8),
    sin(pi*8/8), cos(pi*8/8), sin(pi*10/8), cos(pi*10/8),
    sin(pi*12/8), cos(pi*12/8), sin(pi*14/8), cos(pi*14/8)])
ANT_POS_9 = lam / 2 * np.array([0.0, 0.0, # 14-element array (1,5,8)
    sin(pi*0/8), cos(pi*0/8), sin(pi*2/8), cos(pi*2/8),
    sin(pi*4/8), cos(pi*4/8), sin(pi*6/8), cos(pi*6/8),
    sin(pi*8/8), cos(pi*8/8), sin(pi*10/8), cos(pi*10/8),
    sin(pi*12/8), cos(pi*12/8), sin(pi*14/8), cos(pi*14/8),
    sin(pi*0/5)/2, cos(pi*0/5)/2, sin(pi*2/5)/2, cos(pi*2/5)/2,
    sin(pi*4/5)/2, cos(pi*4/5)/2, sin(pi*6/5)/2, cos(pi*6/5)/2,
    sin(pi*8/5)/2, cos(pi*8/5)/2])
ANT_POS_10 = lam / 2 * np.array([0.0, 0.0, # 14-element array (1,6,7)
    sin(pi*0/7), cos(pi*0/7), sin(pi*2/7), cos(pi*2/7),
    sin(pi*4/7), cos(pi*4/7), sin(pi*6/7), cos(pi*6/7),
    sin(pi*8/7), cos(pi*8/7), sin(pi*10/7), cos(pi*10/7),
    sin(pi*12/7), cos(pi*12/7),
    sin(pi*0/6)/2, cos(pi*0/6)/2, sin(pi*2/6)/2, cos(pi*2/6)/2,
    sin(pi*4/6)/2, cos(pi*4/6)/2, sin(pi*6/6)/2, cos(pi*6/6)/2,
    sin(pi*8/6)/2, cos(pi*8/6)/2, sin(pi*10/6)/2, cos(pi*10/6)/2])
ANT_POS_11 = lam / 2 * np.array([ # 14-element array (0,6,8)
    sin(pi*0/6)/2, cos(pi*0/6)/2, sin(pi*2/6)/2, cos(pi*2/6)/2,
    sin(pi*4/6)/2, cos(pi*4/6)/2, sin(pi*6/6)/2, cos(pi*6/6)/2,
    sin(pi*8/6)/2, cos(pi*8/6)/2, sin(pi*10/6)/2, cos(pi*10/6)/2,
    sin(pi*0/8), cos(pi*0/8), sin(pi*2/8), cos(pi*2/8),
    sin(pi*4/8), cos(pi*4/8), sin(pi*6/8), cos(pi*6/8),
    sin(pi*8/8), cos(pi*8/8), sin(pi*10/8), cos(pi*10/8),
    sin(pi*12/8), cos(pi*12/8), sin(pi*14/8), cos(pi*14/8)])
ANT_POS_12 = lam / 2 * np.array([ # 14-element array (0,5,9)
    sin(pi*0/5)/3, cos(pi*0/5)/3, sin(pi*2/5)/3, cos(pi*2/5)/3,
    sin(pi*4/5)/3, cos(pi*4/5)/3, sin(pi*6/5)/3, cos(pi*6/5)/3,
    sin(pi*8/5)/3, cos(pi*8/5)/3,
    sin(pi*0/9), cos(pi*0/9), sin(pi*2/9), cos(pi*2/9),
    sin(pi*4/9), cos(pi*4/9), sin(pi*6/9), cos(pi*6/9),
    sin(pi*8/9), cos(pi*8/9), sin(pi*10/9), cos(pi*10/9),
    sin(pi*12/9), cos(pi*12/9), sin(pi*14/9), cos(pi*14/9),
    sin(pi*16/9), cos(pi*16/9)])
ANT_POSS = (ANT_POS_1, ANT_POS_2, ANT_POS_3, ANT_POS_4, ANT_POS_5, ANT_POS_6,
    ANT_POS_7, ANT_POS_8, ANT_POS_9, ANT_POS_10, ANT_POS_11, ANT_POS_12)

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
def tool_bar_new(parent):
    toolbar = Frame(parent, height=TB_HEIGHT, bg=BG_COLOR)
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
    
# antenna gain ----------------------------------------------------------------
def ant_gain(lam, pos, az, el, a, e):
    es0 = np.array((sin(az) * cos(el), cos(az) * cos(el), sin(el)))
    es1 = np.array((sin(a) * cos(e), cos(a) * cos(e), sin(e)))
    sig = 0.0
    
    for i in range(len(pos) // 2):
        z = 0.0
        phi = np.dot(np.hstack([pos[i*2:i*2+2], z]), es1 - es0) / lam
        sig += np.exp(2j * pi * phi)
    
    return 10.0 * log10(abs(sig)) # (dB)

# plot antenna gain contour in skyplot -----------------------------------------
def plot_sky(ax, lam, pos, az, el, bins):
    ax.axis('off')
    ax.set_aspect('equal')
    ax.set_xlim(-1.05, 1.05)
    ax.set_ylim(-1.05, 1.05)
    dx = 0.025
    x = np.arange(-1.0, 1.0 + dx, dx)
    y = np.arange(-1.0, 1.0 + dx, dx)
    z = np.zeros([len(y), len(x)])
    for i in range(len(x)):
        for j in range(len(y)):
            a = atan2(x[i], y[j])
            e = (1.0 - sqrt(x[i]**2 + y[j]**2)) * pi / 2.0
            z[j][i] = ant_gain(lam, pos, az, el, a, e)
    cont = ax.contourf(x, y, z, bins, extend='both', cmap='jet')
    draw_mask(ax)
    draw_sky(ax, d_el=15, label=1)
    d = 1.0 - el / pi * 2
    x = d * sin(az)
    y = d * cos(az)
    if d > 0.1:
        ax.arrow(0, 0, (d - 0.1) / d * x, (d - 0.1) / d  * y, head_width=0.05, head_length=0.1,
            ec=FG_COLOR, fc=FG_COLOR)
    ax.plot(0, 0, '.', color=FG_COLOR, ms=10)
    ax.plot(x, y, '.', color=BG_COLOR, ms=12)
    ax.plot(x, y, '.', color=FG_COLOR, ms=10)
    return cont

# plot antenna gain plot in skyplot --------------------------------------------
def plot_gain(ax, lam, pos, az, el, bins):
    ax.axis('off')
    ax.set_aspect('equal')
    ax.set_xlim(-1.05, 1.05)
    ax.set_ylim(-0.15, 1.05)
    e = np.arange(-180.0, 180.0, 1.0) * D2R
    x = np.zeros(len(e))
    y = np.zeros(len(e))
    for i in range(len(e)):
        p = ant_gain(lam, pos, az, el, az, e[i])
        x[i] = np.max([0.0, (p - bins[0]) / (bins[-1] - bins[0])]) * cos(e[i])
        y[i] = np.max([0.0, (p - bins[0]) / (bins[-1] - bins[0])]) * sin(e[i])
    ax.plot(x, y, color=FG_COLOR, lw=0.8)
    draw_sky(ax, d_el=22.5)
    for i in range(5):
        ax.text(i * 0.25, -0.15, '%.0f' % (-10 + 5 * i), ha='center', va='top', fontdict=FONT)
        ax.text(-i * 0.25, -0.15, '%.0f' % (-10 + 5 * i), ha='center', va='top', fontdict=FONT)
    for e in range(0, 91, 30):
        ax.text(1.07 * cos(e * D2R), 1.07 * sin(e * D2R), str(e), ha='center',
            va='center', rotation=e-90, fontdict=FONT)
    ax.plot([-1.05, 1.05], [0, 0], color=FG_COLOR, lw=0.8)
    ax.plot(0, 0, '.', color=FG_COLOR, ms=10)
    ax.plot(cos(el), sin(el), '.', color=FG_COLOR, ms=10)
    ax.arrow(0, 0, 0.9 * cos(el), 0.9 * sin(el), head_width=0.05,
        head_length=0.1, ec=FG_COLOR, fc=FG_COLOR)
    ax.text(0, -0.3, 'Antenna Array Gain (dB)', ha='center', va='top', fontdict=FONT)

# plot antenna element positions -----------------------------------------------
def plot_pos(ax, lam, pos):
    ax.set_aspect('equal')
    ax.set_xlim(-0.14, 0.14)
    ax.set_ylim(-0.15, 0.13)
    ax.axis('off')
    for ratio in np.arange(0.25, 0.75, 0.25):
        ax.add_artist(plt.Circle((0, 0), lam * ratio, color=GR_COLOR, lw=0.8, fill=False))
    ax.add_artist(plt.Circle((0, 0), 0.125, color=FG_COLOR, lw=0.8, fill=False))
    for i in range(len(pos) // 2):
        x = pos[i*2]
        y = pos[i*2+1]
        ax.plot(x, y, 'o', color=FG_COLOR, markerfacecolor=BG_COLOR, ms=30, lw=0.8)
        ax.text(x, y, '%d' % (i + 1), ha='center', va='center', fontdict=FONT)
    ax.text(0, -0.13, 'Antenna Element Positions', ha='center', va='top',
        fontdict=FONT)

# add color bar ----------------------------------------------------------------
def add_colorbar(fig, rect, cont, bins):
    cax = fig.add_axes(rect)
    bar = fig.colorbar(cont, ax=cax, aspect=40, pad=0.05, orientation='horizontal')
    bar.set_ticks(np.arange(bins[0], bins[-1] + 2, 2))
    bar.ax.tick_params(color=FG_COLOR, labelcolor=FG_COLOR,
        labelfontfamily=FONT['family'])
    bar.ax.set_xlabel('Antenna Array Gain (dB)', fontdict=FONT)
    cax.remove()
    return bar
    
# plot antenna array -----------------------------------------------------------
def plot_array(p, type, azel, ant_pos):
    bins = np.arange(-10, 10.1, 0.1)
    p.ax.cla()
    if p.bar != None:
        p.bar.ax.remove()
    if type == 'POS':
        plot_pos(p.ax, lam, ant_pos)
    elif type == 'GAIN':
        plot_gain(p.ax, lam, ant_pos, azel[0] * D2R, azel[1] * D2R, bins)
    elif type == 'SKY':
        cont = plot_sky(p.ax, lam, ant_pos, azel[0] * D2R, azel[1] * D2R, bins)
        p.bar = add_colorbar(p.fig, [0.0, 0.12, 1, 0.1], cont, bins)
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
    p.toolbar = tool_bar_new(p.panel)
    p.azel = [DoubleVar(), DoubleVar()]
    p.scl1 = ttk.Scale(p.toolbar, variable=p.azel[1], to=90, orient='horizontal',
        length=110, command=lambda e: on_azel_change(e, p))
    p.scl1.pack(side=RIGHT, padx=(1, 10))
    p.txt1 = ttk.Label(p.toolbar, text='0\xb0', width=3, anchor=E)
    p.txt1.pack(side=RIGHT, padx=1)
    ttk.Label(p.toolbar, text='EL').pack(side=RIGHT, padx=(6, 1))
    p.scl2 = ttk.Scale(p.toolbar, variable=p.azel[0], to=360, orient='horizontal',
        length=110, command=lambda e: on_azel_change(e, p))
    p.scl2.pack(side=RIGHT, padx=1)
    p.txt2 = ttk.Label(p.toolbar, text='0\xb0', width=4, anchor=E)
    p.txt2.pack(side=RIGHT, padx=1)
    ttk.Label(p.toolbar, text='BEAM DIRECTION  AZ').pack(side=RIGHT, padx=(6, 1))
    ttk.Label(p.toolbar, text='ANT POS').pack(side=LEFT, padx=(10, 4))
    p.box1 = sel_box_new(p.toolbar, vals=[str(i + 1) for i in range(len(ANT_POSS))],
        val='1', width=3)
    p.box1.pack(side=LEFT)
    p.plt1 = plot_new(p.panel, 200, 200)
    p.plt1.c.pack(side=LEFT, expand=1, fill=BOTH, padx=2, pady=2)
    p.panel1 = Frame(p.panel)
    p.panel1.pack(side=LEFT, expand=1, fill=BOTH)
    p.plt2 = plot_new(p.panel1, 100, 100)
    p.plt2.c.pack(expand=1, fill=BOTH, padx=2, pady=2)
    p.plt3 = plot_new(p.panel1, 100, 100)
    p.plt3.c.pack(expand=1, fill=BOTH, padx=2, pady=2)
    p.box1.bind('<<ComboboxSelected>>', lambda e: on_pos_select(e, p))
    p.panel.bind("<Configure>", lambda e: on_plt_configure(e, p))
    p.ant_pos = ANT_POSS[0]
    return p

# antenna position select callback ---------------------------------------------
def on_pos_select(e, p):
    p.ant_pos = ANT_POSS[int(p.box1.get())-1]
    azel = [p.azel[0].get(), p.azel[1].get()]
    update_plt(p, azel)

# plots configure callback -----------------------------------------------------
def on_plt_configure(e, p):
    azel = [p.azel[0].get(), p.azel[1].get()]
    update_plt(p, azel)

# azel change callback ---------------------------------------------------------
def on_azel_change(e, p):
    azel = [p.azel[0].get(), p.azel[1].get()]
    p.txt1.configure(text='%.0f\xb0' % (azel[1]))
    p.txt2.configure(text='%.0f\xb0' % (azel[0]))
    update_plt(p, azel)

# update plots -----------------------------------------------------------------
def update_plt(p, azel):
    plot_array(p.plt1, 'SKY', azel, p.ant_pos)
    plot_array(p.plt2, 'GAIN', azel, p.ant_pos)
    plot_array(p.plt3, 'POS', azel, p.ant_pos)

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
    
    set_styles()
   
    # generate array pages
    p = array_page_new(root)
    
    # main loop of Tk
    root.mainloop()
