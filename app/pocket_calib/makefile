#
#  makefile for pocket_calib
#

SRC = ../../src
LIB = ../../lib
BIN = ../../bin
USE_FFTW ?= 0

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    CC = g++
    INCLUDE += -I$(LIB)/cyusb
    LDLIBS = $(LIB)/win32/libsdr.a $(LIB)/win32/librtk.a -lwinmm -lws2_32
    ifeq ($(USE_FFTW),1)
        LDLIBS += -lfftw3f
    else
        LDLIBS += $(LIB)/win32/libpocketfft.a
    endif
    OPTIONS = -DWIN32
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang++
    LDLIBS = -L/opt/homebrew/lib $(LIB)/macos/libsdr.a $(LIB)/macos/librtk.a -lpthread -lm
    ifeq ($(USE_FFTW),1)
        LDLIBS += -lfftw3f
    else
        LDLIBS += $(LIB)/macos/libpocketfft.a
    endif
    OPTIONS = -I/opt/homebrew/include
else
    CC = g++
    LDLIBS = $(LIB)/linux/libsdr.a $(LIB)/linux/librtk.a -lpthread -lm
    ifeq ($(USE_FFTW),1)
        LDLIBS += -lfftw3f
    else
        LDLIBS += $(LIB)/linux/libpocketfft.a
    endif
    OPTIONS =
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_calib

all: $(TARGET)

pocket_calib: pocket_calib.o

pocket_calib.o: $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)
