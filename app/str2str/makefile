#
# makefile for str2str
#

LIB = ../../lib
BIN = ../../bin

INCLUDE= -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    CC = gcc
    OPTIONS = -DTRACE -DWIN32
    LDLIBS = $(LIB)/win32/librtk.a -lwinmm -lws2_32 -lpthread
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    OPTIONS = -DTRACE -DMACOS -Wno-deprecated
    LDLIBS = $(LIB)/macos/librtk.a -lpthread
else
    CC = gcc
    OPTIONS = -DTRACE
    LDLIBS = $(LIB)/linux/librtk.a -lm -lpthread
endif

WARNOPT = -ansi -pedantic -Wall -Wno-unused-but-set-variable

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = str2str

all  : $(TARGET)

str2str    : str2str.o

str2str.o  : $(LIB)/RTKLIB/src/rtklib.h

install:
	cp $(TARGET) $(BIN)

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

