#
#  makefile for pocket_acq
#

CC = gcc

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    LDLIBS = -static $(LIB)/win32/libsdr.a $(LIB)/win32/librtk.a -lfftw3f -lwinmm \
             -lws2_32
    OPTIONS = -DWIN32 -DAVX2 -mavx2 -mfma
else
    LDLIBS = $(LIB)/linux/libsdr.a $(LIB)/linux/librtk.a -lfftw3f -lpthread -lm
    OPTIONS = -DAVX2 -mavx2 -mfma
endif
ifeq ($(shell uname -m),aarch64)
    OPTIONS = -DNEON
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter

#CFLAGS = -Ofast -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
CFLAGS = -Ofast $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_acq

all: $(TARGET)

pocket_acq: pocket_acq.o

pocket_acq.o: $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

