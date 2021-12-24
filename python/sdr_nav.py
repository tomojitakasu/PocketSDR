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
#  [5] IS-QZSS-L6-001, Quasi-Zenith Satellite System Interface Specification
#      Centimeter Level Augmentation Service, November 5, 2018
#  [6] IS-QZSS-TV-003, Quasi-Zenith Satellite System Interface Specification
#      Positioning Technology Verification Service, December 27, 2019
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-12-24  1.0  new
#
from math import *
import numpy as np
from sdr_func import *
import sdr_fec, sdr_rtk

# constants --------------------------------------------------------------------
THRES_SYNC  = 0.03      # threshold of symbol sync
THRES_LOST  = 0.003     # threshold of symbol lost

# object class -----------------------------------------------------------------
class Nav: pass

# new nav data -----------------------------------------------------------------
def nav_new(nav_opt):
    nav = Nav()
    nav.ssync = nav.fsync = nav.rev = 0
    nav.syms = np.zeros(18000, dtype='uint8')
    nav.tsyms = np.zeros(18000)
    nav.bits = np.zeros(1000, dtype='uint8')
    nav.data = []
    nav.count = [0, 0]
    return nav

# initialize nav data ----------------------------------------------------------
def nav_init(nav):
    nav.ssync = nav.fsync = nav.rev = 0
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
    elif ch.sig == 'L5SI':
        decode_L5SI(ch)
    elif ch.sig == 'G1CA':
        decode_G1CA(ch)
    elif ch.sig == 'G2CA':
        decode_G2CA(ch)
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
    else:
        return

# decode L1CA nav data ([1]) ---------------------------------------------------
def decode_L1CA(ch):
    preamb = (1, 0, 0, 0, 1, 0, 1, 1)
    
    # L1 SBAS
    if (ch.prn >= 120 and ch.prn <= 158):
        decode_SBAS(ch)
        return
    
    # sync nav symbol
    if not sync_symb(ch, 20):
        return
    
    if ch.nav.fsync > 0: # frame sync
        if ch.lock == ch.nav.fsync + 6000:
            decode_LNAV(ch, ch.nav.syms[-308:-8] ^ ch.nav.rev, ch.nav.rev)
    else:
        # sync and decode LNAV subframe
        rev = sync_frame(ch, preamb, ch.nav.syms[-308:])
        if rev >= 0:
            decode_LNAV(ch, ch.nav.syms[-308:-8] ^ rev, rev)

# decode LNAV ------------------------------------------------------------------
def decode_LNAV(ch, syms, rev):
    time = ch.time + ch.coff - 20e-3 * 308
    
    # test LNAV parity
    if test_LNAV_parity(syms):
        ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(syms) # LNAV subframe (300 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$LNAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = 0
        ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,LNAV PARITY ERROR' % (time, ch.sig, ch.prn))

# test LNAV parity -------------------------------------------------------------
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

# decode L1CD nav data ---------------------------------------------------------
def decode_L1CD(ch):
    pass # unsupported

# decode SBAS nav data ---------------------------------------------------------
def decode_SBAS(ch):
    
    # sync nav symbol
    if not sync_symb(ch, 2):
        return
    
    if ch.nav.fsync > 0: # frame sync
        if ch.lock == ch.nav.fsync + 1000:
            search_SBAS_msgs(ch)
    
    elif (ch.lock - ch.nav.ssync) % 250 == 0:
        search_SBAS_msgs(ch)

# search SBAS message ----------------------------------------------------------
def search_SBAS_msgs(ch):
    
    # decode 1/2 FEC (1028 syms -> 508 bits)
    bits = sdr_fec.decode_conv(ch.nav.syms[-1028:] * 255)
    
    # sync SBAS message
    for i in range(250):
        rev = sync_SBAS_msgs(bits[i:i+258])
        if rev >= 0:
            decode_SBAS_msgs(ch, bits[i:i+250] ^ rev, rev)
            break

