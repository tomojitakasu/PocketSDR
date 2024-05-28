#!/bin/bash
#
#  pocket_trk real-time test
#
L1="-sig L1CA -prn 1-32,193-199,120-158"
L6="-sig L6D -prn 193-202 -sig L6E -prn 203-212"

../bin/pocket_trk $L1 $L6 \
-c ../conf/pocket_L1L6_12MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

