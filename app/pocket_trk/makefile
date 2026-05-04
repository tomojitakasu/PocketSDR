#
#  makefile for pocket_trk, fftw_wisdom
#

CC = g++

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    INCLUDE += -I$(LIB)/cyusb -I$(LIB)/SoapySDR/include
    LDLIBS = $(LIB)/win32/libsdr.a $(LIB)/win32/librtk.a $(LIB)/win32/libfec.a \
             $(LIB)/win32/libldpc.a $(LIB)/cyusb/CyAPI.a -lsetupapi -lavrt -lwinmm \
             -lws2_32 -lpthread \
             -static -L$(LIB)/SoapySDR/lib -Wl,-Bdynamic -lSoapySDR -Wl,-Bstatic
    #OPTIONS =
    #LDLIBS += $(LIB)/win32/libpocketfft.a
    OPTIONS = -DFFTW
    LDLIBS += -lfftw3f
    
else ifeq ($(shell uname -sm),Darwin arm64)
    INCLUDE += -I/opt/homebrew/include
    LDLIBS = -L/opt/homebrew/lib $(LIB)/macos/libsdr.a $(LIB)/macos/librtk.a $(LIB)/macos/libfec.a \
             $(LIB)/macos/libldpc.a $(LIB)/macos/libpocketfft.a \
             -lpthread -lm -lusb-1.0 -lSoapySDR
    #OPTIONS = -Wno-deprecated
else
    LDLIBS = $(LIB)/linux/libsdr.a $(LIB)/linux/librtk.a $(LIB)/linux/libfec.a \
             $(LIB)/linux/libldpc.a $(LIB)/linux/libpocketfft.a \
             -lpthread -lm -lusb-1.0 -lSoapySDR
    OPTIONS =
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

#CFLAGS = -Ofast -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
#CFLAGS = -Ofast $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
CFLAGS = -O3 $(INCLUDE) $(WARNOPT) $(OPTIONS) -g

TARGET = pocket_trk fftw_wisdom

all: $(TARGET)

pocket_trk: pocket_trk.o $(LIBSDR)
fftw_wisdom: fftw_wisdom.o

pocket_trk.o: $(SRC)/pocket_sdr.h
fftw_wisdom.o: $(SRC)/pocket_sdr.h

clean:
	rm -f $(TARGET) *.exe *.o *.obj *.stackdump

install:
	cp $(TARGET) $(BIN)

