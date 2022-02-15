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

# start sequence table ---------------------------------------------------------

B2BI_start = ( # PRN 1 - 63
    0o26773275, 0o64773151, 0o22571523, 0o03270234, 0o25271603,
    0o42471422, 0o42071026, 0o10070621, 0o32631660, 0o51031210,
    0o24752203, 0o67353533, 0o25353617, 0o11351722, 0o61351343,
    0o16550441, 0o04153547, 0o37651752, 0o40652553, 0o12451253,
    0o34450664, 0o15313657, 0o56312563, 0o71510447, 0o44513562,
    0o54112445, 0o00111432, 0o55610115, 0o60613030, 0o36410161,
    0o73013021, 0o65010372, 0o12013173, 0o14011703, 0o35360744,
    0o65561461, 0o04561533, 0o35661303, 0o31661552, 0o12463623,
    0o34462214, 0o55062742, 0o25323543, 0o64320656, 0o13121550,
    0o05221747, 0o64741521, 0o17540076, 0o13540627, 0o16541066,
    0o72540775, 0o10640752, 0o05442537, 0o73301542, 0o65500312,
    0o31503365, 0o51102623, 0o70100474, 0o00100015, 0o24402044,
    0o20402615, 0o27426631, 0o10625632)

B2BI_end = ( # PRN 1 - 63
    0o01362377, 0o54270774, 0o41305112, 0o26377564, 0o71754171,
    0o44530033, 0o63454537, 0o52114120, 0o15654621, 0o12615765,
    0o23740542, 0o07467654, 0o52575257, 0o55226274, 0o01160270,
    0o50756326, 0o27542214, 0o10640254, 0o14350465, 0o57452211,
    0o00071604, 0o10263607, 0o13020015, 0o47474176, 0o16076344,
    0o55540654, 0o62507667, 0o63416213, 0o32014021, 0o43533653,
    0o61313161, 0o03246551, 0o07756360, 0o01251744, 0o27367153,
    0o77223601, 0o11666400, 0o35322566, 0o07107560, 0o46612101,
    0o11231514, 0o50710211, 0o34555532, 0o03034702, 0o75766350,
    0o50550432, 0o45030464, 0o01547030, 0o33762036, 0o57616221,
    0o55327237, 0o16072557, 0o64716537, 0o21130334, 0o16343063,
    0o21304050, 0o36574544, 0o31701764, 0o65447760, 0o14703362,
    0o26526364, 0o23410705, 0o34572376)

# code to hex ------------------------------------------------------------------
def code_hex(code):
    hex = 0
    for i in range(len(code)):
        hex = (hex << 1) + (1 if code[i] == 1 else 0)
        if i % 4 == 3:
            print('%1X' % (hex), end='')
            hex = 0
    print('')

# code to octal ----------------------------------------------------------------
def code_oct(code):
    str = ''
    oct = 0
    for i in range(len(code)):
        oct = (oct << 1) + (1 if code[i] == 1 else 0)
        if i % 3 == 2:
            str += '%1d' % (oct)
            oct = 0
    return str

# code to binary ---------------------------------------------------------------
def code_bin(code):
    bin = 0
    for c in code:
       bin = (bin << 1) + (1 if c == 1 else 0)
    return bin

