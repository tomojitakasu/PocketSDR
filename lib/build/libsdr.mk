#
# makefile for Pocket SDR shared library (libsdr.so)
#

CC  = gcc
SRC = ../../src

#INSTALL = ../linux
INSTALL = ../win32

INCLUDE = -I$(SRC)

#OPTIONS =
OPTIONS = -march=native -DAVX2

#CFLAGS = -Ofast $(INCLUDE) $(OPTIONS) -fPIC -g
CFLAGS = -O3 $(INCLUDE) $(OPTIONS) -fPIC -g

#LDLIBS =
LDLIBS = -lfftw3f
#LDLIBS = /mingw64/lib/libfftw3f.a

OBS = sdr_func.o

TARGET = libsdr.so

$(TARGET) : $(OBS)
	$(CC) -shared -o $@ $(OBS) $(LDLIBS)

sdr_func.o : $(SRC)/sdr_func.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_func.c

sdr_func.o : $(SRC)/pocket.h

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
