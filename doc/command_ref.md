<style>
@import url('./pdf_style.css');
</style>


# <p style="text-align:center;">**Pocket SDR: Command Reference**</p>

-------------------------------------------------------------------------------
# **pocket_scan**

## **Synopsis**

    pocket_scan [-e]

## **Description**

Scan and list USB devices.

## **Options**

-e
    Show end point information for USB devices.

## **Examples**


<div class="pagebreak"></div>

------------------------------------------------------------------------------
# **pocket_conf**

## **Synopsis**

    pocket_conf [-s] [-a] [-h] [-p bus[,port]] [conf_file]

## **Description**

Configure or show settings for a Pocket SDR FE device. If conf_file
specified, the settings in the configuration file are set to the Pocket
SDR FE device registers. The configuration is a text file containing
records of MAX2771 register field settings as like follows. The register
field settings are written as keyword = value format or hexadecimal
format. In the case of keyword = value format, a keyword is a field name
shown in MAX2771 manual [1]. Strings after # in a line is treated as
comments. If conf_file omitted, the command shows the settings of the
Pocket SDR FE device in the same format of the configuration file.
    
    Keyword = value format:

    [CHx]
    FCEN     = 97  # ...
    FBW      =  0  # ...
    F3OR5    =  1  # ...
    ...

    Hexadecimal format:

    #CH  ADDR       VALUE
      1  0x00  0xA2241C17
      1  0x01  0x20550288
    ...

## **Options**

- -s

Save the settings to EEPROM of the SDR device. These settings are
also loaded at reset of the Pocket SDR FE device.

- -a

Show all of the register fields.

- -h

Configure or show registers in a hexadecimal format.

- -p [bus[,port]]

USB bus and port number of the Pocket SDR FE device. Without the
option, the command selects the device firstly found.

- conf_file

Path of the configuration file. Without the option, the command shows
current register field settings of the Pocket SDR FE device.

## **References**

[1] maxim integrated, MAX2771 Multiband Universal GNSS Receiver, July 2018


<div class="pagebreak"></div>

------------------------------------------------------------------------------
# **pocket_dump**

## **Synopsis**

    pocket_dump [-t tsec] [-r] [-p bus[,port]] [-c conf_file] [-q]
                [file [file ...]]

## **Description**

Capture and dump digital IF (DIF) data of a Pocket SDR FE device to output
files. To stop capturing, press Ctr-C.

## **Options**

-t tsec

    Data capturing time in seconds.

-r

    Dump raw data of the Pocket SDR FE device without channel separation
    and quantization.

-p bus[,port]

    USB bus and port number of the Pocket SDR FE device. Without the
    option, the command selects the device firstly found.

-c conf_file

    Configure the Pocket SDR FE device with a device configuration file
    before capturing.

-q 

    Suppress showing data dump status.

[file [file ...]]

    Output digital IF data file paths. The first path is for CH1,
    the second one is for CH2 and so on. The second one or the later
    can be omitted. With option -r, only the first path is used. If
    the file path is "", data are not output to anywhere. If the file
    path is "-", data are output to stdout. If all of the file paths
    omitted, the following default file paths are used.
        
    CH1: ch1_YYYYMMDD_hhmmss.bin
    CH2: ch2_YYYYMMDD_hhmmss.bin
    ...
    (YYYYMMDD: dump start date in UTC, hhmmss: dump start time in UTC)


<div class="pagebreak"></div>

-------------------------------------------------------------------------------
# **pocket_acq**

## **Synopsis**

     pocket_acq [-sig sig] [-prn prn[,...]] [-tint tint] [-toff toff]
         [-f freq] [-fi freq] [-d freq] [-nz] file

## **Description**

Search GNSS signals in digital IF data and plot signal search results.
If single PRN number by -prn option, it plots correlation power and
correlation shape of the specified GNSS signal. If multiple PRN numbers
specified by -prn option, it plots C/N0 for each PRN.
 
