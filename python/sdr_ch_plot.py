#
#  Pocket SDR Python Library - GNSS SDR Receiver Channel Plot Functions
#
#  Author:
#  T.TAKASU
#
#  History:
#  2024-02-20  1.0  separated from pocket_trk.py
#
from math import *
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import sdr_code

# global variables -------------------------------------------------------------
plot_title = 'PocketSDR - GNSS SIGNAL TRACKING'
plot_size = (9, 6)
plot_ena = False
plot_opt = (0, 0, 0, 1.0, 0.3)
plot_bg = None

# plot settings ----------------------------------------------------------------
fc, bc, lc, gc = '#003040', 'w', '#404040', '#A0A080'
#fc, bc, lc, gc = 'darkgreen', 'w', '#404040', 'silver'
#fc, bc, lc, gc = 'navy', 'w', 'dimgrey', 'silver'
rect0  = [0.000, 0.000, 1.000, 0.960]
rect1  = [0.080, 0.568, 0.560, 0.380]
rect2  = [0.652, 0.568, 0.280, 0.380]
rect3  = [0.080, 0.198, 0.840, 0.320]
rect4  = [0.080, 0.040, 0.840, 0.110]
rect5  = [-.430, 0.000, 1.800, 1.160]
mpl.rcParams['toolbar'] = 'None'
mpl.rcParams['font.size'] = 9
mpl.rcParams['text.color'] = lc
mpl.rcParams['axes.facecolor'] = bc
mpl.rcParams['axes.edgecolor'] = lc

# initialize receiver channel plot ---------------------------------------------
def init(env, p3d, toff, tspan, ylim):
    fig = plt.figure(plot_title, figsize=plot_size, facecolor=bc)
    global plot_ena, plot_opt
    plot_ena = True
    plot_opt = (env, p3d, toff, tspan, ylim)
    draw(fig)
    fig.canvas.mpl_connect('resize_event', on_resize)
    fig.canvas.mpl_connect('close_event', on_close)
    return fig

# draw receiver channel plot ---------------------------------------------------
def draw(fig):
    fig.clear()
    ax = fig.add_axes(rect0)
    ax.axis('off')
    ax.text(0.5, 1.0, '', ha='center', va='bottom', weight='semibold', fontsize=10)
    if plot_opt[1]:
        draw_3D(fig, rect5, plot_opt)
        ax = fig.axes[0]
        txt = 'SQRT(I^2+Q^2)' if plot_opt[0] else 'I * sign(IP)'
        ax.text(0.50, 0.85, txt, color=fc, ha='center', va='top', weight='semibold')
        ax.text(0.95, 0.95, '', ha='right', va='top'),
        ax.text(0.95, 0.05, '', ha='right', va='bottom'),
        ax.text(0.05, 0.05, '', color=fc, ha='left', va='bottom')
    else:
        draw_corr(fig, rect1, plot_opt)
        draw_IQ  (fig, rect2, plot_opt)
        draw_time(fig, rect3, plot_opt)
        draw_nav (fig, rect4, plot_opt)
    plt.pause(1e-3)
    global plot_bg
    plot_bg = fig.canvas.copy_from_bbox(fig.bbox)

# resize event callback of eceiver channel plot --------------------------------
def on_resize(event):
    draw(event.canvas.figure)

# close event callback of eceiver channel plot ---------------------------------
def on_close(event):
    global plot_ena
    plot_ena = False

# update receiver channel plot ------------------------------------------------
def update(fig, ch=None):
    ax = fig.get_axes()
    if not ch:
        plt.show()
    elif plot_opt[1]:
        update_3D(ax[1], ax[0], ch, plot_opt)
        plt.pause(1e-3)
    elif plot_ena:
        fig.canvas.restore_region(plot_bg)
        draw_axis(ax[0])
        update_corr(ax[1], ch, plot_opt)
        update_IQ  (ax[2], ch, plot_opt)
        update_time(ax[3], ch, plot_opt)
        update_nav (ax[4], ch, plot_opt)
        fig.canvas.blit(fig.bbox)
        fig.canvas.flush_events()

# set title of receiver channel plot -------------------------------------------
def title(fig, ti):
    fig.axes[0].texts[0].set_text(ti)

