#!/bin/bash
#
#  pocket_trk real-time test
#
L1="-sig L1CA -prn 1-32,193-199,120-158 -sig L1S -prn 184-189"
G1="-sig G1CA -prn -7-6 -sig G1OCD -prn 1-27 -sig G1OCP -prn 1-27"
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
B1C="-sig B1CD -prn 19-50 -sig B1CP -prn 19-50"
I1="-sig I1SD -prn 1-14 -sig I1SP -prn 1-14"
L1C="-sig L1CD -prn 1-32,193-199 -sig L1CP -prn 1-32,193-199"
B1I="-sig B1I -prn 1-62"
L2="-sig L2CM -prn 1-32,193-199"
G2="-sig G2CA -prn -7-6 -sig G2OCP -prn 1-27"

../bin/pocket_trk -u $L1 $G1 $E1 $B1C $I1 $L1C $B1I $L2 $G2 \
-c ../conf/pocket_L1G1L2G2_24MHz.conf \
-log pocket_trk_L1G1L2G2_test.log

