#
#  makefile for pocket_scan
#

CC = g++

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

ifeq ($(OS),Windows_NT)
    INCLUDE = -I$(LIB)/cyusb
    OPTIONS = -DWIN32
    LDLIBS = -static $(LIB)/cyusb/CyAPI.a -lsetupapi
else ifeq ($(shell uname -sm),Darwin arm64)
    INCLUDE = -I/opt/homebrew/include 
    OPTIONS = -Wno-deprecated
    LDLIBS = -L/opt/homebrew/lib -lusb-1.0
else
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
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

