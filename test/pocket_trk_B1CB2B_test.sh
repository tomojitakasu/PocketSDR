#!/bin/bash
#
#  pocket_trk real-time test
#
B1C="-sig B1CD -prn 19-50 -sig B1CP -prn 19-50"
B2B="-sig B2BI -prn 19-50,59-62"

../bin/pocket_dump -r -q -c ../conf/pocket_E1E5b_24MHz.conf - |
../bin/pocket_trk -r -f 24 $B1C $B2B \
-log pocket_trk_B1CB2B_test.log

