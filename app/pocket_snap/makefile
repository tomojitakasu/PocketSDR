#
#  makefile for pocket_snap
#

CC = gcc

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    LDLIBS = $(LIB)/win32/libsdr.a $(LIB)/win32/librtk.a -lfftw3f -lwinmm -lws2_32
    OPTIONS = -DWIN32 -DAVX2 -mavx2 -mfma
else
    LDLIBS = $(LIB)/linux/libsdr.a $(LIB)/linux/librtk.a -lfftw3f -lfftw3f_threads -lpthread -lm
    OPTIONS = -DAVX2 -mavx2 -mfma
endif
ifeq ($(shell uname -m),aarch64)
    OPTIONS = -DNEON
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable

#CFLAGS = -Ofast -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
CFLAGS = -Ofast $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_snap

all: $(TARGET)

pocket_snap: pocket_snap.o

pocket_snap.o: $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

