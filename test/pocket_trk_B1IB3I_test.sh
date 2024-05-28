#!/bin/bash
#
#  pocket_trk real-time test
#
B1I="-sig B1I -prn 1-62"
B3I="-sig B3I -prn 1-62"

../bin/pocket_trk $B1I $B3I \
-c ../conf/pocket_B1IB3I_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

