#!/usr/bin/env python3
#
#  uinit test for sdr_ldpc.py
#
import sys, time
sys.path.append('../python')
import numpy as np
import sdr_ldpc

rng = np.random.default_rng()

# sdr_ldpc.decode_LDPC('CNV2_SF2') ---------------------------------------------
def test_01():
    
    hex = 'B837CF639FB181CFF7BFF6AD1D729800DFF35B4257A6CC3BA3' + \
          '510127BD93346DCC27E2FA8E80E74086F1FF32006A805DE07F' + \
          'D6B4FFE804034AA2FC009CB600041FFFFB000200381BD4DC40' + \
          '2C7DDDAA8037AD9BA778E9CFA211D9C223C21311A84C0286F6' + \
          '1BE4603E6EA954EE60F98564FC54690FAF92E09F7D4BF3E4EF' + \
          '4C5B5ECBF02FE678EEDFF38C7BB0C89B3B0726B874F9287092'
    SF2 = read_hex(hex)
    
    for n in range(50):
        err_data = SF2.copy()
        err_data[rng.integers(1200, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('CNV2_SF2', err_data)
        
        if np.all(SF2[:600] == dec_data ^ 1):
            print('test_01 (%3d) OK: nerr=%3d' % (n, nerr))
        else:
            print('test_01 (%3d) NG: nerr=%3d' % (n, nerr))

# sdr_ldpc.decode_LDPC('CNV2_SF3') --------------------------------------------
def test_02():
    hex = '3DEEE0D7E71FF77FBFF86A28B9CBC1691F97FFFFFFFFFFFFFF' + \
          'FFFFFFFFFFFFF5D2977C0718B9DEC318F680AF7205923021BA' + \
          '37F5D6DBB0F80BBC1FFE32F1378EAE0938718'
    SF3 = read_hex(hex)
    
    for n in range(30):
        err_data = SF3.copy()
        err_data[rng.integers(548, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('CNV2_SF3', err_data)
        
        if np.all(SF3[:274] == dec_data ^ 1):
            print('test_02 (%3d) OK: nerr=%3d' % (n, nerr))
        else:
            print('test_02 (%3d) NG: nerr=%3d' % (n, nerr))

# sdr_ldpc.decode_LDPC('IRNV1_SF2') --------------------------------------------
def test_03():
    hex = 'D8374742BAFFF19F0FFFFFFFD1867FFFFF9354B54CFFE827FB' + \
          'A013E404F908D48B8802993BF38C46E01133FF4402B003D9E7' + \
          'F8679408802020FF098C199FD0C8007241EFC3FCC5F586809A' + \
          '4B1FD67C19B911270E48B2E4E0AC3013281B6EAC6B958870E3' + \
          'CBD8CEAFD43CF999EFEE2A2D291FD05007368C621D28AC561A' + \
          '8DF7FACD457E7AEBF58BD6AFEE20F9D29956C59BE299A75ADD'
    SF2 = read_hex(hex)
    
    for n in range(50):
        err_data = SF2.copy()
        err_data[rng.integers(1200, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('IRNV1_SF2', err_data)
        
        if np.all(SF2[:600] == dec_data ^ 1):
            print('test_03 (%3d) OK: nerr=%3d' % (n, nerr))
        else:
            print('test_03 (%3d) NG: nerr=%3d' % (n, nerr))

# sdr_ldpc.decode_LDPC('IRNV1_SF3') --------------------------------------------
def test_04():
    hex = 'FD555555555555555555555555555555555555555555555555' + \
          '5555555555554601CE1A297425A2EB11C5D1891FB31895EE56' + \
          'E776EC3249CC5FC6D9369B16D21E07040554B'
    SF3 = read_hex(hex)
    
    for n in range(30):
        err_data = SF3.copy()
        err_data[rng.integers(548, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('IRNV1_SF3', err_data)
        
        if np.all(SF3[:274] == dec_data ^ 1):
            print('test_04 (%3d) OK: nerr=%3d' % (n, nerr))
        else:
            print('test_04 (%3d) NG: nerr=%3d' % (n, nerr))

# read HEX strings -------------------------------------------------------------
def read_hex(str):
    N = len(str) * 4
    data = np.zeros(N, dtype='uint8')
    for i in range(N):
        data[i] = (int(str[i // 4], 16) >> (3 - i % 4)) & 1
    return data

# data to HEX strings ----------------------------------------------------------
def hex_str(data):
    str = ''
    hex = 0
    for i in range(len(data)):
        hex = (hex << 1) + data[i]
        if i % 4 == 3:
            str += '%1X' % (hex)
            hex = 0
    return str

# main -------------------------------------------------------------------------
if __name__ == '__main__':
    test_01()
    test_02()
    test_03()
    test_04()
