#
#  nmake file for Pocket SDR external shared and static libraries
#

all:
	nmake /nologo /f librtk.mk
	nmake /nologo /f libldpc.mk
	nmake /nologo /f libfec.mk
	nmake /nologo /f libpocketfft.mk
	nmake /nologo /f libsdr.mk

clean:
	nmake /nologo /f libsdr.mk clean
	nmake /nologo /f libpocketfft.mk clean
	nmake /nologo /f libfec.mk clean
	nmake /nologo /f libldpc.mk clean
	nmake /nologo /f librtk.mk clean

install:
	nmake /nologo /f librtk.mk install
	nmake /nologo /f libldpc.mk install
	nmake /nologo /f libfec.mk install
	nmake /nologo /f libpocketfft.mk install
	nmake /nologo /f libsdr.mk install

