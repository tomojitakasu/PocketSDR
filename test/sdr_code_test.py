#!/usr/bin/env python3
#
#  Unit test for sdr_code.py
#
import sys, time
sys.path.append('../python')
sys.path.append('.')
import numpy as np
import sdr_code

#
#  GNSS-DSP-tools (https://github.com/pmonta/GNSS-DSP-tools)
#
sys.path.append('../../GNSS-DSP-tools')
import gnsstools.gps.ca
import gnsstools.gps.l1cp
import gnsstools.gps.l1cd
import gnsstools.gps.l2cm
import gnsstools.gps.l5i
import gnsstools.gps.l5q
import gnsstools.glonass.ca
import gnsstools.galileo.e1b
import gnsstools.galileo.e1c
import gnsstools.galileo.e5ai
import gnsstools.galileo.e5aq
import gnsstools.galileo.e5bi
import gnsstools.galileo.e5bq
import gnsstools.galileo.e6b
import gnsstools.galileo.e6c
import gnsstools.beidou.b1i
import gnsstools.beidou.b1cd
import gnsstools.beidou.b1cp
import gnsstools.beidou.b2ad
import gnsstools.beidou.b2ap
import gnsstools.beidou.b3i

# code to hex string -----------------------------------------------------------
def code_hex(code):
    str = ''
    hex = 0
    for i in range(len(code)):
        hex = (hex << 1) + (1 if code[i] == 1 else 0)
        if i % 4 == 3:
            str += '%1X' % (hex)
            hex = 0
    return str

# code to octal string ---------------------------------------------------------
def code_oct(code):
    str = ''
    oct = 0
    for i in range(len(code)):
        oct = (oct << 1) + (1 if code[i] == 1 else 0)
        if i % 3 == 2:
            str += '%1d' % (oct)
            oct = 0
    return str

# code to binary string --------------------------------------------------------
def code_bin(code):
    str = ''
    for c in code:
       str += ('1' if c == 1 else '0')
    return str

# gen_code() test by references ------------------------------------------------
def test01():
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L1CA', prn)
        code_ref = 2 * gnsstools.gps.ca.ca_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 L1CA: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L1CP', prn)
        code_ref = 2 * gnsstools.gps.l1cp.l1cp_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [-1, 1])
        if not np.all(code == code_ref):
            err = 1
    print('test01 L1CP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L1CD', prn)
        code_ref = 2 * gnsstools.gps.l1cd.l1cd_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [-1, 1])
        if not np.all(code == code_ref):
            err = 1
    print('test01 L1CD: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('L2CM', prn)
        code_ref = 2 * gnsstools.gps.l2cm.l2cm_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [-1, 0])
        if not np.all(code == code_ref):
            err = 1
    for prn in range(159, 211):
        code = sdr_code.gen_code('L2CM', prn)
        code_ref = 2 * gnsstools.gps.l2cm.l2cm_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [-1, 0])
        if not np.all(code == code_ref):
            err = 1
    print('test01 L2CM: %s' % ('NG' if err else 'OK'))
   
    #err = 0
    #for prn in range(1, 64):
    #    code = sdr_code.gen_code('L2CL', prn)
    #    code_ref = 2 * gnsstools.gps.l2cl.l2cl_code(prn) - 1
    #    code_ref = sdr_code.mod_code(code_ref, [1, 0])
    #    if not np.all(code == code_ref):
    #        err = 1
    #for prn in range(159, 211):
    #    code = sdr_code.gen_code('L2CL', prn)
    #    code_ref = 2 * gnsstools.gps.l2cl.l2cl_code(prn) - 1
    #    code_ref = sdr_code.mod_code(code_ref, [1, 0])
    #    if not np.all(code == code_ref):
    #        err = 1
    #print('test01 L2CL: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L5I', prn)
        code_ref = 2 * gnsstools.gps.l5i.l5i_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 L5I : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L5Q', prn)
        code_ref = 2 * gnsstools.gps.l5q.l5q_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 L5Q : %s' % ('NG' if err else 'OK'))
    
    err = 0
    code = sdr_code.gen_code('G1CA', 1)
    code_ref = 2 * gnsstools.glonass.ca.ca_code() - 1
    if not np.all(code == code_ref):
        err = 1
    print('test01 G1CA: %s' % ('NG' if err else 'OK'))
    
    err = 0
    code = sdr_code.gen_code('G2CA', 1)
    code_ref = 2 * gnsstools.glonass.ca.ca_code() - 1
    if not np.all(code == code_ref):
        err = 1
    print('test01 G2CA: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E1B', prn)
        code_ref = 2 * gnsstools.galileo.e1b.e1b_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [-1, 1])
        if not np.all(code == code_ref):
            err = 1
    print('test01 E1B : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E1C', prn)
        code_ref = 2 * gnsstools.galileo.e1c.e1c_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [-1, 1])
        if not np.all(code == code_ref):
            err = 1
    print('test01 E1C : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E5AI', prn)
        code_ref = 2 * gnsstools.galileo.e5ai.e5ai_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 E5AI: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E5AQ', prn)
        code_ref = 2 * gnsstools.galileo.e5aq.e5aq_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 E5AQ: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E5BI', prn)
        code_ref = 2 * gnsstools.galileo.e5bi.e5bi_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 E5BI: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E5BQ', prn)
        code_ref = 2 * gnsstools.galileo.e5bq.e5bq_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 E5BQ: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E6B', prn)
        code_ref = 2 * gnsstools.galileo.e6b.e6b_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 E6B : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E6C', prn)
        code_ref = 2 * gnsstools.galileo.e6c.e6c_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 E6C : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B1I', prn)
        code_ref = 2 * gnsstools.beidou.b1i.b1i_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 B1I : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B1CD', prn)
        code_ref = 2 * gnsstools.beidou.b1cd.b1cd_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [-1, 1])
        if not np.all(code == code_ref):
            err = 1
    print('test01 B1CD: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B1CP', prn)
        code_ref = 2 * gnsstools.beidou.b1cp.b1cp_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [-1, 1])
        if not np.all(code == code_ref):
            err = 1
    print('test01 B1CP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B2I', prn)
        code_ref = 2 * gnsstools.beidou.b1i.b1i_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 B2I : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B2AD', prn)
        code_ref = 2 * gnsstools.beidou.b2ad.b2ad_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 B2aD: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B2AP', prn)
        code_ref = 2 * gnsstools.beidou.b2ap.b2ap_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 B2aP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B3I', prn)
        code_ref = 2 * gnsstools.beidou.b3i.b3i_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test01 B3I : %s' % ('NG' if err else 'OK'))
    
