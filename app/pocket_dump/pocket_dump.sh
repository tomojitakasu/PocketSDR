#!/bin/sh
#
# pocket_dump execution script
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

TSEC=10
FO="-fo 1575.42"
LOG=./pocket_dump.log

case $1 in
    usrp)   ./pocket_dump -driver uhd      -fmt CS16 -f 48  -gain 30 -bw 8 $FO -t $TSEC usrp_l1.bin   2> $LOG ;;
    lime)   ./pocket_dump -driver lime     -fmt CS16 -f 60  -gain 30 -bw 8 $FO -t $TSEC lime_l1.bin   2> $LOG ;;
    blade)  ./pocket_dump -driver bladerf  -fmt CS16 -f 40  -gain 30 -bw 4 $FO -t $TSEC blade_l1.bin  2> $LOG ;;
    pluto)  ./pocket_dump -driver plutosdr -fmt CS8  -f 6   -gain 30 -bw 4 $FO -t $TSEC pluto_l1.bin  2> $LOG ;;
    rtl)    ./pocket_dump -driver rtlsdr   -fmt CS8  -f 2.4 -gain 30 -bw 2 $FO -t $TSEC rtl_l1.bin    2> $LOG ;;
    airspy) ./pocket_dump -driver airspy   -fmt CS16 -f 6   -gain 30 -bw 4 $FO -t $TSEC airspy_l1.bin 2> $LOG ;;
    *)      ./pocket_dump -t $TSEC 2> $LOG ;; # Pocket SDR FE
esac
