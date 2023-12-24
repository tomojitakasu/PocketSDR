#
#  makefile for pocket_snap
#

CC = gcc

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    #LDLIBS = $(LIB)/win32/librtk.a -lfftw3f -lwinmm -lws2_32
    LDLIBS = $(LIB)/win32/librtk.a -lfftw3f -lfftw3f_threads -lwinmm -lws2_32
    OPTIONS = -DWIN32 -DAVX2 -Wno-unused-but-set-variable
else
    LDLIBS = $(LIB)/linux/librtk.a -lfftw3f -lfftw3f_threads -lpthread -lm
    OPTIONS = -DAVX2 -Wno-unused-but-set-variable
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter

#CFLAGS = -Ofast -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
CFLAGS = -Ofast -mavx2 -mfma $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_snap

all: $(TARGET)

pocket_snap: pocket_snap.o sdr_cmn.o sdr_func.o sdr_code.o sdr_code_gal.o

sdr_cmn.o: $(SRC)/sdr_cmn.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_cmn.c
sdr_func.o: $(SRC)/sdr_func.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_func.c
sdr_code.o: $(SRC)/sdr_code.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code.c
sdr_code_gal.o: $(SRC)/sdr_code_gal.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code_gal.c

pocket_snap.o: $(SRC)/pocket_sdr.h
sdr_cmn.o   : $(SRC)/pocket_sdr.h
sdr_func.o  : $(SRC)/pocket_sdr.h
sdr_code.o  : $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

