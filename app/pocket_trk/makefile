#
#  makefile for pocket_trk, fftw_wisdom
#

CC = g++

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

ifeq ($(OS),Windows_NT)
    INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src -I$(LIB)/cyusb
    OPTIONS = -DWIN32 -DAVX2 -mavx2 -mfma
    LIBSDR = $(LIB)/win32/libsdr.a
    LDLIBS = -static $(LIBSDR) $(LIB)/win32/librtk.a $(LIB)/win32/libfec.a \
             $(LIB)/win32/libldpc.a -lfftw3f -lwinmm -lws2_32 $(LIB)/cyusb/CyAPI.a \
             -lsetupapi -lavrt -lwinmm
else
    INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src
    LIBSDR = $(LIB)/linux/libsdr.a
    LDLIBS = $(LIBSDR) $(LIB)/linux/librtk.a $(LIB)/linux/libfec.a \
             $(LIB)/linux/libldpc.a -lfftw3f -lpthread -lm -lusb-1.0
    OPTIONS = -DAVX2 -mavx2 -mfma
endif
ifeq ($(shell uname -m),aarch64)
    OPTIONS = -DNEON
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

#CFLAGS = -Ofast -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
CFLAGS = -Ofast $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_trk fftw_wisdom

all: $(TARGET)

pocket_trk: pocket_trk.o $(LIBSDR)
fftw_wisdom: fftw_wisdom.o

pocket_trk.o: $(SRC)/pocket_sdr.h $(SRC)/pocket_dev.h
fftw_wisdom.o: $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

