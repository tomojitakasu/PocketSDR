#
#  makefile for PocketFFT (libpocketfft.so, libpocketfft.a)
#

SRC = ../pocketfft

ifeq ($(OS),Windows_NT)
    CC = g++
    INSTALL = ../win32
    INCLUDE = -I$(SRC)
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    INSTALL = ../macos
    INCLUDE = -I$(SRC)
else
    CC = g++
    INSTALL = ../linux
    INCLUDE = -I$(SRC)
endif

#OPTIONS = -DPOCKETFFT_NO_MULTITHREADING=1 -DPOCKETFFT_CACHE_SIZE=32000
OPTIONS = -DPOCKETFFT_CACHE_SIZE=96000

CFLAGS = -O3 -std=c++17 $(INCLUDE) $(OPTIONS) -Wall -fPIC -g

OBJ = pocketfft_wrap.o

TARGET = libpocketfft.so libpocketfft.a

all: $(TARGET)

libpocketfft.so: $(OBJ)
	$(CC) -shared -o $@ $(OBJ)

libpocketfft.a: $(OBJ)
	$(AR) r $@ $(OBJ)

pocketfft_wrap.o: $(SRC)/pocketfft_wrap.cpp
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