# sync SBAS message ------------------------------------------------------------
def sync_SBAS_msgs(bits):
    preamb_A = (0, 1, 0, 1, 0, 0, 1, 1)
    preamb_B = (1, 0, 0, 1, 1, 0, 1, 0)
    preamb_C = (1, 1, 0, 0, 0, 1, 1, 0)
    
    if (np.all(bits[0:8] == preamb_A) and np.all(bits[250:258] == preamb_B)) or \
       (np.all(bits[0:8] == preamb_B) and np.all(bits[250:258] == preamb_C)) or \
       (np.all(bits[0:8] == preamb_C) and np.all(bits[250:258] == preamb_A)):
        return 0
    if (np.all(bits[0:8] != preamb_A) and np.all(bits[250:258] != preamb_B)) or \
       (np.all(bits[0:8] != preamb_B) and np.all(bits[250:258] != preamb_C)) or \
       (np.all(bits[0:8] != preamb_C) and np.all(bits[250:258] != preamb_A)):
        return 1
    return -1

# decode SBAS message ----------------------------------------------------------
def decode_SBAS_msgs(ch, bits, rev):
    time = ch.time
    
    # test CRC
    buff = pack_bits(bits, 6)
    if sdr_rtk.crc24q(buff, 29) == sdr_rtk.getbitu(buff, 232, 24):
        ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # SBAS message (250 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$SBAS,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = 0
        ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,SBAS FRAME ERROR' % (time, ch.sig, ch.prn))

# decode L2CM nav data ---------------------------------------------------------
def decode_L2CM(ch):
    pass # unsupported

# decode L5I nav data ----------------------------------------------------------
def decode_L5I(ch):
    
    # L5 SBAS
    if (ch.prn >= 120 and ch.prn <= 158):
        decode_SBAS(ch)
        return
    
    # sync secondary code
    if not sync_sec_code(ch):
        return
    
    if ch.nav.fsync > 0: # frame sync
        if ch.lock == ch.nav.fsync + 3000:
            search_CNAV_frame(ch)
    
    elif (ch.lock - ch.nav.ssync) % 1000 == 0:
        search_CNAV_frame(ch)
    
# search GPS/QZSS CNAV frame ---------------------------------------------------
def search_CNAV_frame(ch):
    preamb = (1, 0, 0, 0, 1, 0, 1, 1)
    
    # decode 1/2 FEC (1228 syms -> 608 bits)
    bits = sdr_fec.decode_conv(ch.nav.syms[-1228:] * 255)
    
    # search and decode CNAV frame
    for i in range(300):
        rev = sync_frame(ch, preamb, bits[i:i+308])
        if rev >= 0:
            decode_CNAV(ch, bits[i:i+300] ^ rev, rev)
            break

# decode GPS/QZSS CNAV subframe ------------------------------------------------
def decode_CNAV(ch, bits, rev):
    time = ch.time + ch.coff - 4e-3 * 270
    
    # test CRC
    buff = pack_bits(bits, 4)
    if sdr_rtk.crc24q(buff, 35) == sdr_rtk.getbitu(buff, 280, 24):
        ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # CNAV subframe (300 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$CNAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = 0
        ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,CNAV FRAME ERROR' % (time, ch.sig, ch.prn))

# decode L5SI nav data ([6]) ---------------------------------------------------
def decode_L5SI(ch):
    decode_SBAS(ch)

# decode G1CA nav data ---------------------------------------------------------
def decode_G1CA(ch):
    pass # unsupported

# decode G2CA nav data ---------------------------------------------------------
def decode_G2CA(ch):
    pass # unsupported

# decode E1B nav data ([2]) ----------------------------------------------------
def decode_E1B(ch):
    preamb = (0, 1, 0, 1, 1, 0, 0, 0, 0, 0)
    
    # add symbol buffer
    add_buff(ch.nav.syms, 1 if ch.trk.P[-1].real >= 0.0 else 0)
    
    if ch.nav.fsync > 0: # frame sync
        if ch.lock == ch.nav.fsync + 500:
            decode_gal_INAV(ch, ch.nav.syms[-510:-10] ^ ch.nav.rev, ch.nav.rev)
    else:
        # sync  and decode Galileo I/NAV pages
        rev = sync_frame(ch, preamb, ch.nav.syms[-510:])
        if rev >= 0:
            decode_gal_INAV(ch, ch.nav.syms[-510:-10] ^ rev, rev)

