#!/bin/bash
#
#  pocket_trk real-time test
#
#C10="-sig L1CA -prn 194,194,194,194,194 -sig L5I -prn 194,194,194,194,194"
C10="-sig L1CA -prn 196,196,196,196,196 -sig L5I -prn 196,196,196,196,196"

C50="$C10 $C10 $C10 $C10 $C10" #  50 CH
C100="$C50 $C50"               # 100 CH
C150="$C50 $C100"              # 150 CH
C200="$C100 $C100"             # 200 CH
C250="$C100 $C150"             # 250 CH
C300="$C100 $C200"             # 300 CH
C350="$C200 $C150"             # 350 CH
C400="$C200 $C200"             # 400 CH
C500="$C300 $C200"             # 500 CH
C600="$C300 $C300"             # 600 CH
C700="$C400 $C300"             # 700 CH
C800="$C400 $C400"             # 800 CH
C900="$C500 $C400"             # 900 CH

../bin/pocket_trk $C250 \
-c ../conf/pocket_L1L5_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

