#!/bin/bash
#
#  pocket_trk real-time test
#
G1OC="-sig G1OCD -prn 1-27 -sig G1OCP -prn 1-27"
G2OC="-sig G2OCP -prn 1-27"

../bin/pocket_trk $G1OC $G2OC \
-c ../conf/pocket_G1OCG2OC_12MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

