# **Pocket SDR FE 8CH - An Open-Source GNSS SDR RF Frontend**

## **Overview**

Pocket SDR FE 8H is an open-source GNSS SDR RF frontend device for Pocket SDR.
The device consists of 8 CH Maxim MAX2771 ([1]) GNSS RF front-end ICs (LNA, mixer,
filter, ADC, frequency synthesizer) and a Cypress EZ-USB FX3 USB 3.0 controller
([2], [3], [4]) to connect to host PCs. All the RF CHs are able to be configured for
GNSS L1 band (1525 - 1610 MHz), or GNSS L2/L5/L6 band (1160 - 1290 MHz).
The frequency of the reference oscillator (TCXO) is 24.000 MHz, and the ADC
sampling rate can be configured up to 48 Msps.

--------------------------------------------------------------------------------

## **Specifications**

* Number of RF inputs  : 8 CH or 2CH
* Number of RF channels: 8 CH
* LO (PLL) Frequency: 1525 ~ 1610 MHz (GNSS L1 band) or
                      1160 ~ 1290 MHz (GNSS L2/L5/L6 band)
* IF Bandwidth: 2 ~ 36 MHz
* Sampling Rate: 4, 6, 8, 12, 16, 24, 32 or 48 Msps
* Sampling Type: I or I/Q sampling, 2 bits resolution
* Host I/F: USB 3.0, type-C (high-speed 480 Mbps or super speed 5 Gbps)
* Power: 5V, USB bus power

--------------------------------------------------------------------------------

## **Directory Structure and Contents**
```
FE_8CH  -+-- HW     Pocket SDR FE 8CH hardware design data
         |   |
         |   +-- v4.0     Pocket SDR FE 8CH PCB H/W data
         |   |   +-- v4.0.kicad.*     KiCAD PCB and circuit design data
         |   |   +-- v4.0.pretty      KiCAD module data
         |   |   +-- v4.0_gerber.zip  gerber data ZIP file
         |   |   +-- v4.0_gerber      gerber data
         |   |   +-- v4.0_parts.xlsx  BOM (parts list) Excel data
         |   |   +-- pocket_sdr_v2.3-eagle-import.kicad_sym  KiCAD symbol data
         |   |   +-- v4.0_case.f3d    Pocket SDR FE 8CH case Autodesk Fusion data
         |   |   +-- v4.0_case_panel_f.f3d frontpanel Autodesk Fusion data
         |   |   +-- v4.0_case_panel_b.f3d backpanel Autodesk Fusion data
         |   |   +-- ...
         |   +-- v4.1     Pocket SDR FE 8CH-2ANT PCB H/W data
         |   |   +-- ...
         |   +-- div_4px2 4PX2 RF Divider H/W data
         |       +-- ...
         +-- FW     Pocket SDR FE 8CH firmware
             +-- Packages  Cypress EZ USB Suite files
             +-- v4.0
                 +-- .metadata  Cypress EZ USB Suite files
                 +-- pocket_fw_v4
                     +-- pocket_fw_v4.c  Pocket SDR FE 8CH F/W source
                     +-- pocket_usb_dscr.c USB descriptors source
                     +-- gpif_conf.cyfx  GPIF II Designer config file 
                     +-- gpif_conf.h     GPIF II Designer generated header file
                     +-- ...
```
--------------------------------------------------------------------------------

## **Rebuild F/W and Write F/W Image to Pocket SDR FE 8CH**

Same as FE 4CH.

--------------------------------------------------------------------------------

## **References**

[1] Maxim integrated, MAX2771 Multiband Universal GNSS Receiver, July 2018

[2] Infineon, CYUSB301X, CYUSB201X, EZ-USB FX3 SuperSpeed USB Controller Datasheet,
Rev. Z, September 29, 2022

[3] Cypress Semiconductor, EZ-USB FX3 Technical Reference Manual, Rev.F, May 9, 2019

[4] Cypress Semiconductor, FX3 Programmers Manual, Rev.K

