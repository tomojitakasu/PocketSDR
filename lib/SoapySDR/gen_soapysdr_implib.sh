#!/bin/sh
#
# Generate a MinGW import lib (libSoapySDR.dll.a) from radioconda's MSVC-built
# SoapySDR.dll, so a MinGW UCRT64 build can link against that DLL and load it
# at runtime — keeping the SoapySDR runtime and its plugins (LMS7Support.dll
# etc.) on the same MSVC C++ ABI.
#
# Why this script exists:
#   radioconda ships only SoapySDR.lib (MSVC COFF, unusable by MinGW ld); no
#   MinGW-compatible import lib is provided. We synthesize one by reading the
#   C-API exports from the DLL with objdump, writing a .def file that names
#   the DLL as "SoapySDR.dll" (no "lib" prefix), and feeding it to dlltool.
#
#   Only the C-API (SoapySDR_*) is bridged. C++ ABI differs between MSVC and
#   MinGW, so C++ symbols cannot cross.
#
# Usage:
#   gen_soapysdr_implib.sh <path/to/SoapySDR.dll> <output libSoapySDR.dll.a>
#
# To refresh after a SoapySDR upgrade, delete the output .dll.a and rebuild.
#
set -e

DLL=$1
OUT=$2
DEF=${OUT%.dll.a}.def

(echo 'LIBRARY SoapySDR.dll'; echo 'EXPORTS'; \
 objdump -p "$DLL" | \
 awk '/Ordinal\/Name Pointer/,/^$/' | \
 awk '/^\t\[/ && NF>=4 {print $NF}' | \
 grep '^SoapySDR') > "$DEF"
dlltool -d "$DEF" -D SoapySDR.dll -l "$OUT"
rm -f "$DEF"

