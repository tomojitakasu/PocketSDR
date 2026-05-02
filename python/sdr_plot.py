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
#GR_COLOR = '#DDDDDD'   # grid color
GR_COLOR = '#E4E4E4'   # grid color
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
    font=None, taxis=0):
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
    plt.font = FONT if not font else font 
    return plt

# plot update ------------------------------------------------------------------
def plot_update(plt):
    plt.c.update()

# plot clear -------------------------------------------------------------------
def plot_clear(plt, tag=''):
    plt.c.delete('all' if tag == '' else tag)

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
def plot_rect(plt, x1, y1, x2, y2, color=FG_COLOR, fill=None, tag=''):
    if color == None: return
    xp1, yp1 = plot_pos(plt, x1, y1)
    xp2, yp2 = plot_pos(plt, x2, y2)
    plt.c.create_rectangle(xp1, yp1, xp2, yp2, outline=color, fill=fill,
        tag=tag)

# plot circle ------------------------------------------------------------------
def plot_circle(plt, x, y, r, color=FG_COLOR, fill=None, tag=''):
    if color == None: return
    xp, yp = plot_pos(plt, x, y)
    xs, ys = plot_scale(plt)
    plt.c.create_oval(xp - r * xs, yp - r * ys, xp + r * xs, yp + r * ys,
        outline=color, fill=fill, tag=tag)

# plot polyline ----------------------------------------------------------------
def plot_poly(plt, x, y, color=FG_COLOR, width=1, fill=0, tag=''):
    if color == None or len(x) <= 1: return
    xp, yp = plot_pos(plt, x, y)
    xp_yp = [(xp[i], yp[i]) for i in range(len(xp))]
    if fill:
        plt.c.create_polygon(xp_yp, outline=color, fill=color, width=width,
            tag=tag)
    else:
        plt.c.create_line(xp_yp, fill=color, width=width, tag=tag)

# plot dots --------------------------------------------------------------------
def plot_dots(plt, x, y, color=FG_COLOR, fill=FG_COLOR, size=3, tag=''):
    if color == None: return
    xp, yp = plot_pos(plt, x, y)
    xp_yp = set([(int(xp[i]), int(yp[i])) for i in range(len(xp))])
    for xy in xp_yp:
        if size <= 2:
            d = size / 2 - 0.5
            plt.c.create_rectangle(xy[0] - d, xy[1] - d, xy[0] + d, xy[1] + d,
                outline=color, fill=color, tag=tag)
        else:
            d = size / 2
            plt.c.create_oval(xy[0] - d, xy[1] - d, xy[0] + d, xy[1] + d,
                outline=color, fill=fill, tag=tag)

# plot text --------------------------------------------------------------------
def plot_text(plt, x, y, text, color=FG_COLOR, anchor=CENTER, font=None,
    angle=0, tag=''):
    if color == None: return
    if font == None: font = plt.font
    xp, yp = plot_pos(plt, x, y)
    plt.c.create_text(xp, yp, text=text, fill=color, font=font, anchor=anchor,
        angle=angle)

# plot frame and ticks ---------------------------------------------------------
def plot_frm(plt, color=FG_COLOR, tag=''):
    if color == None: return
    xs, ys = plot_scale(plt)
    if plt.tick & 1:
        d = TICK_SIZE / ys
        for x in get_ticks(plt.xl, xs, plt.taxis):
            plot_poly(plt, [x, x], [plt.yl[0], plt.yl[0] + d], color, tag=tag)
            plot_poly(plt, [x, x], [plt.yl[1], plt.yl[1] - d], color, tag=tag)
    if plt.tick & 2:
        d = TICK_SIZE / xs
        for y in get_ticks(plt.yl, ys):
            plot_poly(plt, [plt.xl[0], plt.xl[0] + d], [y, y], color, tag=tag)
            plot_poly(plt, [plt.xl[1], plt.xl[1] - d], [y, y], color, tag=tag)
    plot_rect(plt, plt.xl[0], plt.yl[0], plt.xl[1], plt.yl[1], color, tag=tag)

