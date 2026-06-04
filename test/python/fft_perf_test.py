#!/usr/bin/env python3
#
#  FFT/DFT performance test
#
import time
import numpy as np
import numpy.fft as np_fft
import scipy.fftpack as sp_fft

print('%8s  %12s  %10s  %10s  %10s  %10s (ms)' % \
      ('N', 'DTYPE', 'NP-FFT', 'NP-IFFT', 'SP-FFT', 'SP-IFFT'))
n = 100

for N in [1200, 2400, 24000, 24500, 32768, 48000, 65536, 96000, 131072]:
    data = [np.array(np.random.rand(N), dtype='float32'),
            np.array(np.random.rand(N), dtype='float64'),
            np.array(np.random.rand(N), dtype='complex64'),
            np.array(np.random.rand(N), dtype='complex128')]
    
    for i in range(len(data)):
        t = time.time()
        for j in range(n):
            data_fft = np_fft.fft(data[i])
        t1 = (time.time() - t) / n
        
        t = time.time()
        for j in range(n):
            data_ifft = np_fft.ifft(data[i])
        t2 = (time.time() - t) / n
        
        t = time.time()
        for j in range(n):
            data_fft = sp_fft.fft(data[i])
        t3 = (time.time() - t) / n
        
        t = time.time()
        for j in range(n):
            data_ifft = sp_fft.ifft(data[1])
        t4 = (time.time() - t) / n
         
        print('%8d  %12s  %10.3f  %10.3f  %10.3f  %10.3f' % \
              (N, data[i].dtype, t1 * 1e3, t2 * 1e3, t3 * 1e3, t4 * 1e3))

'''
for N in [2500, 3000, 6000, 12000, 16000, 24000]:
    data = np.array(np.random.rand(N), dtype='complex64')
    t = time.time()
    
    for j in range(n):
        data_fft = np_fft.fft(data)
    t1 = (time.time() - t) / n
    
    t = time.time()
    for j in range(n):
        data_ifft = np_fft.ifft(data)
    t2 = (time.time() - t) / n
    
    t = time.time()
    for j in range(n):
        data_fft = sp_fft.fft(data)
    t3 = (time.time() - t) / n
    
    t = time.time()
    for j in range(n):
        data_ifft = sp_fft.ifft(data)
    t4 = (time.time() - t) / n
     
    print('%8d  %12s  %10.3f  %10.3f  %10.3f  %10.3f' % \
          (N, data.dtype, t1 * 1e3, t2 * 1e3, t3 * 1e3, t4 * 1e3))
'''
