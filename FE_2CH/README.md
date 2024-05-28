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