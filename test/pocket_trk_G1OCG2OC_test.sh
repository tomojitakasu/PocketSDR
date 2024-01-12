#!/bin/bash
#
#  pocket_trk real-time test
#
G1OC="-sig G1OCD -prn 1-27 -sig G1OCP -prn 1-27"
G2OC="-sig G2OCP -prn 1-27"

../bin/pocket_dump -r -q -c ../conf/pocket_G1OCG2OC_12MHz.conf - |
../bin/pocket_trk -r -f 12 $G1OC $G2OC \
-log pocket_trk_G1OCG2OC_test.log

