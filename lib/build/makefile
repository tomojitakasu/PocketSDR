#
#  makefile for Pocket SDR external shared libraries
#
#! You need to install external libary source trees as follows.
#!
#! $ git clone https://github.com/quiet/libfec libfec
#! $ git clone https://github.com/radfordneal/LDPC-codes LDPC-codes
#! $ git clone https://github.com/tomojitakasu/RTKLIB -b rtklib_2.4.3 RTKLIB
#!
#! You need to install external shared libary LIBFFTW3 as follows.
#!
#! $ pacman -S mingw-w64-x86_64-fftw (MINGW64)
#! $ sudo apt install libfftw3-dev   (Ubuntu)

all:
	make -f librtk.mk
	make -f libsdr.mk
	make -f libldpc.mk
	make -f libfec.mk
clean:
	make -f librtk.mk clean
	make -f libsdr.mk clean
	make -f libldpc.mk clean
	make -f libfec.mk clean
install:
	make -f librtk.mk install
	make -f libsdr.mk install
	make -f libldpc.mk install
	make -f libfec.mk install

