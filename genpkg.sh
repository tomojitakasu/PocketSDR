#!/bin/bash
#
# generate package
#
PRG=PocketSDR
VER=0.17

cd ..

tar cvf - \
        --exclude ".git" \
        --exclude "*.o" \
        --exclude "linux/*.a" \
        --exclude "linux/*.so" \
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

