#!/bin/bash
#
#  pocket_trk real-time test
#
L1="-sig L1CA -prn 1-32"
B1C="-sig B1CD -prn 19-50 -sig B1CP -prn 19-50"
B2B="-sig B2BI -prn 19-50,59-62"

../bin/pocket_trk $L1 $B1C $B2B \
-c ../conf/pocket_E1E5b_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

