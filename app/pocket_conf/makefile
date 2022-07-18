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
    LDLIBS = -static $(LIB)/cyusb/CyAPI.a -lsetupapi -lavrt -lwinmm
else
    INCLUDE = -I$(SRC)
    OPTIONS =
    LDLIBS = -lusb-1.0 -lpthread
endif

WARNOPT = -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter

CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_conf

all: $(TARGET)

pocket_conf: pocket_conf.o sdr_usb.o sdr_dev.o sdr_conf.o

sdr_usb.o: $(SRC)/sdr_usb.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_usb.c
sdr_dev.o: $(SRC)/sdr_dev.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_dev.c
sdr_conf.o: $(SRC)/sdr_conf.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_conf.c

pocket_conf.o: $(SRC)/pocket_dev.h
sdr_usb.o    : $(SRC)/pocket_dev.h
sdr_dev.o    : $(SRC)/pocket_dev.h
sdr_conf.o   : $(SRC)/pocket_dev.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

