#!/bin/bash
#
#  pocket_trk real-time test
#
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
E6="-sig E6B -prn 1-36 -sig E6C -prn 1-36"

../bin/pocket_trk $E1 $E6 \
-c ../conf/pocket_L1L6_12MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

