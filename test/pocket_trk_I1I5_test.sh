#!/bin/bash
#
#  pocket_trk real-time test
#
I1="-sig I1SD -prn 1-14 -sig I1SP -prn 1-14"
I5="-sig I5S -prn 1-14"

../bin/pocket_trk $I1 $I5 \
-c ../conf/pocket_L1L5_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

