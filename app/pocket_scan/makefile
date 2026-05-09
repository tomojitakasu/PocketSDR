#
#  makefile for pocket_scan
#

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

ifeq ($(OS),Windows_NT)
    CC = g++
    INCLUDE = -I$(LIB)/cyusb
    OPTIONS = -DWIN32
    LDLIBS = -static $(LIB)/cyusb/CyAPI.a -lsetupapi
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang++
    INCLUDE = -I/opt/homebrew/include 
    OPTIONS = -Wno-deprecated
    LDLIBS = -L/opt/homebrew/lib -lusb-1.0
else
    CC = g++
    INCLUDE =
    OPTIONS =
    LDLIBS = -lusb-1.0
endif

WARNOPT = -Wall -Wextra

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_scan

all: $(TARGET)

pocket_scan: pocket_scan.o

clean:
	rm -f $(TARGET) *.exe *.o *.obj *.stackdump

install:
	cp $(TARGET) $(BIN)

