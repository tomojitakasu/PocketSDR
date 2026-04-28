#
#  nmake file for PocketFFT (libpocketfft.dll, libpocketfft.lib)
#

SRC = ..\pocketfft
INSTALL = ..\win32_msvc
CXX = cl
LD = link
AR = lib

CXXFLAGS = /nologo /O2 /W3 /MD /EHsc /std:c++17 /I$(SRC)
OBJ = pocketfft_wrap.obj
TARGET = libpocketfft.dll libpocketfft.lib

all: $(TARGET)

libpocketfft.dll: $(OBJ)
	$(LD) /NOLOGO /DLL /OUT:$@ $(OBJ)

libpocketfft.lib: $(OBJ)
	$(AR) /NOLOGO /OUT:$@ $(OBJ)

pocketfft_wrap.obj: $(SRC)\pocketfft_wrap.cpp
	$(CXX) /c $(CXXFLAGS) /Fo$@ $(SRC)\pocketfft_wrap.cpp

clean:
	-del /Q $(TARGET) *.obj 2>NUL

install:
	-if not exist $(INSTALL) mkdir $(INSTALL)
	-for %f in ($(TARGET)) do copy /Y %f $(INSTALL) >NUL

