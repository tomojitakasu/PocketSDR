#
#  makefile of LDPC-codes shared library (libldpc.so)
#
#! You need to install LDPC-codes source tree as follows.
#!
#! $ git clone https://github.com/radfordneal/LDPC-codes LDPC-codes

CC  = gcc

#! specify directory of LDPC-codes source tree
SRC = ../LDPC-codes

#! uncomment for Windows
INSTALL = ../win32

#! uncomment for Linux
#INSTALL = ../linux

INCLUDE = -I$(SRC)

CFLAGS = -O3 $(INCLUDE) -Wall -fPIC -g

OBJ = rcode.o channel.o dec.o enc.o alloc.o intio.o blockio.o \
      check.o open.o mod2dense.o mod2sparse.o mod2convert.o \
      distrib.o rand.o

TARGET = libldpc.so

$(TARGET) : $(OBJ)
	$(CC) -shared -o $@ $(OBJ)

rcode.o   : $(SRC)/rcode.c
	$(CC) $(CFLAGS) -c $(SRC)/rcode.c
channel.o : $(SRC)/channel.c
	$(CC) $(CFLAGS) -c $(SRC)/channel.c
dec.o     : $(SRC)/dec.c
	$(CC) $(CFLAGS) -c $(SRC)/dec.c
enc.o     : $(SRC)/enc.c
	$(CC) $(CFLAGS) -c $(SRC)/enc.c
alloc.o   : $(SRC)/alloc.c
	$(CC) $(CFLAGS) -c $(SRC)/alloc.c
intio.o   : $(SRC)/intio.c
	$(CC) $(CFLAGS) -c $(SRC)/intio.c
blockio.o : $(SRC)/blockio.c
	$(CC) $(CFLAGS) -c $(SRC)/blockio.c
check.o   : $(SRC)/check.c
	$(CC) $(CFLAGS) -c $(SRC)/check.c
open.o    : $(SRC)/open.c
	$(CC) $(CFLAGS) -c $(SRC)/open.c
mod2dense.o: $(SRC)/mod2dense.c
	$(CC) $(CFLAGS) -c $(SRC)/mod2dense.c
mod2sparse.o: $(SRC)/mod2sparse.c
	$(CC) $(CFLAGS) -c $(SRC)/mod2sparse.c
mod2convert.o: $(SRC)/mod2convert.c
	$(CC) $(CFLAGS) -c $(SRC)/mod2convert.c
distrib.o : $(SRC)/distrib.c
	$(CC) $(CFLAGS) -c $(SRC)/distrib.c
rand.o    : $(SRC)/rand.c
	$(CC) $(CFLAGS) -DRAND_FILE=\"./randfile\" -c $(SRC)/rand.c

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
