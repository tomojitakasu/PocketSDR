#!/bin/bash
#
#  pocket_trk real-time test
#
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
E5A="-sig E5AI -prn 1-36 -sig E5AQ -prn 1-36"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L5_24MHz.conf - |
../bin/pocket_trk -r -f 24 $E1 $E5A \
-log pocket_trk_E1E5A_test.log

