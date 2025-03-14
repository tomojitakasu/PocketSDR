#
# makefile for convbin
#

LIB = ../../lib
BIN = ../../bin

INCLUDE= -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    CC = gcc
    OPTIONS = -DTRACE -DWIN32
    LDLIBS = $(LIB)/win32/librtk.a -lwinmm
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    OPTIONS = -DTRACE -DMACOS -Wno-deprecated
    LDLIBS = $(LIB)/macos/librtk.a
else
    CC = gcc
    OPTIONS = -DTRACE
    LDLIBS = $(LIB)/linux/librtk.a -lm
endif

WARNOPT = -ansi -pedantic -Wall -Wno-unused-but-set-variable

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = convbin

all  : $(TARGET)

convbin    : convbin.o

convbin.o  : $(LIB)/RTKLIB/src/rtklib.h

install:
	cp $(TARGET) $(BIN)

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

