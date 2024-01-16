#!/bin/bash
#
#  pocket_trk real-time test
#
C10="-sig L1CA -prn 194,194,194,194,194,194,194,194,194,194"
C50="$C10 $C10 $C10 $C10 $C10" #  50 CH
C100="$C50 $C50"               # 100 CH
C150="$C50 $C100"              # 150 CH
C200="$C100 $C100"             # 200 CH
C250="$C100 $C150"             # 250 CH

../bin/pocket_dump -r -q -c ../conf/pocket_L1L5_24MHz.conf - |
../bin/pocket_trk -r -f 24 $C150