# draw correlator plot ---------------------------------------------------------
def draw_corr(fig, rect, opt):
    ax = fig.add_axes(rect)
    set_axcolor(ax, lc)
    ax.grid(True, lw=0.3)
    ax.set_xticks([])
    ax.set_ylim([0.0 if opt[0] else -0.1, opt[4]])
    ax.plot([], [], '-', color=gc, lw=0.4)
    ax.plot([], [], '.', color=fc, ms=2)
    ax.plot([], [], '.', color=fc, ms=10)
    ax.plot([], [], '-', color=lc, lw=0.4)
    ax.plot([], [], '-', color=lc, lw=0.4)
    ax.plot([], [], '.', color=lc, ms=6)
    txt = 'SQRT(I^2+Q^2)' if opt[0] else 'I * sign(IP)'
    ax.text(0.03, 0.95, txt, ha='left', va='top', weight='semibold', transform=ax.transAxes)
    ax.text(0.97, 0.05, '(ms)', ha='right', va='bottom', transform=ax.transAxes)
    ax.text(0.97, 0.95, '', color=fc, ha='right', va='top', transform=ax.transAxes)
    return ax

# update correlator plot -------------------------------------------------------
def update_corr(ax, ch, opt):
    x0 = ch.coff * 1e3
    x = x0 + np.array(ch.trk.pos) / ch.fs * 1e3
    y = np.abs(ch.trk.C.real) if opt[0] else ch.trk.C.real * np.sign(ch.trk.C[0].real)
    yl = ax.get_ylim()
    ax.set_xlim([x[4], x[-1]])
    draw_xtick(ax, 6, 3)
    ax.lines[0].set_data(x[4:], y[4:])
    ax.lines[1].set_data(x[4:], y[4:])
    ax.lines[2].set_data(x[:3], y[:3])
    ax.lines[3].set_data([x0, x0], yl)
    ax.lines[4].set_data([x[4], x[-1]], [0, 0])
    ax.lines[5].set_data([x0], [0])
    ax.texts[2].set_text('E=%6.3f P=%6.3f L=%6.3f' % (y[1], y[0], y[2]))
    draw_axis(ax)

# draw I-Q plot ------------------------------------------------------------------
def draw_IQ(fig, rect, opt):
    ax = fig.add_axes(rect)
    set_axcolor(ax, lc)
    ax.set_aspect('equal')
    ax.set_xlim([-opt[4], opt[4]])
    ax.set_ylim([-opt[4], opt[4]])
    ax.yaxis.set_ticklabels([])
    ax.grid(True, lw=0.3)
    ax.plot([], [], '.', color=gc, ms=1)
    ax.plot([], [], '.', color=fc, ms=10)
    ax.plot([0, 0], [-opt[4], opt[4]], color=lc, lw=0.4)
    ax.plot([-opt[4], opt[4]], [0, 0], color=lc, lw=0.4)
    ax.plot(0.0, 0.0, '.', color=lc, ms=6)
    ax.text(0.05, 0.95, 'IP - QP', ha='left', va='top', weight='semibold', transform=ax.transAxes)
    ax.text(0.95, 0.95, '', ha='right', va='top', color=fc, transform=ax.transAxes)
    return ax

# update I-Q plot --------------------------------------------------------------
def update_IQ(ax, ch, opt):
    N = np.min([int(opt[3] / ch.T), len(ch.trk.P)])
    ax.lines[0].set_data(ch.trk.P[-N:].real, ch.trk.P[-N:].imag)
    ax.lines[1].set_data(ch.trk.P[-1].real, ch.trk.P[-1].imag)
    ax.texts[1].set_text('IP=%6.3f\nQP=%6.3f' % (ch.trk.P[-1].real, ch.trk.P[-1].imag))
    draw_axis(ax)

