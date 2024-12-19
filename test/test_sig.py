#!/usr/bin/env python3
#
#  Test ACF, PSD and CODE of Signal
#
import sys, time
sys.path.append('../python')
import numpy as np
import matplotlib.pyplot as plt
import sdr_func, sdr_code

sdr_func.LIBSDR_ENA = 0 # disable LIBSDR

# plot ACF ---------------------------------------------------------------------
def plot_acf(ax, sig, data, code, fs, fc):
    xl, yl = [-1.5, 1.5], [-0.6, 1.2]
    pos = np.arange(-200, 201, 1)
    col = ['r', 'g', 'grey']
    plt.plot(xl, [0, 0], color='#808080', lw=0.8)
    plt.plot([0, 0], yl, color='#808080', lw=0.8)
    plt.plot([0], [0], '.', color='#808080', ms=8)
    for i, f in enumerate(fc):
        data_lp = lowpass(data, fs, f)
        corr = sdr_func.corr_std(data_lp, 0, len(data), fs, 0, 0, code, pos)
        plt.plot(pos / fs * 1e6, corr.real, '-', color=col[i%3], lw=1)
    
    corr = sdr_func.corr_std(data, 0, len(data), fs, 0, 0, code, pos)
    plt.plot(pos / fs * 1e6, corr.real, 'b-', lw=1.5)
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.grid(True, lw=0.4)
    ax.set_xlabel('CODE OFFSET (CHIP)')
    ax.set_ylabel('NORMALIZED CORRELATION')
    ax.set_title('AUTO CORRELATION FUNCTION (' + sig + ')')

# plot PSD ---------------------------------------------------------------------
def plot_psd(ax, sig, data, fs, N):
    xl, yl = [-12e6, 12e6], [-90, -55]
    xi = 1e6 if xl[1] - xl[0] < 15e6 else 2e6
    xt = np.arange(np.floor(xl[0] / xi) * xi, xl[1] + xi, xi)
    yt = np.arange(yl[0], yl[1] + 5, 5)
    plt.plot([0, 0], yl, color='#808080', lw=0.8)
    plt.psd(data, Fs=fs, NFFT=N, c='b', lw=1)
    plt.xticks(xt, ("%.0f" % (x * 1e-6) for x in xt))
    plt.yticks(yt)
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.grid(True, lw=0.4)
    ax.set_xlabel('FREQUENCY (MHz)')
    ax.set_ylabel('PSD (dB/Hz)')
    ax.set_title('POWER SPECTRAL DENSITY (' + sig + ')')

# plot CODE --------------------------------------------------------------------
def plot_code(ax, sig, data, fs, fc, span):
    xl, yl = [0, span], [-1.5, 1.5]
    x = np.arange(len(data)) / fs * 1e3
    col = ['r', 'g', 'grey']
    for i, f in enumerate(fc):
        data_lp = lowpass(data, fs, f)
        plt.plot(x, data_lp.real, '-', color=col[i%3], lw=0.8)
    plt.plot(x, data.real, 'b-', lw=0.8)
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.grid(True, lw=0.4)
    ax.set_xlabel('TIME (ms)')
    ax.set_ylabel('CODE')
    ax.set_title('SPREADING CODE (' + sig + ')')

# lowpass filter ---------------------------------------------------------------
def lowpass(data, fs, fc):
    freq = np.linspace(0, fs, len(data))
    F = np.fft.fft(data)
    F[(freq > fc) & (freq < fs - fc)] = 0
    return np.fft.ifft(F)

# CBOC modulation --------------------------------------------------------------
def cboc(sig, code):
    a, b = np.sqrt(10/11), np.sqrt(1/11)
    if sig == 'E1B':
        CBOC = [a+b, a-b, a+b, a-b, a+b, a-b, -a+b, -a-b, -a+b, -a-b, -a+b, -a-b]
    else: # E1C
        CBOC = [a-b, a+b, a-b, a+b, a-b, a+b, -a-b, -a+b, -a-b, -a+b, -a-b, -a+b]
    code = code[::2]
    ix = np.arange(len(code) * len(CBOC)) // len(CBOC)
    return -code[ix] * np.array(CBOC * len(code), dtype='float')

# main -------------------------------------------------------------------------
if __name__ == '__main__':
    #size, font = (6, 6), 16
    size, font = (8, 6), 10
    plot, sig, prn = 'ACF', 'L1CA', 1
    span, fc = 0.1, []
    fs, N = 100e6, 4096 # sampling rate (sps), FFT size
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-acf':
            plot = 'ACF'
        elif sys.argv[i] == '-psd':
            plot = 'PSD'
        elif sys.argv[i] == '-code':
            plot = 'CODE'
        elif sys.argv[i] == '-sig':
            i += 1
            sig = sys.argv[i]
        elif sys.argv[i] == '-prn':
            i += 1
            prn = int(sys.argv[i])
        elif sys.argv[i] == '-ts':
            i += 1
            span = float(sys.argv[i])
        elif sys.argv[i] == '-lp':
            i += 1
            fc = [float(f) * 1e6 for f in sys.argv[i].split(',')]
        i += 1
    
    code = sdr_code.gen_code(sig, prn)
    
    if sig == 'E1B' or sig == 'E1C':
        code = cboc(sig, code)
    
    T = sdr_code.code_cyc(sig)
    data = sdr_code.res_code(code, T, 0, fs, int(span * fs), 0)
    
    plt.rcParams["font.size"] = font
    fig = plt.figure(plot, figsize=size)
    ax = fig.add_axes((0.09, 0.08, 0.84, 0.84), facecolor='w')
    
    if plot == 'PSD':
        plot_psd(ax, sig, data, fs, N)
    elif plot == 'ACF':
        plot_acf(ax, sig, data, data, fs, fc)
    else:
        plot_code(ax, sig, data, fs, fc, span)
    
    plt.show()
