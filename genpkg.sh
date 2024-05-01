#!/bin/bash
#
# generate package
#
PRG=PocketSDR
VER=0.12

cd ..

tar cvf - \
        --exclude ".git" \
        --exclude "*.o" \
        --exclude "*.a" \
        --exclude "*.so" \
        --exclude "*.bin" \
        --exclude "*.exe" \
        --exclude "*.log" \
        --exclude "*.zip" \
        --exclude "save" \
        --exclude "__pycache__" \
        --exclude "*_gerber" \
        --exclude "*_backups" \
        --exclude "HW/v3.0" \
        ${PRG}/* |
gzip > ${PRG}_${VER}.tar.gz

cd ${PRG}

