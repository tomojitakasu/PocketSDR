#!/bin/bash
#
#  pocket_trk real-time test
#
L1="-sig L1CA -prn 1-32"
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
E5B="-sig E5BI -prn 1-36 -sig E5BQ -prn 1-36"

../bin/pocket_trk $L1 $E1 $E5B \
-c ../conf/pocket_E1E5b_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

