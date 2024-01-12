#
#  Pocket SDR Python Library - GNSS SDR Nav Data Functions
#
#  References:
#  [1] IS-GPS-200K, NAVSTAR GPS Space Segment/Navigation User Segment
#      Interfaces, May 19, 2019
#  [2] Galileo Open Service Signal In Space Interface Control Document -
#      Issue 1, February 2010
#  [3] Galileo E6-B/C Codes Technical Note - Issue 1, January 2019
#  [4] IS-QZSS-PNT-004, Quasi-Zenith Satellite System Interface Specification
#      Satellite Positioning, Navigation and Timing Service, November 5, 2018
#  [5] IS-QZSS-L6-003, Quasi-Zenith Satellite System Interface Specification
#      Centimeter Level Augmentation Service, August 20, 2020
#  [6] IS-QZSS-TV-004, Quasi-Zenith Satellite System Interface Specification
#      Positioning Technology Verification Service, September 27, 2023
#  [7] BeiDou Navigation Satellite System Signal In Space Interface Control
#      Document - Open Service Signal B1I (Version 3.0), February, 2019
#  [8] BeiDou Navigation Satellite System Signal In Space Interface Control
#      Document - Open Service Signal B1C (Version 1.0), December, 2017
#  [9] BeiDou Navigation Satellite System Signal In Space Interface Control
#      Document - Open Service Signal B2a (Version 1.0), December, 2017
#  [10] BeiDou Navigation Satellite System Signal In Space Interface Control
#      Document - Open Service Signal B2b (Version 1.0), July, 2020
#  [11] BeiDou Navigation Satellite System Signal In Space Interface Control
#      Document - Open Service Signal B3I (Version 1.0), February, 2018
#  [12] IS-GPS-800F, Navstar GPS Space Segment / User Segment L1C Interfaces,
#      March 4, 2019
#  [13] IS-GPS-705A, Navstar GPS Space Segment / User Segment L5 Interfaces,
#      June 8, 2010
#  [14] Global Navigation Satellite System GLONASS Interface Control Document
#      Navigational radiosignal In bands L1, L2 (Edition 5.1), 2008
#  [15] IRNSS SIS ICD for Standard Positioning Service version 1.1, August,
#      2017
#  [16] GLONASS Interface Control Document Code Devision Multiple Access Open
#      Service Navigation Signal in L3 frequency band Edition 1.0, 2016
#  [17] NavIC Signal in Space ICD for Standard Positioning Service in L1
#      Frequency version 1.0, August, 2023
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-12-24  1.0  new
#  2022-01-04  1.1  support sync to secondary code for pilot signals
#  2022-01-13  1.2  support L1CD, L6D, L6E, G1CA, G2CA, B1I, B2I, B3I
#  2022-01-20  1.3  support I5S, ISS
#                   move sec-code sync to sdr_ch.py
#  2022-01-28  1.4  support G3OCD
#  2023-12-28  1.5  fix L1CA_SBAS, L5I, L5I_SBAS, L5SI, G1CA and G3OCD
#  2024-01-06  1.6  support I1SD
#  2024-01-12  1.7  support B1CD, B2AD, B2BI
#
from math import *
import numpy as np
from sdr_func import *
import sdr_fec, sdr_rtk, sdr_code, sdr_ldpc

# constants --------------------------------------------------------------------
THRES_SYNC  = 0.04      # threshold for symbol sync
THRES_LOST  = 0.003     # threshold for symbol lost

BCH_CORR_TBL = ( # BCH(15,11,1) error correction table ([7] Table 5-2)
    0b000000000000000, 0b000000000000001, 0b000000000000010, 0b000000000010000,
    0b000000000000100, 0b000000100000000, 0b000000000100000, 0b000010000000000,
    0b000000000001000, 0b100000000000000, 0b000001000000000, 0b000000010000000,
    0b000000001000000, 0b010000000000000, 0b000100000000000, 0b001000000000000)

# code caches ------------------------------------------------------------------
CNV2_SF1   = {}
BCNV1_SF1A = {}
BCNV1_SF1B = {}
IRNV1_SF1  = {}

# nav data class ---------------------------------------------------------------
class Nav: pass

# new nav data -----------------------------------------------------------------
def nav_new(nav_opt):
    nav = Nav()
    nav.ssync = 0       # symbol sync time as lock count (0: no-sync)
    nav.fsync = 0       # nav frame sync time as lock count (0: no-sync)
    nav.rev = 0         # code polarity (0: normal, 1: reversed)
    nav.seq = 0         # sequence number (TOW, TOI, ...)
    nav.nerr = 0        # number of error corrected
    nav.syms = np.zeros(18000, dtype='uint8') # nav symbols buffer
    nav.tsyms = np.zeros(18000) # nav symbols time (for debug)
    nav.data = []       # navigation data buffer
    nav.count = [0, 0]  # navigation data count (OK, error)
    nav.opt = nav_opt   # navigation option string
    return nav

# initialize nav data ----------------------------------------------------------
def nav_init(nav):
    nav.ssync = nav.fsync = nav.rev = nav.seq = 0
    nav.syms[:] = 0
    nav.tsyms[:] = 0.0

