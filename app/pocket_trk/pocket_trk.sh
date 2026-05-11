#!/bin/sh
#
# pocket_trk execution script
#

# for SoapySDR via radioconda
case $(uname -s 2>/dev/null) in
    MINGW*|MSYS*|CYGWIN*)
        export HOME="${USERPROFILE:-$HOME}"
        RADIOCONDA="$HOME/radioconda"
        export PATH="$RADIOCONDA/Library/bin:$PATH"
        export SOAPY_SDR_PLUGIN_PATH="$RADIOCONDA/Library/lib/SoapySDR/modules0.8"
        ;;
    Darwin|Linux)
        RADIOCONDA="$HOME/radioconda"
        export PATH="$RADIOCONDA/bin:$PATH"
        export SOAPY_SDR_PLUGIN_PATH="$RADIOCONDA/lib/SoapySDR/modules0.8"
        ;;
esac

OPT="-fo 1575.42 -sig L1CA -prn 1-32,194-199 -sig E1B -prn 1-36 -opt pocket_trk_default.conf"

OUT="-nmea :10020 -rtcm :10021 -log :10022"
#LOG=/dev/null
LOG=./pocket_trk.log

case $1 in
    usrp)   ./pocket_trk -driver uhd      -fmt CS16 -f 48  -gain 30 -bw 8 $OPT $OUT 2> $LOG;;
    lime)   ./pocket_trk -driver lime     -fmt CS16 -f 60  -gain 30 -bw 8 $OPT $OUT 2> $LOG;;
    blade)  ./pocket_trk -driver bladerf  -fmt CS16 -f 40  -gain 30 -bw 4 $OPT $OUT 2> $LOG;;
    pluto)  ./pocket_trk -driver plutosdr -fmt CS8  -f 6   -gain 30 -bw 4 -fd 20000 $OPT $OUT 2> $LOG;;
    rtl)    ./pocket_trk -driver rtlsdr   -fmt CS8  -f 2.4 -gain 30 -bw 2 $OPT $OUT 2> $LOG;;
    airspy) ./pocket_trk -driver airspy   -fmt CS16 -f 6   -gain 30 -bw 4 $OPT $OUT 2> $LOG;;
    *)      ./pocket_trk $OPT $OUT 2> $LOG;; # Pocket SDR FE
esac

