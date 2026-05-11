#
#  nmake file for Pocket SDR GNSS-SDR library for MSVC (libsdr.dll, libsdr.lib)
#

SRC = ..\..\src
INSTALL = ..\win32_msvc
CC = cl
CXX = cl
LD = link
AR = lib
LDFLAGS = /NOLOGO /DLL

# SoapySDR headers are bundled under lib/SoapySDR/include. The import library
# (SoapySDR.lib) is not bundled — set SOAPY_LIB_DIR to a directory containing
# SoapySDR.lib (e.g. PothosSDR install). SoapySDR support is auto-disabled if
# the import library cannot be found.
SOAPY_LIB_DIR = C:\Program Files\PothosSDR\lib

!IF EXIST("$(SOAPY_LIB_DIR)\SoapySDR.lib")
SOAPY_DEF = /DSOAPYSDR
SOAPY_LIB = "$(SOAPY_LIB_DIR)\SoapySDR.lib"
!ELSE
SOAPY_DEF =
SOAPY_LIB =
!ENDIF

INCFLAGS = /I$(SRC) /I..\RTKLIB\src /I..\cyusb /I..\pocketfft /I..\SoapySDR\include
CFLAGS = /nologo /O2 /W3 /MD /arch:AVX2 /utf-8 $(SOAPY_DEF) /DAVX2 /DWIN32 /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS $(INCFLAGS)
CXXFLAGS = $(CFLAGS) /TP
LDLIBS = librtk.lib libfec.lib libldpc.lib libpocketfft.lib ..\win32_msvc\CyAPI.lib $(SOAPY_LIB) setupapi.lib avrt.lib wsock32.lib winmm.lib user32.lib

OBJ = sdr_cmn.obj sdr_func.obj sdr_code.obj sdr_code_gal.obj sdr_ch.obj \
      sdr_nav.obj sdr_pvt.obj sdr_rcv.obj sdr_fec.obj sdr_ldpc.obj sdr_nb_ldpc.obj \
      sdr_usb.obj sdr_dev.obj sdr_conf.obj sdr_sdev.obj sdr_array.obj

TARGET = libsdr.dll libsdr.lib

all: $(TARGET)

libsdr.dll: $(OBJ)
	$(LD) $(LDFLAGS) /DEF:libsdr.def /OUT:$@ $(OBJ) $(LDLIBS)

sdr_cmn.obj: $(SRC)\sdr_cmn.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_cmn.c
sdr_func.obj: $(SRC)\sdr_func.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_func.c
sdr_code.obj: $(SRC)\sdr_code.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_code.c
sdr_code_gal.obj: $(SRC)\sdr_code_gal.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_code_gal.c
sdr_ch.obj: $(SRC)\sdr_ch.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_ch.c
sdr_nav.obj: $(SRC)\sdr_nav.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_nav.c
sdr_pvt.obj: $(SRC)\sdr_pvt.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_pvt.c
sdr_rcv.obj: $(SRC)\sdr_rcv.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_rcv.c
sdr_fec.obj: $(SRC)\sdr_fec.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_fec.c
sdr_ldpc.obj: $(SRC)\sdr_ldpc.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_ldpc.c
sdr_nb_ldpc.obj: $(SRC)\sdr_nb_ldpc.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_nb_ldpc.c
sdr_usb.obj: $(SRC)\sdr_usb.c $(SRC)\pocket_sdr.h
	$(CXX) /c $(CXXFLAGS) /Fo$@ $(SRC)\sdr_usb.c
sdr_dev.obj: $(SRC)\sdr_dev.c $(SRC)\pocket_sdr.h
	$(CXX) /c $(CXXFLAGS) /Fo$@ $(SRC)\sdr_dev.c
sdr_conf.obj: $(SRC)\sdr_conf.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_conf.c
sdr_sdev.obj: $(SRC)\sdr_sdev.c $(SRC)\pocket_sdr.h
	$(CXX) /c $(CXXFLAGS) /Fo$@ $(SRC)\sdr_sdev.c
sdr_array.obj: $(SRC)\sdr_array.c $(SRC)\pocket_sdr.h
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\sdr_array.c

clean:
	-del /Q $(TARGET) *.obj *.exp 2>NUL

install:
	-if not exist $(INSTALL) mkdir $(INSTALL)
	-for %f in ($(TARGET)) do copy /Y %f $(INSTALL) >NUL









