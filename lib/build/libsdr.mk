#
#  makefile for Pocket SDR GNSS-SDR library (libsdr.so, libsdr.a)
#
#! You need to install libfftw3 as follows.
#!
#! $ pacman -S mingw-w64-x86_64-fftw (MINGW64)
#! $ sudo apt install libfftw3-dev   (Ubuntu)
#! $ brew install libfftw            (Mac OS)

SRC = ../../src

ifeq ($(OS),Windows_NT)
    CC = g++
    INSTALL = ../win32
    INCLUDE = -I$(SRC) -I../RTKLIB/src -I../cyusb
    OPTIONS = -DWIN32 -DAVX2 -mavx2 -mfma
    LDLIBS = -static ./librtk.a ./libfec.a ./libldpc.a -lfftw3f -lwinmm \
             ../cyusb/CyAPI.a -lpthread -lsetupapi -lavrt -lwsock32
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    INSTALL = ../macos
    INCLUDE = -I$(SRC) -I../RTKLIB/src -I/opt/homebrew/include
    OPTIONS = -DMACOS -DNEON -Wno-deprecated
    LDLIBS = -L/opt/homebrew/lib ./librtk.a ./libfec.a ./libldpc.a -lfftw3f \
             -lusb-1.0 -lpthread
else
    CC = g++
    INSTALL = ../linux
    INCLUDE = -I$(SRC) -I../RTKLIB/src
    OPTIONS = -DAVX2 -mavx2 -mfma
    LDLIBS = ./librtk.a ./libfec.a ./libldpc.a -lfftw3f -lpthread -lusb-1.0 -lm \
             -lpthread
endif
ifeq ($(shell uname -m),aarch64)
    OPTIONS = -DNEON
endif

#CFLAGS = -Ofast -march=native $(INCLUDE) $(OPTIONS) -Wall -fPIC -g
CFLAGS = -Ofast $(INCLUDE) $(OPTIONS) -Wall -fPIC -g

OBJ = sdr_cmn.o sdr_func.o sdr_code.o sdr_code_gal.o sdr_ch.o \
      sdr_nav.o sdr_pvt.o sdr_rcv.o sdr_fec.o sdr_ldpc.o sdr_nb_ldpc.o \
      sdr_usb.o sdr_dev.o sdr_conf.o

TARGET = libsdr.so libsdr.a

all : $(TARGET)

libsdr.so: $(OBJ)
	$(CC) -shared -o $@ $(OBJ) $(LDLIBS)

libsdr.a: $(OBJ)
	$(AR) r $@ $(OBJ)

sdr_cmn.o : $(SRC)/sdr_cmn.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_cmn.c

sdr_func.o : $(SRC)/sdr_func.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_func.c

sdr_code.o : $(SRC)/sdr_code.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code.c

sdr_code_gal.o : $(SRC)/sdr_code_gal.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code_gal.c

sdr_ch.o   : $(SRC)/sdr_ch.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_ch.c

sdr_nav.o  : $(SRC)/sdr_nav.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_nav.c

sdr_pvt.o  : $(SRC)/sdr_pvt.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_pvt.c

sdr_rcv.o  : $(SRC)/sdr_rcv.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_rcv.c

sdr_fec.o  : $(SRC)/sdr_fec.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_fec.c

sdr_ldpc.o : $(SRC)/sdr_ldpc.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_ldpc.c

sdr_nb_ldpc.o : $(SRC)/sdr_nb_ldpc.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_nb_ldpc.c

sdr_usb.o : $(SRC)/sdr_usb.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_usb.c

sdr_dev.o : $(SRC)/sdr_dev.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_dev.c

sdr_conf.o : $(SRC)/sdr_conf.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_conf.c

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

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