# main -------------------------------------------------------------------------
if __name__ == '__main__':
    
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L1CA', prn)
        code_ref = 2 * gnsstools.gps.ca.ca_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test L1CA: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L1CP', prn)
        code_ref = 2 * gnsstools.gps.l1cp.l1cp_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [1, -1])
        if not np.all(code == code_ref):
            err = 1
    print('test L1CP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L1CD', prn)
        code_ref = 2 * gnsstools.gps.l1cd.l1cd_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [1, -1])
        if not np.all(code == code_ref):
            err = 1
    print('test L1CD: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('L2CM', prn)
        code_ref = 2 * gnsstools.gps.l2cm.l2cm_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [1, 0])
        if not np.all(code == code_ref):
            err = 1
    for prn in range(159, 211):
        code = sdr_code.gen_code('L2CM', prn)
        code_ref = 2 * gnsstools.gps.l2cm.l2cm_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [1, 0])
        if not np.all(code == code_ref):
            err = 1
    print('test L2CM: %s' % ('NG' if err else 'OK'))
   
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
    #print('test L2CL: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L5I', prn)
        code_ref = 2 * gnsstools.gps.l5i.l5i_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test L5I : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 211):
        code = sdr_code.gen_code('L5Q', prn)
        code_ref = 2 * gnsstools.gps.l5q.l5q_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test L5Q : %s' % ('NG' if err else 'OK'))
    
    err = 0
    code = sdr_code.gen_code('G1CA', 1)
    code_ref = 2 * gnsstools.glonass.ca.ca_code() - 1
    if not np.all(code == code_ref):
        err = 1
    print('test G1CA: %s' % ('NG' if err else 'OK'))
    
    err = 0
    code = sdr_code.gen_code('G2CA', 1)
    code_ref = 2 * gnsstools.glonass.ca.ca_code() - 1
    if not np.all(code == code_ref):
        err = 1
    print('test G2CA: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E1B', prn)
        code_ref = 2 * gnsstools.galileo.e1b.e1b_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [1, -1])
        if not np.all(code == code_ref):
            err = 1
    print('test E1B : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E1C', prn)
        code_ref = 2 * gnsstools.galileo.e1c.e1c_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [1, -1])
        if not np.all(code == code_ref):
            err = 1
    print('test E1C : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E5aI', prn)
        code_ref = 2 * gnsstools.galileo.e5ai.e5ai_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test E5aI: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E5aQ', prn)
        code_ref = 2 * gnsstools.galileo.e5aq.e5aq_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test E5aQ: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E5bI', prn)
        code_ref = 2 * gnsstools.galileo.e5bi.e5bi_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test E5bI: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E5bQ', prn)
        code_ref = 2 * gnsstools.galileo.e5bq.e5bq_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test E5bQ: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E6B', prn)
        code_ref = 2 * gnsstools.galileo.e6b.e6b_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test E6B : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 51):
        code = sdr_code.gen_code('E6C', prn)
        code_ref = 2 * gnsstools.galileo.e6c.e6c_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test E6C : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B1I', prn)
        code_ref = 2 * gnsstools.beidou.b1i.b1i_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test B1I : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B1CD', prn)
        code_ref = 2 * gnsstools.beidou.b1cd.b1cd_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [1, -1])
        if not np.all(code == code_ref):
            err = 1
    print('test B1CD: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B1CP', prn)
        code_ref = 2 * gnsstools.beidou.b1cp.b1cp_code(prn) - 1
        code_ref = sdr_code.mod_code(code_ref, [1, -1])
        if not np.all(code == code_ref):
            err = 1
    print('test B1CP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B2I', prn)
        code_ref = 2 * gnsstools.beidou.b1i.b1i_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test B2I : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B2AD', prn)
        code_ref = 2 * gnsstools.beidou.b2ad.b2ad_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test B2aD: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B2AP', prn)
        code_ref = 2 * gnsstools.beidou.b2ap.b2ap_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test B2aP: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B2BI', prn)
        if code_bin(code[:24]) != B2BI_start[prn-1] or \
           code_bin(code[-24:]) != B2BI_end[prn-1]:
            err = 1
    print('test B2bI: %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 64):
        code = sdr_code.gen_code('B3I', prn)
        code_ref = 2 * gnsstools.beidou.b3i.b3i_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test B3I : %s' % ('NG' if err else 'OK'))
    
    err = 0
    for prn in range(1, 210):
        code = sdr_code.sec_code('L1CP', prn)
        code_ref = 2 * gnsstools.gps.l1cp.secondary_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test L1CPo: %s' % ('NG' if err else 'OK'))

    err = 0
    for prn in range(1, 63):
        code = sdr_code.sec_code('B1CP', prn)
        code_ref = 2 * gnsstools.beidou.b1cp.secondary_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test B1CPo: %s' % ('NG' if err else 'OK'))

    err = 0
    for prn in range(1, 63):
        code = sdr_code.sec_code('B2AP', prn)
        code_ref = 2 * gnsstools.beidou.b2ap.secondary_code(prn) - 1
        if not np.all(code == code_ref):
            err = 1
    print('test B2aPo: %s' % ('NG' if err else 'OK'))

