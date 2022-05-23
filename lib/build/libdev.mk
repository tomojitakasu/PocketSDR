#
#  makefile for Pocket SDR device library (libdev.so, libdev.a)
#
#! You need to install libfftw3 as follows.
#!
#! $ pacman -S mingw-w64-x86_64-fftw (MINGW64)
#! $ sudo apt install libfftw3-dev   (Ubuntu)

CC  = g++
SRC = ../../src

#! uncomment for Windows
INSTALL = ../win32
INCLUDE = -I$(SRC) -I../cyusb
OPTIONS = -DWIN32
LDLIBS = ../cyusb/CyAPI.a -lsetupapi -lavrt

#! uncomment for Linux
#INSTALL = ../linux
#INCLUDE = -I$(SRC)
#OPTIONS =
#LDLIBS =

CFLAGS = -Ofast -march=native $(INCLUDE) $(OPTIONS) -Wall -fPIC -g

OBJ = sdr_usb.o sdr_dev.o sdr_conf.o

TARGET = libpocket.so libpocket.a

all : $(TARGET)

libpocket.so: $(OBJ)
	$(CC) -shared -o $@ $(OBJ) $(LDLIBS)

libpocket.a: $(OBJ)
	$(AR) r $@ $(OBJ)

sdr_usb.o : $(SRC)/sdr_usb.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_usb.c

sdr_dev.o : $(SRC)/sdr_dev.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_dev.c

sdr_conf.o : $(SRC)/sdr_conf.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_conf.c

sdr_usb.o  : $(SRC)/pocket_dev.h
sdr_dev.o  : $(SRC)/pocket_dev.h
sdr_conf.o : $(SRC)/pocket_dev.h

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
