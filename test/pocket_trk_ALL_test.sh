#!/bin/bash
#
#  pocket_trk real-time test
#
L1="-sig L1CA -prn 1-32,193-199,120-158 -sig L1S -prn 184-189"
G1="-sig G1CA -prn -7-6 -sig G1OCD -prn 1-27 -sig G1OCP -prn 1-27"
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
B1C="-sig B1CD -prn 19-50 -sig B1CP -prn 19-50"
I1="-sig I1SD -prn 1-14"
L1C="-sig L1CD -prn 1-32,193-199 -sig L1CP -prn 1-32,193-199"
L2="-sig L2CM -prn 1-32,193-199"
G2="-sig G2CA -prn -7-6 -sig G2OCP -prn 1-27"
L5="-sig L5I -prn 1-32,193-199,120-158 -sig L5Q -prn 1-32,193-199,120-158"
E5A="-sig E5AI -prn 1-36 -sig E5AQ -prn 1-36"
E5B="-sig E5BI -prn 1-36 -sig E5BQ -prn 1-36"
B2I="-sig B2I -prn 1-18"
B2A="-sig B2AD -prn 19-50 -sig B2AP -prn 19-50"
B2B="-sig B2BI -prn 19-50,59-62"
I5="-sig I5S -prn 1-14"
#L6="-sig L6D -prn 193-202 -sig L6E -prn 203-212"
E6="-sig E6B -prn 1-36 -sig E6C -prn 1-36"
B3I="-sig B3I -prn 1-62"

../bin/pocket_trk $L1 $G1 $E1 $B1C $I1 $L1C $L2 $G2 $L5 $E5A $B2A $I5 $L6 $E6 $B3I \
-c ../conf/pocket_ALL_48MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@
