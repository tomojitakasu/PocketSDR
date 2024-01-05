#!/bin/bash
#
#  pocket_trk real-time test
#
L1CA="-sig L1CA -prn 1-32,193-199,120-158"
L6="-sig L6D -prn 193-199 -sig L6E -prn 203-209"
E6="-sig E6B -prn 1-36 -sig E6C -prn 1-36"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L6_12MHz.conf - |
../bin/pocket_trk -r -f 12 -fi 3,0 $L1CA $L6 $E6 \
-log pocket_trk_L1L6_test.log

