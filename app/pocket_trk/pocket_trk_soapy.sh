#!/bin/sh

./pocket_trk.exe -driver lime -fmt CS16 -f 64 -fo 1575.42 -gain 30 -bw 6 -sig L1CA -prn 1-32 -sig E1B -prn 1-36
#./pocket_trk.exe -driver uhd -fmt CS16 -f 32 -fo 1575.42 -gain 30 -bw 6 -sig L1CA -prn 1-32 -sig E1B -prn 1-36
#./pocket_trk.exe -driver rtlsdr -fmt CS8 -f 1.6 -fo 1575.42 -gain 30 -bw 1.2 -sig L1CA -prn 1-32 -sig E1B -prn 1-36

