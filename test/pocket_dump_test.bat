@echo off

for /l %%i in (0, 1, 10000) do (
    ..\bin\pocket_dump -t 30 ch1.bin ch2.bin -c ../conf/pocket_L1L5_24MHz.conf
    python ..\python/pocket_trk.py ch1.bin -f 24 -fi 6 -prn 194
)

