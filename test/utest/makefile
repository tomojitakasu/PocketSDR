#
#  makefile for test codes
#

CC = gcc

SRC = ../../src
LIB = ../../lib

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    FFTW = -lfftw3f
    #FFTW = /mingw64/lib/libfftw3f.a
    LDLIBS = -static $(LIB)/win32/librtk.a $(LIB)/win32/libfec.a $(LIB)/win32/libldpc.a $(FFTW) -lwinmm -lws2_32
    OPTIONS = -DWIN32 -DAVX2 -mavx2 -mfma
else
    LDLIBS = $(LIB)/linux/librtk.a $(LIB)/linux/libfec.a $(LIB)/linux/libldpc.a -lfftw3f -lpthread -lm
    OPTIONS = -DAVX2 -mavx2 -mfma
endif
ifeq ($(shell uname -m),aarch64)
    OPTIONS = -DNEON
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

#CFLAGS = -Ofast -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
CFLAGS = -Ofast $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = sdr_func_c_test

all: $(TARGET)

sdr_func_c_test: sdr_cmn.o sdr_func.o sdr_code.o sdr_code_gal.o

sdr_cmn.o: $(SRC)/sdr_cmn.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_cmn.c
sdr_func.o: $(SRC)/sdr_func.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_func.c
sdr_code.o: $(SRC)/sdr_code.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code.c
sdr_code_gal.o: $(SRC)/sdr_code_gal.c
	$(CC) -c $(CFLAGS) $(SRC)/sdr_code_gal.c

sdr_func_c_test.o: $(SRC)/pocket_sdr.h
sdr_cmn.o   : $(SRC)/pocket_sdr.h
sdr_func.o  : $(SRC)/pocket_sdr.h
sdr_code.o  : $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.stackdump

test:
	./$(TARGET)