# decode nav data --------------------------------------------------------------
def nav_decode(ch):
    if ch.sig == 'L1CA':
        decode_L1CA(ch)
    elif ch.sig == 'L1S':
        decode_L1S(ch)
    elif ch.sig == 'L1CB':
        decode_L1CB(ch)
    elif ch.sig == 'L1CD':
        decode_L1CD(ch)
    elif ch.sig == 'L2CM':
        decode_L2CM(ch)
    elif ch.sig == 'L5I':
        decode_L5I(ch)
    elif ch.sig == 'L6D':
        decode_L6D(ch)
    elif ch.sig == 'L6E':
        decode_L6E(ch)
    elif ch.sig == 'L5SI':
        decode_L5SI(ch)
    elif ch.sig == 'L5SIV':
        decode_L5SIV(ch)
    elif ch.sig == 'G1CA':
        decode_G1CA(ch)
    elif ch.sig == 'G2CA':
        decode_G2CA(ch)
    elif ch.sig == 'G3OCD':
        decode_G3OCD(ch)
    elif ch.sig == 'E1B':
        decode_E1B(ch)
    elif ch.sig == 'E5AI':
        decode_E5AI(ch)
    elif ch.sig == 'E5BI':
        decode_E5BI(ch)
    elif ch.sig == 'E6B':
        decode_E6B(ch)
    elif ch.sig == 'B1I':
        decode_B1I(ch)
    elif ch.sig == 'B1CD':
        decode_B1CD(ch)
    elif ch.sig == 'B2I':
        decode_B2I(ch)
    elif ch.sig == 'B2AD':
        decode_B2AD(ch)
    elif ch.sig == 'B2BI':
        decode_B2BI(ch)
    elif ch.sig == 'B3I':
        decode_B3I(ch)
    elif ch.sig == 'I1SD':
        decode_I1SD(ch)
    elif ch.sig == 'I5S':
        decode_I5S(ch)
    elif ch.sig == 'ISS':
        decode_ISS(ch)
    else:
        return

# decode L1CA nav data ([1]) ---------------------------------------------------
def decode_L1CA(ch):
    preamb = (1, 0, 0, 0, 1, 0, 1, 1)
    
    if ch.prn >= 120 and ch.prn <= 158: # L1 SBAS
        decode_SBAS(ch)
        return
    
    if not sync_symb(ch, 20): # sync symbol
        return
    
    if ch.nav.fsync > 0: # sync LNAV subframe
        if ch.lock == ch.nav.fsync + 6000:
            rev = sync_frame(ch, preamb, ch.nav.syms[-308:])
            if rev == ch.nav.rev:
                decode_LNAV(ch, ch.nav.syms[-308:-8] ^ rev, rev)
            else:
                ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= 20 * 308 + 1000:
        # sync and decode LNAV subframe
        rev = sync_frame(ch, preamb, ch.nav.syms[-308:])
        if rev >= 0:
            decode_LNAV(ch, ch.nav.syms[-308:-8] ^ rev, rev)

# decode LNAV ([1]) ------------------------------------------------------------
def decode_LNAV(ch, syms, rev):
    time = ch.time - 20e-3 * 308
    
    if test_LNAV_parity(syms):
        ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(syms) # LNAV subframe (300 bits)
        ch.nav.seq = sdr_rtk.getbitu(data, 30, 17) # tow (x 6s)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$LNAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,LNAV PARITY ERROR' % (time, ch.sig, ch.prn))

# test LNAV parity ([1]) -------------------------------------------------------
def test_LNAV_parity(syms):
    mask = (0x2EC7CD2, 0x1763E69, 0x2BB1F34, 0x15D8F9A, 0x1AEC7CD, 0x22DEA27)
    
    buff = 0
    for i in range(10):
        for j in range(30):
            buff = (buff << 1) | syms[i*30+j]
        if buff & (1 << 30):
            buff ^= 0x3FFFFFC0
        for j in range(6):
            if xor_bits((buff >> 6) & mask[j]) != (buff >> (5 - j)) & 1:
                return False
    return True

# decode L1S nav data ([4]) ----------------------------------------------------
def decode_L1S(ch):
    decode_SBAS(ch)

# decode L1CB nav data ([4]) ---------------------------------------------------
def decode_L1CB(ch):
    decode_L1CA(ch)

# decode L1CD nav data ([12]) --------------------------------------------------
def decode_L1CD(ch):
    
    # add symbol buffer
    add_buff(ch.nav.syms, 1 if ch.trk.P[-1].real >= 0.0 else 0)
    
    if ch.nav.fsync > 0: # sync CNAV-2 frame
        if ch.lock == ch.nav.fsync + 1800:
            toi = (ch.nav.seq + 1) % 400
            rev = sync_CNV2_frame(ch, ch.nav.syms[-1852:], toi)
            sym = ch.nav.syms[-1800] # WN MSB in SF2
            if rev == ch.nav.rev and (sym ^ rev):
                decode_CNV2(ch, ch.nav.syms[-1852:-52] ^ rev, rev, toi)
            else:
                ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= 1852 + 100:
        # search and decode CNAV-2 frame
        for toi in range(400):
            rev = sync_CNV2_frame(ch, ch.nav.syms[-1852:], toi)
            sym = ch.nav.syms[-1800] # WN MSB in SF2
            if rev >= 0 and (sym ^ rev):
                decode_CNV2(ch, ch.nav.syms[-1852:-52] ^ rev, rev, toi)
                break

