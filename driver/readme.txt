How to install the driver for Pocket SDR FE.

1. Windows

(1) Get "EZ-USB FX3 Software Development Kit (SDK)" for Windows [1]. You need
    registration in the Infineon developer site to download the file.
(2) Execute the SDK installer (ezusbfx3sdk_1.3.5_Windows_x32-x64.exe). The SDK
    is installed to "C:\Program Files (x86)\Cypress\EZ-USB FX3 SDK\1.3" as
    default.
(3) If the error ".NET Framework 3.5 or later required" shown in the SDK
    installation, download .NET Framework 3.5 from Microsoft site [2] and
    install it.
(4) Attach a Pocket SDR FE 2CH, 4CH or 8CH to the PC, open Device Manager, and look
    for the device "EZ-USB" (FE 2CH) or "FX3" (FE 4CH, 8CH).
(5) Select the device, right-click and execute "Update Driver", select "Browse
    my computer for drivers", and input the directory "driver" in SDK with
    "Include subfolders" checked.
(6) If the driver properly installed, the device could be recognized as
    "Cypress FX2LP Sample Device" (FE 2CH) or "Cypress FX3 USB
    StreamerExample Device" (FE 4CH, 8CH).   

2. Linux, Raspberry Pi OS or macOS

(1) No need to install the Cypress driver for EZ-USB FX2LP or FX3.
(2) Pocket SDR utilizes instead a cross-platform USB host driver libusb-1.0 [3]
    as a shared library for these OS.
(3) Refer "Installation for Linux or Raspberry Pi OS" or "Installation for
    macOS" [4]. The installation procedure also includes how to install the
    libusb-1.0 package.

REFERENCES

[1] https://www.infineon.com/cms/en/design-support/tools/sdk/usb-controllers-sdk/ez-usb-fx3-software-development-kit/
[2] https://dotnet.microsoft.com/en-us/download/dotnet-framework/thank-you/net35-sp1-web-installer
[3] https://libusb.info/
[4] https://github.com/tomojitakasu/PocketSDR
