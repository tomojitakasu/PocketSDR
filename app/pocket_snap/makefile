#
#  makefile for pocket_snap
#

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

ifeq ($(OS),Windows_NT)
    CC = g++
    INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src -I$(LIB)/cyusb
    LDLIBS = $(LIB)/win32/libsdr.a $(LIB)/win32/librtk.a -lfftw3f -lwinmm -lws2_32
    OPTIONS = -DWIN32 -DAVX2 -mavx2 -mfma
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src
    LDLIBS = -L/opt/homebrew/lib $(LIB)/macos/libsdr.a $(LIB)/macos/librtk.a \
             -lfftw3f -lpthread -lm
    OPTIONS = -I/opt/homebrew/include
else
    CC = g++
    INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src
    LDLIBS = $(LIB)/linux/libsdr.a $(LIB)/linux/librtk.a -lfftw3f -lpthread -lm
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

