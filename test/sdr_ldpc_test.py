#!/usr/bin/env python3
#
#  uinit test for sdr_ldpc.py
#
import sys, time
sys.path.append('../python')
import numpy as np
import sdr_ldpc, sdr_nb_ldpc

rng = np.random.default_rng()

# sdr_ldpc.decode_LDPC('CNV2_SF2') ---------------------------------------------
def test_01():
    
    hex = 'B837CF639FB181CFF7BFF6AD1D729800DFF35B4257A6CC3BA3' + \
          '510127BD93346DCC27E2FA8E80E74086F1FF32006A805DE07F' + \
          'D6B4FFE804034AA2FC009CB600041FFFFB000200381BD4DC40' + \
          '2C7DDDAA8037AD9BA778E9CFA211D9C223C21311A84C0286F6' + \
          '1BE4603E6EA954EE60F98564FC54690FAF92E09F7D4BF3E4EF' + \
          '4C5B5ECBF02FE678EEDFF38C7BB0C89B3B0726B874F9287092'
    SF = read_hex(hex)
    ok, ng = 0, 0
    t = time.time()
    
    for n in range(50):
        err_data = SF.copy()
        err_data[rng.integers(1200, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('CNV2_SF2', err_data)
        
        if np.all(SF[:600] == dec_data ^ 1):
            #print('test_01 (%3d) OK: nerr=%3d' % (n, nerr))
            ok += 1
        else:
            #print('test_01 (%3d) NG: nerr=%3d' % (n, nerr))
            ng += 1
    
    t = time.time() - t
    print('test_01: N = %4d OK/NG = %3d/%3d TIME =%7.3fs' % (len(SF), ok, ng, t))

# sdr_ldpc.decode_LDPC('CNV2_SF3') --------------------------------------------
def test_02():
    hex = '3DEEE0D7E71FF77FBFF86A28B9CBC1691F97FFFFFFFFFFFFFF' + \
          'FFFFFFFFFFFFF5D2977C0718B9DEC318F680AF7205923021BA' + \
          '37F5D6DBB0F80BBC1FFE32F1378EAE0938718'
    SF = read_hex(hex)
    ok, ng = 0, 0
    t = time.time()
    
    for n in range(30):
        err_data = SF.copy()
        err_data[rng.integers(548, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('CNV2_SF3', err_data)
        
        if np.all(SF[:274] == dec_data ^ 1):
            #print('test_02 (%3d) OK: nerr=%3d' % (n, nerr))
            ok += 1
        else:
            #print('test_02 (%3d) NG: nerr=%3d' % (n, nerr))
            ng += 1
    
    t = time.time() - t
    print('test_02: N = %4d OK/NG = %3d/%3d TIME =%7.3fs' % (len(SF), ok, ng, t))

# sdr_ldpc.decode_LDPC('IRNV1_SF2') --------------------------------------------
def test_03():
    hex = 'D8374742BAFFF19F0FFFFFFFD1867FFFFF9354B54CFFE827FB' + \
          'A013E404F908D48B8802993BF38C46E01133FF4402B003D9E7' + \
          'F8679408802020FF098C199FD0C8007241EFC3FCC5F586809A' + \
          '4B1FD67C19B911270E48B2E4E0AC3013281B6EAC6B958870E3' + \
          'CBD8CEAFD43CF999EFEE2A2D291FD05007368C621D28AC561A' + \
          '8DF7FACD457E7AEBF58BD6AFEE20F9D29956C59BE299A75ADD'
    SF = read_hex(hex)
    ok, ng = 0, 0
    t = time.time()
    
    for n in range(50):
        err_data = SF.copy()
        err_data[rng.integers(1200, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('IRNV1_SF2', err_data)
        
        if np.all(SF[:600] == dec_data ^ 1):
            #print('test_03 (%3d) OK: nerr=%3d' % (n, nerr))
            ok += 1
        else:
            #print('test_03 (%3d) NG: nerr=%3d' % (n, nerr))
            ng += 1
    
    t = time.time() - t
    print('test_03: N = %4d OK/NG = %3d/%3d TIME =%7.3fs' % (len(SF), ok, ng, t))

# sdr_ldpc.decode_LDPC('IRNV1_SF3') --------------------------------------------
def test_04():
    hex = 'FD555555555555555555555555555555555555555555555555' + \
          '5555555555554601CE1A297425A2EB11C5D1891FB31895EE56' + \
          'E776EC3249CC5FC6D9369B16D21E07040554B'
    SF = read_hex(hex)
    ok, ng = 0, 0
    t = time.time()
    
    for n in range(30):
        err_data = SF.copy()
        err_data[rng.integers(548, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('IRNV1_SF3', err_data)
        
        if np.all(SF[:274] == dec_data ^ 1):
            #print('test_04 (%3d) OK: nerr=%3d' % (n, nerr))
            ok += 1
        else:
            #print('test_04 (%3d) NG: nerr=%3d' % (n, nerr))
            ng += 1
    
    t = time.time() - t
    print('test_04: N = %4d OK/NG = %3d/%3d TIME =%7.3fs' % (len(SF), ok, ng, t))

# sdr_ldpc.decode_LDPC('BCNV1_SF2') --------------------------------------------
def test_05():
    hex = 'E29417E5E488CFF56B7400A6E5A913FFDDFDA2BC2D24FFDE13' + \
          '646E5871242E58AF0ABB304DEDF129AA1F300113FF63FF5C33' + \
          'FE285BF9EF7FF77D446CEC29AFFB643FF850009735FF686476' + \
          '22A2022B600C9CDB862DACD6F7631DCA245DF40282E5EFA760' + \
          'FEE9B388139C4556285A26804BBB190A362A57170332578BDD' + \
          '0CC86477BE72F8652E50BFFDC15657AEB45B0CA4D72DF218CA'
    SF = read_hex(hex)
    ok, ng = 0, 0
    t = time.time()
    
    for n in range(50):
        err_data = SF.copy()
        err_data[rng.integers(1200, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('BCNV1_SF2', err_data)
        
        if np.all(SF[:600] == dec_data ^ 1):
            #print('test_05 (%3d) OK: nerr=%3d' % (n, nerr))
            ok += 1
        else:
            #print('test_05 (%3d) NG: nerr=%3d' % (n, nerr))
            ng += 1
    
    t = time.time() - t
    print('test_05: N = %4d OK/NG = %3d/%3d TIME =%7.3fs' % (len(SF), ok, ng, t))

# sdr_ldpc.decode_LDPC('BCNV1_SF3') --------------------------------------------
def test_06():
    hex = 'FBFFF43E4E32A1BB5BCB5F7FEFBF80040FFFFFFB92D8E297F0' + \
          '8FDFFFFFFF8BAD2ED0ED560D1E7B17EF7E1D466C7E14095B75' + \
          '3528732B0DBBED18F225F8E1BC388956'
    SF = read_hex(hex)
    ok, ng = 0, 0
    t = time.time()
    
    for n in range(30):
        err_data = SF.copy()
        err_data[rng.integers(528, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('BCNV1_SF3', err_data)
        
        if np.all(SF[:264] == dec_data ^ 1):
            #print('test_06 (%3d) OK: nerr=%3d' % (n, nerr))
            ok += 1
        else:
            #print('test_06 (%3d) NG: nerr=%3d' % (n, nerr))
            ng += 1
    
    t = time.time() - t
    print('test_06: N = %4d OK/NG = %3d/%3d TIME =%7.3fs' % (len(SF), ok, ng, t))

# sdr_ldpc.decode_LDPC('BCNV2') ------------------------------------------------
def test_07():
    hex = '5CA92F4075A0006DDCC02A522FFD64695BB0008809750F4B6C' + \
          '0087B26E469E3B6FD2C19F13FAD5D6C360E72ED54C2607B594' + \
          'A4CF0EDB258BBD81AFDF27A700E0BB872ACAA9B0A73B'
    SF = read_hex(hex)
    ok, ng = 0, 0
    t = time.time()
    
    for n in range(30):
        err_data = SF.copy()
        err_data[rng.integers(576, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('BCNV2', err_data)
        
        if np.all(SF[:288] == dec_data):
            #print('test_07 (%3d) OK: nerr=%3d' % (n, nerr))
            ok += 1
        else:
            #print('test_07 (%3d) NG: nerr=%3d' % (n, nerr))
            ng += 1
    
    t = time.time() - t
    print('test_07: N = %4d OK/NG = %3d/%3d TIME =%7.3fs' % (len(SF), ok, ng, t))

# sdr_ldpc.decode_LDPC('BCNV3') ------------------------------------------------
def test_08():
    hex = '79B9BF475BB772627ACA00937800F6435791290D282004101F' + \
          'FEFC0000011B49C75A03DC08D2F01B2DFE098366E90DF20005' + \
          'BC17F8265E2D8E02CD89E047DF0CEDC6740BD2CFA83AF11A1F' + \
          '69B4788A31A3A8C49FBAA27BB5B9798E1F40C9CF58717DA2D4' + \
          '416B00875AD79686186DAF781D981E0AAA14E7588C1'
    SF = read_hex(hex)
    ok, ng = 0, 0
    t = time.time()
    
    for n in range(50):
        err_data = SF.copy()
        err_data[rng.integers(972, size=n)] ^= 1
        
        dec_data, nerr = sdr_ldpc.decode_LDPC('BCNV3', err_data)
        
        if np.all(SF[:486] == dec_data):
            #print('test_08 (%3d) OK: nerr=%3d' % (n, nerr))
            ok += 1
        else:
            #print('test_08 (%3d) NG: nerr=%3d' % (n, nerr))
            ng += 1
    
    t = time.time() - t
    print('test_08: N = %4d OK/NG = %3d/%3d TIME =%7.3fs' % (len(SF), ok, ng, t))

# MAX_ITER and NM_EMS tuning ---------------------------------------------------
def test_09():
    hex = '79B9BF475BB772627ACA00937800F6435791290D282004101F' + \
          'FEFC0000011B49C75A03DC08D2F01B2DFE098366E90DF20005' + \
          'BC17F8265E2D8E02CD89E047DF0CEDC6740BD2CFA83AF11A1F' + \
          '69B4788A31A3A8C49FBAA27BB5B9798E1F40C9CF58717DA2D4' + \
          '416B00875AD79686186DAF781D981E0AAA14E7588C1'
    SF = read_hex(hex)
    
    for niter in (5, 10, 15, 20, 25):
        for nm_ems in (2, 3, 4, 5, 6, 8, 10, 12):
            sdr_nb_ldpc.MAX_ITER = niter
            sdr_nb_ldpc.NM_EMS = nm_ems
            
            ok, ng = 0, 0
            t = time.time()
            for n in range(50):
                err_data = SF.copy()
                err_data[rng.integers(972, size=n)] ^= 1
                
                dec_data, nerr = sdr_ldpc.decode_LDPC('BCNV3', err_data)
                
                if np.all(SF[:486] == dec_data):
                    #print('test_08 (%3d) OK: nerr=%3d' % (n, nerr))
                    ok += 1
                else:
                    #print('test_08 (%3d) NG: nerr=%3d' % (n, nerr))
                    ng += 1
            
            t = time.time() - t
            print('test_09: MAX_ITER=%2d NM_EMS=%2d N = %4d OK/NG = %3d/%3d TIME =%7.3fs' % (
                niter, nm_ems, len(SF), ok, ng, t))

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
    test_05()
    test_06()
    test_07()
    test_08()
    #test_09()
