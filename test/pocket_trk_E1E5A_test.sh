#!/bin/bash
#
#  pocket_trk real-time test
#
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
E5A="-sig E5AI -prn 1-36 -sig E5AQ -prn 1-36"

../bin/pocket_trk $E1 $E5A \
-c ../conf/pocket_L1L5_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

