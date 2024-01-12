#!/bin/bash
#
#  pocket_trk real-time test
#
G1OC="-sig G1OCD -prn 1-27 -sig G1OCP -prn 1-27"
G3OC="-sig G3OCD -prn 1-27 -sig G3OCP -prn 1-27"

../bin/pocket_dump -r -q -c ../conf/pocket_G1OCG3OC_24MHz.conf - |
../bin/pocket_trk -r -f 24 $G1OC $G3OC \
-log pocket_trk_G1OCG3OC_test.log

