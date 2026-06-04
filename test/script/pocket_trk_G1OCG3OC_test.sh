#!/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "$SCRIPT_DIR" || exit 1
#
#  pocket_trk real-time test
#
G1OC="-sig G1OCD -prn 1-27 -sig G1OCP -prn 1-27"
G3OC="-sig G3OCD -prn 1-27 -sig G3OCP -prn 1-27"

../../bin/pocket_trk $G1OC $G3OC \
-c ../../conf/pocket_G1OCG3OC_24MHz.conf \
-rtcm :10015 -nmea :10016 -log :10017 $@

