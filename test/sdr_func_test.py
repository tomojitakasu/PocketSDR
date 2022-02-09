#!/usr/bin/env python3
#
#  unit test driver for sdr_func.py
#
import sys, time
sys.path.append('../python')
import numpy as np
import sdr_func, sdr_code
import matplotlib.pyplot as plt

# to hex -----------------------------------------------------------------------
def to_hex(data):
    hex = ''
    for i in range(len(data)):
        hex += '%02X' % (data[i])
        if i % 32 == 31:
            hex += '\n'
    return hex

# to plot ----------------------------------------------------------------------
def to_plot(x, y):
    plt.figure()
    plt.plot(x, y, '-k', lw=0.4)
    plt.show()

# generate data ----------------------------------------------------------------
def gen_data(N):
    return np.array(np.array(np.random.rand(N) * 4, dtype='int8') * 2 - 3,
        dtype='complex64')

# test pack_bits(), unpack_bits() ----------------------------------------------
def test_01():
    
    for N in [0, 12, 24, 56, 114, 135]:
        
        bits = np.array(np.random.rand(N) * 2, dtype='uint8')
        
        pack_bits = sdr_func.pack_bits(bits)
        
        unpack_bits = sdr_func.unpack_bits(pack_bits, N)
        
        #print('bits       =', to_hex(bits))
        #print('pack_bits  =', to_hex(pack_bits))
        #print('unpack_bits=', to_hex(unpack_bits))
        
        if np.all(bits == unpack_bits):
            print('test_01: OK N=%6d' % (N))
        else:
            print('test_01: NG N=%6d' % (N))

# test mix_carr() --------------------------------------------------------------
def test_02():
    fs = 24e6
    fc = 1350.0
    phi = 45.4234
    ix = 59
    
    for N in (3000, 6000, 12000, 24000, 48000, 96000, 192000):
        data = gen_data(N)
        
        sdr_func.LIBSDR_ENA = False
        data_carr1 = sdr_func.mix_carr(data, 0, len(data), fs, fc, phi)
        
        sdr_func.LIBSDR_ENA = True
        data_carr2 = sdr_func.mix_carr(data, 0, len(data), fs, fc, phi)
        
        d = np.abs(data_carr1.real - data_carr2.real)
        e = np.abs(data_carr1.imag - data_carr2.imag)
        
        if np.all(d < 2e-6):
            print('test_02: OK N=%6d err_max=%9.7f %9.7f' % (N, np.max(d), np.max(e)))
        else:
            print('test_02: NG N=%6d err_max=%9.7f %9.7f' % (N, np.max(d), np.max(e)))

# test corr_std() --------------------------------------------------------------
def test_03():
    fs = 24e6
    fc = 13500.0
    phi = 123.456
    pos = range(-20, 21)
    coff = 1.345
    
    for N in (32, 3000, 6000, 12000, 24000, 48000, 96000):
        data = gen_data(N)
        code = sdr_code.res_code(sdr_code.gen_code('L6D', 194), 4e-3, coff, fs, N)
        
        sdr_func.LIBSDR_ENA = False
        C1 = sdr_func.corr_std(data, 0, N, fs, fc, phi, code, pos)
        
        sdr_func.LIBSDR_ENA = True
        C2 = sdr_func.corr_std(data, 0, N, fs, fc, phi, code, pos)
        
        d = np.abs(C1.real - C2.real)
        e = np.abs(C1.imag - C2.imag)
        
        if np.all(d < 1e-4) and np.all(e < 1e-4):
            print('test_03: OK N=%6d err_max=%9.7f %9.7f' % (N, np.max(d), np.max(e)))
        else:
            print('test_03: NG N=%6d err_max=%9.7f %9.7f' % (N, np.max(d), np.max(e)))
    

