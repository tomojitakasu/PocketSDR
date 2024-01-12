#!/bin/bash
#
#  pocket_trk real-time test
#
B1C="-sig B1CD -prn 19-50 -sig B1CP -prn 19-50"
B2A="-sig B2AD -prn 19-50 -sig B2AP -prn 19-50"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L5_24MHz.conf - |
../bin/pocket_trk -r -f 24 $B1C $B2A \
-log pocket_trk_B1CB2A_test.log

