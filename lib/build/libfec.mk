#
#  makefile of LIBFEC shared library (libfec.so)
#
#! You need to install LIBFEC source tree as follows.
#!
#! $ git clone https://github.com/quiet/libfec libfec

#! specify directory of LIBFEC source tree
SRC = ../libfec

ifeq ($(OS),Windows_NT)
    INSTALL = ../win32
    EXT = so
else ifeq ($(shell uname -sm),Darwin arm64)
    INSTALL = ../macos
    EXT = dylib
else
    INSTALL = ../linux
    EXT = so
endif

ifeq ($(shell uname -m),aarch64)
    CONF_OPT = --build=arm
endif

TARGET = libfec.so libfec.a

all :
	DIR=`pwd`; \
	cd $(SRC); \
	./configure $(CONF_OPT); \
	sed 's/-lc//' < makefile > makefile.p; \
	mv makefile.p makefile; \
	make; \
	cd $$DIR; \
    cp $(SRC)/libfec.a ./libfec.a
	cp $(SRC)/libfec.$(EXT) ./libfec.so

clean:
	DIR=`pwd`; \
	cd $(SRC); \
	make clean; \
	cd $$DIR; \
	rm -f $(TARGET)

install:
	cp $(TARGET) $(INSTALL)
