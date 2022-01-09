#!/usr/bin/env python
#
#  PocketSDR Python AP - Plot PSD and histgrams of digital IF data
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-10-20  0.1  new
#  2021-12-01  1.0  rename pocket_plot.py -> pocket_psd.py
#                   add option -h
#  2021-12-10  1.1  improve plotting time
#  2022-01-10  1.2  add OFFSET and SIGMA in histgram plot
#
import sys
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import sdr_func

# show usage --------------------------------------------------------------------
def show_usage():
    print('Usage: pocket_psd.py [-t tint] [-f freq] [-IQ] [-h] [-n NFFT] file')
    exit()

# plot PSD ---------------------------------------------------------------------
def plot_psd(fig, rect, IQ, fs, fc, bc):
    yl = [-80, -45]
    if IQ == 1: # I
        xl = [0, fs * 0.5]
    else: # IQ
        xl = [-fs * 0.5, fs * 0.5]
    ax = fig.add_axes(rect, facecolor=bc)
    xi = 1e6 if xl[1] - xl[0] < 15e6 else 2e6
    xt = np.arange(np.floor(xl[0] / xi) * xi, xl[1] + xi, xi)
    yt = np.arange(yl[0], yl[1] + 5, 5)
    plt.xticks(xt, ("%.0f" % (x * 1e-6) for x in xt))
    plt.yticks(yt)
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.grid(True, lw=0.4)
    p = ax.text(0.97, 0.97, '', ha='right', va='top', color=fc, transform=ax.transAxes)
    return ax, p

# update PSD -------------------------------------------------------------------
def update_psd(ax, p, data, IQ, fs, time, N, fc):
    for line in ax.get_lines():
        line.remove()
    if IQ == 2: # IQ
        N = int(N / 2)
    plt.sca(ax)
    plt.psd(data, Fs=fs, NFFT=N, c=fc, lw=0.3)
    p.set_text('Fs = %6.3f MHz\nT= %7.3f s' % (fs / 1e6, time))
    ax.set_xlabel('Frequency (MHz)')

# plot histgram ----------------------------------------------------------------
def plot_hist_d(fig, rect, text, fc, bc):
    ax = fig.add_axes(rect, facecolor=bc)
    ax.set_xticks(np.arange(-5, 6, 1))
    ax.set_xlim([-5, 5])
    ax.set_ylim([0, 0.5])
    ax.tick_params(labelleft=False)
    if text == 'I':
        ax.tick_params(labelbottom=False)
    ax.grid(True, lw=0.4)
    ax.text(0.07, 0.935, text, ha='center', va='top', color=fc,
        transform=ax.transAxes)
    p = ax.text(0.95, 0.935, '', ha='right', va='top', color=fc,
        transform=ax.transAxes)
    return ax, p

# plot histgrams ---------------------------------------------------------------
def plot_hist(fig, rect, fc, bc):
    h = rect[3] / 2 - 0.02
    rect1 = [rect[0], rect[1] + h + 0.04, rect[2], h]
    rect2 = [rect[0], rect[1], rect[2], h]
    ax1, p1 = plot_hist_d(fig, rect1, 'I', fc, bc)
    ax2, p2 = plot_hist_d(fig, rect2, 'Q', fc, bc)
    ax2.set_xlabel('Quantized Value')
    return (ax1, ax2), (p1, p2)

# update histgram --------------------------------------------------------------
def update_hist_d(ax, p, data, fc):
    for q in ax.patches:
        q.remove()
    if len(data) > 0:
        bins = np.arange(-5.5, 6.5, 1)
        plt.sca(ax)
        plt.hist(data, bins=bins, density=True, rwidth=0.7, color=fc)
        p.set_text('OFFSET = %.3f\nSIGMA = %.3f' % (np.mean(data), np.std(data)))

# update histgrams -------------------------------------------------------------
def update_hist(ax, p, data, IQ, fc):
    if IQ == 1: # I
        update_hist_d(ax[0], p[0], data.real, fc)
        update_hist_d(ax[1], p[1], [], fc)
    else: # IQ
        update_hist_d(ax[0], p[0], data.real, fc)
        update_hist_d(ax[1], p[1], data.imag, fc)

#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     pocket_psd.py [-t tint] [-f freq] [-IQ] [-h] [-n NFFT] file
# 
#   Description
# 
#     Plot PSD (power spectrum density) and histgram of input digital IF data.
# 
#   Options ([]: default)
#  
#     -t tint
#         Time interval for PSD and histgram in seconds. [0.01]
# 
#     -f freq
#         Sampling frequency of digital IF data in MHz. [24.000]
#
#     -IQ
#         I/Q sampling type of digital IF data. [no]
#
#     -h
#         Enable histgram plots. [no]
#
#     -n NFFT
#         Number of FFT data points for PSD. [4096]
# 
#
if __name__ == '__main__':
    window = 'PocketSDR - POWER SPECTRAL DENSITY'
    size = (9, 6)
    tint = 0.01
    fs = 24e6
    IQ = 1
    N = 4096
    hist = 0
    file = ''
    fc = 'darkblue'
    bc = 'w'
    rect0 = [0.08, 0.09, 0.84, 0.85]
    rect1 = [0.08, 0.09, 0.56, 0.85]
    rect2 = [0.67, 0.09, 0.30, 0.85]
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-t':
            i += 1
            tint = float(sys.argv[i])
        elif sys.argv[i] == '-f':
            i += 1
            fs = float(sys.argv[i]) * 1e6
        elif sys.argv[i] == '-n':
            i += 1
            N = int(sys.argv[i])
        elif sys.argv[i] == '-IQ':
            IQ = 2
        elif sys.argv[i] == '-h':
            hist = 1
        elif sys.argv[i][0] == '-':
            show_usage()
        else:
            file = sys.argv[i];
        i += 1
    
    if file == '':
        print('Specify input file.')
        exit()
    
    mpl.rcParams['toolbar'] = 'None';
    mpl.rcParams['font.size'] = 9
    fig = plt.figure(window, figsize=size)
    ax0 = fig.add_axes(rect0)
    ax0.axis('off')
    ax0.set_title('Digital IF data: FILE = ' + file, fontsize=10)
    if hist:
        ax1, p1 = plot_psd(fig, rect1, IQ, fs, fc, bc)
        ax2, p2 = plot_hist(fig, rect2, fc, bc)
    else:
        ax1, p1 = plot_psd(fig, rect0, IQ, fs, fc, bc)
    
    try:
        for i in range(0, 10000000):
            data = sdr_func.read_data(file, fs, IQ, tint, toff=tint * i)
            
            if plt.figure(window) != fig: # window closed
                exit()
            
            if len(data) >= int(fs * tint):
                if hist:
                    update_psd(ax1, p1, data, IQ, fs, tint * i, N, fc)
                    update_hist(ax2, p2, data, IQ, fc)
                else:
                    update_psd(ax1, p1, data, IQ, fs, tint * i, N, fc)
            plt.pause(1e-3) 
    
    except KeyboardInterrupt:
        exit()

