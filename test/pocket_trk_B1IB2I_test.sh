#!/bin/bash
#
#  pocket_trk real-time test
#
B1I="-sig B1I -prn 1-62"
B2I="-sig B2I -prn 1-18"

../bin/pocket_trk $B1I $B2I \
-c ../conf/pocket_B1IB2I_12MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

