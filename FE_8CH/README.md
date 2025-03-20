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

* **Number of RF Inputs**: 8 CH or 2 CH
* **Number of RF Channels**: 8 CH
* **LO (PLL) Frequency**: 1525 ~ 1610 MHz (GNSS L1 band) or
                      1160 ~ 1290 MHz (GNSS L2/L5/L6 band)
* **IF Bandwidth**: 2.5 ~ 36 MHz
* **Sampling Rate**: 4, 6, 8, 12, 16, 24, 32 or 48 Msps
* **Sampling Type**: I-sampling 2 or 3 bits,  I/Q-sampling 2 bits
* **Host I/F**: USB 3.0 (USB 3.2 Gen1, super-speed 5Gbps), type-C
* **Power Supply**: 5V 330 mA, USB bus power

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
                     +-- Release
                         +-- pocket_Fw_v4.img Binary image of the F/W
                         +-- ...
```
--------------------------------------------------------------------------------

## **Rebuild F/W and Write F/W Image to Pocket SDR FE 8CH**

* Install Cypress EZ-USB FX3 Development Kit (ref [5]) to a Windows PC. As default,
it is installed to C:\Program Files (x86)\Cypress\EZ-USB FX3 SDK.
* Execute EZ USB Suite (EZ-USB FX3 SDK\1.3\Eclipse\ezUsbSuite.exe).
* Execute Menu File - Switch Workspace - Others, select <install_dir>\PocketSDR\FE_8CH\FW\v4.0
and open the workspace.
* Select pocket_fw_v4 in Project Explorer and execute Menu Project - Build All and
you can get a F/W image <install_dir>\PocketSDR\FE_8CH\FW/v4.0\pocket_fw_v4\Release\pocket_fw_v4.img.
* Disable EEPROM by connecting the J2 jumper pins on the Pocket SDR FE 8CH PCB and
attach the Pocket SDR FE 8CH via USB cable to the PC.
* Execute USB Control Center (EZ-USB FX3 SDK\1.3\bin\CyControl.exe).
* You can see "Cypress FX3 USB BootLoader Device".
* Re-enable EEPROM by disconnecting the J2 jumper pins on the Pocket SDR FE 8CH PCB.
* In USB Control Center, select "Cypress FX3 USB Bootloader Device", execute menu
Program - FX3 - I2C EEPROM, select the F/W image <install_dir>\PocketSDR\FE_8CH\FW\v4.0\pocket_fw_v4\Release\pocket_fw_v4.img
and open it.
* If you see "Programming succeeded." in status bar, the F/W is properly written
to the PocketSDR FE 8CH.

--------------------------------------------------------------------------------

## **References**

[1] Maxim integrated, MAX2771 Multiband Universal GNSS Receiver, July 2018

[2] Infineon, CYUSB301X, CYUSB201X, EZ-USB FX3 SuperSpeed USB Controller Datasheet,
Rev. Z, September 29, 2022

[3] Cypress Semiconductor, EZ-USB FX3 Technical Reference Manual, Rev.F, May 9, 2019

[4] Cypress Semiconductor, FX3 Programmers Manual, Rev.K

[5] Infineon, EZ-USB FX3 Software Development Kit
    (https://www.infineon.com/cms/jp/design-support/tools/sdk/usb-controllers-sdk/ez-usb-fx3-software-development-kit/)
