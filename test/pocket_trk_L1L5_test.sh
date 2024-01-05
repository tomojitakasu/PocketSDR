#!/bin/bash
#
#  pocket_trk real-time test
#
L1C="-sig L1CD -prn 1-32,193-199 -sig L1CP -prn 1-32,193-199"
L5="-sig L5I -prn 1-32,193-199,120-158 -sig L5Q -prn 1-32,193-199,120-158"
L5S="-sig L5SI -prn 184-189 -sig L5SQ -prn 184-189"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L5_24MHz.conf - |
../bin/pocket_trk -r -f 24 -fi 6,0 $L1C $L5 $L5S \
-log pocket_trk_L1L5_test.log

