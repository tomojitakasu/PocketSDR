#
#  makefile for Pocket SDR APs
#

CC = gcc

BIN = ../bin

INCLUDE = -I/mingw64/include
WARNOPT = -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
OPTIONS =

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

LIBUSB = -L/mingw64/lib -llibusb-1.0
#LIBUSB = -lusb-1.0

LDLIBS = $(LIBUSB)

TARGET = pocket_conf pocket_dump pocket_scan
SCRIPT =

all: $(TARGET)

pocket_conf: pocket_conf.o sdr_conf.o sdr_usb.o sdr_cmn.o
pocket_dump: pocket_dump.o sdr_conf.o sdr_dev.o sdr_usb.o sdr_cmn.o
pocket_scan: pocket_scan.o

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

