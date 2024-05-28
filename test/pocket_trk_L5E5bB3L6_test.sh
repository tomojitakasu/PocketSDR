#!/bin/bash
#
#  pocket_trk real-time test
#
L5="-sig L5I -prn 1-32,193-199,120-158 -sig L5Q -prn 1-32,193-199,120-158"
E5A="-sig E5AI -prn 1-36 -sig E5AQ -prn 1-36"
B2A="-sig B2AD -prn 19-50 -sig B2AP -prn 19-50"
I5="-sig I5S -prn 1-14"
L5S="-sig L5SI -prn 184-189 -sig L5SQ -prn 184-189"
E5B="-sig E5BI -prn 1-36 -sig E5BQ -prn 1-36"
B2I="-sig B2I -prn 1-18"
B2I="-sig B2I -prn 1-18"
B2B="-sig B2BI -prn 19-50,59-62"
B3I="-sig B3I -prn 1-62"
#L6="-sig L6D -prn 193-202 -sig L6E -prn 203-212"
E6="-sig E6B -prn 1-36 -sig E6C -prn 1-36"

../bin/pocket_trk $L5 $E5A $B2A $I5 $L5S $E5B $B2I $B2B $B3I $L6 $E6 \
-c ../conf/pocket_L5E5bB3L6_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

