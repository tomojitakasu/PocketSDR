#
#  makefile for Pocket SDR GNSS-SDR library (libsdr.so, libsdr.a)
#
#! You need to install libfftw3 as follows.
#!
#! $ pacman -S mingw-w64-x86_64-fftw (MINGW64)
#! $ sudo apt install libfftw3-dev   (Ubuntu)

CC  = gcc
SRC = ../../src

#! uncomment for Windows
INSTALL = ../win32
INCLUDE = -I$(SRC) -I../RTKLIB/src
OPTIONS = -DWIN32 -DAVX2
LDLIBS = $(INSTALL)/librtk.so -lfftw3f -lwinmm

#! uncomment for Linux
#INSTALL = ../linux
#INCLUDE = -I$(SRC) -I../RTKLIB/src
#OPTIONS = -DAVX2
#LDLIBS = $(INSTALL)/librtk.so -lfftw3f

CFLAGS = -Ofast -march=native $(INCLUDE) $(OPTIONS) -Wall -fPIC -g

OBJ = sdr_cmn.o sdr_func.o sdr_code.o sdr_code_gal.o

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

sdr_cmn.o  : $(SRC)/pocket_sdr.h
sdr_func.o : $(SRC)/pocket_sdr.h
sdr_code.o : $(SRC)/pocket_sdr.h
sdr_code_gal.o : $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
