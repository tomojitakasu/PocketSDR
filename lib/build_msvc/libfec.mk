#
#  nmake file for LIBFEC (libfec.so, libfec.lib)
#

SRC = ..\libfec
BUILD = build_msvc
INSTALL = ..\win32_msvc

TARGET = libfec.lib

all: $(TARGET)

libfec.lib:
	cd $(SRC) && cmake -S . -B $(BUILD) -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL -DCMAKE_C_FLAGS="/w" -DCMAKE_C_FLAGS_RELEASE="/O2 /w /Qspectre-"
	cd $(SRC) && cmake --build $(BUILD) --config Release --target fec
	copy /Y $(SRC)\$(BUILD)\Release\fec.lib libfec.lib >NUL

clean:
	-del /Q $(TARGET) 2>NUL
	-if exist $(SRC)\$(BUILD) rmdir /S /Q $(SRC)\$(BUILD)

install:
	-if not exist $(INSTALL) mkdir $(INSTALL)
	-for %f in ($(TARGET)) do copy /Y %f $(INSTALL) >NUL

