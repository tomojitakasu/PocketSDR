#
#  makefile for RTKLIB shared library (librtk.so)
#
#! You need to install RTKLIB 2.4.3 source tree as follows.
#!
#! $ git clone https://github.com/tomojitakasu/RTKLIB -b rtklib_2.4.3 RTKLIB

CC = gcc

#! specify directory of RTKLIB source tree
SRC = ../RTKLIB/src

#! uncomment for Windows
INSTALL = ../win32
OPTIONS= -DENAGLO -DENAGAL -DENAQZS -DENACMP -DENAIRN -DNFREQ=5 -DEXOBS=3 -DSVR_REUSEADDR -DTRACE -DWIN32
LDLIBS = -lwsock32 -lwinmm

#! uncomment for Linuex
#INSTALL = ../linux
#OPTIONS= -DENAGLO -DENAGAL -DENAQZS -DENACMP -DENAIRN -DNFREQ=5 -DEXOBS=3 -DSVR_REUSEADDR -DTRACE
#LDLIBS =

INCLUDE= -I$(SRC)
WARNOPTS = -ansi -pedantic -Wall -Wno-unused-but-set-variable -Wno-unused-function -Wno-unused-const-variable

CFLAGS = -O3 $(INCLUDE) $(OPTIONS) $(WARNOPTS) -fPIC -g

OBJ = rtkcmn.o tides.o rtksvr.o rtkpos.o postpos.o geoid.o solution.o lambda.o sbas.o \
      stream.o rcvraw.o rtcm.o rtcm2.o rtcm3.o rtcm3e.o preceph.o options.o \
      pntpos.o ppp.o ppp_ar.o ephemeris.o rinex.o ionex.o convrnx.o convkml.o \
      convgpx.o streamsvr.o tle.o gis.o datum.o download.o \
      novatel.o ublox.o ss2.o crescent.o skytraq.o javad.o nvs.o binex.o rt17.o septentrio.o \
      rtklib_wrap.o

TARGET = librtk.so

$(TARGET) : $(OBJ)
	$(CC) -shared -o $@ $(OBJ) $(LDLIBS)

rtkcmn.o   : $(SRC)/rtkcmn.c
	$(CC) -c $(CFLAGS) $(SRC)/rtkcmn.c
rtksvr.o   : $(SRC)/rtksvr.c
	$(CC) -c $(CFLAGS) $(SRC)/rtksvr.c
rtkpos.o   : $(SRC)/rtkpos.c
	$(CC) -c $(CFLAGS) $(SRC)/rtkpos.c
postpos.o  : $(SRC)/postpos.c
	$(CC) -c $(CFLAGS) $(SRC)/postpos.c
geoid.o    : $(SRC)/geoid.c
	$(CC) -c $(CFLAGS) $(SRC)/geoid.c
solution.o : $(SRC)/solution.c
	$(CC) -c $(CFLAGS) $(SRC)/solution.c
lambda.o   : $(SRC)/lambda.c
	$(CC) -c $(CFLAGS) $(SRC)/lambda.c
sbas.o     : $(SRC)/sbas.c
	$(CC) -c $(CFLAGS) $(SRC)/sbas.c
stream.o   : $(SRC)/stream.c
	$(CC) -c $(CFLAGS) $(SRC)/stream.c
rcvraw.o   : $(SRC)/rcvraw.c
	$(CC) -c $(CFLAGS) $(SRC)/rcvraw.c
rtcm.o     : $(SRC)/rtcm.c
	$(CC) -c $(CFLAGS) $(SRC)/rtcm.c
rtcm2.o    : $(SRC)/rtcm2.c
	$(CC) -c $(CFLAGS) $(SRC)/rtcm2.c
rtcm3.o    : $(SRC)/rtcm3.c
	$(CC) -c $(CFLAGS) $(SRC)/rtcm3.c
rtcm3e.o   : $(SRC)/rtcm3e.c
	$(CC) -c $(CFLAGS) $(SRC)/rtcm3e.c
