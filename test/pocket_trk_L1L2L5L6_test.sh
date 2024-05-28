#!/bin/bash
#
#  pocket_trk real-time test
#
L1="-sig L1CA -prn 1-32,193-199,120-158"
L1S="-sig L1S -prn 184-189"
E1="-sig E1B -prn 1-36"
B1I="-sig B1I -prn 1-62"
B1C="-sig B1CD -prn 19-50"
I1="-sig I1SD -prn 1-14"
L1C="-sig L1CD -prn 1-32,193-199"
L2="-sig L2CM -prn 1-32,193-199"
L5="-sig L5I -prn 1-32,193-199,120-158"
L5S="-sig L5SI -prn 184-189"
E5A="-sig E5AI -prn 1-36"
B2A="-sig B2AD -prn 19-50"
I5="-sig I5S -prn 1-14"
#L6="-sig L6D -prn 193-202 -sig L6E -prn 203-212"
L6="-sig L6D -prn 193-202"
E6="-sig E6B -prn 1-36"
B3I="-sig B3I -prn 1-62"

../bin/pocket_trk $L1 $L1S $E1 $B1I $B1C $I1 $L1C $L2 $L5 $L5S $E5A $B2A $I5 $L6 $E6 $B3I \
-c ../conf/pocket_L1L2L5L6_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@