## **Options ([]: default)**
  
-sig sig

    GNSS signal type ID (L1CA, L2CM, ...). See below for details. [L1CA]
 
-prn prn[,...]

    PRN numbers of the GNSS signal separated by ','. A PRN number can be a
    PRN number range like 1-32 with start and end PRN numbers. For GLONASS
    FDMA signals (G1CA, G2CA), the PRN number is treated as FCN (frequency
    channel number). [1]
 
-tint tint

    Integration time in ms to search GNSS signals. [code cycle]
 
-toff toff

    Time offset from the start of digital IF data in ms. [0.0]
 
-f freq

    Sampling frequency of digital IF data in MHz. [12.0]

-fi freq

    IF frequency of digital IF data in MHz. The IF frequency equals 0, the
    IF data is treated as IQ-sampling (zero-IF). [0.0]

-d freq[,freq]

    Reference and max Doppler frequency to search the signal in Hz.
    [0.0,5000.0]

-nz

    Disalbe zero-padding for circular colleration to search the signal.
    [enabled]

-h

    Show usage and signal type IDs

file

    File path of the input digital IF data. The format should be a series of
    int8_t (signed byte) for real-sampling (I-sampling) or interleaved int8_t
    for complex-sampling (IQ-sampling). PocketSDR and AP pocket_dump can be
    used to capture such digital IF data.
    If the tag file <file>.tag of the input IF data exists, the format,
    the sampling frequency, the LO frequencies and the sampling types
    are automatically recognized by the tag file and the options -fmt,
    -f, -fi, and -IQ are ignored.


<div class="pagebreak"></div>

-------------------------------------------------------------------------------
# **pocket_trk**

