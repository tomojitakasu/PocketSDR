#!/usr/bin/env python3
#
# plot ACF and PSD of signal code
#
import sys, time
sys.path.append('../../python')
import numpy as np
import sdr_func, sdr_code
import matplotlib.pyplot as plt

# plot ACF ---------------------------------------------------------------------
def plot_acf(ax, data, code, fs, fc, xl, yl):
    if xl == None: xl = [-1.5, 1.5]
    if yl == None: yl = [-0.8, 1.2]
    pos = np.arange(-200, 201, 1)
    cplx = np.iscomplexobj(code)
    repl = np.conj(code) if cplx else code
    
    def signed(c):
        if not cplx:
            return c.real
        # rotate so peak (max |c|) is real-positive
        i0 = np.argmax(np.abs(c))
        return (c * np.exp(-1j * np.angle(c[i0]))).real
    
    plt.plot(xl, [0, 0], color='#808080', lw=0.8)
    plt.plot([0, 0], yl, color='#808080', lw=0.8)
    plt.plot([0], [0], '.', color='#808080', ms=8)
    
    #data_lp = lowpass(data, fs, fc[0])
    #corr = sdr_func.corr_std(data_lp, 0, len(data), fs, 0, 0, repl, pos)
    #plt.plot(pos / fs * 1e6, signed(corr), 'r-', lw=1)
    
    #data_lp = lowpass(data, fs, fc[1])
    #corr = sdr_func.corr_std(data_lp, 0, len(data), fs, 0, 0, repl, pos)
    #plt.plot(pos / fs * 1e6, signed(corr), 'g-', lw=1)
    
    corr = sdr_func.corr_std(data, 0, len(data), fs, 0, 0, repl, pos)
    plt.plot(pos / fs * 1e6, signed(corr), 'b-', lw=1.5)
    
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    plt.yticks(np.arange(yl[0], yl[1] + 0.2, 0.2))
    ax.grid(True, lw=0.4)

# plot PSD ---------------------------------------------------------------------
def plot_psd(ax, data, fs, N, xl, yl):
    if xl == None: xl = [-20e6, 20e6]
    if yl == None: yl = [-90, -55]
    xi = 5e6 if xl[1] - xl[0] < 50e6 else 20e6
    xt = np.arange(np.floor(xl[0] / xi) * xi, xl[1] + xi, xi)
    yt = np.arange(yl[0], yl[1] + 5, 5)
    plt.plot([0, 0], yl, color='#808080', lw=0.8)
    plt.psd(data, Fs=fs, NFFT=N, c='b', lw=1)
    plt.xticks(xt, ("%.0f" % (x * 1e-6) for x in xt))
    plt.yticks(yt)
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.grid(True, lw=0.4)
    ax.set_xlabel('')
    ax.set_ylabel('')

# plot CODE -------------------------------------------------------------------
def plot_code(ax, data, fs, fc, xl, yl):
    if xl == None: xl = [0, 40]
    if yl == None: yl = [-1.5, 1.5]
    x = np.arange(len(data)) / fs * 1e3
    #data_lp = lowpass(data, fs, fc[0])
    #plt.plot(x, data_lp, 'r-', lw=0.8)
    #data_lp = lowpass(data, fs, fc[1])
    #plt.plot(x, data_lp, 'g-', lw=0.8)
    plt.plot(x * 1e3, data, 'b-', lw=0.8)
    ax.set_xlim(xl)
    ax.set_ylim(yl)
    ax.grid(True, lw=0.4)
    ax.set_xlabel('TIME (ms)')
    ax.set_ylabel('')

# lowpass filter ---------------------------------------------------------------
def lowpass(data, fs, fc):
    freq = np.linspace(0, fs, len(data))
    F = np.fft.fft(data)
    F[(freq > fc) & (freq < fs - fc)] = 0
    return np.fft.ifft(F)

# generate Galileo E1B/C CBOC(6,1,11) ------------------------------------------
def gen_cboc_e1(sig, prn):
    code = sdr_code.gen_code(sig, prn)[::2] # w/o sub-carrier
    T = sdr_code.code_cyc(sig)
    a, b = np.sqrt(10/11), np.sqrt(1/11)
    if sig == 'E1B':
        SC = [a+b, a-b, a+b, a-b, a+b, a-b, -a+b, -a-b, -a+b, -a-b, -a+b, -a-b]
    else: # E1C
        SC = [a-b, a+b, a-b, a+b, a-b, a+b, -a-b, -a+b, -a-b, -a+b, -a-b, -a+b]
    ix = np.arange(len(code) * len(SC)) // len(SC)
    code = code[ix] * np.array(SC * len(code), dtype='float')
    return code, T

