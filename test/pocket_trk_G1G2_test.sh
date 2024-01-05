#!/bin/bash
#
#  pocket_trk real-time test
#
G1="-sig G1CA -prn -7-6"
G2="-sig G2CA -prn -7-6"

../bin/pocket_dump -r -q -c ../conf/pocket_G1G2_12MHz.conf - |
../bin/pocket_trk -r -f 12 $G1 $G2 \
-log pocket_trk_G1G2_test.log