# draw time plot ---------------------------------------------------------------
def draw_time(fig, rect, opt):
    ax = fig.add_axes(rect)
    set_axcolor(ax, lc)
    ax.set_ylim(-opt[4], opt[4])
    ax.grid(True, lw=0.3)
    ax.set_xticks([])
    ax.plot([], [], '-', color=gc, lw=0.3, ms=2)
    ax.plot([], [], '-', color=fc, lw=0.5, ms=2)
    ax.plot([], [], '.', color=gc, ms=10)
    ax.plot([], [], '.', color=fc, ms=10)
    ax.plot([], [], '.', color='r', ms=3)
    ax.text(0.015, 0.94, 'IP', ha='left', va='top', color=fc, weight='semibold', transform=ax.transAxes)
    ax.text(0.045, 0.94, 'QP', ha='left', va='top', color=gc, weight='semibold', transform=ax.transAxes)
    ax.text(0.015, 0.03, 'TIME (s)', ha='left', va='bottom', transform=ax.transAxes)
    ax.text(0.985, 0.94, '', ha='right', va='top', transform=ax.transAxes)
    ax.text(0.985, 0.03, '', ha='right', va='bottom', transform=ax.transAxes)
    return ax

# update time plot -------------------------------------------------------------
def update_time(ax, ch, opt):
    N = np.min([int(opt[3] / ch.T), len(ch.trk.P)])
    time = ch.time + np.arange(-N+1, 1) * ch.T
    P = ch.trk.P[-N:]
    t0 = opt[2] if ch.lock < N else ch.time - N * ch.T
    ax.set_xlim(t0, t0 + N * ch.T * 1.008)
    ax.lines[0].set_data(time, P.imag)
    ax.lines[1].set_data(time, P.real)
    ax.lines[2].set_data(ch.time, P[-1].imag)
    ax.lines[3].set_data(ch.time, P[-1].real)
    ax.lines[4].set_data(ch.nav.tsyms, np.zeros(len(ch.nav.tsyms))) # for debug
    txt1, txt2 = ch_status(ch)
    ax.texts[3].set_text(txt1)
    ax.texts[4].set_text(txt2)
    draw_xtick(ax, 5, 5)
    draw_axis(ax)

# draw nav plot ----------------------------------------------------------------
def draw_nav(fig, rect, opt):
    ax = fig.add_axes(rect)
    set_axcolor(ax, lc)
    ax.grid(False)
    ax.set_xticks([])
    ax.set_yticks([])
    ax.text(-0.035, 0.48, 'NAV\nDATA', ha='center', va='center', weight='semibold',
        transform=ax.transAxes)
    ax.text(0.01, 0.9, '', ha='left', va='top', color=fc,
        transform=ax.transAxes, fontname='monospace')
    return ax

# update nav plot --------------------------------------------------------------
def update_nav(ax, ch, opt):
    N = len(ch.nav.data)
    txt = ''
    for i in range(0 if N <= 4 else N - 4, N):
        txt += '%7.2f: ' % (ch.nav.data[i][0])
        for j in range(len(ch.nav.data[i][1])):
            txt += '%02X' % (ch.nav.data[i][1][j])
            if j >= 42:
                txt += '...'
                break
        txt += '\n'
    ax.texts[1].set_text(txt)
    draw_axis(ax)

# draw 3D plot -----------------------------------------------------------------
def draw_3D(fig, rect, opt):
    ax = fig.add_axes(rect, projection='3d', facecolor='None')
    ax.grid(False)
    ax.set_zlim([opt[4] * 0.01, opt[4]])
    ax.set_xlabel('TIME (s)')
    ax.set_ylabel('Code Offset (ms)')
    ax.set_zlabel('Correlation')
    ax.set_box_aspect((3, 2.5, 0.6))
    ax.xaxis.pane.set_visible(False)
    ax.yaxis.pane.set_visible(False)
    ax.zaxis.pane.set_visible(False)
    ax.view_init(35, -50)
    ax.plot([], [], [], '.', color=gc, ms=2)
    ax.plot([0], [np.nan], [np.nan], '.', color=fc, ms=4)
    ax.plot([0], [np.nan], [np.nan], '-', color=fc, lw=0.4)
    ax.plot([], [], [], '-', color=fc, lw=0.8)
    ax.plot([], [], [], '.', color=fc, ms=10)
    return ax

