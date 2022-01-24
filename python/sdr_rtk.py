#
#  Pocket SDR Python Library - RTKLIB Wrapper Functions
#
#  References:
#  [1] RTKLIB: An Open Source Program Package for GNSS Positioning
#      (https://github.com/tomojitakasu/RTKLIB)
#
#  Author:
#  T.TAKASU
#
#  History:
#  2021-12-24  1.0  new
#  2022-01-13  1.1  add API test_glostr()
#  2022-01-19  1.2  add stream function APIs
#
import os, time, platform
from ctypes import *
import numpy as np

# constants --------------------------------------------------------------------
STR_SERIAL   = 1
STR_FILE     = 2
STR_TCPSVR   = 3
STR_TCPCLI   = 4
STR_NTRIPSVR = 5
STR_NTRIPCLI = 6
STR_NTRIPCAS = 9
STR_UDPSVR   = 10
STR_UDPCLI   = 11

STR_MODE_R   = 0x1
STR_MODE_W   = 0x2
STR_MODE_RW  = 0x3

# load RTKLIB ([1]) ------------------------------------------------------------
env = platform.platform()
dir = os.path.dirname(__file__)
if 'Windows' in env:
    librtk = cdll.LoadLibrary(dir + '/../lib/win32/librtk.so')
elif 'Linux' in env:
    librtk = cdll.LoadLibrary(dir + '/../lib/linux/librtk.so')
else:
    printf('load librtk.so error for %s' % (env))
    exit()

# extract unsigned bits --------------------------------------------------------
def getbitu(data, pos, len):
    if data.dtype != 'uint8':
        return 0
    librtk.getbitu.restype = c_uint32
    p = data.ctypes.data_as(POINTER(c_uint8))
    return librtk.getbitu(p, c_int(pos), c_int(len))

# extract signed bits ----------------------------------------------------------
def getbits(data, pos, len):
    if data.dtype != 'uint8':
        return 0
    librtk.getbits.restype = c_int32
    p = data.ctypes.data_as(POINTER(c_uint8))
    return librtk.getbits(p, c_int(pos), c_int(len))

# set unsigned bits ------------------------------------------------------------
def setbitu(data, pos, len, val):
    if data.dtype != 'uint8':
        return
    p = data.ctypes.data_as(POINTER(c_uint8))
    librtk.setbitu(p, c_int(pos), c_int(len), c_uint32(val))

# set signed bits --------------------------------------------------------------
def setbits(data, pos, len, val):
    if data.dtype != 'uint8':
        return
    p = data.ctypes.data_as(POINTER(c_uint8))
    librtk.setbits(p, c_int(pos), c_int(len), c_int32(val))

# CRC 16 -----------------------------------------------------------------------
def crc16(data, len):
    if data.dtype != 'uint8':
        return 0
    librtk.rtk_crc16.restype = c_uint32
    p = data.ctypes.data_as(POINTER(c_uint8))
    return librtk.rtk_crc16(p, c_int(len))

# CRC 24Q ----------------------------------------------------------------------
def crc24q(data, len):
    if data.dtype != 'uint8':
        return 0
    librtk.rtk_crc24q.restype = c_uint32
    p = data.ctypes.data_as(POINTER(c_uint8))
    return librtk.rtk_crc24q(p, c_int(len))

# CRC 32 -----------------------------------------------------------------------
def crc32(data, len):
    if data.dtype != 'uint8':
        return 0
    librtk.rtk_crc32.restype = c_uint32
    p = data.ctypes.data_as(POINTER(c_uint8))
    return librtk.rtk_crc32(p, c_int(len))

# test GLONASS string ----------------------------------------------------------
def test_glostr(data):
    if data.dtype != 'uint8':
        return 0
    librtk.test_glostr.restype = c_int32
    p = data.ctypes.data_as(POINTER(c_uint8))
    return librtk.test_glostr(p)

# open stream ------------------------------------------------------------------
def stropen(type, mode, path):
    librtk.strnew.restype = c_void_p;
    stream = librtk.strnew()
    if not stream:
        return None
    if not librtk.stropen(c_void_p(stream), c_int32(type), c_int32(mode),
               c_char_p(path.encode())):
        print('stropen error mode=%d path=%s' % (mode, path))
        librtk.strfree(c_void_p(stream))
        return None
    return stream
        
# close stream -----------------------------------------------------------------
def strclose(stream):
    if str != None:
        librtk.strclose(c_void_p(stream))
        librtk.strfree(c_void_p(stream))

# write stream -----------------------------------------------------------------
def strwrite(stream, buff):
    if str == None or buff.dtype != 'uint8':
        return 0
    p = buff.ctypes.data_as(POINTER(c_uint8))
    return librtk.strwrite(c_void_p(stream), p, len(buff))

# read stream ------------------------------------------------------------------
def strread(stream, buff):
    if stream == None or buff.dtype != 'uint8':
        return 0
    p = buff.ctypes.data_as(POINTER(c_uint8))
    return librtk.strread(c_void_p(stream), p, len(buff))

# write line to stream ---------------------------------------------------------
def strwritel(stream, line):
    if stream == None:
        return 0
    buff = create_string_buffer(line)
    return librtk.strwrite(c_void_p(stream), buff, len(line))

# read line from stream --------------------------------------------------------
def strreadl(stream):
    if stream == None:
        return 0
    buff = create_string_buffer(1)
    line = ''
    while True:
        if librtk.strread(c_void_p(stream), buff, 1) == 0:
            time.sleep(0.01)
            continue
        line += buff.value.decode()
        if line[-1] == '\n':
            return line

# get stream status ------------------------------------------------------------
def strstat(stream):
    msg = create_string_buffer(256)
    stat = librtk.strstat(c_void_p(stream), msg)
    return stat, msg.value.decode()

# get stream summary -----------------------------------------------------------
def strsum(stream):
    inb, inr, outb, outr = c_int(), c_int(), c_int(), c_int()
    librtk.strsum(c_void_p(streaam), byref(inb), byref(inr), byref(outb),
        byref(outr))
    return inb.value, inr.value, outb.value, outr.value

