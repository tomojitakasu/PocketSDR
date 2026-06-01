#
#  makefile for Pocket SDR external shared and static libraries
#
#! You need to install external libary source trees as follows.
#!
#! $ git clone https://github.com/quiet/libfec libfec
#! $ git clone https://github.com/radfordneal/LDPC-codes LDPC-codes
#! To use FFTW3 instead of PocketFFT, install FFTW3 and build with USE_FFTW=1.

USE_FFTW ?= 0

all:
	make -f librtk.mk
	make -f libldpc.mk
	make -f libfec.mk
ifneq ($(USE_FFTW),1)
	make -f libpocketfft.mk
endif
	make -f libsdr.mk
clean:
	make -f librtk.mk clean
	make -f libldpc.mk clean
	make -f libfec.mk clean
	make -f libpocketfft.mk clean
	make -f libsdr.mk clean
install:
	make -f librtk.mk install
	make -f libldpc.mk install
	make -f libfec.mk install
ifneq ($(USE_FFTW),1)
	make -f libpocketfft.mk install
endif
	make -f libsdr.mk install
