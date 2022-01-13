#
#  makefile for Pocket SDR APs
#

CC = g++
#CC = gcc

BIN = ../bin

INCLUDE = -I../lib/cyusb
OPTIONS = -DCYUSB

WARNOPT = -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

LDLIBS = ../lib/cyusb/CyAPI.a -lsetupapi -lwinmm -lavrt
#LDLIBS = -lusb-1.0

LDFLAGS = -static

TARGET = pocket_conf pocket_dump pocket_scan

all: $(TARGET)

pocket_conf: pocket_conf.o sdr_conf.o sdr_usb.o sdr_cmn.o
pocket_dump: pocket_dump.o sdr_conf.o sdr_dev.o sdr_usb.o sdr_cmn.o
pocket_scan: pocket_scan.o
	$(CC) -o $@ pocket_scan.o -lusb-1.0

pocket_scan.o: pocket_scan.c
	$(CC) -c -o $@ pocket_scan.c -g

pocket_conf.o: pocket.h
pocket_dump.o: pocket.h
pocket_scan.o: pocket.h
sdr_cmn.o    : pocket.h
sdr_usb.o    : pocket.h
sdr_dev.o    : pocket.h
sdr_conf.o   : pocket.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(SCRIPT) $(BIN)