# decode Galileo I/NAV pages ([2]) ---------------------------------------------
def decode_gal_INAV(ch, syms, rev):
    time = ch.time + ch.coff - 4e-3 * 270
    
    # decode Galileo symbols (240 syms x 2 -> 114 bits x 2)
    bits1 = decode_gal_syms(syms[ 10:250], 30, 8)
    bits2 = decode_gal_syms(syms[260:500], 30, 8)
    
    # test even and odd pages
    if bits1[0] != 0 or bits2[0] != 1:
        return
    
    # test CRC
    bits = np.hstack([bits1, bits2[:106]])
    buff = pack_bits(bits, 4)
    if sdr_rtk.crc24q(buff, 25) == sdr_rtk.getbitu(buff, 200, 24):
        ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # I/NAV 2 pages (220 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$INAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = 0
        ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,INAV FRAME ERROR' % (time, ch.sig, ch.prn))

# decode E5AI nav data ([2]) ---------------------------------------------------
def decode_E5AI(ch):
    preamb = (1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0)
    
    # sync secondary code
    if not sync_sec_code(ch):
        return

    if ch.nav.fsync > 0: # frame sync
        if ch.lock == ch.nav.fsync + 10000:
            decode_gal_FNAV(ch, ch.nav.syms[-512:-12] ^ ch.nav.rev, ch.nav.rev)
    else:
        # sync and decode Galileo F/NAV page
        rev = sync_frame(ch, preamb, ch.nav.syms[-512:])
        if rev >= 0:
            decode_gal_FNAV(ch, ch.nav.syms[-512:-12] ^ rev, rev)

# decode Galileo F/NAV page ([2]) ----------------------------------------------
def decode_gal_FNAV(ch, syms, rev):
    time = ch.time + ch.coff - 20e-3 * 512
    
    # decode Galileo symbols (488 syms -> 238 bits)
    bits = decode_gal_syms(syms[12:500], 61, 8)
    
    # test CRC
    buff = pack_bits(bits, 2)
    if sdr_rtk.crc24q(buff, 27) == sdr_rtk.getbitu(buff, 216, 24):
        ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # F/NAV page (238 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$FNAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = 0
        ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,FNAV FRAME ERROR' % (time, ch.sig, ch.prn))

# decode E5BI nav data ---------------------------------------------------------
def decode_E5BI(ch):
    pass # unsuppored

# decode E6B nav data ([3]) ----------------------------------------------------
def decode_E6B(ch):
    preamb = (1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0)
    
    # add symbol
    add_buff(ch.nav.syms, 1 if ch.trk.P[-1].real >= 0.0 else 0)
    
    if ch.nav.fsync > 0: # frame sync
        if ch.lock == ch.nav.fsync + 1000:
            decode_gal_CNAV(ch, ch.nav.syms[-1016:-16] ^ ch.nav.rev, ch.nav.rev)
    else:
        # sync and decode Galileo C/NAV frame
        rev = sync_frame(ch, preamb, ch.nav.syms[-1016:])
        if rev >= 0:
            decode_gal_CNAV(ch, ch.nav.syms[-1016:-16] ^ rev, rev)

# decode Galileo C/NAV frame ([3]) ---------------------------------------------
def decode_gal_CNAV(ch, syms, rev):
    time = ch.time + ch.coff - 1e-3 * 1016
    
    # decode Galileo symbols (984 syms -> 486 bits)
    bits = decode_gal_syms(syms[16:], 123, 8)
    
    # test CRC
    buff = pack_bits(bits, 2)
    if sdr_rtk.crc24q(buff, 58) == sdr_rtk.getbitu(buff, 464, 24):
        ch.nav.fsync = ch.lock
        ch.nav.rev = rev
        data = pack_bits(bits) # C/NAV frame (486 bits)
        ch.nav.data.append((time, data))
        ch.nav.count[0] += 1
        log(3, '$CNAV,%.3f,%s,%d,%s' % (time, ch.sig, ch.prn, hex_str(data)))
    else:
        ch.nav.fsync = 0
        ch.nav.rev = 0
        ch.nav.count[1] += 1
        log(3, '$LOG,%.3f,%s,%d,CNAV FRAME ERROR' % (time, ch.sig, ch.prn))

