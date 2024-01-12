#!/bin/bash
#
#  pocket_trk real-time test
#
E1="-sig E1B -prn 1-36 -sig E1C -prn 1-36"
E5B="-sig E5BI -prn 1-36 -sig E5BQ -prn 1-36"

../bin/pocket_dump -r -q -c ../conf/pocket_E1E5b_24MHz.conf - |
../bin/pocket_trk -r -f 24 $E1 $E5B \
-log pocket_trk_E1E5B_test.log

