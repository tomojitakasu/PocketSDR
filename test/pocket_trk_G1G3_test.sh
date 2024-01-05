#!/bin/bash
#
#  pocket_trk real-time test
#
G1="-sig G1CA -prn -7-6"
G3="-sig G3OCD -prn 1-24 -sig G3OCP -prn 1-24"

../bin/pocket_dump -r -q -c ../conf/pocket_G1G3_24MHz.conf - |
../bin/pocket_trk -r -f 24 $G1 $G3 \
-log pocket_trk_G1G3_test.log

