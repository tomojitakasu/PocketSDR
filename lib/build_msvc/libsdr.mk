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

SOAPY_ROOT = "C:\Program Files\PothosSDR"
#INCFLAGS = /I$(SRC) /I..\RTKLIB\src /I..\cyusb /I..\pocketfft
#CFLAGS = /nologo /O2 /W3 /MD /arch:AVX2 /DAVX2 /DWIN32 /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS $(INCFLAGS)
INCFLAGS = /I$(SRC) /I..\RTKLIB\src /I..\cyusb /I..\pocketfft /I$(SOAPY_ROOT)\include
CFLAGS = /nologo /O2 /W3 /MD /arch:AVX2 /DSOAPYSDR /DAVX2 /DWIN32 /DWIN32_LEAN_AND_MEAN /D_CRT_SECURE_NO_WARNINGS $(INCFLAGS)
CXXFLAGS = $(CFLAGS) /TP
#LDLIBS = librtk.lib libfec.lib libldpc.lib libpocketfft.lib ..\win32_msvc\CyAPI.lib setupapi.lib avrt.lib wsock32.lib winmm.lib user32.lib
LDLIBS = librtk.lib libfec.lib libldpc.lib libpocketfft.lib ..\win32_msvc\CyAPI.lib $(SOAPY_ROOT)\lib\SoapySDR.lib setupapi.lib avrt.lib wsock32.lib winmm.lib user32.lib

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









