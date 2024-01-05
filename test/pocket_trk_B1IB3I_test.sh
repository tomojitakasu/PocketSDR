#!/bin/bash
#
#  pocket_trk real-time test
#
B1I="-sig B1I -prn 1-62"
B3I="-sig B3I -prn 1-62"

../bin/pocket_dump -r -q -c ../conf/pocket_B1IB3I_24MHz.conf - |
../bin/pocket_trk -r -f 24 $B1I $B3I \
-log pocket_trk_B1IB3I_test.log

