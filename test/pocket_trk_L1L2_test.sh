#!/bin/bash
#
#  pocket_trk real-time test
#
L1C="-sig L1CD -prn 1-32,193-199 -sig L1CP -prn 1-32,193-199"
L2C="-sig L2CM -prn 1-32,193-199"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L2_6MHz.conf - |
../bin/pocket_trk -r -f 6 $L1C $L2C \
-log pocket_trk_L1L2_test.log

