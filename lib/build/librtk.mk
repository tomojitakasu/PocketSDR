#
#  makefile for RTKLIB library (librtk.so, librtk.a)
#

SRC = ../RTKLIB/src

ifeq ($(OS),Windows_NT)
    CC = gcc
    INSTALL = ../win32
    OPTIONS= -DSVR_REUSEADDR -DTRACE -DWIN32
    LDLIBS = -lwsock32 -lwinmm
else ifeq ($(shell uname -sm),Darwin arm64)
    CC = clang
    INSTALL = ../macos
    OPTIONS= -DSVR_REUSEADDR -DTRACE -DMACOS -Wno-deprecated
    LDLIBS =
else
    CC = gcc
    INSTALL = ../linux
    OPTIONS= -DSVR_REUSEADDR -DTRACE
    LDLIBS =
endif

INCLUDE= -I$(SRC)
WARNOPTS = -ansi -pedantic -Wall -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-const-variable

CFLAGS = -O3 $(INCLUDE) $(OPTIONS) $(WARNOPTS) -fPIC -g

OBJ = rtkcmn.o tides.o rtkpos.o geoid.o solution.o lambda.o sbas.o \
      stream.o rcvraw.o rtcm.o rtcm2.o rtcm3.o rtcm3e.o preceph.o options.o \
      pntpos.o ppp.o ppp_ar.o ephemeris.o rinex.o ionex.o convrnx.o \
      streamsvr.o binex.o ublox.o novatel.o septentrio.o rtklib_wrap.o

TARGET = librtk.so librtk.a

all : $(TARGET)

librtk.so : $(OBJ)
	$(CC) -shared -o $@ $(OBJ) $(LDLIBS)

librtk.a  : $(OBJ)
	$(AR) r $@ $(OBJ)

rtkcmn.o   : $(SRC)/rtkcmn.c
	$(CC) -c $(CFLAGS) $<
rtkpos.o   : $(SRC)/rtkpos.c
	$(CC) -c $(CFLAGS) $<
postpos.o  : $(SRC)/postpos.c
	$(CC) -c $(CFLAGS) $<
geoid.o    : $(SRC)/geoid.c
	$(CC) -c $(CFLAGS) $<
solution.o : $(SRC)/solution.c
	$(CC) -c $(CFLAGS) $<
lambda.o   : $(SRC)/lambda.c
	$(CC) -c $(CFLAGS) $<
sbas.o     : $(SRC)/sbas.c
	$(CC) -c $(CFLAGS) $<
stream.o   : $(SRC)/stream.c
	$(CC) -c $(CFLAGS) $<
rcvraw.o   : $(SRC)/rcvraw.c
	$(CC) -c $(CFLAGS) $<
rtcm.o     : $(SRC)/rtcm.c
	$(CC) -c $(CFLAGS) $<
rtcm2.o    : $(SRC)/rtcm2.c
	$(CC) -c $(CFLAGS) $<
rtcm3.o    : $(SRC)/rtcm3.c
	$(CC) -c $(CFLAGS) $<
rtcm3e.o   : $(SRC)/rtcm3e.c
	$(CC) -c $(CFLAGS) $<
preceph.o  : $(SRC)/preceph.c
	$(CC) -c $(CFLAGS) $<
options.o  : $(SRC)/options.c
	$(CC) -c $(CFLAGS) $<
pntpos.o   : $(SRC)/pntpos.c
	$(CC) -c $(CFLAGS) $<
ppp.o      : $(SRC)/ppp.c
	$(CC) -c $(CFLAGS) $<
ppp_ar.o   : $(SRC)/ppp_ar.c
	$(CC) -c $(CFLAGS) $<
tides.o    : $(SRC)/tides.c
	$(CC) -c $(CFLAGS) $<
ephemeris.o: $(SRC)/ephemeris.c
	$(CC) -c $(CFLAGS) $<
rinex.o    : $(SRC)/rinex.c
	$(CC) -c $(CFLAGS) $<
ionex.o    : $(SRC)/ionex.c
	$(CC) -c $(CFLAGS) $<
convrnx.o  : $(SRC)/convrnx.c
	$(CC) -c $(CFLAGS) $<
streamsvr.o: $(SRC)/streamsvr.c
	$(CC) -c $(CFLAGS) $<
binex.o    : $(SRC)/rcv/binex.c
	$(CC) -c $(CFLAGS) $<
novatel.o  : $(SRC)/rcv/novatel.c
	$(CC) -c $(CFLAGS) $<
ublox.o    : $(SRC)/rcv/ublox.c
	$(CC) -c $(CFLAGS) $<
septentrio.o: $(SRC)/rcv/septentrio.c
	$(CC) -c $(CFLAGS) $<

rtkcmn.o   : $(SRC)/rtklib.h
rtksvr.o   : $(SRC)/rtklib.h
rtkpos.o   : $(SRC)/rtklib.h
postpos.o  : $(SRC)/rtklib.h
geoid.o    : $(SRC)/rtklib.h
solution.o : $(SRC)/rtklib.h
lambda.o   : $(SRC)/rtklib.h
sbas.o     : $(SRC)/rtklib.h
rcvraw.o   : $(SRC)/rtklib.h
rtcm.o     : $(SRC)/rtklib.h
rtcm2.o    : $(SRC)/rtklib.h
rtcm3.o    : $(SRC)/rtklib.h
rtcm3e.o   : $(SRC)/rtklib.h
preceph.o  : $(SRC)/rtklib.h
options.o  : $(SRC)/rtklib.h
pntpos.o   : $(SRC)/rtklib.h
ppp.o      : $(SRC)/rtklib.h
ppp_ar.o   : $(SRC)/rtklib.h
tides.o    : $(SRC)/rtklib.h
ephemeris.o: $(SRC)/rtklib.h
rinex.o    : $(SRC)/rtklib.h
ionex.o    : $(SRC)/rtklib.h
convrnx.o  : $(SRC)/rtklib.h
streamsvr.o: $(SRC)/rtklib.h
binex.o    : $(SRC)/rtklib.h
novatel.o  : $(SRC)/rtklib.h
ublox.o    : $(SRC)/rtklib.h
septentrio.o: $(SRC)/rtklib.h

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
