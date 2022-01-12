# makefile of shared library for LDPC-codes
#
# LDPC-codes:
#     https://github.com/radfordneal/LDPC-codes
#

CC = gcc

CFLAGS = -Wall -O3 -fPIC -g

LIBS = rcode.o channel.o dec.o enc.o alloc.o intio.o blockio.o \
       check.o open.o mod2dense.o mod2sparse.o mod2convert.o \
       distrib.o rand.o

TARGET = libldpc.so

$(TARGET) : $(LIBS)
	$(CC) -shared -o $@ $(LIBS)

rand.o: rand.c
	$(CC) $(CFLAGS) -DRAND_FILE=\"./randfile\" -c -o $@ rand.c

clean:
	rm -f $(TARGET) *.o
