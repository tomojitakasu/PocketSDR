#!/bin/bash
#
# generate package
#
PRG=PocketSDR
VER=0.14

cd ..

tar cvf - \
        --exclude ".git" \
        --exclude "*.o" \
        --exclude "win32/*.a" \
        --exclude "linux/*.a" \
        --exclude "macos/*.a" \
        --exclude "*.so" \
        --exclude "*.bin" \
        --exclude "*.exe" \
        --exclude "*.log" \
        --exclude "*.zip" \
        --exclude "save" \
        --exclude "__pycache__" \
        --exclude "*_gerber" \
        --exclude "*-backups" \
        --exclude "doc/ref/*" \
        --exclude "doc/src/*" \
        --exclude "FE_*" \
        ${PRG}/* |
gzip > ${PRG}_${VER}.tar.gz

cd ${PRG}

