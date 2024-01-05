#!/bin/bash
#
#  pocket_trk real-time test
#
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
E5A="-sig E5AI -prn 1-36 -sig E5AQ -prn 1-36"
B2A="-sig B2AD -prn 19-46 -sig B2AP -prn 19-46"
I5S="-sig I5S -prn 1-10"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L5_24MHz.conf - |
../bin/pocket_trk -r -f 24 -fi 6,0 $E1 $E5A $B2A $I5S \
-log pocket_trk_E1E5A_test.log

