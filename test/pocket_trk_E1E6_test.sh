#!/bin/bash
#
#  pocket_trk real-time test
#
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
E6="-sig E6B -prn 1-36 -sig E6C -prn 1-36"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L6_12MHz.conf - |
../bin/pocket_trk -r -f 12 -fi 3,0 $E1 $E6 \
-log pocket_trk_E1E6_test.log

