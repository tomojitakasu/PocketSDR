#
#  makefile for pocket_dump
#

CC = gcc

SRC = ../../src
BIN = ../../bin

INCLUDE =
OPTIONS =
LDLIBS = -static -lusb-1.0

WARNOPT = -Wall -Wextra

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_scan

all: $(TARGET)

pocket_scan: pocket_scan.o

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

