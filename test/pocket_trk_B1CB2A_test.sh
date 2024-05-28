#!/bin/bash
#
#  pocket_trk real-time test
#
B1C="-sig B1CD -prn 19-50 -sig B1CP -prn 19-50"
B2A="-sig B2AD -prn 19-50 -sig B2AP -prn 19-50"

../bin/pocket_trk $B1C $B2A \
-c ../conf/pocket_L1L5_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