# generate Galileo E5 AltBOC(15,10) baseband chip sequence ---------------------
def gen_altboc_e5(prn):
    eaI = sdr_code.gen_code('E5AI', prn).astype('float')
    eaQ = sdr_code.gen_code('E5AQ', prn).astype('float')
    ebI = sdr_code.gen_code('E5BI', prn).astype('float')
    ebQ = sdr_code.gen_code('E5BQ', prn).astype('float')
    T = sdr_code.code_cyc('E5AI')
    n = len(eaI)
    eaIp = eaQ * ebI * ebQ
    eaQp = eaI * ebI * ebQ
    ebIp = ebQ * eaI * eaQ
    ebQp = ebI * eaI * eaQ
    a1 = (np.sqrt(2.0) + 1.0) / 2.0
    a2 = (np.sqrt(2.0) - 1.0) / 2.0
    scS = np.array([+a1, +0.5, -0.5, -a1, -a1, -0.5, +0.5, +a1], dtype='float')
    scP = np.array([-a2, +0.5, -0.5, +a2, +a2, -0.5, +0.5, -a2], dtype='float')
    
    scS_sh = np.roll(scS, 2)  # sc_S(t - Ts/4)
    scP_sh = np.roll(scP, 2)  # sc_P(t - Ts/4)
    sc_aD = scS - 1j * scS_sh # E5a data sub-carrier
    sc_bD = scS + 1j * scS_sh # E5b data sub-carrier
    sc_aP = scP - 1j * scP_sh # E5a product sub-carrier
    sc_bP = scP + 1j * scP_sh # E5b product sub-carrier
    M = 12
    ci = np.arange(n * M) // M
    si = np.arange(n * M) % 8
    ca_d = eaI [ci] + 1j * eaQ [ci]
    cb_d = ebI [ci] + 1j * ebQ [ci]
    ca_p = eaIp[ci] + 1j * eaQp[ci]
    cb_p = ebIp[ci] + 1j * ebQp[ci]
    code = (ca_d * sc_aD[si] + cb_d * sc_bD[si] + ca_p * sc_aP[si] + cb_p * sc_bP[si]) / (2 * np.sqrt(2))
    return code, T

# main -------------------------------------------------------------------------
def main():
    size, font = (6, 6), 12
    plot, sig, prn = 'ACF', 'L1CA', 1
    span = 0.02
    xl, yl = None, None
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-a':
            plot = 'ACF'
        elif sys.argv[i] == '-p':
            plot = 'PSD'
        elif sys.argv[i] == '-c':
            plot = 'CODE'
        elif sys.argv[i] == '-sig':
            i += 1
            sig = sys.argv[i]
        elif sys.argv[i] == '-prn':
            i += 1
            prn = int(sys.argv[i])
        elif sys.argv[i] == '-xl':
            i += 1
            xl = [float(s) for s in sys.argv[i].split(',')]
        elif sys.argv[i] == '-yl':
            i += 1
            yl = [float(s) for s in sys.argv[i].split(',')]
        i += 1
    
    fs = 500e6 if plot == 'PSD' else 100e6 # sampling rate (sps)
    fc = [8e6, 16e6] # lowpass filter cutoff freq (Hz)
    N = 4096
    
    sdr_func.LIBSDR_ENA = 0 # disable LIBSDR
    
    if sig == 'E1B' or sig == 'E1C':
        code, T = gen_cboc_e1(sig, prn)
    elif sig == 'L1M':
        code = sdr_code.gen_code('L1CD', prn)[::2] # w/o sub-carrier
        T = 2e-3
        sub_carr = [1, -1, 1, -1]
        code = sdr_code.mod_code(code, sub_carr)
    elif sig == 'E5AltBOC' or sig == 'E5AB':
        code, T = gen_altboc_e5(prn)
    else:
        code = sdr_code.gen_code(sig, prn)
        T = sdr_code.code_cyc(sig)
    
    if plot == 'CODE':
        size = (8, 6)
    
    data = sdr_code.res_code(code, T, 0, fs, int(span * fs), 0)
    
    plt.rcParams["font.size"] = font
    fig = plt.figure(plot, figsize=size)
    ax = fig.add_axes((0.1, 0.1, 0.8, 0.8), facecolor='w')
    
    if plot == 'PSD':
        plot_psd(ax, data, fs, N, xl, yl)
    elif plot == 'ACF':
        plot_acf(ax, data, data, fs, fc, xl, yl)
    else:
        plot_code(ax, data, fs, fc, xl, yl)
    
    plt.show()

if __name__ == '__main__':
    main()
