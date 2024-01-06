# **Pocket SDR - An Open-Source GNSS SDR, ver. 0.9**

## **Overview**

Pocket SDR is an open-source GNSS (Global Navigation Satellite System) receiver
based on the SDR (software defined radio) technology. It consists of a RF front-
end device, some utilities for the device and GNSS-SDR APs (application programs)
written in Python and C. It supports almost all signals for GPS, GLONASS,
Galileo, QZSS, BeiDou, NavIC and SBAS.

The RF front-end device consists of 2 CH Maxim MAX2771 GNSS RF front-end IC
(LNA, mixer, filter, ADC, frequency synthesizer) and Cypress EZ-USB FX2LP USB
2.0 controller to connect to host PCs. The front-end CH1 is dedicated for GNSS
L1 band (1525 - 1610 MHz) and CH2 is for GNSS L2/L5/L6 band (1160 - 1290 MHz).
The frequency of the reference oscillator (TCXO) is 24.000 MHz and ADC sampling
frequency can be configured up to 24 MHz.

Pocket SDR contains some utility programs for the RF front-end device to
configure the device, capture and dump the digitized IF (inter-frequency) data.
These supports Windows, Linux and other environments.

Pocket SDR also provides GNSS-SDR APs to show the PSD (power spectrum density)
of captured IF data, search the GNSS signals, track these signals and decode
navigation data in them. The supported GNSS signals are as follows. For details
for these signals and signal IDs used in the Pocket SDR APs, refer 
[Pocket SDR Signal IDs](/doc/signal_IDs.pdf).

GPS: L1C/A, L1CP, L1CD, L2CM, L5I, L5Q, GLONASS: L1C/A, L2C/A, L3OCD, L3OCP,
Galileo: E1B, E1C, E5aI, E5aQ, E5bI, E5bQ, E6B, E6C, QZSS: L1C/A, L1C/B, L1CP,
L1CD, L1S, L2CM, L5I, L5Q, L5SI, L5SQ, L6D, L6E, BeiDou: B1I, B1CP, B1CD, B2I,
B2aD, B2aP, B2bI, B3I, NavIC: L5-SPS, SBAS: L1C/A, L5I, L5Q

These APs are written in Python and C by very compact way. They are easily
modified by users to add user's unique algorithms. 

<img src="image/pocket_sdr_image.jpg" width=80%>

The introduction of Pocket SDR is shown in the following slides.

