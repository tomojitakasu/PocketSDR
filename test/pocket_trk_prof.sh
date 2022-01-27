#!/bin/sh
#
#  profiling pocket_trk.py
#

python3 -m cProfile -s cumtime ../python/pocket_trk.py L1.bin -f 12 -fi 3 -sig L1CA -prn 193,194,196 -q > prof1.txt

python3 -m cProfile -s cumtime ../python/pocket_trk.py L6.bin -f 12        -sig L6D -prn 193,194,196 -q > prof2.txt

