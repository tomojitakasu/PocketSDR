#
#  makefile of LIBFEC shared library (libfec.so)
#
#! You need to install LIBFEC source tree as follows.
#!
#! $ git clone https://github.com/quiet/libfec libfec

#! specify directory of LIBFEC source tree
SRC = ../libfec

#! uncomment for Windows
INSTALL = ../win32

#! uncomment for Linuex
#INSTALL = ../linux

TARGET = libfec.so

$(TARGET) :
	DIR=`pwd`; \
	cd $(SRC); \
	./configure; \
	sed 's/-lc//' < makefile > makefile.p; \
	mv makefile.p makefile; \
	make; \
	cd $$DIR; \
	cp $(SRC)/$@ .

clean:
	DIR=`pwd`; \
	cd $(SRC); \
	make clean; \
	cd $$DIR; \
	rm -f $(TARGET)

install:
	cp $(TARGET) $(INSTALL)
