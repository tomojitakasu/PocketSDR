#
#  makefile for pocket_conf
#

CC = g++

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

ifeq ($(OS),Windows_NT)
    INCLUDE = -I$(SRC) -I$(LIB)/cyusb
    OPTIONS = -DWIN32
    LDLIBS = -static $(LIB)/win32/libsdr.a $(LIB)/cyusb/CyAPI.a -lsetupapi -lavrt -lwinmm
else ifeq ($(shell uname -sm),Darwin arm64)
    INCLUDE = -I$(SRC) -I/opt/homebrew/include
    OPTIONS = -Wno-deprecated
    LDLIBS = -L/opt/homebrew/lib $(LIB)/macos/libsdr.a -lusb-1.0 -lpthread
else
    INCLUDE = -I$(SRC)
    OPTIONS =
    LDLIBS = $(LIB)/linux/libsdr.a -lusb-1.0 -lpthread
endif

WARNOPT = -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_conf

all: $(TARGET)

pocket_conf: pocket_conf.o

pocket_conf.o: $(SRC)/pocket_dev.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

