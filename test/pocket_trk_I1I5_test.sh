#!/bin/bash
#
#  pocket_trk real-time test
#
I1="-sig I1SD -prn 1-14 -sig I1SP -prn 1-14"
I5="-sig I5S -prn 1-14"

../bin/pocket_dump -r -q -c ../conf/pocket_L1L5_24MHz.conf - |
../bin/pocket_trk -r -f 24 $I1 $I5 \
-log pocket_trk_I1I5_test.log

