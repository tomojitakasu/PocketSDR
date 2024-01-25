#
#  makefile for pocket_trk, fftw_wisdom
#

CC = gcc

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    LDLIBS = -static $(LIB)/win32/librtk.a $(LIB)/win32/libfec.a $(LIB)/win32/libldpc.a -lfftw3f -lwinmm -lws2_32
    OPTIONS = -DWIN32 -DAVX2
else
    LDLIBS = $(LIB)/linux/librtk.a $(LIB)/linux/libfec.a $(LIB)/linux/libldpc.a -lfftw3f -lpthread -lm
    OPTIONS = -DAVX2
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

#CFLAGS = -Ofast -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
CFLAGS = -Ofast -mavx2 -mfma $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_trk fftw_wisdom

all: $(TARGET)

pocket_trk: pocket_trk.o sdr_cmn.o sdr_func.o sdr_code.o sdr_code_gal.o \
  sdr_ch.o sdr_nav.o sdr_fec.o sdr_ldpc.o sdr_nb_ldpc.o
fftw_wisdom: fftw_wisdom.o sdr_cmn.o sdr_func.o

sdr_cmn.o: $(SRC)/sdr_cmn.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_cmn.c
sdr_func.o: $(SRC)/sdr_func.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_func.c
sdr_code.o: $(SRC)/sdr_code.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code.c
sdr_code_gal.o: $(SRC)/sdr_code_gal.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code_gal.c
sdr_ch.o: $(SRC)/sdr_ch.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_ch.c
sdr_nav.o: $(SRC)/sdr_nav.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_nav.c
sdr_fec.o: $(SRC)/sdr_fec.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_fec.c
sdr_ldpc.o: $(SRC)/sdr_ldpc.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_ldpc.c
sdr_nb_ldpc.o: $(SRC)/sdr_nb_ldpc.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_nb_ldpc.c

pocket_trk.o: $(SRC)/pocket_sdr.h
fftw_wisdom.o: $(SRC)/pocket_sdr.h
sdr_cmn.o   : $(SRC)/pocket_sdr.h
sdr_func.o  : $(SRC)/pocket_sdr.h
sdr_code.o  : $(SRC)/pocket_sdr.h
sdr_ch.o    : $(SRC)/pocket_sdr.h
sdr_nav.o   : $(SRC)/pocket_sdr.h
sdr_ldpc.o  : $(SRC)/pocket_sdr.h
sdr_nb_ldpc.o: $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

install:
	cp $(TARGET) $(BIN)

