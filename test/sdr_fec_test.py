#!/usr/bin/env python3
#
#  uinit test for sdr_fec.py
#
import sys, time
sys.path.append('../python')
import numpy as np
import sdr_fec, sdr_func

# test sdr_fec.encode_conv(), sdr_fec.decode_conv() ----------------------------
def test_01():
    
    for N in [0, 12, 24, 56, 114, 160, 201, 244, 1024, 2009, 4096, 9120]:
        
        np.random.seed(0)
        org_data = np.array(np.random.rand(N) * 2, dtype='uint8')
        
        enc_data = sdr_fec.encode_conv(org_data)
        
        dec_data = sdr_fec.decode_conv(enc_data * 255)
        
        #print('org_data=', to_hex(org_data))
        #print('enc_data=', to_hex(enc_data))
        #print('dec_data=', to_hex(dec_data))
        
        if np.all(org_data == dec_data):
            print('test_01 OK: N=%d' % (N))
        else:
            print('test_01 NG: N=%d' % (N))

# test sdr_fec.encode_rs(), sdr_fec.decode_rs() --------------------------------
def test_02():
    
    for n in range(0, 60, 5):
        org_data = np.array(np.random.rand(223) * 256, dtype='uint8')
        
        enc_data = np.zeros(255, dtype='uint8')
        enc_data[:223] = org_data
        
        # encode RS(255,223)
        sdr_fec.encode_rs(enc_data)
        
        # add errors
        for i in range(n):
            enc_data[int(np.random.rand() * 223)] ^= 0x55
        
        dec_data = np.array(enc_data, dtype='uint8')
        
        # decode RS(255,223)
        nerr = sdr_fec.decode_rs(dec_data)
        
        #print('org_data=', to_hex(org_data))
        #print('enc_data=', to_hex(enc_data))
        #print('dif_data=', to_hex(org_data ^ enc_data[:223]))
        #print('dec_data=', to_hex(dec_data))
        #print('dif_data=', to_hex(org_data ^ dec_data[:223]))
        
        if np.all(dec_data[:223] == org_data):
            print('test_02 OK: NERR=%d %d' % (n, nerr))
        elif n > 16 and nerr == -1:
            print('test_02 OK: NERR=%d %d' % (n, nerr))
        else:
            print('test_02 NG: NERR=%d %d' % (n, nerr))

# test sdr_fec.encode_rs() inversed --------------------------------------------
def test_03():
    N = 300
    org_data = np.array(np.random.rand(N) * 2, dtype='uint8')
    rev_data = np.array(org_data ^ 1, dtype='uint8')
    enc_data1 = sdr_fec.encode_conv(org_data)
    enc_data2 = sdr_fec.encode_conv(rev_data)
    
    print('test_03: org_data  = %s' % (to_bin(org_data)))
    print('test_03: rev_data  = %s' % (to_bin(rev_data)))
    print('test_03: enc_data1 = %s' % (to_bin(enc_data1)))
    print('test_03: enc_data2 = %s' % (to_bin(enc_data2)))
    print('test_03: enc_data12= %s' % (to_bin(enc_data1 ^ enc_data2)))
    
# to hex -----------------------------------------------------------------------
def to_hex(data):
    hex = ''
    for i in range(len(data)):
        hex += '%02X' % (data[i])
    return hex

# to bin -----------------------------------------------------------------------
def to_bin(data):
    bin = ''
    for i in range(len(data)):
        bin += '%d' % (data[i] & 1)
    return bin

# main -------------------------------------------------------------------------
if __name__ == '__main__':
    test_01()
    test_02()
    #test_03()
