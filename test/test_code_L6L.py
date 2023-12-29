#!/usr/bin/env python3
#
# test L6L code
#
import numpy as np

# generate code by LFSR
def LFSR(N, R, tap, n):
    CHIP = (-1, 1)
    code = np.zeros(N, dtype='int8')
    for i in range(N):
        code[i] = CHIP[R & 1]
        print('%8d: %07o' % (i + 1, rev_reg(R, 20)))
        R = (xor_bits(R & tap) << (n - 1)) | (R >> 1)
    return code

# reverse bits in shift register
def rev_reg(R, N):
    RR = 0
    for i in range(N):
        RR = (RR << 1) | ((R >> i) & 1)
    return RR

# exclusive-or of all bits
def xor_bits(X):
    return bin(X).count('1') % 2

# generate L61 code2
if __name__ == '__main__':
    N = 1048575   # L61 code2
    R = 0o0000304 # prn 193
    code = LFSR(N, rev_reg(R, 20), 0b00000000000001010011, 20)
