// 
//  Pocket SDR - Scan and List USB Devices.
//
//  Author:
//  T.TAKASU
//
//  History:
//  2021-10-11  0.1  new
//  2022-01-04  1.0  change version
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

// constants and macros ----------------------------------------------------------------
#define PROG_NAME       "pocket_scan" // program name

// show usage --------------------------------------------------------------------------
static void show_usage(void)
{
    printf("Usage: %s [-e]\n", PROG_NAME);
    exit(0);
}

// get USB device string ---------------------------------------------------------------
static void get_usb_string(struct libusb_device *dev, char *name, size_t size)
{
    libusb_device_handle *h;
    char *p = name, buff[64];
    
    name[0] ='\0';
    
    if (libusb_open(dev, &h)) return;
    
    for (int i = 1; i < 5; i++) {
        if (libusb_get_string_descriptor_ascii(h, i, (uint8_t *)buff,
                sizeof(buff)) < 0) {
            break;
        }
        if (p + strlen(buff) + 2 < name + size) {
            p += sprintf(p, " %s", buff);
        }
    }
    libusb_close(h);
}

// scan USB devices --------------------------------------------------------------------
static int scan_usb(int ep)
{
    static const char *speed[] =
        {"UNKNOWN", "LOW", "FULL", "HIGH", "SUPER", "SUPER_PLUS"};
    libusb_device **devs;
    ssize_t n;
    
    libusb_init(NULL);
    
    if ((n = libusb_get_device_list(NULL, &devs)) <= 0) {
        fprintf(stderr, "USB device list get error.\n");
        return 0;
    }
    for (int i = 0; i < n; i++) {
        struct libusb_device_descriptor desc;
        struct libusb_config_descriptor *cfg;
        const struct libusb_interface_descriptor *ifdesc;
        char str[64];
        
        if (libusb_get_device_descriptor(devs[i], &desc) < 0) continue;
        
        get_usb_string(devs[i], str, sizeof(str));
        
        printf("(%2d) BUS=%2d PORT=%2d SPEED=%-5s ID=%04X:%04X %s\n", i,
             libusb_get_bus_number(devs[i]), libusb_get_port_number(devs[i]),
             speed[libusb_get_device_speed(devs[i])], desc.idVendor,
             desc.idProduct, str);
        
        if (!ep) continue;
        
        libusb_get_config_descriptor(devs[i], 0, &cfg);
        
        for (int j = 0; j < (int)cfg->bNumInterfaces; j++) {
            for (int k = 0; k < cfg->interface[j].num_altsetting; k++) {
                ifdesc = &cfg->interface[j].altsetting[k];
                
                for (int m = 0; m < ifdesc->bNumEndpoints; m++) {
                    printf("%5sIF=%2d ALT=%2d EP=%2d DIR=%s MAXSIZE=%4d\n", "",
                        j, k, ifdesc->endpoint[m].bEndpointAddress & 0x0F,
                        (ifdesc->endpoint[m].bEndpointAddress & 0x80) ? "IN " : "OUT",
                        ifdesc->endpoint[m].wMaxPacketSize);
                }
            }
        }
        libusb_free_config_descriptor(cfg);
    }
    libusb_free_device_list(devs, 0);
    return 1;
}

//------------------------------------------------------------------------------
//  Synopsis
//
//    pocket_scan [-e]
//
//  Description
//
//    Scan and list USB devices.
//
//  Options
//
//    -e
//        Show end point information for USB devices.
//
int main(int argc, char **argv)
{
    int i, ep = 0;
    
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-e")) {
            ep = 1;
        }
        else if (!strncmp(argv[i], "-", 1)) {
            show_usage();
        }
    }
    return scan_usb(ep) ? 0 : -1;
}

