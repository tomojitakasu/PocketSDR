#
#  makefile for Pocket SDR GNSS-SDR library (libsdr.so, libsdr.a)
#
#! To use FFTW3 instead of PocketFFT, you need to install libfftw3.
#!
#! $ pacman -S mingw-w64-x86_64-fftw (UCRT64)
#! $ sudo apt install libfftw3-dev   (Ubuntu)
#! $ brew install libfftw            (Mac OS)

SRC = ../../src

# PocketFFT
INCLUDE = -I$(SRC) -I../RTKLIB/src -I../pocketfft
OPTIONS =
LIBS = ./librtk.a ./libfec.a ./libldpc.a ./libpocketfft.a

# FFTW3
#INCLUDE = -I$(SRC) -I../RTKLIB/src
#OPTIONS = -DFFTW
#LIBS = ./librtk.a ./libfec.a ./libldpc.a -lfftw3f

ifeq ($(OS),Windows_NT)
    CC = gcc
    CX = g++
    LD = g++
    INSTALL = ../win32
    INCLUDE += -I../cyusb
    OPTIONS += -DWIN32 -DAVX2 -mavx2 -mfma
    LDLIBS = $(LIBS) -lpthread ../cyusb/CyAPI.a -lsetupapi -lavrt -lwsock32 -lwinmm
    LDLIBS += -static
    
    OPTIONS += -DSOAPYSDR
    INCLUDE += -I../SoapySDR/include
    LDLIBS += -L../SoapySDR/lib -Wl,-Bdynamic -lSoapySDR -Wl,-Bstatic
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    CX = $(CC)
    INSTALL = ../macos
    INCLUDE += -I/opt/homebrew/include
    OPTIONS += -DMACOS -DNEON -Wno-deprecated
    LDLIBS = -L/opt/homebrew/lib $(LIBS) -lusb-1.0 -lpthread
    
    OPTIONS += -DSOAPYSDR
    LDLIBS += -lSoapySDR
else
    CC = gcc
    CX = g++
    LD = g++
    INSTALL = ../linux
    INCLUDE +=
    ifeq ($(shell uname -m),aarch64)
        OPTIONS += -DNEON
    else
        OPTIONS += -DAVX2 -mavx2 -mfma
    endif
    LDLIBS = $(LIBS) -lpthread -lusb-1.0 -lm -lpthread
    
    OPTIONS += -DSOAPYSDR
    LDLIBS += -lSoapySDR
endif

#CFLAGS = -Ofast -march=native $(INCLUDE) $(OPTIONS) -Wall -fPIC -g
#CFLAGS = -Ofast $(INCLUDE) $(OPTIONS) -Wall -fPIC -g
CFLAGS = -O3 $(INCLUDE) $(OPTIONS) -Wall -fPIC -g

OBJ = sdr_cmn.o sdr_func.o sdr_code.o sdr_code_gal.o sdr_ch.o \
      sdr_nav.o sdr_pvt.o sdr_rcv.o sdr_fec.o sdr_ldpc.o sdr_nb_ldpc.o \
      sdr_usb.o sdr_dev.o sdr_conf.o sdr_sdev.o sdr_array.o

TARGET = libsdr.so libsdr.a

all : $(TARGET)

libsdr.so: $(OBJ)
	$(LD) -shared -o $@ $(OBJ) $(LDLIBS)
libsdr.a: $(OBJ)
	$(AR) r $@ $(OBJ)
sdr_cmn.o : $(SRC)/sdr_cmn.c
	$(CC) -c $(CFLAGS) $<
sdr_func.o : $(SRC)/sdr_func.c
	$(CC) -c $(CFLAGS) $<
sdr_code.o : $(SRC)/sdr_code.c
	$(CC) -c $(CFLAGS) $<
sdr_code_gal.o : $(SRC)/sdr_code_gal.c
	$(CC) -c $(CFLAGS) $<
sdr_ch.o   : $(SRC)/sdr_ch.c
	$(CC) -c $(CFLAGS) $<
sdr_nav.o  : $(SRC)/sdr_nav.c
	$(CC) -c $(CFLAGS) $<
sdr_pvt.o  : $(SRC)/sdr_pvt.c
	$(CC) -c $(CFLAGS) $<
sdr_rcv.o  : $(SRC)/sdr_rcv.c
	$(CC) -c $(CFLAGS) $<
sdr_fec.o  : $(SRC)/sdr_fec.c
	$(CC) -c $(CFLAGS) $<
sdr_ldpc.o : $(SRC)/sdr_ldpc.c
	$(CC) -c $(CFLAGS) $<
sdr_nb_ldpc.o : $(SRC)/sdr_nb_ldpc.c
	$(CC) -c $(CFLAGS) $<
sdr_usb.o : $(SRC)/sdr_usb.c
	$(CX) -c $(CFLAGS) $<
sdr_dev.o : $(SRC)/sdr_dev.c
	$(CX) -c $(CFLAGS) $<
sdr_conf.o : $(SRC)/sdr_conf.c
	$(CC) -c $(CFLAGS) $<
sdr_sdev.o : $(SRC)/sdr_sdev.c
	$(CC) -c $(CFLAGS) $<
sdr_array.o : $(SRC)/sdr_array.c
	$(CC) -c $(CFLAGS) $<

sdr_cmn.o  : $(SRC)/pocket_sdr.h
sdr_func.o : $(SRC)/pocket_sdr.h
sdr_code.o : $(SRC)/pocket_sdr.h
sdr_ch.o   : $(SRC)/pocket_sdr.h
sdr_nav.o  : $(SRC)/pocket_sdr.h
sdr_pvt.o  : $(SRC)/pocket_sdr.h
sdr_rcv.o  : $(SRC)/pocket_sdr.h
sdr_fec.o  : $(SRC)/pocket_sdr.h
sdr_ldpc.o : $(SRC)/pocket_sdr.h
sdr_nb_ldpc.o: $(SRC)/pocket_sdr.h
sdr_usb.o  : $(SRC)/pocket_sdr.h
sdr_dev.o  : $(SRC)/pocket_sdr.h
sdr_conf.o : $(SRC)/pocket_sdr.h
sdr_sdev.o : $(SRC)/pocket_sdr.h
sdr_array.o : $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
