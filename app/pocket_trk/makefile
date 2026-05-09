#
#  makefile for pocket_trk, fftw_wisdom
#

SRC = ../../src
LIB = ../../lib
BIN = ../../bin

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    CC = g++
    INCLUDE += -I$(LIB)/cyusb -I$(LIB)/SoapySDR/include
    LDLIBS = $(LIB)/win32/libsdr.a $(LIB)/win32/librtk.a $(LIB)/win32/libfec.a \
             $(LIB)/win32/libldpc.a $(LIB)/cyusb/CyAPI.a -lsetupapi -lavrt -lwinmm \
             -lws2_32 -lpthread \
             -static -L$(LIB)/SoapySDR/lib -Wl,-Bdynamic -lSoapySDR -Wl,-Bstatic
    
    LDLIBS += $(LIB)/win32/libpocketfft.a
    #LDLIBS += -lfftw3f
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang++
    INCLUDE += -I/opt/homebrew/include
    LDLIBS = -L/opt/homebrew/lib $(LIB)/macos/libsdr.a $(LIB)/macos/librtk.a $(LIB)/macos/libfec.a \
             $(LIB)/macos/libldpc.a -lpthread -lm -lusb-1.0 -lSoapySDR
    
    LDLIBS += $(LIB)/macos/libpocketfft.a
    #LDLIBS += -lfftw3f
else
    CC = g++
    LDLIBS = $(LIB)/linux/libsdr.a $(LIB)/linux/librtk.a $(LIB)/linux/libfec.a \
             $(LIB)/linux/libldpc.a -lpthread -lm -lusb-1.0 -lSoapySDR
    
    LDLIBS += $(LIB)/linux/libpocketfft.a
    #LDLIBS += -lfftw3f
endif

WARNOPT = -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

#CFLAGS = -O3 -march=native $(INCLUDE) $(WARNOPT) $(OPTIONS) -g
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

