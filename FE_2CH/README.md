# **Pocket SDR FE 2CH - An Open-Source GNSS SDR RF Frontend**

## **Overview**

Pocket SDR FE 2CH is an open-source GNSS SDR RF frontend device for Pocket SDR.
The device consists of 2 CH Maxim MAX2771 ([1]) GNSS RF front-end ICs (LNA, mixer,
filter, ADC, frequency synthesizer) and a Cypress EZ-USB FX2LP USB 2.0 controller
([2]) to connect to host PCs. The front-end CH1 is dedicated for GNSS L1 band
(1525 - 1610 MHz), and CH2 is for GNSS L2/L5/L6 band (1160 - 1290 MHz). The
frequency of the reference oscillator (TCXO) is 24.000 MHz, and the ADC sampling
frequency can be configured up to 32 MHz.

--------------------------------------------------------------------------------

## **Specifications**

* LO (PLL) Frequency: CH1 1525 ~ 1610 MHz (GNSS L1 band),
                      CH2 1160 ~ 1290 MHz (GNSS L2/L5/L6 band)
* IF Bandwidth: 2 ~ 24 MHz
* Sampling Rate: 4, 6, 8, 10, 12, 16, 20, 24 and 32 Msps
* Sampling Type: I or I/Q sampling, 2 bits resolution
* Host I/F: USB 2.0, micro-B (ver.2.1) or type-C (ver.2.3)
* Power: 5V 140 mA, USB bus power

--------------------------------------------------------------------------------

## **Directory Structure and Contents**
```
FE_2CH --+-- FW     Pocket SDR FE 2CH F/W and build environment
         |   |
         |   +-- v2.1     ver.2.1
         |       +-- pocket_fw.c    Source program of the F/W
         |       +-- pocket_fw.hex  hex format file of the F/W image
         |       +-- pocket_fw.iic  iic format file of the F/W image
         |       +-- ...              
         |       +-- cypress        framework and utilities for Cypress EZ-USB
         |                          FX2LP F/W
         |
         +-- HW     Pocket SDR FE 2CH hardware design data
             |
             +-- v2.1     ver.2.1 (2-layer, USB micro-B)
             |   +-- pocket_sdr_v2.1.brd  PCB design Eagle data
             |   +-- pocket_sdr_v2.1.sch  Circuit schematic Eagle data
             |   +-- pocket_sdr_v2.1_parts.xlsx  BOM (parts list) Excel data
             |   +-- pocket_sdr_case.f3d  Case panel Fusion360 3D-CAD data
             |   +-- ...
             |
             +-- v2.2     ver.2.2
             |   +-- ...
             |
             +-- v2.3     ver.2.3 (4-layer, USB type-C)
                 +-- pocket_sdr_v2.3.kicad.*  KiCAD PCB and circuit design data
                 +-- pocket_sdr_v2.3.kicad_sim  KiCAD simbol data
                 +-- pocket_sdr_v2.3.pretty  KiCAD module data
                 +-- pocket_sdr_*.f3d,2mf  Case panel Fusion360 3D-CAD data
                 +-- pocket_sdr_v2.3_parts.xlsx  BOM (parts list) Excel data
                 +-- ...
```
--------------------------------------------------------------------------------

## **Rebuild F/W and Write F/W Image to Pocket SDR FE 2CH**

* Install Cypress EZ-USB FX2LP Development Kit (ref [3]) to a Windows PC. As
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

--------------------------------------------------------------------------------

## **References**

[1] Maxim integrated, MAX2771 Multiband Universal GNSS Receiver, July 2018

[2] Cypress, EZ-USB FX2LP USB Microcontroller High-Speed USB Peripheral 
  Controller, Rev. AB, December 6, 2018

[3] Cypress, CY3684 EZ-USB FX2LP Development Kit
    (https://www.cypress.com/documentation/development-kitsboards/cy3684-ez-usb-fx2lp-development-kit)

