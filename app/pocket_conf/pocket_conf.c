// 
//  Pocket SDR C AP - Pocket SDR FE Device Configurator.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2021-10-20  0.1  new
//  2022-01-04  1.0  support C++.
//  2024-06-29  1.1  support API changes of sdr_conf.c
//
#include "pocket_dev.h"

// constants and macro ---------------------------------------------------------
#define PROG_NAME       "pocket_conf" // program name

// show usage ------------------------------------------------------------------
static void show_usage(void)
{
    printf("Usage: %s [-s] [-a] [-h] [conf_file]\n", PROG_NAME);
    exit(0);
}

//------------------------------------------------------------------------------
//  Synopsis
//
//    pocket_conf [-s] [-a] [-h] [-p bus[,port]] [conf_file]
//
//  Description
//
//    Configure or show settings for a Pocket SDR FE device. If conf_file
//    specified, the settings in the configuration file are set to the Pocket
//    SDR FE device registers. The configuration is a text file containing
//    records of MAX2771 register field settings as like follows. The register
//    field settings are written as keyword = value format or hexadecimal
//    format. In the case of keyword = value format, a keyword is a field name
//    shown in MAX2771 manual [1]. Strings after # in a line is treated as
//    comments. If conf_file omitted, the command shows the settings of the
//    Pocket SDR FE device in the same format of the configuration file.
//    
//    Keyword = value format:
//
//    [CHx]
//    FCEN     = 97  # ...
//    FBW      =  0  # ...
//    F3OR5    =  1  # ...
//    ...
//
//    Hexadecimal format:
//
//    #CH  ADDR       VALUE
//      1  0x00  0xA2241C17
//      1  0x01  0x20550288
//    ...
//
//  Options
//
//    -s
//        Save the settings to EEPROM of the SDR device. These settings are
//        also loaded at reset of the Pocket SDR FE device.
//
//    -a
//        Show all of the register fields.
//
//    -h
//        Configure or show registers in a hexadecimal format.
//
//    -p [bus[,port]]
//        USB bus and port number of the Pocket SDR FE device. Without the
//        option, the command selects the device firstly found.
//
//    conf_file
//        Path of the configuration file. Without the option, the command shows
//        current register field settings of the Pocket SDR FE device.
//
//  References
//
//    [1] maxim integrated, MAX2771 Multiband Universal GNSS Receiver, July 2018
//
int main(int argc, char **argv)
{
    sdr_dev_t *dev;
    const char *file = "";
    int i, bus = -1, port = -1, opt1 = 0, opt2 = 0;
    
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s")) {
            opt1 |= 1;
        }
        else if (!strcmp(argv[i], "-a")) {
            opt2 |= 1;
        }
        else if (!strcmp(argv[i], "-h")) {
            opt1 |= 4;
            opt2 |= 4;
        }
        else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            sscanf(argv[++i], "%d,%d", &bus, &port);
        }
        else if (!strncmp(argv[i], "-", 1)) {
            show_usage();
        }
        else {
            file = argv[i];
        }
    }
    if (!(dev = sdr_dev_open(bus, port))) {
        return -1;
    }
    if (*file) {
        if (!sdr_conf_write(dev, file, opt1)) {
            sdr_dev_close(dev);
            return -1;
        }
        printf("%s device settings are changed%s.\n", SDR_DEV_NAME,
            (opt1 & 1) ? " and saved to EEPROM" : "");
    }
    else {
        if (!sdr_conf_read(dev, "", opt2)) {
            sdr_dev_close(dev);
            return -1;
        }
    }
    sdr_dev_close(dev);
    return 0;
}
