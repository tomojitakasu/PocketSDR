#!/bin/sh
#
#  test driver for pocket_acq
#
../bin/pocket_dump -t 10 L1.bin L2.bin -c ../conf/pocket_L1L2_4MHz.conf

../bin/pocket_acq L1.bin -f 4 -sig L1CA -prn 1-32,193-199
../bin/pocket_acq L1.bin -f 4 -sig L1CA -prn 120-158
../bin/pocket_acq L1.bin -f 4 -sig L1S  -prn 184-191
../bin/pocket_acq L2.bin -f 4 -sig L2CM -prn 1-32,193-199

../bin/pocket_dump -t 10 L1a.bin L5.bin -c ../conf/pocket_L1L5_24MHz.conf

../bin/pocket_acq L1a.bin -f 24 -sig L1CP -prn 1-32,193-199
../bin/pocket_acq L1a.bin -f 24 -sig L1CD -prn 1-32,193-199
../bin/pocket_acq L1a.bin -f 24 -sig E1B  -prn 1-36
../bin/pocket_acq L1a.bin -f 24 -sig E1C  -prn 1-36
../bin/pocket_acq L1a.bin -f 24 -sig B1CP -prn 19-46
../bin/pocket_acq L1a.bin -f 24 -sig B1CD -prn 19-46
../bin/pocket_acq L1a.bin -f 24 -sig I1SP -prn 1-14
../bin/pocket_acq L1a.bin -f 24 -sig I1SD -prn 1-14

../bin/pocket_acq L5.bin -f 24 -sig L5I  -prn 1-32,193-199
../bin/pocket_acq L5.bin -f 24 -sig L5Q  -prn 1-32,193-199
../bin/pocket_acq L5.bin -f 24 -sig L5I  -prn 120-158
../bin/pocket_acq L5.bin -f 24 -sig L5Q  -prn 120-158
../bin/pocket_acq L5.bin -f 24 -sig L5SI -prn 184-189
../bin/pocket_acq L5.bin -f 24 -sig L5SQ -prn 184-189
../bin/pocket_acq L5.bin -f 24 -sig E5AI -prn 1-36
../bin/pocket_acq L5.bin -f 24 -sig E5AQ -prn 1-36
../bin/pocket_acq L5.bin -f 24 -sig B2AP -prn 19-50
../bin/pocket_acq L5.bin -f 24 -sig B2AD -prn 19-50
../bin/pocket_acq L5.bin -f 24 -sig I5S  -prn 1-14

../bin/pocket_dump -t 10 L1b.bin L6.bin -c ../conf/pocket_L1L6_12MHz.conf

../bin/pocket_acq L6.bin -f 12 -sig L6D -prn 193-199
../bin/pocket_acq L6.bin -f 12 -sig L6E -prn 203-209
../bin/pocket_acq L6.bin -f 12 -sig E6B -prn 1-36
../bin/pocket_acq L6.bin -f 12 -sig E6C -prn 1-36

../bin/pocket_dump -t 10 G1.bin G2.bin -c ../conf/pocket_G1G2_12MHz.conf

../bin/pocket_acq G1.bin -f 12 -sig G1CA -prn -7-6
../bin/pocket_acq G2.bin -f 12 -sig G2CA -prn -7-6

../bin/pocket_dump -t 10 G1OC.bin G3OC.bin -c ../conf/pocket_G1OCG3OC_24MHz.conf

../bin/pocket_acq G1OC.bin -f 24 -sig G1OCD -prn 1-27
../bin/pocket_acq G1OC.bin -f 24 -sig G1OCP -prn 1-27
../bin/pocket_acq G3OC.bin -f 24 -sig G3OCD -prn 1-27
../bin/pocket_acq G3OC.bin -f 24 -sig G3OCP -prn 1-27

../bin/pocket_dump -t 10 E1.bin E5b.bin -c ../conf/pocket_E1E5b_24MHz.conf

../bin/pocket_acq E1.bin  -f 24 -sig E1B  -prn 1-36
../bin/pocket_acq E1.bin  -f 24 -sig E1C  -prn 1-36
../bin/pocket_acq E5b.bin -f 24 -sig E5BI -prn 1-36
../bin/pocket_acq E5b.bin -f 24 -sig E5BQ -prn 1-36
../bin/pocket_acq E5b.bin -f 24 -sig B2BI -prn 19-50

../bin/pocket_dump -t 10 B1I.bin B2I.bin -c ../conf/pocket_B1IB2I_12MHz.conf

../bin/pocket_acq B1I.bin -f 12 -sig B1I -prn 1-62
../bin/pocket_acq B2I.bin -f 12 -sig B2I -prn 1-18

../bin/pocket_dump -t 10 B1Ia.bin B3I.bin -c ../conf/pocket_B1IB3I_24MHz.conf

../bin/pocket_acq B1Ia.bin -f 24 -sig B1I -prn 1-62
../bin/pocket_acq B3I.bin  -f 24 -sig B3I -prn 1-62
