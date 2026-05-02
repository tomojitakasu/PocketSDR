#
#  makefile for pocket_snap
#

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    CC = g++
    INCLUDE += -I$(LIB)/cyusb
    LDLIBS = $(LIB)/win32/libsdr.a $(LIB)/win32/librtk.a -lwinmm -lws2_32
    LDLIBS += $(LIB)/win32/libpocketfft.a
    #LDLIBS += -lfftw3f
    OPTIONS = -DWIN32 -DAVX2 -mavx2 -mfma
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    LDLIBS = -L/opt/homebrew/lib $(LIB)/macos/libsdr.a $(LIB)/macos/librtk.a -lpthread -lm
    LDLIBS += $(LIB)/win32/libpocketfft.a
    #LDLIBS += -lfftw3f
    OPTIONS = -I/opt/homebrew/include
else
    CC = g++
    LDLIBS = $(LIB)/linux/libsdr.a $(LIB)/linux/librtk.a -lpthread -lm
    LDLIBS += $(LIB)/linux/libpocketfft.a
    #LDLIBS += -lfftw3f
    OPTIONS = -DAVX2 -mavx2 -mfma
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable

#CFLAGS = -Ofast -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
CFLAGS = -Ofast $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_snap

all: $(TARGET)

pocket_snap: pocket_snap.o

pocket_snap.o: $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.obj *.stackdump

install:
	cp $(TARGET) $(BIN)

