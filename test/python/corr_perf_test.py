#!/usr/bin/env python3
#
#  correlator performance test
#
import time
import numpy as np

types = ('int8', 'int16', 'float16', 'float32', 'float64', 'complex64', 'complex128')
N = (6000, 12000, 24000, 48000, 96000)
n = 10000
#n = 1000
t = np.zeros(len(N))

print('%10s  %10s' % ('DTYPE', 'RESULT'), end='')
for i in range(len(N)):
    print('  %10d' % (N[i]), end='')
print(' (ms)')

for type in types:
    for i in range(len(N)):
        data1 = np.array(np.random.rand(N[i]) * 255, dtype=type)
        data2 = np.array(np.random.rand(N[i]) * 255, dtype=type)

        t0 = time.time()
        for j in range(n):
            corr = np.dot(data1, data2)
        t[i] = (time.time() - t0) / n
        
    print('%10s  %10s  %10.5f  %10.5f  %10.5f  %10.5f  %10.5f' % \
          (type, corr.dtype, t[0] * 1e3, t[1] * 1e3, t[2] * 1e3, t[3] * 1e3, t[4] * 1e3))

print()

