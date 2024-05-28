#!/bin/bash
#
#  pocket_trk real-time test
#
L1="-sig L1CA -prn 1-32,193-199,120-158"
L1S="-sig L1S -prn 184-189"
L5="-sig L5I -prn 1-32,193-199,120-158 -sig L5Q -prn 1-32,193-199,120-158"
L5S="-sig L5SI -prn 184-189 -sig L5SQ -prn 184-189"

../bin/pocket_trk $L1 $L1S $L5 $L5S \
-c ../conf/pocket_L1L5_32MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

