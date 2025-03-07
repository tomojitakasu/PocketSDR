#!/bin/bash
#
# Log of Serial Device
#
ISTR=serial://ttyACM1:115200
OSTR1=../../log/mosaic_%Y%m%d%h%M.sbf::S=1
OSTR2=tcpsvr://:10016
LOG=../../log/log_serial.log

../bin/str2str -in $ISTR -out $OSTR1 -out $OSTR2 # 2> $LOG

