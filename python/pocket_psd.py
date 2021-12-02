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
#
import sys, math
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import sdr_func

# show usage --------------------------------------------------------------------
def show_usage():
    print('Usage: pocket_psd.py [-t tint] [-f freq] [-IQ] [-h] [-n NFFT] file')
    exit()

# plot PSD ---------------------------------------------------------------------
def plot_psd(fig, rect, data, IQ, fs, time, N, fc, bc):
    yl = [-80, -45]
    
    if IQ == 1: # I
        xl = [0, fs * 0.5]
    else: # IQ
        xl = [-fs * 0.5, fs * 0.5]
        N = int(N / 2)
    
    ax = fig.add_axes(rect, facecolor=bc)
    plt.psd(data, Fs=fs, NFFT=N, c=fc, lw=0.3)
    xi = 1e6 if xl[1] - xl[0] < 15e6 else 2e6
    xt = np.arange(np.floor(xl[0] / xi) * xi, xl[1] + xi, xi)
    yt = np.arange(yl[0], yl[1] + 5, 5)
    plt.xticks(xt, ("%.0f" % (x * 1e-6) for x in xt))
    plt.yticks(yt)
    plt.xlim(xl)
    plt.ylim(yl)
    ax.set_xlabel('Frequency (MHz)')
    ax.grid(True, lw=0.4)
    ax.text(0.97, 0.97, 'Fs = %6.3f MHz\nT= %7.3f s' % (fs / 1e6, time),
        ha='right', va='top', color=fc, transform=ax.transAxes)
    return ax

# plot histgram ----------------------------------------------------------------
def plot_hist_d(fig, rect, data, text, fc, bc):
    bins = np.arange(-5.5, 6.5, 1)
    ax = fig.add_axes(rect, facecolor=bc)
    if len(data) > 0:
        plt.hist(data, bins=bins, density=True, rwidth=0.7, color=fc)
    ax.set_xticks(np.arange(-5, 6, 1))
    ax.set_xlim([-5, 5])
    ax.set_ylim([0, 0.5])
    ax.tick_params(labelleft=False)
    if text == 'I':
        ax.tick_params(labelbottom=False)
    ax.grid(True, lw=0.4)
    ax.text(0.07, 0.935, text, ha='center', va='top', color=fc,
        transform=ax.transAxes)
    return ax

# plot histgrams ---------------------------------------------------------------
def plot_hist(fig, rect, data, IQ, fc, bc):
    h = rect[3] / 2 - 0.02
    rect1 = [rect[0], rect[1] + h + 0.04, rect[2], h]
    rect2 = [rect[0], rect[1], rect[2], h]
    
    if IQ == 1: # I
        ax1 = plot_hist_d(fig, rect1, data, 'I', fc, bc)
        ax2 = plot_hist_d(fig, rect2, []  , 'Q', fc, bc)
    else: # IQ
        ax1 = plot_hist_d(fig, rect1, np.real(data), 'I', fc, bc)
        ax2 = plot_hist_d(fig, rect2, np.imag(data), 'Q', fc, bc)
    
    ax2.set_xlabel('Quantized Value')

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
    
    try:
        for i in range(0, 10000000):
            data = sdr_func.read_data(file, fs, IQ, tint, toff=tint * i)
            
            if plt.figure(window) != fig: # window closed
                exit()
            
            if len(data) >= int(fs * tint):
                fig.clear() 
                ax1 = fig.add_axes(rect0)
                ax1.axis('off')
                ax1.set_title('Digital IF data: FILE = ' + file, fontsize=10)
                if hist:
                    ax2 = plot_psd(fig, rect1, data, IQ, fs, tint * i, N, fc, bc)
                    ax3 = plot_hist(fig, rect2, data, IQ, fc, bc)
                else:
                    ax2 = plot_psd(fig, rect0, data, IQ, fs, tint * i, N, fc, bc)
            plt.pause(1e-3) 
    
    except KeyboardInterrupt:
        exit()

