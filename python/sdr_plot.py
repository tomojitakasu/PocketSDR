#
#  Pocket SDR Python Library - SDR Plot Functions
#
#  Author:
#  T.TAKASU
#
#  History:
#  2024-06-10  1.0  new
#
import time
from math import *
import numpy as np
from tkinter import *

# constants --------------------------------------------------------------------
BG_COLOR = 'white'     # background color
FG_COLOR = '#555555'   # foreground color
GR_COLOR = '#DDDDDD'   # grid color
TICK_SIZE = 6          # tick size
FONT = ('Tahoma', 9, 'normal')

# general object class ---------------------------------------------------------
class Obj: pass

# set font ---------------------------------------------------------------------
def set_font(font):
    global FONT
    FONT = font

# get tick positions -----------------------------------------------------------
def get_ticks(xl, xs, taxis=0):
    ti = (0, 15, 30, 60, 120, 300, 900, 1800, 3600, 7200)
    xp = xl[1] - xl[0]
    if xp <= 0.0 or xs <= 0.0:
        return []
    if xp > 500 / xs:
        xp = 500 / xs
    if not taxis:
        xt = pow(10.0, floor(log10(xp))) * 0.2
        if   xp / xt > 20.0: xt *= 5.0
        elif xp / xt > 10.0: xt *= 2.5
    else:
        xt = ti[np.min(np.where(ti > xp * 0.15))]
    return np.arange(ceil((xl[0] - 1e-6) / xt), xl[1] / xt + 1e-6) * xt

# generate plot ----------------------------------------------------------------
def plot_new(parent, width, height, xlim=(0, 1), ylim=(0, 1),
    margin=(35, 25, 25, 25), tick=15, aspect=0, title='', xlabel='', ylabel='',
    font=FONT, taxis=0):
    plt = Obj()
    plt.c = Canvas(parent, width=width, height=height, bg=BG_COLOR)
    plt.m = margin # (left, right, top, bottom)
    plt.xl = np.array(xlim if xlim[0] < xlim[1] else (0, 1))
    plt.yl = np.array(ylim if ylim[0] < ylim[1] else (0, 1))
    plt.tick = tick # 1:xtick + 2:ytick + 4:xtick-label + 8:ytick-label
    plt.taxis = taxis # time axis 0:off, 1:on
    plt.aspect = aspect # 0:independent,1:xy-equal
    plt.title = title
    plt.xlabel = xlabel
    plt.ylabel = ylabel
    plt.font = font
    return plt

# plot update ------------------------------------------------------------------
def plot_update(plt):
    plt.c.update()

# plot clear -------------------------------------------------------------------
def plot_clear(plt):
    plt.c.delete('all')

# set plot x-limit -------------------------------------------------------------
def plot_xlim(plt, xlim):
    plt.xl = np.array(xlim)

# set plot y-limit -------------------------------------------------------------
def plot_ylim(plt, ylim):
    plt.yl = np.array(ylim)

# plot scale -------------------------------------------------------------------
def plot_scale(plt):
    w, h = plt.c.winfo_width(), plt.c.winfo_height()
    xs = (w - plt.m[0] - plt.m[1]) / (plt.xl[1] - plt.xl[0])
    ys = (h - plt.m[2] - plt.m[3]) / (plt.yl[1] - plt.yl[0])
    if plt.aspect:
        xs = ys = xs if xs < ys else ys
    return xs, ys

# plot position ----------------------------------------------------------------
def plot_pos(plt, x, y):
    xs, ys = plot_scale(plt)
    xp = plt.m[0] + (plt.c.winfo_width () - plt.m[0] - plt.m[1]) / 2
    yp = plt.m[2] + (plt.c.winfo_height() - plt.m[2] - plt.m[3]) / 2
    xp += (x - (plt.xl[0] + plt.xl[1]) / 2) * xs
    yp += ((plt.yl[0] + plt.yl[1]) / 2 - y) * ys
    return xp, yp

# plot rectangle ---------------------------------------------------------------
def plot_rect(plt, x1, y1, x2, y2, color=FG_COLOR, fill=None):
    if color == None: return
    xp1, yp1 = plot_pos(plt, x1, y1)
    xp2, yp2 = plot_pos(plt, x2, y2)
    plt.c.create_rectangle(xp1, yp1, xp2, yp2, outline=color, fill=fill)

# plot circle ------------------------------------------------------------------
def plot_circle(plt, x, y, r, color=FG_COLOR, fill=None):
    if color == None: return
    xp, yp = plot_pos(plt, x, y)
    xs, ys = plot_scale(plt)
    plt.c.create_oval(xp - r * xs, yp - r * ys, xp + r * xs, yp + r * ys,
        outline=color, fill=fill)

# plot polyline ----------------------------------------------------------------
def plot_poly(plt, x, y, color=FG_COLOR, width=1):
    if color == None or len(x) <= 1: return
    xp, yp = plot_pos(plt, x, y)
    xp_yp = [(xp[i], yp[i]) for i in range(len(xp))]
    plt.c.create_line(xp_yp, fill=color, width=width)