## **Synopsis**

    pocket_trk [-sig sig -prn prn[,...] [-rfch ch[,...]] ...]
        [-fmt {INT8|INT8X2|RAW8|RAW16|RAW32}] [-f freq] [-fo freq[,...]]
        [-IQ {1|2}[,...]] [-toff toff] [-ti tint] [-p bus,[,port]
        [-c conf_file] [-log path] [-nmea path] [-rtcm path] [-raw path]
        [-w file] [file]

## **Description**

It searchs and tracks GNSS signals in the input digital IF data, extract
observation data, decode navigation data and generate PVT solutions. The
observation and navigation data can be output as a RTCM3 stream. The PVT
solutions can be output as a NMEA stream. The observation data and
raw navigation data and some event logs can be output as a log stream.

## **Options ([]: default)**

-sig sig -prn prn[,...] [-rfch ch[,...]] ...

    A GNSS signal type ID (L1CA, L2CM, ...) and a PRN number list of the
    signal. For signal type IDs, refer pocket_acq.py manual. The PRN
    number list shall be PRN numbers or PRN number ranges like 1-32 with
    the start and the end numbers. They are separated by ",". For
    GLONASS FDMA signals (G1CA, G2CA), the PRN number is treated as the
    FCN (frequency channel number). To assign the signal to specific RF
    channel(s), -rfch option can be followed. Specify the assigned RF
    channel list with "," or "-". Without the -rfch option, RF channel
    for the signal is automatically assigned. The signal option can be
    repeated for multiple GNSS signals to be tracked.

-fmt {INT8|INT8X2|RAW8|RAW16|RAW32}

    Specify IF data format as follows: INT8 = int8 (I-sampling), INT8X2 =
    interleaved int8 (IQ-sampling), RAW8 = Pocket SDR FE 2CH raw (packed
    8 bits), RAW16 = Pocket SDR FE 4CH raw (packed 16 bits), RAW32 =
    Pocket SDR FE 8CH raw (packed 32 bits) [INT8X2]

-f freq

    Specify the sampling frequency of the IF data in MHz. [12.0]

-fo freq[,...]

    Specify LO frequency for each RF channel in MHz. In case of the
    IF data format as RAW8, RAW16 or RAW32, multiple (2, 4 or 8)
    frequencies have to be specified separated by ",".

-IQ {1|2}[,...]

    Specify the sampling type (1 = I-sampline, 2 = IQ-sampling) for each
    RF channel separated by "," in case of the IF data foramt as RAW8,
    RAW16 or RAW32. [2,2,2,2,2,2,2,2]

-toff toff

    Time offset from the start of the IF data in s. [0.0]

-tscale scale

    Time scale to replay the IF data file. [1.0]

-ti tint

    Update interval of the signal tracking status in seconds. If 0
    specified, the signal tracking status is suppressed. [0.1]

-p bus[,port]

    USB bus and port number of the Pocket SDR FE device in case of IF data
    input from the device.

-c conf_file

    Configure the Pocket SDR FE device with a device configuration file
    before signal acquisition and tracking.

-log path

    A stream path to write the signal tracking log. The log includes
    observation data, navigation data, PVT solutions and some event logs.
    The stream path should be one of the followings.

    (1) local file file path without ':'. The file path can be contain
        time keywords (%Y, %m, %d, %h, %M) as same as the RTKLIB stream.
    (2) TCP server  :port
    (3) TCP client  address:port

-nmea path

    A stream path to write PVT solutions as NMEA GNRMC, GNGGA and GNGSV
    sentences. The stream path is as same as the -log option.
         
-rtcm path

    A stream path to write raw observation and navigation data as RTCM3.3
    messages. The stream path is as same as the -log option.

-raw path

    A stream path to write raw IF data. The stream path is as same as the
    -log option.

-h height

    Specify the console height (rows). [64]

-w file

    Specify the FFTW wisdowm file. [../python/fftw_wisdom.txt]

[file]

    A file path of the input IF data. The Pocket SDR FE deveice and
    pocket_dump can be used to capture such digitized IF data.

    If the tag file <file>.tag for the input IF data exists, the format,
    the sampling frequency, the LO frequencies and the sampling types
    are automatically recognized by the tag file. In this case, the
    options -fmt, -f, -fo, and -IQ are ignored.

    If the file path omitted, the input is taken from a Pocket SDR FE
    device directly. In this case, the format, the sampling frequency,
    the LO frequencies and the sampling types are automatically
    configured according to the device information.


<div class="pagebreak"></div>

-------------------------------------------------------------------------------
# **pocket_snap**

## **Synopsis**
 
    pocket_snap [-ts time] [-pos lat,lon,hgt] [-ti sec] [-toff toff]
         [-f freq] [-fi freq] [-tint tint] [-sys sys[,...]] [-v] [-w file]
         -nav file [-out file] file
 
## **Description**
 
Snapshot positioning with GNSS signals in digitized IF file.
 
## **Options ([]: default)**
  
-ts time

    Captured start time in UTC as YYYY/MM/DD HH:mm:ss format.
    [parsed by file name]

-pos lat,lon,hgt

    Coarse receiver position as latitude, longitude in degree and
    height in m. [no coarse position]
 
-ti sec

    Time interval of positioning in seconds. (0.0: single) [0.0]
 
-toff toff

    Time offset from the start of digital IF data in seconds. [0.0]
 
-f freq

    Sampling frequency of digital IF data in MHz. [12.0]

-fi freq

    IF frequency of digital IF data in MHz. The IF frequency equals 0, the
    IF data is treated as IQ-sampling (zero-IF). [0.0]

-tint tint

    Integration time for signal search in msec. [20.0]

-sys sys[,...]

    Select navigation system(s) (G=GPS,E=Galileo,J=QZSS,C=BDS). [G]

-v

    Enable verpose status display.

-w file

    Specify FFTW wisdowm file. [../python/fftw_wisdom.txt]

-nav file

    RINEX navigation data file.

-out file

    Output solution file as RTKLIB solution format.

file

    Digitized IF data file.

