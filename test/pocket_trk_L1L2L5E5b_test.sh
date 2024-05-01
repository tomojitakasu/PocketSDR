#!/bin/bash
#
#  pocket_trk real-time test
#
L1="-sig L1CA -prn 1-32,193-199,120-158"
L1S="-sig L1S -prn 184-189"
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
B1C="-sig B1CD -prn 19-50 -sig B1CP -prn 19-50"
I1="-sig I1SD -prn 1-14 -sig I1SP -prn 1-14"
L1C="-sig L1CD -prn 1-32,193-199 -sig L1CP -prn 1-32,193-199"
L2="-sig L2CM -prn 1-32,193-199"
L5="-sig L5I -prn 1-32,193-199,120-158 -sig L5Q -prn 1-32,193-199,120-158"
L5S="-sig L5SI -prn 184-189"
E5A="-sig E5AI -prn 1-36 -sig E5AQ -prn 1-36"
B2A="-sig B2AD -prn 19-50 -sig B1AP -prn 19-50"
I5="-sig I5S -prn 1-14"
G3OC="-sig G3OCD -prn 1-27 -sig G3OCP -prn 1-27"
E5B="-sig E5BI -prn 1-36 -sig E5BQ -prn 1-36"
B2I="-sig B2I -prn 1-18"
B2B="-sig B2BI -prn 19-50,59-62"

../bin/pocket_trk -u $L1 $L1S $E1 $B1C $I1 $L1C $L2 $L5 $L5S $E5A $B2A $I5 $G3OC $E5B $B2I $B2B \
-c ../conf/pocket_L1L2L5E5b_24MHz.conf \
-log pocket_trk_L1L2L5E5b_test.log

