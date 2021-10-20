--------------------------------------------------------------------------------

  PocketSDR - An open-source GNSS SDR Frontend Device, ver. 0.1

--------------------------------------------------------------------------------

1. Overview

PocketSDR is an open-source SDR (software defined radio) based GNSS (Global 
Navigation Satellite System) frontend device. It supports almost all signal
bands for GPS, GLONASS, Galileo, QZSS, BeiDou, NavIC and SBAS. PocketSDR
consists of 2 CH Maxium MAX2771 GNSS RF front-end (LNA, mixer, filter, ADC,
frequency synthesizer) and Cypress EZ-USB FX2LP USB 2.0 interface controller.
The frontend CH1 is dedicated for GNSS L1 band (1525 - 1610 MHz) and CH2 is for
GNSS L2/L5/L6 band (1160 - 1290 MHz). The frequency of the reference oscillator
(TCXO) is 24.000 MHz and ADC sampling frequency can be configured up to 24 MHz.
PocketSDR contains also some utility programs to configure the device, capture
and dump the digitized IF (interfreqency) data. These are supported Windows,
Linux and other environments.

--------------------------------------------------------------------------------

2. Package Structure:

PocketSDR --+-- bin     PocketSDR utility binary programs for Windows
            +-- src     PocketSDR utility source programs
            +-- conf    Configuration files for device settings
            +-- util    Windows driver installation utility
            +-- doc     Documents
            +-- FW      Firmware source programs and images
            |   +-- cypress  Cypress libraries for EZ-USB firmware development
            +-- HW      PocketSDR CAD data and parts list for hardware

--------------------------------------------------------------------------------

3. Instalation for Windows:

(1) Extract PocketSDR.zip to an appropriate directory <install_dir>.

(2) Attach PocketSDR to PC via USB cable.

(3) Install USB driver (WinUSB) for PocketSDR.
  (a) Execute zadig-2.6.exe in <install_dir>\PocketSDR\util.
  (b) Execute menu Options - List All Devices and select "EZ-USB" (USBID 04B4
      1004). 
  (c) Select WinUSB (v6.1.xxxx.xxxxx) and Push "Replace Driver" or  "Reinstall
      Driver".

(4) Add the PocketSDR binary programs path (<install_dir>\PocketSDR\bin) to 
    the command search path (Path) of Windows environment variables.

(5) To rebuild the binary programs, you need MinGW64 and libusb-1.0 library. 
    Refer MSYS2 (https://www.msys2.org/) for details.

--------------------------------------------------------------------------------

4. Instalation for Linux

(1) Extract PocketSDR.zip to an appropriate directory <install_dir>.
$ unzip PocketSDR.zip

(2) Install libusb-1.0 developtment package. For Ubuntu:
$ sudo apt install libusb-1.0-0-dev

(3) Move to the source program directory, edit makefile and build utilities.
$ cd <install_dir>/src
$ vi makefile
...
#LIBUSB = -L/mingw64/lib -llibusb-1.0
LIBUSB = -lusb-1.0
...
$ make
$ make install

(4) Add the PocketSDR binary programs path (<install_dir>/PocketSDR/bin) to 
    the command search path.

--------------------------------------------------------------------------------

5. Utility programs

PocketSDR contains the following utility programs.

(1) pocket_conf: SDR device configurator
(2) pocket_scan: Scan and list USB Devices
(3) pocket_dump: Capture and dump digital IF data of SDR device
(4) pocket_plot.py: Plot PSD and histrgams of digital IF data

For details, refer comment lines in src/pocket_conf.c, src/pocket_scan.c, 
src/pocket_dump.c and src/pocket_plot.py. You need Python 3, Numpy and 
matplotlib to execute pocket_plot.py.

--------------------------------------------------------------------------------

6. References

[1] Maxum integrated, MAX2771 Multiband Universal GNSS Receiver, July 2018

[2] Cypress, EZ-USB FX2LP USB Microcontroller High-Speed USB Peripheral 
    Controller, Rev. AB, December 6, 2018

--------------------------------------------------------------------------------

7. History:

2021-10-20  0.1  1st draft version

