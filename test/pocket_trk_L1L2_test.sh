#!/bin/bash
#
#  pocket_trk real-time test
#
L1CA="-sig L1CA -prn 1-32,193-199,120-158"
L1S="-sig L1S -prn 184-191"
L2C="-sig L2CM -prn 1-32,193-199"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L2_6MHz.conf - |
../bin/pocket_trk -r -f 6 $L1CA $L1S $L2C \
-log pocket_trk_L1L2_test.log

