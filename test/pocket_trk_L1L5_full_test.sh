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
L5="-sig L5I -prn 1-32,193-199,120-158 -sig L5Q -prn 1-32,193-199,120-158"
L5S="-sig L5SI -prn 184-189 -sig L5SQ -prn 184-189"
E5A="-sig E5AI -prn 1-36 -sig E5AQ -prn 1-36"
B2A="-sig B2AD -prn 19-50 -sig B2AP -prn 19-50"
I5="-sig I5S -prn 1-14"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L5_24MHz.conf - |
../bin/pocket_trk -r -f 24 $L1 $L1S $E1 $B1C $I1 $L1C $L5 $L5S $E5A $B2A $I5 \
-log pocket_trk_L1L5_full_test.log

