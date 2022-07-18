#
#  makefile for pocket_acq
#

CC = gcc

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src
OPTIONS = -DAVX2

ifeq ($(OS),Windows_NT)
    LDLIBS = -static $(LIB)/win32/librtk.a -lfftw3f -lwinmm -lws2_32
else
    LDLIBS = $(LIB)/linux/librtk.a -lfftw3f -lpthread -lm
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter

#CFLAGS = -Ofast -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
CFLAGS = -Ofast -mavx2 -mfma $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_acq

all: $(TARGET)

pocket_acq: pocket_acq.o sdr_cmn.o sdr_func.o sdr_code.o sdr_code_gal.o

sdr_cmn.o: $(SRC)/sdr_cmn.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_cmn.c
sdr_func.o: $(SRC)/sdr_func.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_func.c
sdr_code.o: $(SRC)/sdr_code.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code.c
sdr_code_gal.o: $(SRC)/sdr_code_gal.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code_gal.c

pocket_acq.o: $(SRC)/pocket_sdr.h
sdr_cmn.o   : $(SRC)/pocket_sdr.h
sdr_func.o  : $(SRC)/pocket_sdr.h
sdr_code.o  : $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

