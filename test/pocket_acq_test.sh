#!/bin/sh
#
#  test driver for pocket_acq.py
#

../bin/pocket_dump -t 10 L1.bin L2.bin -c ../conf/pocket_L1L2_4MHz.conf

../python/pocket_acq.py L1.bin -f 4 -np -sig L1CA -prn 1-32,193-199
../python/pocket_acq.py L1.bin -f 4 -np -sig L1CA -prn 120-158
../python/pocket_acq.py L1.bin -f 4 -np -sig L1S  -prn 184-191
../python/pocket_acq.py L2.bin -f 4 -np -sig L2CM -prn 1-32,193-199

../bin/pocket_dump -t 10 L1a.bin L5.bin -c ../conf/pocket_L1L5_24MHz.conf

../python/pocket_acq.py L1a.bin -f 24 -np -sig L1CP -prn 1-32,193-199
../python/pocket_acq.py L1a.bin -f 24 -np -sig L1CD -prn 1-32,193-199
../python/pocket_acq.py L1a.bin -f 24 -np -sig E1B  -prn 1-36
../python/pocket_acq.py L1a.bin -f 24 -np -sig E1C  -prn 1-36
../python/pocket_acq.py L1a.bin -f 24 -np -sig B1CP -prn 19-50
../python/pocket_acq.py L1a.bin -f 24 -np -sig B1CD -prn 19-50
../python/pocket_acq.py L1a.bin -f 24 -np -sig I1SP -prn 1-14
../python/pocket_acq.py L1a.bin -f 24 -np -sig I1SD -prn 1-14

../python/pocket_acq.py L5.bin -f 24 -np -sig L5I  -prn 1-32,193-199
../python/pocket_acq.py L5.bin -f 24 -np -sig L5Q  -prn 1-32,193-199
../python/pocket_acq.py L5.bin -f 24 -np -sig L5I  -prn 120-158
../python/pocket_acq.py L5.bin -f 24 -np -sig L5Q  -prn 120-158
../python/pocket_acq.py L5.bin -f 24 -np -sig L5SI -prn 184-189
../python/pocket_acq.py L5.bin -f 24 -np -sig L5SQ -prn 184-189
../python/pocket_acq.py L5.bin -f 24 -np -sig E5AI -prn 1-36
../python/pocket_acq.py L5.bin -f 24 -np -sig E5AQ -prn 1-36
../python/pocket_acq.py L5.bin -f 24 -np -sig B2AP -prn 19-50
../python/pocket_acq.py L5.bin -f 24 -np -sig B2AD -prn 19-50
../python/pocket_acq.py L5.bin -f 24 -np -sig I5S  -prn 1-14

../bin/pocket_dump -t 10 L1b.bin L6.bin -c ../conf/pocket_L1L6_12MHz.conf

../python/pocket_acq.py L6.bin -f 12 -np -sig L6D -prn 193-199
../python/pocket_acq.py L6.bin -f 12 -np -sig L6E -prn 203-209
../python/pocket_acq.py L6.bin -f 12 -np -sig E6B -prn 1-36
../python/pocket_acq.py L6.bin -f 12 -np -sig E6C -prn 1-36

../bin/pocket_dump -t 10 G1.bin G2.bin -c ../conf/pocket_G1G2_12MHz.conf

../python/pocket_acq.py G1.bin -f 12 -np -sig G1CA -prn -7-6
../python/pocket_acq.py G2.bin -f 12 -np -sig G2CA -prn -7-6

../bin/pocket_dump -t 10 G1OC.bin G3OC.bin -c ../conf/pocket_G1OCG3OC_24MHz.conf

../python/pocket_acq.py G1OC.bin -f 24 -np -sig G1OCD -prn 1-27
../python/pocket_acq.py G1OC.bin -f 24 -np -sig G1OCP -prn 1-27
../python/pocket_acq.py G3OC.bin -f 24 -np -sig G3OCD -prn 1-27
../python/pocket_acq.py G3OC.bin -f 24 -np -sig G3OCP -prn 1-27

../bin/pocket_dump -t 10 E1.bin E5b.bin -c ../conf/pocket_E1E5b_24MHz.conf

../python/pocket_acq.py E1.bin  -f 24 -np -sig E1B  -prn 1-36
../python/pocket_acq.py E1.bin  -f 24 -np -sig E1C  -prn 1-36
../python/pocket_acq.py E5b.bin -f 24 -np -sig E5BI -prn 1-36
../python/pocket_acq.py E5b.bin -f 24 -np -sig E5BQ -prn 1-36
../python/pocket_acq.py E5b.bin -f 24 -np -sig B2BI -prn 19-50

../bin/pocket_dump -t 10 B1I.bin B2I.bin -c ../conf/pocket_B1IB2I_12MHz.conf

../python/pocket_acq.py B1I.bin -f 12 -np -sig B1I -prn 1-62
../python/pocket_acq.py B2I.bin -f 12 -np -sig B2I -prn 1-18

../bin/pocket_dump -t 10 B1Ia.bin B3I.bin -c ../conf/pocket_B1IB3I_24MHz.conf

../python/pocket_acq.py B1Ia.bin -f 24 -np -sig B1I -prn 1-62
../python/pocket_acq.py B3I.bin  -f 24 -np -sig B3I -prn 1-62