# test corr_fft() --------------------------------------------------------------
def test_04():
    fs = 24e6
    fc = 13500.0
    phi = 123.456
    pos = range(-40, 41)
    coff = 1.345
    
    for N in (12000, 24000, 36000, 40000, 45000, 46000, 48000, 96000):
        data = sdr_func.mix_carr(gen_data(N), 0, N, fs, fc, phi)
        code = sdr_code.gen_code_fft(sdr_code.gen_code('L6D', 194), 4e-3, coff, fs, N//2, N//2)
        
        sdr_func.LIBSDR_ENA = False
        C1 = sdr_func.corr_fft(data, 0, N, fs, fc, phi, code)
        
        sdr_func.LIBSDR_ENA = True
        C2 = sdr_func.corr_fft(data, 0, N, fs, fc, phi, code)
        
        d1 = np.abs(C1.real - C2.real)
        d2 = np.abs(C1.imag - C2.imag)
        
        if np.all(d1 < 0.01) and np.all(d2 < 0.01):
            print('test_04: OK N=%6d err_max=%9.7f %9.7f' % (N, np.max(d1), np.max(d2)))
        else:
            print('test_04: NG N=%6d err_max=%9.7f %9.7f' % (N, np.max(d1), np.max(d2)))

# test performance() -----------------------------------------------------------
def test_05():
    n = 5000
    fs = 12e6
    fc = 13500.0
    pos = [0, -3, 3, -80]
    coff = 1.345
    phi = 3.456
    
    print('test_05: performance')
    print('%6s %9s%9s%9s  %6s%15s%6s(ms)' % ('', '', 'Python', '', '', 'C+AVX2+FFTW3F', ''))
    print('%6s  %8s %8s %8s   %8s %8s %8s' % (
        'N  ', 'mix_carr', 'corr_std', 'corr_fft', 'mix_carr', 'corr_std', 'corr_fft'))
    
    for N in (12000, 16000, 24000, 32000, 32768, 48000, 65536, 96000):
        data = sdr_func.mix_carr(gen_data(N), 0, N, fs, fc, phi)
        code = sdr_code.gen_code('L6D', 194)
        code_res = sdr_code.res_code(code, 4e-3, coff, fs, N)
        code_fft = sdr_code.gen_code_fft(code, 4e-3, coff, fs, N)
        
        sdr_func.LIBSDR_ENA = False
        
        tt = time.time()
        for i in range(n):
            data_carr1 = sdr_func.mix_carr(data, 0, N, fs, fc, phi)
        t1 = (time.time() - tt) * 1e3 / n
        
        tt = time.time()
        for i in range(n):
            C1 = sdr_func.corr_std(data, 0, N, fs, fc, phi, code_res, pos)
        t2 = (time.time() - tt) * 1e3 / n
        
        tt = time.time()
        for i in range(n):
            C1 = sdr_func.corr_fft(data, 0, N, fs, fc, phi, code_fft)
        t3 = (time.time() - tt) * 1e3 / n
        
        sdr_func.LIBSDR_ENA = True
        
        tt = time.time()
        for i in range(n):
            data_carr1 = sdr_func.mix_carr(data, 0, N, fs, fc, phi)
        t4 = (time.time() - tt) * 1e3 / n
        
        tt = time.time()
        for i in range(n):
            C1 = sdr_func.corr_std(data, 0, N, fs, fc, phi, code_res, pos)
        t5 = (time.time() - tt) * 1e3 / n
        
        C1 = sdr_func.corr_fft(data, 0, N, fs, fc, phi, code_fft)
        tt = time.time()
        for i in range(n):
            C1 = sdr_func.corr_fft(data, 0, N, fs, fc, phi, code_fft)
        t6 = (time.time() - tt) * 1e3 / n
        
        print('%6d  %8.4f %8.4f %8.4f   %8.4f %8.4f %8.4f' % (
            N, t1, t2, t3, t4, t5, t6))

# test main --------------------------------------------------------------------
if __name__ == '__main__':
    test_01()
    test_02()
    test_03()
    test_04()
    test_05()

