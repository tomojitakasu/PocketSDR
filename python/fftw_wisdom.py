#!/usr/bin/env python3
#
#  Pocket SDR Python AP - Generate FFTW wisdom
#
#  Author:
#  T.TAKASU
#
#  History:
#  2022-01-20  1.0  new
#
import time, sys, os, platform
from ctypes import *

# load external library --------------------------------------------------------
dir = os.path.dirname(__file__)
env = platform.platform()
try:
    if 'Windows' in env:
        libsdr = cdll.LoadLibrary(dir + '/../lib/win32/libsdr.so')
    elif 'Linux' in env:
        libsdr = cdll.LoadLibrary(dir + '/../lib/linux/libsdr.so')
    else:
        raise
except:
    print('load libsdr.so error (%s)' % (env))
    exit()

#-------------------------------------------------------------------------------
#
#   Synopsis
# 
#     fftw_wisdowm [-n size] [file]
# 
#   Description
# 
#     Generate FFTW wisdom. FFTW wisdom is used to optimize FFT and IFFT
#     performance by FFTW in target environment.
# 
#   Options ([]: default)
#  
#     -n size
#         FFT and IFFT size. [48000]
#
#     file
#         Output FFTW wisdom file. [fftw_wisdom.txt]
#
if __name__ == '__main__':
    N = 48000
    file = 'fftw_wisdom.txt'
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '-n':
            i += 1
            N = int(sys.argv[i])
        else:
            file = sys.argv[i]
        i += 1
    
    tt = time.time()
    
    if libsdr.gen_fftw_wisdom(c_char_p(file.encode()), N):
        print('FFTW wisdom generated as %s (N=%d).' % (file, N))
    else:
        print('FFTW wisdom generation error.')
    
    print('  TIME(s) = %.3f' % (time.time() - tt))
    
