#
#  makefile for Pocket SDR external shared and static libraries
#
#! To use FFTW3 instead of PocketFFT, install FFTW3 and build with USE_FFTW=1.

USE_FFTW ?= 0

all:
	make -f librtk.mk
ifneq ($(USE_FFTW),1)
	make -f libpocketfft.mk
endif
	make -f libsdr.mk
clean:
	make -f librtk.mk clean
	make -f libpocketfft.mk clean
	make -f libsdr.mk clean
install:
	make -f librtk.mk install
ifneq ($(USE_FFTW),1)
	make -f libpocketfft.mk install
endif
	make -f libsdr.mk install
