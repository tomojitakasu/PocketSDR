#
#  nmake file of LDPC-codes library (libldpc.dll, libldpc.lib)
#

SRC = ..\LDPC-codes
INSTALL = ..\win32_msvc
CC = cl
LD = link
AR = lib

CFLAGS = /nologo /O2 /W3 /MD /I$(SRC)
LDLIBS =

OBJ = rcode.obj channel.obj dec.obj enc.obj alloc.obj intio.obj blockio.obj \
      check.obj open.obj mod2dense.obj mod2sparse.obj mod2convert.obj \
      distrib.obj rand.obj

TARGET = libldpc.dll libldpc.lib

all: $(TARGET)

libldpc.dll: $(OBJ)
	$(LD) /NOLOGO /DLL /OUT:$@ $(OBJ) $(LDLIBS)

libldpc.lib: $(OBJ)
	$(AR) /NOLOGO /OUT:$@ $(OBJ)

rcode.obj: $(SRC)\rcode.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\rcode.c
channel.obj: $(SRC)\channel.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\channel.c
dec.obj: $(SRC)\dec.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\dec.c
enc.obj: $(SRC)\enc.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\enc.c
alloc.obj: $(SRC)\alloc.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\alloc.c
intio.obj: $(SRC)\intio.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\intio.c
blockio.obj: $(SRC)\blockio.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\blockio.c
check.obj: $(SRC)\check.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\check.c
open.obj: $(SRC)\open.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\open.c
mod2dense.obj: $(SRC)\mod2dense.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\mod2dense.c
mod2sparse.obj: $(SRC)\mod2sparse.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\mod2sparse.c
mod2convert.obj: $(SRC)\mod2convert.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\mod2convert.c
distrib.obj: $(SRC)\distrib.c
	$(CC) /c $(CFLAGS) /Fo$@ $(SRC)\distrib.c
rand.obj: $(SRC)\rand.c
	$(CC) /c $(CFLAGS) /DRAND_FILE=\"./randfile\" /Fo$@ $(SRC)\rand.c

clean:
	-del /Q $(TARGET) *.obj 2>NUL

install:
	-if not exist $(INSTALL) mkdir $(INSTALL)
	-for %f in ($(TARGET)) do copy /Y %f $(INSTALL) >NUL

