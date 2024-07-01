#
#  makefile for pocket_dump
#

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

ifeq ($(OS),Windows_NT)
    CC = g++
    INCLUDE = -I$(SRC) -I$(LIB)/cyusb -I$(LIB)/RTKLIB/src
    OPTIONS = -DWIN32
    LDLIBS = -static $(LIB)/win32/libsdr.a $(LIB)/cyusb/CyAPI.a -lsetupapi -lavrt -lwinmm
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src -I/opt/homebrew/include
    OPTIONS = -Wno-deprecated
    LDLIBS = -L/opt/homebrew/lib $(LIB)/macos/libsdr.a -lusb-1.0 -lpthread
else
    CC = g++
    INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src
    OPTIONS =
    LDLIBS = $(LIB)/linux/libsdr.a -lusb-1.0 -lpthread
endif

WARNOPT = -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_dump

all: $(TARGET)

pocket_dump: pocket_dump.o

pocket_dump.o: $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

