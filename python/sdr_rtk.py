#!/usr/bin/env python
#
#  PocketSDR Python Library - RTKLIB Wrapper Functions
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
#
import os
from ctypes import *
import numpy as np

# load RTKLIB ([1]) ------------------------------------------------------------
dir = os.path.dirname(__file__)
try:
    librtk = cdll.LoadLibrary(dir + '/../lib/win32/librtk.so')
except:
    librtk = cdll.LoadLibrary(dir + '/../lib/linux/librtk.so')

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
    