# plot grid --------------------------------------------------------------------
def plot_grid(plt, color=GR_COLOR, tag=''):
    if color == None: return
    xs, ys = plot_scale(plt)
    if plt.tick & 1:
        for x in get_ticks(plt.xl, xs, plt.taxis):
            plot_poly(plt, [x, x], plt.yl, color, tag=tag)
    if plt.tick & 2:
        for y in get_ticks(plt.yl, ys):
            plot_poly(plt, plt.xl, [y, y], color, tag=tag)

# plot tick labels -------------------------------------------------------------
def plot_tick_labels(plt, color=FG_COLOR, tag=''):
    if color == None: return
    xs, ys = plot_scale(plt)
    if plt.tick & 4:
        for x in get_ticks(plt.xl, xs, plt.taxis):
            text = '%.9g' % (x) if not plt.taxis else time_label(x)
            plot_text(plt, x, plt.yl[0] - 3 / ys, text=text, color=color,
                anchor=N, tag=tag)
    if plt.tick & 8:
        for y in get_ticks(plt.yl, ys):
            plot_text(plt, plt.xl[0] - 3 / xs, y, text='%.9g' % (y),
                color=color, anchor=E, tag=tag)

# time label -------------------------------------------------------------------
def time_label(x):
    x = int(x - 86400 * floor(x / 86400))
    return '%02d:%02d:%02d' % (x // 3600, x % 3600 // 60, x % 60)

# plot axis --------------------------------------------------------------------
def plot_axis(plt, fcolor=FG_COLOR, gcolor=GR_COLOR, tcolor=FG_COLOR, tag=''):
    w, h = plt.c.winfo_width(), plt.c.winfo_height()
    plt.c.create_polygon(0, 0, 0, h + 1, plt.m[0], h + 1, plt.m[0], plt.m[2],
        w + 1, plt.m[2], w + 1, 0, outline=BG_COLOR, fill=BG_COLOR, tag=tag)
    plt.c.create_polygon(0, h - plt.m[3], 0, h + 1, w + 1, h + 1, w + 1, 0,
        w - plt.m[1], 0, w - plt.m[1], h - plt.m[3], outline=BG_COLOR,
        fill=BG_COLOR, tag=tag)
    plot_grid(plt, gcolor)
    plot_frm(plt, fcolor)
    if tcolor == None: return
    plot_tick_labels(plt, tcolor)
    xp, yp = plot_pos(plt, (plt.xl[0] + plt.xl[1]) / 2,
        (plt.yl[0] + plt.yl[1]) / 2)
    plt.c.create_text(xp, plt.m[2] - 3, text=plt.title, anchor=S,
        font=(plt.font[0], plt.font[1] + 1, 'bold'), fill=tcolor, tag=tag)
    plt.c.create_text(xp, h - plt.m[3] + 18, text=plt.xlabel, anchor=N,
        font=plt.font, fill=tcolor, tag=tag)
    plt.c.create_text(plt.m[0] - 28, yp, text=plt.ylabel, anchor=S, angle=90,
        font=plt.font, fill=tcolor, tag=tag)

# plot skyplot -----------------------------------------------------------------
def plot_sky(plt, color=FG_COLOR, gcolor=GR_COLOR, tag=''):
    for az in np.arange(0, 360, 30):
        x, y = sin(az * pi / 180), cos(az * pi / 180)
        plot_poly(plt, [0, x], [0, y], gcolor, tag=tag)
        text = ('%.0f' % (az)) if az % 90 else 'NESW'[az//90]
        plot_text(plt, x * 1.01, y * 1.01, text, color=color, anchor=S,
            angle=-az, tag=tag)
    for el in np.arange(0, 90, 30):
        plot_circle(plt, 0, 0, (90 - el) / 90, color if el < 5 else gcolor,
            tag=tag)

# generate jet-style colormap LUT (list of '#RRGGBB' hex strings) -------------
_JET_LUT_CACHE = {}
def jet_lut(n=64):
    if n in _JET_LUT_CACHE: return _JET_LUT_CACHE[n]
    keys = np.array([
        [  0,   0, 143],   # 0.000 dark blue
        [  0,   0, 255],   # 0.125 blue
        [  0, 255, 255],   # 0.375 cyan
        [255, 255,   0],   # 0.625 yellow
        [255,   0,   0],   # 0.875 red
        [128,   0,   0],   # 1.000 dark red
    ], dtype=float)
    pos = np.array([0.0, 0.125, 0.375, 0.625, 0.875, 1.0])
    t = np.linspace(0, 1, n)
    rgb = np.empty((n, 3), dtype=int)
    for c in range(3):
        rgb[:, c] = np.clip(np.interp(t, pos, keys[:, c]), 0, 255).astype(int)
    lut = ['#%02x%02x%02x' % (r, g, b) for r, g, b in rgb]
    _JET_LUT_CACHE[n] = lut
    return lut

# render a 2D float array as a heatmap PhotoImage centered at (xc, yc) -------
#   data    (M, M) array; data[0] is the TOP row in plot (row index increases
#           downward in canvas).
#   half    image spans plot range [xc-half, xc+half] in both axes (the actual
#           rendered extent may be slightly larger due to integer zoom; the
#           function returns the actual half-extent so callers can place
#           accompanying overlays/masks correctly).
#   vmin/vmax   color-scale range. Values are clipped to [vmin, vmax].
#   lut     list of '#RRGGBB' hex strings; defaults to jet_lut().
def plot_image(plt, xc, yc, half, data, vmin, vmax, lut=None, tag=''):
    if lut is None: lut = jet_lut()
    nlev = len(lut)
    norm = (data - vmin) / (vmax - vmin)
    idx = np.clip((norm * (nlev - 1)).astype(int), 0, nlev - 1)
    color_arr = np.array(lut, dtype=object)[idx]
    M = data.shape[0]
    rows = [' '.join(color_arr[j]) for j in range(M)]
    pdata = '{' + '} {'.join(rows) + '}'
    img = getattr(plt, '_img_cache', None)
    if img is None or img.width() != M:
        img = PhotoImage(width=M, height=M)
        plt._img_cache = img
    img.put(pdata, to=(0, 0))
    xs, _ = plot_scale(plt)
    if xs <= 0: return half
    z = max(1, int(np.ceil(2.0 * half * xs / M)))
    img_disp = img if z == 1 else img.zoom(z, z)
    plt._img_disp_ref = img_disp                  # hold ref to prevent GC
    xp, yp = plot_pos(plt, xc, yc)
    plt.c.create_image(xp, yp, image=img_disp, anchor=CENTER, tag=tag)
    return (M * z) / (2.0 * xs)                   # actual half-extent (plot)

# mask the area outside the unit circle in a sky plot (between the unit circle
# and a bounding square at ±half plot units). Useful for hiding overflow from
# plot_image overlays. ------------------------------------------------------
def plot_sky_mask(plt, half, color=BG_COLOR, n_arc=24, tag=''):
    xs, _ = plot_scale(plt)
    if xs <= 0: return
    xc, yc = plot_pos(plt, 0, 0)
    h_px = half * xs
    ts = np.linspace(0, pi / 2, n_arc + 1)
    card = [(1, 0), (0, -1), (-1, 0), (0, 1)]      # E, N, W, S (canvas frame)
    for q in range(4):
        ang = ts + q * pi / 2
        ax = xc + xs * np.cos(ang)
        ay = yc - xs * np.sin(ang)
        cs, ce = card[q], card[(q + 1) % 4]
        pts = []
        for k in range(len(ang)):
            pts.append(float(ax[k])); pts.append(float(ay[k]))
        pts += [xc + ce[0]*h_px, yc + ce[1]*h_px,
                xc + (cs[0]+ce[0])*h_px, yc + (cs[1]+ce[1])*h_px,
                xc + cs[0]*h_px, yc + cs[1]*h_px]
        plt.c.create_polygon(*pts, fill=color, outline='', tag=tag)