T.Takasu, An Open Source GNSS SDR: Development and Application, IPNTJ Next GNSS
Technology WG, Feb 21, 2022
(https://gpspp.sakura.ne.jp/paper2005/IPNTJ_NEXTWG_202202.pdf)

--------------------------------------------------------------------------------

## **Package Structure**
```
PocketSDR --+-- bin     Pocket SDR utilities and APs binary programs for Windows
            +-- app     Pocket SDR utilities and APs source programs
            +-- src     Pocket SDR library source programs
            +-- python  Pocket SDR Python scripts
            +-- lib     External library for utilities and APs
            +-- conf    Configuration files for device settings
            +-- driver  Windows driver for EZ-USB FX2LP/FX3 (cyusb3.sys) ([4])
            +-- doc     Documents (ref [1], [2])
            +-- FW      Firmware source programs and images
            |   +-- cypress  Cypress libraries for EZ-USB firmware development
            |                (ref [4])
            +-- HW      Pocket SDR RF frontend CAD data and parts list
            |           (*.brd and *.sch are for Eagle, *.f3d is for Fusion 360)
            +-- image   Image files for documents
            +-- sample  Sample digital IF data captured by Pocket SDR
            +-- test    Test codes
```

--------------------------------------------------------------------------------

## **Installation for Windows**

* Extract PocketSDR.zip to an appropriate directory <install_dir>.

* Attach Pocket SDR RF frontend to PC via USB cable.

* Install USB driver (CYUSB) for Pocket SDR RF frontend according to
  PocketSDR\driver\readme.txt.

* Add the Pocket SDR binary programs path (<install_dir>\PocketSDR\bin) to 
  the command search path (Path) of Windows environment variables.

* Add the Pocket SDR Python scripts path (<install_dir>\PocketSDR\python) to 
  the command search path (Path) of Windows environment variables.

* To rebuild the binary programs, you need MinGW64. Refer MSYS2
  (https://www.msys2.org/) for details.

* In MinGW64 environment, you need fftw3 library. To install fftw3 library.
```
    $ pacman -S mingw-w64-x86_64-fftw
```

--------------------------------------------------------------------------------

## **Installation for Linux**

* Extract PocketSDR.zip to an appropriate directory <install_dir>.
```
    $ unzip PocketSDR.zip
```
* Install libusb-1.0 developtment package. For Ubuntu:
```
    $ sudo apt install libusb-1.0-0-dev
```
* Install libfftw3 developtment package. For Ubuntu:
```
    $ sudo apt install libfftw3-dev
```
* Move to the library directory ({5],[6],[7]}), install external library source
 trees as follows:
```
    $ cd <install_dir>/lib
    $ git clone https://github.com/quiet/libfec libfec
    $ git clone https://github.com/radfordneal/LDPC-codes LDPC-codes
    $ git clone https://github.com/tomojitakasu/RTKLIB -b rtklib_2.4.3 RTKLIB
```
* Move to the library build directory, build libraries.
```
    $ cd <install_dir>/lib/build
    $ make
    $ make install
```
* Move to the source program directory, build utilities and APs.
```
    $ cd <install_dir>/app
    $ make
    $ make install
```
* Add the Pocket SDR binary programs path (<install_dir>/PocketSDR/bin) to 
  the command search path.

* Usually you need to have root permission to access USB devices. So you add
sudo to execute pocket_conf, pocket_dump like:
```
   $ sudo pocket_conf ../conf/pocket_L1L6_12MHz.conf
   $ sudo pocket_dump -t 10 ch1.bin ch2.bin
```

--------------------------------------------------------------------------------

## **Utility Programs for RF frontend**

Pocket SDR contains the following utility programs.

- **pocket_conf**: SDR device configurator
- **pocket_scan**: Scan and list USB Devices
- **pocket_dump**: Capture and dump digital IF data of SDR device

For details, refer comment lines in src/pocket_conf.c, src/pocket_scan.c, 
src/pocket_dump.c.

--------------------------------------------------------------------------------

## **GNSS-SDR APs (Application Programs)**

Pocket SDR contains the following application programs for GNSS-SDR.

- **pocket_psd.py** : Plot PSD and histograms of digital IF data
- **pocket_acq.py** : GNSS signal acquisition in digital IF data
- **pocket_trk.py** : GNSS signal tracking and navigation data decoding in digital IF data
- **pocket_snap.py**: Snapshot positioning with digital IF data
- **pocket_plot.py**: Plot GNSS signal tracking log by pocket_trk.py
- **pocket_acq**    : C-version of pocket_acq.py (w/o graph plots)
- **pocket_trk**    : C-version of pocket_trk.py (w/o graph plots)
- **pocket_snap**   : C-version of pocket_snap.py

For details, refer comment lines in python/pocket_psd.py, python/pocket_acq.py,
python/pocket_trk.py, python/pocket_snap.py and python/pocket_plot.py. You need
Python 3, Numpy, Scipy and matplotlib to execute Python scripts.

--------------------------------------------------------------------------------

## **Execution Examples of Utility Programs and GNSS-SDR APs**

```
    $ sudo pocket_conf
    ...
    $ sudo pocket_conf conf/pocket_L1L6_12MHz.conf
    Pocket SDR device settings are changed.
    
    $ sudo pocket_dump -t 5 ch1.bin ch2.bin
      TIME(s)    T   CH1(Bytes)   T   CH2(Bytes)   RATE(Ks/s)
          5.0    I     60047360  IQ    120094720      11985.5
    
    $ pocket_psd.py ch1.bin -f 12 -h
    $ pocket_acq.py ch1.bin -f 12 -fi 3 -sig L1CA -prn 1-32,193-199
    SIG= L1CA, PRN=   1, COFF=  0.23492 ms, DOP= -1519 Hz, C/N0= 33.6 dB-Hz
    SIG= L1CA, PRN=   2, COFF=  0.98558 ms, DOP=  2528 Hz, C/N0= 33.8 dB-Hz
    SIG= L1CA, PRN=   3, COFF=  0.96792 ms, DOP=  3901 Hz, C/N0= 33.7 dB-Hz
    SIG= L1CA, PRN=   4, COFF=  0.96192 ms, DOP= -1957 Hz, C/N0= 40.4 dB-Hz
    ...
    $ pocket_acq.py ch1.bin -f 12 -fi 3 -sig L1CA -prn 4

    $ pocket_acq.py ch1.bin -f 12 -fi 3 -sig L1CA -prn 8 -3d

    $ pocket_acq.py ch2.bin -f 12 -sig L6D -prn 194 -p
    
    $ pocket_trk.py ch1.bin -f 12 -fi 3 -sig L1CA -prn 1-32
    TIME(s):      5.00                                                             SRCH:   0  LOCK:  8/ 32
    CH   SIG PRN STATE  LOCK(s) C/N0 (dB-Hz)         COFF(ms) DOP(Hz)    ADR(cyc) SYNC  #NAV #ERR #LOL NER
     4  L1CA   4  LOCK     4.99 41.7 |||||||        0.9680586 -1947.0     -9713.3 -B--     0    0    0   0
     7  L1CA   7  LOCK     4.99 35.1 |||            0.4019474  3676.0     18351.1 -B--     0    0    0   0
     8  L1CA   8  LOCK     4.99 46.5 ||||||||||     0.4476165  2532.0     12638.9 -B--     0    0    0   0
     9  L1CA   9  LOCK     4.99 37.6 |||||          0.9303968  -376.9     -1867.8 -B--     0    0    0   0
    16  L1CA  16  LOCK     4.99 46.6 |||||||||||    0.9330322  -418.9     -2089.4 -B--     0    0    0   0
    18  L1CA  18  LOCK     4.99 42.4 ||||||||       0.7254252 -1763.4     -8791.6 -B--     0    0    0   0
    26  L1CA  26  LOCK     4.99 45.8 ||||||||||     0.7427075 -1445.7     -7211.6 -B--     0    0    0   0
    31  L1CA  31  LOCK     4.99 45.0 ||||||||||     0.7491922 -3013.1    -15036.7 -B--     0    0    0   0
    ...
    $ pocket_trk.py ch1.bin -f 12 -fi 3 -sig E1B -prn 18 -p
    ...
    $ pocket_trk.py ch2.bin -f 12 -sig E6B -prn 4 -log trk.log -p -ts 0.2
    ...
``` 

<img src="image/image001.jpg" width=49%>
<img src="image/image002.jpg" width=49%>
<img src="image/image003.jpg" width=49%>
<img src="image/image004.jpg" width=49%>
<img src="image/image005.jpg" width=49%>
<img src="image/image006.jpg" width=49%>
<img src="image/image007.jpg" width=49%>

--------------------------------------------------------------------------------

## **Real-time Signal Tracking with pocket_dump and pocket_trk**

With pocket_dump and pocket_trk, you can track GNSS signals in real-time.
By using -r option in pocket_dump and pocket_trk, the captured 2 CH IF data can
be handled as raw data format. In addition, multiple signal tracking feature is
added to pocket_trk (C-version of pocket_trk.py) in ver. 0.9.

For example, to track L1C/A and L5I signals of GPS in real-time, the following
commands can be used. In this case, the tracking log is output as a TCP server with
the port number 5070. For detailed options, please refer the comment lines in the
source codes of app/pocket_dump/pocket_dump.c or app/pocket_trk/pocket_trk.c.
For other samples, please refer test/pocket_trk_*_test.sh.

``` 
    $ sudo pocket_dump -r -q - -c conf/pocket_L1L5_24MHz.conf | \
    pocket_trk -r -f 24 -fi 6,0 -sig L1CA -prn 1-32 -sig L5I -prn 1-32 \
    -log :5070
``` 

<img src="image/image008.jpg" width=100%>

--------------------------------------------------------------------------------

## **Rebuild F/W and Write F/W Image to Pocket SDR RF frontend**

* Install Cypress EZ-USB FX2LP Development Kit (ref [4]) to a Windows PC. As
default, it is installed to C:\Cypress and C:\Keil.

* Execute Keil uVision2 (C:\Keil\UV2\uv2.exe).

* Execute Menu Project - Open Project, select <install_dir>\PocketSDR\FW\pocket_fw.Uv2>
and open the project.

* Execute Menu Project - Rebuild all target files and you can get a F/W image
as <install_dir>\PocketSDR\FW\pocket_fw.iic.

* Attach Pocket SDR RF frontend via USB cable to the PC.

* Execute USB Control Center (C:\Cypress\USB\CY3684_EZ-USB_FX2LP_DVK\1.1\Windows Applications\
c_sharp\controlcenter\bin\Release\CyControl.exe).

* Select Cypress FX2LP Sample Device, execute menu Program - FX2 - 64KB EEPROM,
select the F/W image <install_dir>\PocketSDR\FW\pocket_fw.iic and open it.

* If you see "Programming succeeded." in status bar, the F/W is properly written
to PocketSDR.

* To use utility programs for Pocket SDR, you need to reinstall WinUSB driver for
Pocket SDR. Refer "Installation for Windows" above.

--------------------------------------------------------------------------------

## **References**

[1] Maxim integrated, MAX2771 Multiband Universal GNSS Receiver, July 2018

[2] Cypress, EZ-USB FX2LP USB Microcontroller High-Speed USB Peripheral 
  Controller, Rev. AB, December 6, 2018

[3] (deleted)

[4] Cypress, CY3684 EZ-USB FX2LP Development Kit
    (https://www.cypress.com/documentation/development-kitsboards/cy3684-ez-usb-fx2lp-development-kit)

[5] https://github.com/quiet/libfec

[6] https://github.com/radfordneal/LDPC-codes

[7] https://github.com/tomojitakasu/RTKLIB

--------------------------------------------------------------------------------

## **History**

- 2021-10-20  0.1  1st draft version
- 2021-10-25  0.2  Add Rebuild F/W and Write F/W Image to PocketSDR
- 2021-12-01  0.3  Add and modify Python scripts
- 2021-12-25  0.4  Add and modify Python scripts
- 2022-01-05  0.5  Fix several problems.
- 2022-01-13  0.6  Add and modify Python scripts
- 2022-02-15  0.7  Improve performance, Add some Python scripts.
- 2022-07-08  0.8  Add C-version of pocket_acq.py and pocket_trk.py.
- 2024-01-03  0.9  Add C-version of pocket_snap.py.
                   pocket_trk supports multi-signal and multi-theading