# decode Galileo symbols--------------------------------------------------------
def decode_gal_syms(syms, ncol, nrow):
    
    # decode block-interleave
    syms = syms.reshape(nrow, ncol).T.ravel()
    
    # decode 1/2 FEC
    syms[1::2] ^= 1 # invert G2
    return sdr_fec.decode_conv(syms * 255)

# decode B1I nav data ----------------------------------------------------------
def decode_B1I(ch):
    pass # unsupported (D1, D2)

# decode B1CD nav data ---------------------------------------------------------
def decode_B1CD(ch):
    pass # unsupported (CNV1)

# decode B2I nav data ----------------------------------------------------------
def decode_B2I(ch):
    decode_B1I

# decode B2AD nav data ---------------------------------------------------------
def decode_B2AD(ch):
    pass # unsupported (CNV2)

# decode B2BI nav data ---------------------------------------------------------
def decode_B2BI(ch):
    pass # unsupported (CNV3)

# decode B3I nav data ----------------------------------------------------------
def decode_B3I(ch):
    decode_B1I

# sync navigation symbol by polarity transition --------------------------------
def sync_symb(ch, N):
    n = 1 if N <= 2 else 2
    code = [-1] * n + [1] * n
    
    if ch.nav.ssync == 0:
        P = np.dot(ch.trk.P[-n * 2:].real, code) / (n * 2)
        if abs(P) >= THRES_SYNC:
            ch.nav.ssync = ch.lock - n
            log(4, '$LOG,%.3f,%s,%d,SYMBOL SYNC (%.3f)' % (ch.time, ch.sig, ch.prn, P))
    
    elif (ch.lock - ch.nav.ssync) % N == 0:
        P = np.mean(ch.trk.P[-N:].real)
        if abs(P) >= THRES_LOST:
            add_buff(ch.nav.syms, 1 if P >= 0.0 else 0)
            add_buff(ch.nav.tsyms, ch.time)
            return True
        else:
            ch.nav.ssync = 0
            log(4, '$LOG,%.3f,%s,%d,SYMBOL LOST (%.3f)' % (ch.time, ch.sig, ch.prn, P))
    return False

# sync secondary code ----------------------------------------------------------
def sync_sec_code(ch):
    N = len(ch.sec_code)
    
    if ch.nav.ssync == 0:
        P = np.dot(ch.trk.P[-N:].real, ch.sec_code) / N
        if abs(P) >= THRES_SYNC:
            ch.nav.ssync = ch.lock
            ch.nav.rev = 1 if P < 0.0 else 0 # polarity
            log(4, '$LOG,%.3f,%s,%d,SECCODE SYNC (%.3f)' % (ch.time, ch.sig, ch.prn, P))
    
    elif (ch.lock - ch.nav.ssync) % N == 0:
        P = np.dot(ch.trk.P[-N:].real, ch.sec_code) / N
        if abs(P) >= THRES_LOST:
            add_buff(ch.nav.syms, (1 if P >= 0.0 else 0) ^ ch.nav.rev)
            add_buff(ch.nav.tsyms, ch.time)
            return True
        else:
            ch.nav.ssync = 0
            log(4, '$LOG,%.3f,%s,%d,SECCODE LOST (%.3f)' % (ch.time, ch.sig, ch.prn, P))
    return False

# sync navigation frame by preambles -------------------------------------------
def sync_frame(ch, preamb, bits):
    N = len(preamb)
    
    if np.all(bits[:N] == preamb) and np.all(bits[-N:] == preamb):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (N)' % (ch.time, ch.sig, ch.prn))
        return 0 # normal
    
    elif np.all(bits[:N] != preamb) and np.all(bits[-N:] != preamb):
        log(4, '$LOG,%.3f,%s,%d,FRAME SYNC (R)' % (ch.time, ch.sig, ch.prn))
        return 1 # reversed
    
    else:
        return -1 # unsync

