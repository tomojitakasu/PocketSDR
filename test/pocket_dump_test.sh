#/bin/sh
#
# test drive for pocket_dump
#
#

while true; do
    #../bin/pocket_dump -t 30 ch1.bin ch2.bin -c ../conf/pocket_L1L6_12MHz.conf
    #python3 ../python/pocket_trk.py ch1.bin -f 12 -fi 3 -prn 194
    ../bin/pocket_dump -t 30 ch1.bin ch2.bin -c ../conf/pocket_L1L5_24MHz.conf
    python3 ../python/pocket_trk.py ch1.bin -f 24 -fi 6 -prn 194
    #../bin/pocket_dump -t 30 ch1.bin ch2.bin -c ../conf/pocket_L1L5_32MHz.conf
    #python3 ../python/pocket_trk.py ch1.bin -f 32 -fi 8 -prn 194
done

