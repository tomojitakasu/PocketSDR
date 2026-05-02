#!/bin/bash
#
# generate backup
#
PRG=PocketSDR
TIME=`date +'%Y%m%d%H%M%S'`

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
        --exclude "*.nmea" \
        --exclude "*.rtcm3" \
        --exclude "*.log" \
        --exclude "*.zip" \
        --exclude "save" \
        --exclude "__pycache__" \
        --exclude "*_gerber" \
        --exclude "*_backups" \
        ${PRG}/* |
gzip > ${PRG}_backup_${TIME}.tar.gz

cd ${PRG}