preceph.o  : $(SRC)/preceph.c
	$(CC) -c $(CFLAGS) $(SRC)/preceph.c
options.o  : $(SRC)/options.c
	$(CC) -c $(CFLAGS) $(SRC)/options.c
pntpos.o   : $(SRC)/pntpos.c
	$(CC) -c $(CFLAGS) $(SRC)/pntpos.c
ppp.o      : $(SRC)/ppp.c
	$(CC) -c $(CFLAGS) $(SRC)/ppp.c
ppp_ar.o   : $(SRC)/ppp_ar.c
	$(CC) -c $(CFLAGS) $(SRC)/ppp_ar.c
tides.o    : $(SRC)/tides.c
	$(CC) -c $(CFLAGS) $(SRC)/tides.c
ephemeris.o: $(SRC)/ephemeris.c
	$(CC) -c $(CFLAGS) $(SRC)/ephemeris.c
rinex.o    : $(SRC)/rinex.c
	$(CC) -c $(CFLAGS) $(SRC)/rinex.c
ionex.o    : $(SRC)/ionex.c
	$(CC) -c $(CFLAGS) $(SRC)/ionex.c
convrnx.o  : $(SRC)/convrnx.c
	$(CC) -c $(CFLAGS) $(SRC)/convrnx.c
convkml.o  : $(SRC)/convkml.c
	$(CC) -c $(CFLAGS) $(SRC)/convkml.c
convgpx.o  : $(SRC)/convgpx.c
	$(CC) -c $(CFLAGS) $(SRC)/convgpx.c
streamsvr.o: $(SRC)/streamsvr.c
	$(CC) -c $(CFLAGS) $(SRC)/streamsvr.c
tle.o      : $(SRC)/tle.c
	$(CC) -c $(CFLAGS) $(SRC)/tle.c
gis.o      : $(SRC)/gis.c
	$(CC) -c $(CFLAGS) $(SRC)/gis.c
datum.o    : $(SRC)/datum.c
	$(CC) -c $(CFLAGS) $(SRC)/datum.c
download.o : $(SRC)/download.c
	$(CC) -c $(CFLAGS) $(SRC)/download.c

novatel.o  : $(SRC)/rcv/novatel.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/novatel.c
ublox.o    : $(SRC)/rcv/ublox.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/ublox.c
ss2.o      : $(SRC)/rcv/ss2.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/ss2.c
crescent.o : $(SRC)/rcv/crescent.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/crescent.c
skytraq.o  : $(SRC)/rcv/skytraq.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/skytraq.c
javad.o    : $(SRC)/rcv/javad.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/javad.c
nvs.o      : $(SRC)/rcv/nvs.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/nvs.c
binex.o    : $(SRC)/rcv/binex.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/binex.c
rt17.o     : $(SRC)/rcv/rt17.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/rt17.c
septentrio.o: $(SRC)/rcv/septentrio.c
	$(CC) -c $(CFLAGS) $(SRC)/rcv/septentrio.c

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
convkml.o  : $(SRC)/rtklib.h
convgpx.o  : $(SRC)/rtklib.h
streamsvr.o: $(SRC)/rtklib.h
tle.o      : $(SRC)/rtklib.h
gis.o      : $(SRC)/rtklib.h
datum.o    : $(SRC)/rtklib.h
download.o : $(SRC)/rtklib.h

novatel.o  : $(SRC)/rtklib.h
ublox.o    : $(SRC)/rtklib.h
ss2.o      : $(SRC)/rtklib.h
crescent.o : $(SRC)/rtklib.h
skytraq.o  : $(SRC)/rtklib.h
javad.o    : $(SRC)/rtklib.h
nvs.o      : $(SRC)/rtklib.h
binex.o    : $(SRC)/rtklib.h
rt17.o     : $(SRC)/rtklib.h
septentrio.o: $(SRC)/rtklib.h

clean:
	rm -f $(TARGET) *.o

install:
	cp $(TARGET) $(INSTALL)