# sync CNAV-2 frame by subframe 1 symbols ([12]) -------------------------------
def sync_CNV2_frame(ch, syms, toi):
    
    # generate CNAV-2 subframe 1 symbols
    global CNV2_SF1
    if len(CNV2_SF1) == 0:
        for t in range(400):
            code = sdr_code.LFSR(51, sdr_code.rev_reg(t & 0xFF, 8), 0b10011111, 8)
            bit9 = (t >> 8) & 1
            CNV2_SF1[t] = np.hstack([bit9, ((code + 1) // 2) ^ bit9])
    
    SF1 = CNV2_SF1[toi]
    SFn = CNV2_SF1[(toi + 1) % 400]
    
    if np.all(syms[:52] == SF1) and np.all(syms[-52:] == SFn):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (N) TOI=%d' % (ch.time, ch.sig, ch.prn, toi))
        return 1 # normal
    
    if np.all(syms[:52] != SF1) and np.all(syms[-52:] != SFn):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (R) TOI=%d' % (ch.time, ch.sig, ch.prn, toi))
        return 0 # reversed
    return -1

# decode CNAV-2 frame ([12]) ---------------------------------------------------
def decode_CNV2(ch, syms, rev, toi):
    time = ch.time - 18.52
    
    # decode block-interleave (38 x 46 = 1748 syms)
    syms_d = syms[52:].reshape(46, 38).T.ravel()
    
    # decode LDPC (1200 + 548 syms -> 600 + 274 bits)
    SF2, nerr1 = sdr_ldpc.decode_LDPC('CNV2_SF2', syms_d[:1200])
    SF3, nerr2 = sdr_ldpc.decode_LDPC('CNV2_SF3', syms_d[1200:])
    
    if test_CRC(SF2) and test_CRC(SF3):
        bits = np.hstack([unpack_data(toi, 9), SF2, SF3])
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        ch.nav.seq = toi
        ch.nav.nerr = nerr1 + nerr2
        data = pack_bits(bits) # CNAV-2 frame (9 + 600 + 274 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$CNV2,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.ssync = ch.nav.fsync = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,CNV2 FRAME ERROR' % (time, ch.sig, ch.prn))

# decode SBAS nav data ---------------------------------------------------------
def decode_SBAS(ch):
    
    if not sync_symb(ch, 2): # sync symbol
        return
    
    if ch.nav.fsync > 0: # sync SBAS message
        if ch.lock >= ch.nav.fsync + 1000:
            search_SBAS_msgs(ch)
        elif ch.lock > ch.nav.fsync + 2000:
            ch.nav.fsync = ch.nav.rev = 0
    elif ch.lock > 1088 + 1000:
        search_SBAS_msgs(ch)

# search SBAS message ----------------------------------------------------------
def search_SBAS_msgs(ch):
    
    # decode 1/2 FEC (544 syms -> 258 + 8 bits)
    bits = sdr_fec.decode_conv(ch.nav.syms[-544:] * 255)
    
    # search and decode SBAS message
    rev = sync_SBAS_msgs(bits[:258])
    if rev >= 0:
        decode_SBAS_msgs(ch, bits[:250] ^ rev, rev)

# sync SBAS message ------------------------------------------------------------
def sync_SBAS_msgs(bits):
    preamb = ((0, 1, 0, 1, 0, 0, 1, 1), (1, 0, 0, 1, 1, 0, 1, 0),
              (1, 1, 0, 0, 0, 1, 1, 0))
    
    for i in range(3):
        j = (i + 1) % 3
        if np.all(bits[:8] == preamb[i]) and np.all(bits[-8:] == preamb[j]):
            return 0
        if np.all(bits[:8] != preamb[i]) and np.all(bits[-8:] != preamb[j]):
            return 1
    return -1

# decode SBAS message ----------------------------------------------------------
def decode_SBAS_msgs(ch, bits, rev):
    time = ch.time - 1e-3 * 1088
    
    if test_CRC(bits):
        ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # SBAS message (250 bits)
        if ch.sig == 'L1CA':
            ch.nav.seq = sdr_rtk.getbitu(data, 8, 6) # SBAS message type
        else:
            ch.nav.seq = sdr_rtk.getbitu(data, 6, 6) # L5 SBAS message type
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$SBAS,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,SBAS FRAME ERROR' % (time, ch.sig, ch.prn))

# decode L2CM nav data ---------------------------------------------------------
def decode_L2CM(ch):
    
    # add symbol buffer
    add_buff(ch.nav.syms, 1 if ch.trk.P[-1].real >= 0.0 else 0)
    
    if ch.nav.fsync > 0: # sync CNAV subframe
        if ch.lock >= ch.nav.fsync + 600:
            search_CNAV_frame(ch)
        elif ch.lock > ch.nav.fsync + 1200:
            ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    elif ch.lock > 644 + 50:
        search_CNAV_frame(ch)

# decode L5I nav data ([13]) ---------------------------------------------------
def decode_L5I(ch):
    
    if (ch.prn >= 120 and ch.prn <= 158): # L5 SBAS
        decode_L5_SBAS(ch)
        return
    
    if not sync_sec_code(ch): # sync secondary code
        return
    
    if ch.nav.fsync > 0: # sync CNAV subframe
        if ch.lock >= ch.nav.fsync + 6000:
            search_CNAV_frame(ch)
        elif ch.lock > ch.nav.fsync + 12000:
            ch.nav.fsync = ch.nav.rev = 0
    elif ch.lock > 6440 + 1000:
        search_CNAV_frame(ch)

# search CNAV subframe ([13]) --------------------------------------------------
def search_CNAV_frame(ch):
    preamb = (1, 0, 0, 0, 1, 0, 1, 1)
    
    # decode 1/2 FEC (644 syms -> 308 + 8 bits)
    bits = sdr_fec.decode_conv(ch.nav.syms[-644:] * 255)
    
    # search and decode CNAV subframe
    rev = sync_frame(ch, preamb, bits[:308])
    if rev >= 0:
        decode_CNAV(ch, bits[:300] ^ rev, rev)

# decode CNAV subframe ([13]) --------------------------------------------------
def decode_CNAV(ch, bits, rev):
    time = ch.time - 1e-3 * 6440
    
    if test_CRC(bits):
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # CNAV subframe (300 bits)
        ch.nav.seq = sdr_rtk.getbitu(data, 20, 17) # tow count
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$CNAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,CNAV FRAME ERROR' % (time, ch.sig, ch.prn))

# decode L5SI nav data ([6]) ---------------------------------------------------
def decode_L5SI(ch):
    decode_SBAS(ch) # normal mode

# decode L5SI verification mode nav data ([6]) ---------------------------------
def decode_L5SIV(ch):
    decode_L5_SBAS(ch)

# decode L5 SBAS nav data ------------------------------------------------------
def decode_L5_SBAS(ch):
    
    if not sync_sec_code(ch): # sync secondary code
        return
    
    if ch.nav.fsync > 0: # sync L5 SBAS message
        if ch.lock >= ch.nav.fsync + 1000:
            search_L5_SBAS_msgs(ch)
        elif ch.lock > ch.nav.fsync + 2000:
            ch.nav.fsync = ch.nav.rev = 0
    elif ch.lock >= 3093 + 1000:
        search_L5_SBAS_msgs(ch)

# search L5 SBAS message -------------------------------------------------------
def search_L5_SBAS_msgs(ch):
    
    # decode 1/2 FEC (1546 syms -> 758 + 8 bits)
    bits = sdr_fec.decode_conv(ch.nav.syms[-1546:] * 255)
    
    # search and decode SBAS message
    rev = sync_L5_SBAS_msgs(bits[:758])
    if rev >= 0:
        decode_SBAS_msgs(ch, bits[500:750] ^ rev, rev)

# sync L5 SBAS message ---------------------------------------------------------
def sync_L5_SBAS_msgs(bits):
    preamb = ((0, 1, 0, 1), (1, 1, 0, 0), (0, 1, 1, 0), (1, 0, 0, 1),
        (0, 0, 1, 1), (1, 0, 1, 0))
    
    for i in range(6):
        j, k, m = (i + 1) % 6, (i + 2) % 6, (i + 3) % 6
        if np.all(bits[:4] == preamb[i]) and \
           np.all(bits[250:254] == preamb[j]) and \
           np.all(bits[500:504] == preamb[k]) and \
           np.all(bits[750:754] == preamb[m]):
            return 0
        if np.all(bits[:4] != preamb[i]) and \
           np.all(bits[250:254] != preamb[j]) and \
           np.all(bits[500:504] != preamb[k]) and \
           np.all(bits[750:754] != preamb[m]):
            return 1
    return -1

# decode L6D nav data ([5]) ----------------------------------------------------
def decode_L6D(ch):
    if ch.nav.fsync > 0: # sync L6 frame
        if ch.lock == ch.nav.fsync + 250:
            decode_L6_frame(ch, ch.nav.syms[-255:])
     
    elif ch.lock >= 255:
        # sync and decode L6 frame
        decode_L6_frame(ch, ch.nav.syms[-255:])

# sync and decode L6 frame ([5]) -----------------------------------------------
def decode_L6_frame(ch, syms):
    preamb = np.array([0x1A, 0xCF, 0xFC, 0x1D, ch.prn], dtype='uint8')
    
    # sync 2 premable differences
    n1 = np.count_nonzero((syms[1:5] - syms[0]) == preamb[1:] - preamb[0])
    n2 = np.count_nonzero((syms[-5:] - syms[0]) == preamb[:]  - preamb[0])
    
    if n1 + n2 < 8: # test # of symbol matchs
        ch.nav.ssync = ch.nav.fsync = 0
        return
    
    # restore symbols
    time = ch.time - 4e-3 * 255
    syms = np.array(syms[:250] + (int(preamb[0]) - syms[0]), dtype='uint8')
    
    # decode RS(255,223) and correct errors
    buff = np.hstack([np.zeros(9, dtype='uint8'), syms[4:250]])
    ch.nerr = sdr_fec.decode_rs(buff)
    
    if ch.nerr >= 0:
        ch.nav.ssync = ch.nav.fsync = ch.lock
        data = np.hstack([syms[:4], buff[9:]]) # L6 frame (250 syms)
        ch.nav.seq = sdr_rtk.getbitu(data, 40, 5) # vender + facility id
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$L6FRM,%.3f,%s,%d,%d,%s' % (time, ch.sig, ch.prn, ch.nerr, hex_str(data)))
    else:
        ch.nav.ssync = ch.nav.fsync = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,L6FRM RS ERROR' % (time, ch.sig, ch.prn))

# decode L6E nav data ----------------------------------------------------------
def decode_L6E(ch):
    decode_L6D(ch)

# decode G1CA nav data ([14]) --------------------------------------------------
def decode_G1CA(ch):
    time_mark = (1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0,
        0, 1, 0, 0, 1, 0, 1, 1, 0)
    
    if not sync_symb(ch, 10): # sync symbol
        return
    
    if ch.nav.fsync > 0: # sync GLONASS nav string
        if ch.lock >= ch.nav.fsync + 2000:
            rev = sync_frame(ch, time_mark, ch.nav.syms[-230:])
            if rev == ch.nav.rev:
                decode_glo_str(ch, ch.nav.syms[-200:] ^ rev, rev)
        elif ch.lock > ch.nav.fsync + 4000:
            ch.nav.fsync = ch.nav.rev = 0
    elif ch.lock >= 2300 + 2000:
        # sync and decode GLONASS nav string
        rev = sync_frame(ch, time_mark, ch.nav.syms[-230:])
        if rev >= 0:
            decode_glo_str(ch, ch.nav.syms[-200:] ^ rev, rev)

# decode GLONASS nav string ([14]) ---------------------------------------------
def decode_glo_str(ch, syms, rev):
    time = ch.time - 1e-3 * 2000
    
    # handle meander and relative code transformation ([14] fig.3.4)
    data = pack_bits(np.hstack([0, syms[0:168:2] ^ syms[2:170:2]]))
    
    if sdr_rtk.test_glostr(data):
        ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$GSTR,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,GSTR HAMMING ERROR' % (time, ch.sig, ch.prn))

# decode G2CA nav data ---------------------------------------------------------
def decode_G2CA(ch):
    decode_G1CA(ch)

# decode G3OCD nav data ([16]) -------------------------------------------------
def decode_G3OCD(ch):
    
    if not sync_sec_code(ch): # sync secondary code
        return
    
    if ch.nav.fsync > 0: # sync GLONASS L3OCD nav string
        if ch.lock >= ch.nav.fsync + 3000:
            search_glo_L3OCD_str(ch)
        elif ch.lock > ch.nav.fsync + 6000:
            ch.nav.fsync = ch.nav.rev = 0
    elif ch.lock > ch.nav.ssync + 6680:
        search_glo_L3OCD_str(ch)

# swap convolutional code G1 and G2 --------------------------------------------
def swap_syms(syms):
    ssyms = np.zeros(len(syms), dtype='uint8')
    ssyms[0::2] = syms[1::2]
    ssyms[1::2] = syms[0::2]
    return ssyms

# search GLONASS L3OCD nav string ----------------------------------------------
def search_glo_L3OCD_str(ch):
    preamb = (0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0)
    
    # decode 1/2 FEC (668 syms -> 320 + 8 bits)
    bits = sdr_fec.decode_conv(swap_syms(ch.nav.syms[-668:]) * 255)
    
    # search and decode GLONASS L3OCD nav string
    rev = sync_frame(ch, preamb, bits[:320])
    if rev >= 0:
        decode_glo_L3OCD_str(ch, bits[:300] ^ rev, rev)

# decode GLONASS L3OCD nav string ----------------------------------------------
def decode_glo_L3OCD_str(ch, bits, rev):
    time = ch.time - 1e-3 * 328
    
    if test_CRC(bits):
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # GLONASS L3OCD nav string (300 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$G3OCD,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,G3OCD STRING ERROR' % (time, ch.sig, ch.prn))

# decode E1B nav data ([2]) ----------------------------------------------------
def decode_E1B(ch):
    preamb = (0, 1, 0, 1, 1, 0, 0, 0, 0, 0)
    
    # add symbol buffer
    add_buff(ch.nav.syms, 1 if ch.trk.P[-1].real >= 0.0 else 0)
    
    if ch.nav.fsync > 0: # sync frame
        if ch.lock == ch.nav.fsync + 500:
            rev = sync_frame(ch, preamb, ch.nav.syms[-510:])
            if rev == ch.nav.rev:
                decode_gal_INAV(ch, ch.nav.syms[-510:-10] ^ rev, rev)
            else:
                ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= 510 + 250:
        # sync and decode Galileo I/NAV pages
        rev = sync_frame(ch, preamb, ch.nav.syms[-510:])
        if rev >= 0:
            decode_gal_INAV(ch, ch.nav.syms[-510:-10] ^ rev, rev)

# decode Galileo I/NAV pages ([2]) ---------------------------------------------
def decode_gal_INAV(ch, syms, rev):
    time = ch.time + ch.coff - 4e-3 * 510
    
    # decode Galileo symbols (240 syms x 2 -> 114 bits x 2)
    bits1 = decode_gal_syms(syms[ 10:250], 30, 8)
    bits2 = decode_gal_syms(syms[260:500], 30, 8)
    
    # test even and odd pages
    if bits1[0] != 0 or bits2[0] != 1:
        ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
        return
    
    bits = np.hstack([bits1, bits2[:106]])
    if test_CRC(bits):
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # Galileo I/NAV 2 pages (220 bits)
        ch.nav.seq = sdr_rtk.getbitu(data, 0, 6) # subframe ID
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$INAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,INAV FRAME ERROR' % (time, ch.sig, ch.prn))

# decode E5AI nav data ([2]) ---------------------------------------------------
def decode_E5AI(ch):
    preamb = (1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0)
    
    if not sync_sec_code(ch): # sync secondary code
        return
    
    if ch.nav.fsync > 0: # sync Galileo F/NAV page
        if ch.lock == ch.nav.fsync + 10000:
            rev = sync_frame(ch, preamb, ch.nav.syms[-512:])
            if rev == ch.nav.rev:
                decode_gal_FNAV(ch, ch.nav.syms[-512:-12] ^ rev, rev)
            else:
                ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= len(ch.sec_code) * 512 + 250:
        # sync and decode Galileo F/NAV page
        rev = sync_frame(ch, preamb, ch.nav.syms[-512:])
        if rev >= 0:
            decode_gal_FNAV(ch, ch.nav.syms[-512:-12] ^ rev, rev)

# decode Galileo F/NAV page ([2]) ----------------------------------------------
def decode_gal_FNAV(ch, syms, rev):
    time = ch.time + ch.coff - 20e-3 * 512
    
    # decode Galileo symbols (488 syms -> 238 bits)
    bits = decode_gal_syms(syms[12:500], 61, 8)
    
    if test_CRC(bits):
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # Galileo F/NAV page (238 bits)
        ch.nav.seq = sdr_rtk.getbitu(data, 0, 6) # page type
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$FNAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,FNAV FRAME ERROR' % (time, ch.sig, ch.prn))

# decode E5BI nav data ([2]) ---------------------------------------------------
def decode_E5BI(ch):
    preamb = (0, 1, 0, 1, 1, 0, 0, 0, 0, 0)
    
    if not sync_sec_code(ch): # sync secondary code
        return
    
    if ch.nav.fsync > 0: # sync frame
        if ch.lock == ch.nav.fsync + 2000:
            rev = sync_frame(ch, preamb, ch.nav.syms[-510:])
            if rev == ch.nav.rev:
                decode_gal_INAV(ch, ch.nav.syms[-510:-10] ^ rev, rev)
            else:
                ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= len(ch.sec_code) * 510 + 250:
        # sync and decode Galileo I/NAV pages
        rev = sync_frame(ch, preamb, ch.nav.syms[-510:])
        if rev >= 0:
            decode_gal_INAV(ch, ch.nav.syms[-510:-10] ^ rev, rev)

# decode E6B nav data ([3]) ----------------------------------------------------
def decode_E6B(ch):
    preamb = (1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0)
    
    # add symbol buffer
    add_buff(ch.nav.syms, 1 if ch.trk.P[-1].real >= 0.0 else 0)
    
    if ch.nav.fsync > 0: # sync Galileo C/NAV page
        if ch.lock == ch.nav.fsync + 1000:
            rev = sync_frame(ch, preamb, ch.nav.syms[-1016:])
            if rev == ch.nav.rev:
                decode_gal_CNAV(ch, ch.nav.syms[-1016:-16] ^ rev, rev)
            else:
                ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= 1016 + 1000:
        # sync and decode Galileo C/NAV page
        rev = sync_frame(ch, preamb, ch.nav.syms[-1016:])
        if rev >= 0:
            decode_gal_CNAV(ch, ch.nav.syms[-1016:-16] ^ rev, rev)

# decode Galileo C/NAV page ([3]) ----------------------------------------------
def decode_gal_CNAV(ch, syms, rev):
    time = ch.time + ch.coff - ch.T * 1016
    
    # decode Galileo symbols (984 syms -> 486 bits)
    bits = decode_gal_syms(syms[16:], 123, 8)
    
    if test_CRC(bits):
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # C/NAV frame (486 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$CNAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,CNAV FRAME ERROR' % (time, ch.sig, ch.prn))

# decode Galileo symbols ([2]) -------------------------------------------------
def decode_gal_syms(syms, ncol, nrow):
    
    # decode block-interleave
    syms = syms.reshape(nrow, ncol).T.ravel()
    
    # decode 1/2 FEC
    syms[1::2] ^= 1 # invert G2
    return sdr_fec.decode_conv(syms * 255)

# decode B1I nav data ([7]) ----------------------------------------------------
def decode_B1I(ch):
    if ch.prn >= 6 and ch.prn <= 58:
       return decode_B1I_D1(ch)
    else:
       return decode_B1I_D2(ch)

# decode B1I D1 nav data ([7]) -------------------------------------------------
def decode_B1I_D1(ch):
    preamb = (1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0)
    
    if not sync_sec_code(ch): # sync secondary code
        return
    
    if ch.nav.fsync > 0: # sync frame
        if ch.lock == ch.nav.fsync + 6000:
            rev = sync_frame(ch, preamb, ch.nav.syms[-311:])
            if rev == ch.nav.rev:
                decode_D1D2NAV(ch, 1, ch.nav.syms[-311:-11] ^ rev, rev)
            else:
                ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= len(ch.sec_code) * 311 + 1000:
        # sync and decode BDS D1 NAV subframe
        rev = sync_frame(ch, preamb, ch.nav.syms[-311:])
        if rev >= 0:
            decode_D1D2NAV(ch, 1, ch.nav.syms[-311:-11] ^ rev, rev)

# decode BDS D1/D2 NAV subframe ([7]) ------------------------------------------
def decode_D1D2NAV(ch, type, syms, rev):
    time = ch.time - ch.T * len(ch.sec_code) * 311
    
    syms[15:30] = decode_D1D2_BCH(syms[15:30])
    for i in range(30, 300, 30):
        s1 = decode_D1D2_BCH(syms[i  :i+30:2])
        s2 = decode_D1D2_BCH(syms[i+1:i+31:2])
        syms[i:i+30] = np.hstack([s1[:11], s2[:11], s1[11:], s2[11:]])
    
    ch.nav.ssync = ch.nav.fsync = ch.lock
    ch.nav.rev = rev
    data = pack_bits(syms) # D1/D2 NAV subframe (300 bits)
    ch.nav.data.append((time, data))
    ch.nav.count[0] += 1
    log(3, '$D%dNAV,%.3f,%s,%d,%s' % (time, type, ch.sig, ch.prn, hex_str(data)))

# decode symbols by BCH(15,11,1) ([7] Figure 5-4) ------------------------------
def decode_D1D2_BCH(syms):
    R = 0
    for i in range(15):
        R = (syms[i] << 3) ^ ((R & 1) * 0b1100) ^ (R >> 1)
    return syms ^ unpack_data(BCH_CORR_TBL[R], 15) # correct error

# decode B1I D2 nav data ([7]) -------------------------------------------------
def decode_B1I_D2(ch):
    preamb = (1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0)
    
    if not sync_symb(ch, 2): # sync symbol
        return
    
    if ch.nav.fsync > 0: # sync frame
        if ch.lock == ch.nav.fsync + 600:
            rev = sync_frame(ch, preamb, ch.nav.syms[-311:])
            if rev == ch.nav.rev:
                decode_D1D2NAV(ch, 2, ch.nav.syms[-311:-11] ^ rev, rev)
            else:
                ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= 2 * 311 + 1000:
        # sync and decode BDS D2 NAV subframe
        rev = sync_frame(ch, preamb, ch.nav.syms[-311:])
        if rev >= 0:
            decode_D1D2NAV(ch, 2, ch.nav.syms[-311:-11] ^ rev, rev)

# decode B1CD nav data ([8]) ---------------------------------------------------
def decode_B1CD(ch):
    
    # add symbol buffer
    add_buff(ch.nav.syms, 1 if ch.trk.P[-1].real >= 0.0 else 0)
    
    if ch.nav.fsync > 0: # sync B-CNAV1 frame
        if ch.lock == ch.nav.fsync + 1800:
            soh = (ch.nav.seq + 1) % 200
            rev = sync_BCNV1_frame(ch, ch.nav.syms[-1872:], soh)
            if rev == ch.nav.rev:
                decode_BCNV1(ch, ch.nav.syms[-1872:-72] ^ rev, rev, soh)
            else:
                ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= 1872 + 100:
        # search and decode B-CNAV1 frame
        for soh in range(200):
            rev = sync_BCNV1_frame(ch, ch.nav.syms[-1872:], soh)
            if rev >= 0:
                decode_BCNV1(ch, ch.nav.syms[-1872:-72] ^ rev, rev, soh)
                break

# sync B1CD B-CNAV1 frame by subframe 1 symbols ([8]) -------------------------
def sync_BCNV1_frame(ch, syms, soh):
    
    # generate CNAV-1 subframe 1 symbols (21 + 51 syms)
    global BCNV1_SF1A, BCNV1_SF1B
    if not ch.prn in BCNV1_SF1A:
        code = sdr_code.LFSR(21, sdr_code.rev_reg(ch.prn, 6), 0b010111, 6)
        BCNV1_SF1A[ch.prn] = (code + 1) // 2
    if len(BCNV1_SF1B) == 0:
        for soh in range(200):
            code = sdr_code.LFSR(51, sdr_code.rev_reg(soh, 8), 0b10011111, 8)
            BCNV1_SF1B[soh] = (code + 1) // 2
    
    SF1 = np.hstack([BCNV1_SF1A[ch.prn], BCNV1_SF1B[soh]])
    SFn = np.hstack([BCNV1_SF1A[ch.prn], BCNV1_SF1B[(soh + 1) % 200]])
    
    if np.all(syms[:72] == SF1) and np.all(syms[-72:] == SFn):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (N) SOH=%d' % (ch.time, ch.sig, ch.prn, soh))
        return 1 # normal
    
    if np.all(syms[:72] != SF1) and np.all(syms[-72:] != SFn):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (R) SOH=%d' % (ch.time, ch.sig, ch.prn, soh))
        return 0 # reversed
    return -1

# decode B1CD B-CNAV1 frame ([8]) ----------------------------------------------
def decode_BCNV1(ch, syms, rev, soh):
    time = ch.time - 18.72
    
    # decode block-interleave of SF2,3 (36 x 48 = 1728 syms)
    symsr = syms[72:].reshape(48, 36).T
    syms2 = np.zeros(1200, dtype='uint8')
    syms3 = np.zeros(528 , dtype='uint8')
    for i in range(11):
       syms2[i*96   :i*96+48] = symsr[i*3  ] # SF2
       syms2[i*96+48:i*96+96] = symsr[i*3+1] # SF2
       syms3[i*48   :i*48+48] = symsr[i*3+2] # SF3
    for i in range(22, 25):
       syms2[i*48   :i*48+48] = symsr[i+11 ] # SF2
    
    # decode LDPC (1200 + 528 syms -> 600 + 264 bits)
    SF2, nerr1 = sdr_ldpc.decode_LDPC('BCNV1_SF2', syms2)
    SF3, nerr2 = sdr_ldpc.decode_LDPC('BCNV1_SF3', syms3)
    bits = np.hstack([unpack_data(ch.prn, 6), unpack_data(soh, 8), SF2, SF3])
    
    if test_CRC(SF2) and test_CRC(SF3):
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        ch.nav.seq = soh
        ch.nav.nerr = nerr1 + nerr2
        data = pack_bits(bits) # CNAV-2 frame (14 + 600 + 264 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$BCNV1,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,BCNV1 FRAME ERROR' % (time, ch.sig, ch.prn))

# decode B2I nav data ([7]) ----------------------------------------------------
def decode_B2I(ch):
    decode_B1I(ch)

# decode B2AD nav data ([9]) ---------------------------------------------------
def decode_B2AD(ch):
    preamb = (
        1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0)
    
    if not sync_sec_code(ch): # sync secondary code
        return
    
    if ch.nav.fsync > 0: # sync frame
        if ch.lock >= ch.nav.fsync + 3000:
            rev = sync_frame(ch, preamb, ch.nav.syms[-624:])
            if rev >= 0:
                decode_BCNV2(ch, ch.nav.syms[-624:-24] ^ rev, rev)
        elif ch.lock > ch.nav.fsync + 6000:
            ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= len(ch.sec_code) * 624 + 1000:
        # sync and decode B-CNAV2 frame
        rev = sync_frame(ch, preamb, ch.nav.syms[-624:])
        if rev >= 0:
            decode_BCNV2(ch, ch.nav.syms[-624:-24] ^ rev, rev)

# decode B2AD B-CNAV2 frame ([9]) ----------------------------------------------
def decode_BCNV2(ch, syms, rev):
    time = ch.time - 3.12
    
    # decode LDPC (576 syms -> 288 bits)
    bits, nerr = sdr_ldpc.decode_LDPC('BCNV2', syms[24:])
    
    if test_CRC(bits):
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # B-CNAV2 frame (288 bits)
        ch.nav.seq = sdr_rtk.getbitu(data, 12, 18) # SOW
        ch.nav.nerr = nerr
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$BCNV2,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,BCNV2 FRAME ERROR' % (time, ch.sig, ch.prn))

# decode B2BI nav data ([10]) --------------------------------------------------
def decode_B2BI(ch):
    preamb = (1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0)
    preamb = np.hstack([preamb, unpack_data(ch.prn, 6)])
    
    # add symbol buffer
    add_buff(ch.nav.syms, 1 if ch.trk.P[-1].real >= 0.0 else 0)
    
    if ch.nav.fsync > 0: # sync frame
        if ch.lock >= ch.nav.fsync + 1000:
            rev = sync_frame(ch, preamb, ch.nav.syms[-1022:])
            if rev >= 0:
                decode_BCNV3(ch, ch.nav.syms[-1022:-22] ^ rev, rev)
        elif ch.lock > ch.nav.fsync + 2000:
            ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= 1022 + 1000:
        # sync and decode B-CNAV3 frame
        rev = sync_frame(ch, preamb, ch.nav.syms[-1022:])
        if rev >= 0:
            decode_BCNV3(ch, ch.nav.syms[-1022:-22] ^ rev, rev)

# decode B2BI B-CNAV3 frame ([10]) --------------------------------------------
def decode_BCNV3(ch, syms, rev):
    time = ch.time - 1.22
    
    # decode LDPC (972 syms -> 486 bits)
    bits, nerr = sdr_ldpc.decode_LDPC('BCNV3', syms[28:])
    
    if test_CRC(bits):
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # B-CNAV3 frame (486 bits)
        if ch.prn <= 5 or ch.prn >= 59: # PPP-B2b
            ch.nav.seq = sdr_rtk.getbitu(data, 0, 6) # message type
        else:
            ch.nav.seq = sdr_rtk.getbitu(data, 6, 20) # SOW
        ch.nav.nerr = nerr
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$BCNV3,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,BCNV3 FRAME ERROR' % (time, ch.sig, ch.prn))

# decode B3I nav data ([11]) ---------------------------------------------------
def decode_B3I(ch):
    decode_B1I(ch)

# decode I1SD nav data ([17]) --------------------------------------------------
def decode_I1SD(ch):
    
    # add symbol buffer
    add_buff(ch.nav.syms, 1 if ch.trk.P[-1].real >= 0.0 else 0)
    
    if ch.nav.fsync > 0: # sync NavIC L1-SPS NAV frame
        if ch.lock == ch.nav.fsync + 1800:
            toi = (ch.nav.seq + 1) % 400
            rev = sync_IRNV1_frame(ch, ch.nav.syms[-1852:], toi)
            if rev == ch.nav.rev:
                decode_IRNV1(ch, ch.nav.syms[-1852:-52] ^ rev, rev, toi)
            else:
                ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= 1852 + 100:
        # search and decode NavIC L1-SPS NAV frame
        for toi in range(400):
            rev = sync_IRNV1_frame(ch, ch.nav.syms[-1852:], toi)
            if rev >= 0:
                decode_IRNV1(ch, ch.nav.syms[-1852:-52] ^ rev, rev, toi)
                break

# sync I1SD NavIC L1-SPS NAV frame by subframe 1 symbols ([17]) ----------------
def sync_IRNV1_frame(ch, syms, toi):
    
    # generate NavIC L1-SPS NAV subframe 1 symbols
    global IRNV1_SF1
    if len(IRNV1_SF1) == 0:
        for t in range(400):
            code = sdr_code.LFSR(52, sdr_code.rev_reg(t+1, 9), 0b110111111, 9)
            IRNV1_SF1[t] = (code + 1) // 2
    
    SF1 = IRNV1_SF1[toi]
    SFn = IRNV1_SF1[(toi + 1) % 400]
    
    if np.all(syms[:52] == SF1) and np.all(syms[-52:] == SFn):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (N) TOI=%d' % (ch.time, ch.sig, ch.prn, toi+1))
        return 1 # normal
    
    if np.all(syms[:52] != SF1) and np.all(syms[-52:] != SFn):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (R) TOI=%d' % (ch.time, ch.sig, ch.prn, toi+1))
        return 0 # reversed
    return -1

# decode I1SD NavIC L1-SPS NAV frame ([17]) ------------------------------------
def decode_IRNV1(ch, syms, rev, toi):
    time = ch.time - 18.52
    
    # decode block-interleave (38 x 46 = 1748 syms)
    syms_d = syms[52:].reshape(46, 38).T.ravel()
    
    # decode LDPC (1200 + 548 syms -> 600 + 274 bits)
    SF2, nerr1 = sdr_ldpc.decode_LDPC('IRNV1_SF2', syms_d[:1200])
    SF3, nerr2 = sdr_ldpc.decode_LDPC('IRNV1_SF3', syms_d[1200:])
    
    if test_CRC(SF2) and test_CRC(SF3):
        bits = np.hstack([unpack_data(toi, 9), SF2, SF3])
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        ch.nav.seq = toi
        ch.nav.nerr = nerr1 + nerr2
        data = pack_bits(bits) # NavIC L1-SPS frame (9 + 600 + 274 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$IRNV1,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.ssync = ch.nav.fsync = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,IRNV1 FRAME ERROR' % (time, ch.sig, ch.prn))

# decode I5S nav data ([15]) ---------------------------------------------------
def decode_I5S(ch):
    preamb = (1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0) # 0xEB90
    
    if not sync_symb(ch, 20): # sync symbol
        return
    
    if ch.nav.fsync > 0: # sync NavIC SPS NAV subframe
        if ch.lock == ch.nav.fsync + 12000:
            rev = sync_frame(ch, preamb, ch.nav.syms[-616:])
            if rev == ch.nav.rev:
                decode_IRN_NAV(ch, ch.nav.syms[-616:-16] ^ rev, rev)
            else:
                ch.nav.ssync = ch.nav.fsync = ch.nav.rev = 0
    
    elif ch.lock >= 20 * 616 + 1000:
        # sync and decode NavIC SPS NAV subframe
        rev = sync_frame(ch, preamb, ch.nav.syms[-616:])
        if rev >= 0:
            decode_IRN_NAV(ch, ch.nav.syms[-616:-16] ^ rev, rev)

# decode I5S NavIC SPS NAV frame ([15]) ---------------------------------------
def decode_IRN_NAV(ch, syms, rev):
    time = ch.time - 20e-3 * 616
    
    # decode block-interleave
    syms = syms[16:].reshape(8, 73).T.ravel()
    
    # decode 1/2 FEC (584 syms -> 297 bits -> 286 bits)
    bits = sdr_fec.decode_conv(syms * 255)[:286]
    
    if test_CRC(bits):
        ch.nav.ssync = ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # NavIC SPS NAV subframe (286 bits)
        ch.nav.seq = sdr_rtk.getbitu(data, 8, 17) # TOWC
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$IRNAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.ssync = ch.nav.fsync = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,IRNAV FRAME ERROR' % (time, ch.sig, ch.prn))
    
# decode ISS nav data ([15]) ---------------------------------------------------
def decode_ISS(ch):
    decode_I5S(ch)

# sync nav symbols by bit transition -------------------------------------------
def sync_symb(ch, N):
    n = 1 if N <= 2 else 2
    code = [-1] * n + [1] * n
    
    if ch.nav.ssync == 0:
        P = np.dot(ch.trk.P[-2*n:].real, code) / (2 * n)
        if abs(P) >= THRES_SYNC:
            ch.nav.ssync = ch.lock - n
            log(4, '$LOG,%.3f,%s,%d,SYMBOL SYNC (%.3f)' % (ch.time, ch.sig, ch.prn, P))
    
    elif (ch.lock - ch.nav.ssync) % N == 0:
        P = np.mean(ch.trk.P[-N:].real)
        if abs(P) >= THRES_LOST:
            add_buff(ch.nav.syms, 1 if P >= 0.0 else 0)
            #add_buff(ch.nav.tsyms, ch.time) # for debug
            return True
        else:
            ch.nav.ssync = ch.nav.rev = 0
            log(4, '$LOG,%.3f,%s,%d,SYMBOL LOST (%.3f)' % (ch.time, ch.sig, ch.prn, P))
    return False

# sync secondary code ----------------------------------------------------------
def sync_sec_code(ch):
    N = len(ch.sec_code)
    
    if N < 2 or ch.trk.sec_sync == 0 or (ch.lock - ch.trk.sec_sync) % N != 0:
        return False
    else:
        add_buff(ch.nav.syms, 1 if np.mean(ch.trk.P[-N:].real) >= 0.0 else 0)
        return True

# sync nav frame by 2 preambles ------------------------------------------------
def sync_frame(ch, preamb, bits):
    N = len(preamb)
    
    if np.all(bits[:N] == preamb) and np.all(bits[-N:] == preamb):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (N)' % (ch.time, ch.sig, ch.prn))
        return 0 # normal
    
    if np.all(bits[:N] != preamb) and np.all(bits[-N:] != preamb):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (R)' % (ch.time, ch.sig, ch.prn))
        return 1 # reversed
    return -1

# test CRC (CRC24Q) ------------------------------------------------------------
def test_CRC(bits):
    N = (len(bits) - 24 + 7) // 8 * 8
    buff = pack_bits(bits, N + 24 - len(bits)) # aligned right
    return sdr_rtk.crc24q(buff, N // 8) == sdr_rtk.getbitu(buff, N, 24)

