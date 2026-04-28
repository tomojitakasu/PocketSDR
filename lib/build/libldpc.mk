#
#  makefile of LDPC-codes library (libldpc.so, libldpc.a)
#
#! You need to install LDPC-codes source tree as follows.
#!
#! $ git clone https://github.com/radfordneal/LDPC-codes LDPC-codes

#! specify directory of LDPC-codes source tree
SRC = ../LDPC-codes

ifeq ($(OS),Windows_NT)
    CC = gcc
    INSTALL = ../win32
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    INSTALL = ../macos
else
    CC = gcc
    INSTALL = ../linux
endif

INCLUDE = -I$(SRC)

CFLAGS = -O3 $(INCLUDE) -Wall -fPIC -g

OBJ = rcode.o channel.o dec.o enc.o alloc.o intio.o blockio.o \
      check.o open.o mod2dense.o mod2sparse.o mod2convert.o \
      distrib.o rand.o

TARGET = libldpc.so libldpc.a

all: $(TARGET)

libldpc.so : $(OBJ)
	$(CC) -shared -o $@ $(OBJ)

libldpc.a : $(OBJ)
	$(AR) r $@ $(OBJ)

rcode.o   : $(SRC)/rcode.c
	$(CC) $(CFLAGS) -c $<
channel.o : $(SRC)/channel.c
	$(CC) $(CFLAGS) -c $<
dec.o     : $(SRC)/dec.c
	$(CC) $(CFLAGS) -c $<
enc.o     : $(SRC)/enc.c
	$(CC) $(CFLAGS) -c $<
alloc.o   : $(SRC)/alloc.c
	$(CC) $(CFLAGS) -c $<
intio.o   : $(SRC)/intio.c
	$(CC) $(CFLAGS) -c $<
blockio.o : $(SRC)/blockio.c
	$(CC) $(CFLAGS) -c $<
check.o   : $(SRC)/check.c
	$(CC) $(CFLAGS) -c $<
open.o    : $(SRC)/open.c
	$(CC) $(CFLAGS) -c $<
mod2dense.o: $(SRC)/mod2dense.c
	$(CC) $(CFLAGS) -c $<
mod2sparse.o: $(SRC)/mod2sparse.c
	$(CC) $(CFLAGS) -c $<
mod2convert.o: $(SRC)/mod2convert.c
	$(CC) $(CFLAGS) -c $<
distrib.o : $(SRC)/distrib.c
	$(CC) $(CFLAGS) -c $<
rand.o    : $(SRC)/rand.c
	$(CC) $(CFLAGS) -DRAND_FILE=\"./randfile\" -c $<

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
