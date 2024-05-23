#
#  makefile for pocket_dump
#

CC = g++

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

ifeq ($(OS),Windows_NT)
    INCLUDE = -I$(SRC) -I$(LIB)/cyusb -I$(LIB)/RTKLIB/src
    OPTIONS = -DWIN32
    LDLIBS = -static $(LIB)/win32/libsdr.a $(LIB)/cyusb/CyAPI.a -lsetupapi -lavrt -lwinmm
else
    INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src
    OPTIONS =
    LDLIBS = $(LIB)/linux/libsdr.a -lusb-1.0 -lpthread
endif

WARNOPT = -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_dump

all: $(TARGET)

pocket_dump: pocket_dump.o

pocket_dump.o: $(SRC)/pocket_dev.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