# sec_code() test by references ------------------------------------------------
def test02():
    err = 0
    for prn in range(1, 210):
        code = sdr_code.sec_code('L1CP', prn)
        code_ref = 2 * gnsstools.gps.l1cp.secondary_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test02 L1CP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 63):
        code = sdr_code.sec_code('B1CP', prn)
        code_ref = 2 * gnsstools.beidou.b1cp.secondary_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test02 B1CP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 63):
        code = sdr_code.sec_code('B2AP', prn)
        code_ref = 2 * gnsstools.beidou.b2ap.secondary_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test02 B2aP: %s' % ('NG' if err else 'OK'))

# gen_code() test by dump ------------------------------------------------------
def test03():
    print('test03 L6D:')
    for prn in range(193, 202):
        code = sdr_code.gen_code('L6D', prn)
        str = code_oct(code)
        print('PRN=%3d CODE= %s ... %s' % (prn, str[:8], str[-8:]))
    
    print('test03 L6E:')
    for prn in range(203, 212):
        code = sdr_code.gen_code('L6E', prn)
        str = code_oct(code)
        print('PRN=%3d CODE= %s ... %s' % (prn, str[:8], str[-8:]))
    
    print('test03 G1OCD:')
    for prn in (0, 1, 2, 61, 62, 63):
        code = sdr_code.gen_code('G1OCD', prn)
        str1 = code_hex(code[:64:2])
        str2 = code_hex(code[-64::2])
        print('PRN=%3d CODE= %s ... %s' % (prn, str1, str2))
    
    print('test03 G1OCP:')
    for prn in (0, 1, 2, 61, 62, 63):
        code = sdr_code.gen_code('G1OCP', prn)
        str1 = code_hex(code[3:128:4])
        str2 = code_hex(code[-125::4])
        print('PRN=%3d CODE= %s ... %s' % (prn, str1, str2))
    
    print('test03 G2OCP:')
    for prn in (0, 1, 2, 61, 62, 63):
        code = sdr_code.gen_code('G2OCP', prn)
        str1 = code_hex(code[3:128:4])
        str2 = code_hex(code[-125::4])
        print('PRN=%3d CODE= %s ... %s' % (prn, str1, str2))
    
    print('test03 G3OCD:')
    for prn in (0, 1, 2, 61, 62, 63):
        code = sdr_code.gen_code('G3OCD', prn)
        str1 = code_hex(code[:32])
        str2 = code_hex(code[-32:])
        print('PRN=%3d CODE= %s ... %s' % (prn, str1, str2))
    
    print('test03 G3OCP:')
    for prn in (0, 1, 2, 61, 62, 63):
        code = sdr_code.gen_code('G3OCP', prn)
        str1 = code_hex(code[:32])
        str2 = code_hex(code[-32:])
        print('PRN=%3d CODE= %s ... %s' % (prn, str1, str2))
    
    print('test03 B2BI:')
    for prn in (1, 2, 3, 61, 62, 63):
        code = sdr_code.gen_code('B2BI', prn)
        str = code_oct(code)
        print('PRN=%3d CODE= %s ... %s' % (prn, str[:8], str[-8:]))
    
    print('test03 I5S:')
    for prn in (1, 2, 3, 12, 13, 14):
        code = sdr_code.gen_code('I5S', prn)
        str = code_oct(np.hstack([-1, -1, code]))
        print('PRN=%3d CODE= %s ...' % (prn, str[:4]))
    
    print('test03 ISS:')
    for prn in (1, 2, 3, 12, 13, 14):
        code = sdr_code.gen_code('ISS', prn)
        str = code_oct(np.hstack([-1, -1, code]))
        print('PRN=%3d CODE= %s ...' % (prn, str[:4]))
    
    print('test03 I1SD:')
    for prn in (1, 2, 3, 12, 13, 14):
        code = sdr_code.gen_code('I1SD', prn)
        str = code_oct(code[1::2])
        print('PRN=%3d CODE= %s ... %s' % (prn, str[:8], str[-8:]))
    
    print('test03 I1SP:')
    for prn in (1, 2, 3, 12, 13, 14):
        code = sdr_code.gen_code('I1SP', prn)
        str = code_oct(code[1::2])
        print('PRN=%3d CODE= %s ... %s' % (prn, str[:8], str[-8:]))
    
# sec_code() test by dump ------------------------------------------------------
def test04():
    print('test04 I1SP:')
    for prn in (1, 2, 3, 12, 13, 14):
        code = sdr_code.sec_code('I1SP', prn)
        str = code_oct(code)
        print('PRN=%3d CODE= %s ... %s' % (prn, str[:8], str[-8:]))

# main -------------------------------------------------------------------------
if __name__ == '__main__':
    test01()
    test02()
    test03()
    test04()
    
