#!/bin/bash
#
#  pocket_trk real-time test
#
G1="-sig G1CA -prn -7-6"
G2="-sig G2CA -prn -7-6"

../bin/pocket_trk $G1 $G2 \
-c ../conf/pocket_G1G2_12MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