# update 3D plot ---------------------------------------------------------------
def update_3D(ax, ax0, ch, opt):
    N = int(opt[3] / ch.T)
    t0 = opt[2] if ch.lock < N else ch.time - N * ch.T
    xl = [t0, t0 + N * ch.T]
    x = np.full(len(ch.trk.pos), ch.time)
    #y0 = ch.coff * 1e3
    y0 = 0.0
    y = y0 + np.array(ch.trk.pos) / ch.fs * 1e3
    z = np.abs(ch.trk.C) if opt[0] else ch.trk.C.real * np.sign(ch.trk.C[0].real)
    xt, yt, zt = ax.lines[1].get_data_3d()
    xp, yp, zp = ax.lines[2].get_data_3d()
    ix = np.max(np.array(np.where(xp <= xl[0])))
    xp = np.hstack([xp[ix:], x[4:], np.nan])
    yp = np.hstack([yp[ix:], y[4:], np.nan])
    zp = np.hstack([zp[ix:], z[4:], np.nan])
    ix = np.where(xt > xl[0])
    xt = np.hstack([xt[ix], ch.time])
    yt = np.hstack([yt[ix], y[0]])
    zt = np.hstack([zt[ix], z[0]])
    y1 = yt[len(yt)//2]
    yl = [y1 + ch.trk.pos[4] / ch.fs * 1.3e3, y1 + ch.trk.pos[-1] / ch.fs * 1.3e3]
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.set_xbound(xl)
    ax.set_ybound(yl)
    ax.lines[0].set_data_3d(xt, yt, np.zeros(len(zt)))
    ax.lines[1].set_data_3d(xt, yt, zt)
    ax.lines[2].set_data_3d(xp, yp, zp)
    ax.lines[3].set_data_3d(x[4:], y[4:], z[4:])
    ax.lines[4].set_data_3d(x[:3], y[:3], z[:3])
    txt1, txt2 = ch_status(ch)
    ax0.texts[2].set_text(txt1)
    ax0.texts[3].set_text(txt2)
    ax0.texts[4].set_text('E=%6.3f P=%6.3f L=%6.3f' % (z[1], z[0], z[2]))

# draw axis --------------------------------------------------------------------
def draw_axis(ax):
    for a in ax.lines:
        ax.draw_artist(a)
    for a in ax.texts:
        ax.draw_artist(a)

# draw x ticks -----------------------------------------------------------------
def draw_xtick(ax, n1, n2):
    xl = ax.get_xlim()
    yl = ax.get_ylim()
    y = yl[0] - (yl[1] - yl[0]) * 0.043
    for a in ax.lines[n1:]: a.remove()
    for a in ax.texts[n2:]: a.remove()
    for x in get_ticks(xl):
        ax.plot([x, x], yl, color='grey', lw=0.2)
        ax.text(x, y, '%.4g' % (x), va='top', ha='center', color=lc)

# get tick positions -----------------------------------------------------------
def get_ticks(xl):
    xs = xl[1] - xl[0]
    xt = pow(10.0, floor(log10(xs))) * 0.2
    if   xs / xt > 20.0: xt *= 5.0
    elif xs / xt > 10.0: xt *= 2.5
    return np.arange(ceil(xl[0] / xt), xl[1] / xt) * xt

# set axis colors --------------------------------------------------------------
def set_axcolor(ax, color):
    ax.tick_params(color=color)
    plt.setp(ax.get_xticklabels(), color=color)
    plt.setp(ax.get_yticklabels(), color=color)

# receiver channel status ------------------------------------------------------
def ch_status(ch):
    sync_stat = ('S' if ch.trk.sec_sync > 0 else '-') + \
        ('B' if ch.nav.ssync > 0 else '-') + \
        ('F' if ch.nav.fsync > 0 else '-') + ('R' if ch.nav.rev else '-')
    txt1 = 'COFF=%10.7f ms DOP=%8.1f Hz ADR=%10.1f cyc C/N0=%5.1f dB-Hz' % (
        ch.coff * 1e3, ch.fd, ch.adr, ch.cn0)
    txt2 = 'SYNC=%s #NAV=%4d #ERR=%2d #LOL=%2d NER=%2d SEQ=%6d' % (
        sync_stat, ch.nav.count[0], ch.nav.count[1], ch.lost, ch.nav.nerr,
        ch.nav.seq)
    return txt1, txt2