# plot dots --------------------------------------------------------------------
def plot_dots(plt, x, y, color=FG_COLOR, fill=FG_COLOR, size=3):
    if color == None: return
    xp, yp = plot_pos(plt, x, y)
    xp_yp = set([(int(xp[i]), int(yp[i])) for i in range(len(xp))])
    d = size / 2
    for xy in xp_yp:
        if size <= 2:
            plt.c.create_rectangle(xy[0] - d, xy[1] - d, xy[0] + d, xy[1] + d,
                outline=color, fill=color)
        else:
            plt.c.create_oval(xy[0] - d, xy[1] - d, xy[0] + d, xy[1] + d,
                outline=color, fill=fill)

# plot text --------------------------------------------------------------------
def plot_text(plt, x, y, text, color=FG_COLOR, anchor=CENTER, font=None,
    angle=0):
    if color == None: return
    if font == None: font = plt.font
    xp, yp = plot_pos(plt, x, y)
    plt.c.create_text(xp, yp, text=text, fill=color, font=font, anchor=anchor,
        angle=angle)

# plot frame and ticks ---------------------------------------------------------
def plot_frm(plt, color=FG_COLOR):
    if color == None: return
    xs, ys = plot_scale(plt)
    if plt.tick & 1:
        d = TICK_SIZE / ys
        for x in get_ticks(plt.xl, xs, plt.taxis):
            plot_poly(plt, [x, x], [plt.yl[0], plt.yl[0] + d], color)
            plot_poly(plt, [x, x], [plt.yl[1], plt.yl[1] - d], color)
    if plt.tick & 2:
        d = TICK_SIZE / xs
        for y in get_ticks(plt.yl, ys):
            plot_poly(plt, [plt.xl[0], plt.xl[0] + d], [y, y], color)
            plot_poly(plt, [plt.xl[1], plt.xl[1] - d], [y, y], color)
    plot_rect(plt, plt.xl[0], plt.yl[0], plt.xl[1], plt.yl[1], color)

# plot grid --------------------------------------------------------------------
def plot_grid(plt, color=GR_COLOR):
    if color == None: return
    xs, ys = plot_scale(plt)
    if plt.tick & 1:
        for x in get_ticks(plt.xl, xs, plt.taxis):
            plot_poly(plt, [x, x], plt.yl, color)
    if plt.tick & 2:
        for y in get_ticks(plt.yl, ys):
            plot_poly(plt, plt.xl, [y, y], color)

# plot tick labels -------------------------------------------------------------
def plot_tick_labels(plt, color=FG_COLOR):
    if color == None: return
    xs, ys = plot_scale(plt)
    if plt.tick & 4:
        for x in get_ticks(plt.xl, xs, plt.taxis):
            text = '%.9g' % (x) if not plt.taxis else time_label(x)
            plot_text(plt, x, plt.yl[0] - 3 / ys, text=text, color=color,
                anchor=N)
    if plt.tick & 8:
        for y in get_ticks(plt.yl, ys):
            plot_text(plt, plt.xl[0] - 3 / xs, y, text='%.9g' % (y),
                color=color, anchor=E)

# time label -------------------------------------------------------------------
def time_label(x):
    x = int(x - 86400 * floor(x / 86400))
    return '%02d:%02d:%02d' % (x // 3600, x % 3600 // 60, x % 60)

# plot axis --------------------------------------------------------------------
def plot_axis(plt, fcolor=FG_COLOR, gcolor=GR_COLOR, tcolor=FG_COLOR):
    w, h = plt.c.winfo_width(), plt.c.winfo_height()
    plt.c.create_polygon(0, 0, 0, h + 1, plt.m[0], h + 1, plt.m[0], plt.m[2],
        w + 1, plt.m[2], w + 1, 0, outline=BG_COLOR, fill=BG_COLOR)
    plt.c.create_polygon(0, h - plt.m[3], 0, h + 1, w + 1, h + 1, w + 1, 0,
        w - plt.m[1], 0, w - plt.m[1], h - plt.m[3], outline=BG_COLOR,
        fill=BG_COLOR)
    plot_grid(plt, gcolor)
    plot_frm(plt, fcolor)
    if tcolor == None: return
    plot_tick_labels(plt, tcolor)
    xp, yp = plot_pos(plt, (plt.xl[0] + plt.xl[1]) / 2,
        (plt.yl[0] + plt.yl[1]) / 2)
    plt.c.create_text(xp, plt.m[2] - 3, text=plt.title, anchor=S,
        font=(plt.font[0], plt.font[1] + 1, 'bold'), fill=tcolor)
    plt.c.create_text(xp, h - plt.m[3] + 18, text=plt.xlabel, anchor=N,
        font=plt.font, fill=tcolor)
    plt.c.create_text(plt.m[0] - 28, yp, text=plt.ylabel, anchor=S, angle=90,
        font=plt.font, fill=tcolor)

# plot skyplot -----------------------------------------------------------------
def plot_sky(plt, color=FG_COLOR, gcolor=GR_COLOR):
    for az in np.arange(0, 360, 30):
        x, y = sin(az * pi / 180), cos(az * pi / 180)
        plot_poly(plt, [0, x], [0, y], gcolor)
        text = ('%.0f' % (az)) if az % 90 else 'NESW'[az//90]
        plot_text(plt, x * 1.01, y * 1.01, text, color=color, anchor=S, angle=-az)
    for el in np.arange(0, 90, 30):
        plot_circle(plt, 0, 0, (90 - el) / 90, color if el < 5 else gcolor)
