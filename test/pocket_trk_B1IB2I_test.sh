#!/bin/bash
#
#  pocket_trk real-time test
#
B1I="-sig B1I -prn 1-62"
B2I="-sig B2I -prn 1-18"

../bin/pocket_dump.exe -r -q -c ../conf/pocket_B1IB2I_12MHz.conf - |
../bin/pocket_trk.exe -r -f 12 $B1I $B2I \
-log pocket_trk_B1IB2I_test.log

