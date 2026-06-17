#
#  makefile for pocket_trk, fftw_wisdom
#

SRC = ../../src
LIB = ../../lib
BIN = ../../bin
USE_SOAPY ?= 1
USE_FFTW ?= 0

INCLUDE = -I$(SRC) -I$(LIB)/RTKLIB/src

ifeq ($(OS),Windows_NT)
    CC = g++
    INCLUDE += -I$(LIB)/cyusb
    LDLIBS = $(LIB)/win32/libsdr.a $(LIB)/win32/librtk.a \
             $(LIB)/cyusb/CyAPI.a -lsetupapi -lavrt -lwinmm \
             -lws2_32 -lpthread -static
    ifeq ($(USE_SOAPY),1)
        INCLUDE += -I$(LIB)/SoapySDR/include
        LDLIBS += -L$(LIB)/SoapySDR/lib -Wl,-Bdynamic -lSoapySDR -Wl,-Bstatic
    endif
    
    ifeq ($(USE_FFTW),1)
        LDLIBS += -lfftw3f
    else
        LDLIBS += $(LIB)/win32/libpocketfft.a
    endif
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang++
    INCLUDE += -I/opt/homebrew/include
    LDLIBS = -L/opt/homebrew/lib $(LIB)/macos/libsdr.a $(LIB)/macos/librtk.a \
             -lpthread -lm -lusb-1.0
    ifeq ($(USE_SOAPY),1)
        LDLIBS += -lSoapySDR
    endif
    
    ifeq ($(USE_FFTW),1)
        LDLIBS += -lfftw3f
    else
        LDLIBS += $(LIB)/macos/libpocketfft.a
    endif
else
    CC = g++
    LDLIBS = $(LIB)/linux/libsdr.a $(LIB)/linux/librtk.a -lpthread -lm -lusb-1.0
    ifeq ($(USE_SOAPY),1)
        LDLIBS += -lSoapySDR
    endif
    
    ifeq ($(USE_FFTW),1)
        LDLIBS += -lfftw3f
    else
        LDLIBS += $(LIB)/linux/libpocketfft.a
    endif
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
