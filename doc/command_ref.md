<style>
@import url('./pdf_style.css');
</style>

# Pocket SDR Command Reference

<div style="text-align: right;">
<strong>ver.0.15b  2026-05-09</strong>
</div>

---

## Table of Contents

- [pocket_scan](#sec-pocket_scan)
- [pocket_conf](#sec-pocket_conf)
- [pocket_dump](#sec-pocket_dump)
- [pocket_acq](#sec-pocket_acq)
- [pocket_trk](#sec-pocket_trk)
- [pocket_snap](#sec-pocket_snap)
- [pocket_calib](#sec-pocket_calib)
- [convbin](#sec-convbin)
- [str2str](#sec-str2str)


<div class="pagebreak"></div>
<a id="sec-pocket_scan"></a>

## pocket_scan

---
<br>

### Synopsis

```
pocket_scan [-e]
```

### Description

Scan and list USB devices visible to the host. On Windows the Cypress CyAPI library is used; on Linux/macOS libusb-1.0 is used. The output lists USB address/bus/port, link speed, VID/PID and a device-name string.

### Options

- `-e`
  - Show endpoint information (interface, alt-setting, endpoint, direction, max packet size) for each device.


<div class="pagebreak"></div>
<a id="sec-pocket_conf"></a>

## pocket_conf

---
<br>

### Synopsis

```
pocket_conf [-s] [-a] [-h] [-p bus[,port]] [-v] [conf_file]
```

### Description

Configure or show the MAX2771 register settings of a Pocket SDR FE device. If `conf_file` is given, the settings in the configuration file are written to the device registers. The configuration file is a text file containing MAX2771 register field settings in either keyword=value form or hexadecimal form. Comments after `#` are ignored. If `conf_file` is omitted, the command prints the current device settings in the same format.

Keyword=value form:

```
[CHx]
FCEN     = 97  # ...
FBW      =  0  # ...
F3OR5    =  1  # ...
...
```

Hexadecimal form:

```
#CH  ADDR       VALUE
  1  0x00  0xA2241C17
  1  0x01  0x20550288
...
```

### Options

- `-s`
  - Save the settings to the device EEPROM. Saved settings are also loaded automatically at device reset.
- `-a`
  - Show all register fields (including reserved bits).
- `-h`
  - Read or write registers in hexadecimal form.
- `-p bus[,port]`
  - USB bus and port number of the target Pocket SDR FE device. Without this option, the first device found is selected.
- `-v`
  - Print version and exit.
- `conf_file`
  - Path of the configuration file. Without this argument, the current register field settings of the device are printed.

### References

[1] Maxim Integrated, MAX2771 Multiband Universal GNSS Receiver, July 2018.


<div class="pagebreak"></div>
<a id="sec-pocket_dump"></a>

## pocket_dump

---
<br>

### Synopsis

```
pocket_dump [-t tsec] [-r] [-p bus[,port]] [-c conf_file] [-q] [-v]
            [path [path ...]]
```

### Description

Capture and dump digital IF (DIF) data from a Pocket SDR FE device to one output file per RF channel. Capturing stops on the configured duration (`-t`), or by Ctrl-C. A `<path>.tag` companion file is written for each output, recording the IF format, sampling rate, LO frequencies, and per-CH sampling type / bit width so downstream tools (e.g. `pocket_trk`, `pocket_acq`) can auto-configure.

### Options

- `-t tsec`
  - Capture duration in seconds. `0` or omitted: capture until Ctrl-C.
- `-r`
  - Dump the raw bit-packed device output without per-channel separation or quantization. With `-r`, only the first output path is used.
- `-p bus[,port]`
  - USB bus and port number of the target device. Without this option, the first device found is selected.
- `-c conf_file`
  - Configure the device with a `pocket_conf`-format file before capture starts.
- `-q`
  - Suppress the runtime status display.
- `-v`
  - Print version and exit.
- `[path [path ...]]`
  - Output file paths. The k-th path is the file for CH(k); trailing paths can be omitted (those CHs are not written). With `-r`, only the first path is used.
  - `""` discards the data for that CH; `-` writes to stdout (binary mode on Windows).
  - If all paths are omitted, the default paths are:
    - `chN_YYYYMMDD_hhmmss.bin` for each CH N (UTC start date/time).


<div class="pagebreak"></div>
<a id="sec-pocket_acq"></a>

## pocket_acq

---
<br>

### Synopsis

```
pocket_acq [-sig sig] [-prn prn[,...]] [-tint tint] [-toff toff]
           [-fmt fmt] [-f freq] [-fi freq] [-d freq[,freq]] [-nz]
           [-w file] [-v] file
```

### Description

Search GNSS signals in a digital IF data file and report acquisition results (Doppler, code offset, C/N0). For each PRN, parallel-code FFT search with non-coherent integration is performed.

If a tag file `<file>.tag` exists alongside the input, the IF format, sampling frequency, LO frequencies, and per-CH sampling type are taken from the tag and the corresponding command-line options are ignored.

### Options ([]: default)

- `-sig sig`
  - GNSS signal type ID (`L1CA`, `L2CM`, `L5I`, ...). For supported IDs see `doc/signal_IDs.pdf`. [`L1CA`]
- `-prn prn[,...]`
  - PRN numbers, comma-separated. A range like `1-32` is accepted. For GLONASS FDMA signals (`G1CA`, `G2CA`) this is the FCN. [`1`]
- `-tint tint`
  - Coherent integration time in ms. [code cycle]
- `-toff toff`
  - Time offset from the start of the IF data in ms. [`0.0`]
- `-fmt fmt`
  - IF data format: `INT8`, `INT8X2`, `RAW8`, `RAW16`, `RAW16I`, or `RAW32`. [`INT8X2`]
- `-f freq`
  - Sampling frequency of the IF data in MHz. [`12.0`]
- `-fi freq`
  - IF frequency in MHz. `0` means IQ (zero-IF) sampling. [`0.0`]
- `-d ref_dop[,max_dop]`
  - Reference and maximum Doppler frequency to search in Hz. [`0.0,5000.0`]
- `-nz`
  - Disable zero-padding for circular correlation. [enabled]
- `-w file`
  - FFTW wisdom file. [`../python/fftw_wisdom.txt`]
- `-v`
  - Print version and exit.
- `-h`
  - Show usage and exit.
- `file`
  - Input digital IF data file. The format is a series of `int8_t` (I-sampling) or interleaved `int8_t` (IQ-sampling), unless overridden by the tag file.


<div class="pagebreak"></div>
<a id="sec-pocket_trk"></a>

## pocket_trk

---
<br>

### Synopsis

```
pocket_trk [-sig sig -prn prn[,...] [-rfch ch[,...]] ...]
           [-fmt {INT8|INT8X2|RAW8|RAW16|RAW16I|RAW32|CS8|CS16}]
           [-f freq] [-fo freq[,...]] [-IQ {1|2}[,...]] [-bits {2|3}[,...]]
           [-toff toff] [-tscale scale] [-ti tint]
           [-p bus[,port]] [-c conf_file]
           [-driver name] [-gain gain] [-bw bw]
           [-log path] [-nmea path] [-rtcm path] [-raw path]
           [-h height] [-opt file] [-debug file] [-v] [file]
```

### Description

Search and track GNSS signals in input digital IF data, extract observation data, decode navigation data, and generate PVT solutions. Observation and navigation data can be streamed as RTCM3, PVT solutions as NMEA, and raw IF data and event logs can be streamed independently.

The input can be a local file, a TCP stream, a Pocket SDR FE device, or a SoapySDR-supported device (LimeSDR, HackRF, RTL-SDR, ...). If the file path is omitted and `-driver` is not specified, the input is taken from a Pocket SDR FE device directly. For file inputs, if `<file>.tag` exists the IF format, sampling rate, LO frequencies, and per-CH sampling types are auto-configured from the tag, and the corresponding `-fmt`, `-f`, `-fo`, and `-IQ` options are ignored.

### Options ([]: default)

- `-sig sig -prn prn[,...] [-rfch ch[,...]] ...`
  - GNSS signal type ID (`L1CA`, `L2CM`, ...) and PRN list. PRNs are comma-separated and may use ranges (e.g. `1-32`). For GLONASS FDMA signals (`G1CA`, `G2CA`), the PRN is treated as the FCN.
  - `-rfch` assigns the signal to specific RF channel(s) (comma- or `-`-separated). Without `-rfch`, the RF channel is auto-selected. The whole `-sig`/`-prn`/`-rfch` triple may be repeated to track multiple signal types.
- `-fmt {INT8|INT8X2|RAW8|RAW16|RAW16I|RAW32|CS8|CS16}`
  - Input IF data format. `INT8` = int8 (I), `INT8X2` = interleaved int8 (IQ), `RAW8/16/32` = Pocket SDR FE 2/4/8CH packed raw, `RAW16I` = Spider SDR FE packed raw (8CH), `CS8`/`CS16` = SoapySDR complex int8/int16. [`INT8X2`]
- `-f freq`
  - Sampling frequency of the IF data in MHz. [`12.0`]
- `-fo freq[,...]`
  - LO frequency for each RF channel in MHz, comma-separated. With `RAW8/16/32`, 2/4/8 frequencies are required.
- `-IQ {1|2}[,...]`
  - Per-RF-CH sampling type: `1` = I, `2` = IQ. Used with `RAW8/16/32`. [`2,2,2,2,2,2,2,2`]
- `-bits {2|3}[,...]`
  - Per-RF-CH sample bit width (2 or 3). Used with `RAW8/16/32` when `-IQ` is `1` (I-sampling). [`2,2,2,2,2,2,2,2`]
- `-toff toff`
  - Time offset from the start of the IF data in s. [`0.0`]
- `-tscale scale`
  - Replay time scale for file inputs. [`1.0`]
- `-ti tint`
  - Update interval of the runtime tracking status in s. `0` suppresses the status display. [`0.1`]
- `-p bus[,port]`
  - USB bus and port number of the Pocket SDR FE device, when input is from the device.
- `-c conf_file`
  - Configure the Pocket SDR FE device with a `pocket_conf`-format file before tracking starts.
- `-driver name`
  - SoapySDR driver name (`lime`, `hackrf`, `rtlsdr`, `sdrplay`, `bladerf`, ...). Selects a SoapySDR device as the input.
- `-gain gain`
  - SoapySDR overall gain in dB.
- `-bw bw`
  - SoapySDR analog filter bandwidth in MHz.
- `-log path`
  - Output stream path for the tracking log (observation, navigation, PVT, events). The stream path is one of:
    - `(1)` local file path without `:`. Time keywords (`%Y`, `%m`, `%d`, `%h`, `%M`) are expanded as in RTKLIB streams.
    - `(2)` `:port` (TCP server)
    - `(3)` `address:port` (TCP client)
- `-nmea path`
  - Output stream path for PVT solutions as NMEA `GNRMC`, `GNGGA`, and `GNGSV` sentences. Same path syntax as `-log`.
- `-rtcm path`
  - Output stream path for raw observations and navigation data as RTCM3.3 messages. Same path syntax as `-log`.
- `-raw path`
  - Output stream path for raw IF data. Same path syntax as `-log`.
- `-h height`
  - Console height (rows) for the runtime status display. [`64`]
- `-opt file`
  - System options file. See `app/pocket_trk/pocket_trk_default.conf` for the available keys (loop bandwidths, integration times, C/N0 thresholds, array calibration / beam-forming options, etc.). [none]
- `-debug file`
  - Enable RTKLIB trace output to the given file (trace level 3).
- `-v`
  - Print version and exit.
- `[file]`
  - Input IF data file path. If omitted and no `-driver` is given, the input is taken from a Pocket SDR FE device. With `<file>.tag` present, format/sampling parameters are auto-configured.


<div class="pagebreak"></div>
<a id="sec-pocket_snap"></a>

## pocket_snap

---
<br>

### Synopsis

```
pocket_snap [-ts time] [-pos lat,lon,hgt] [-ti sec] [-toff toff]
            [-f freq] [-fi freq] [-tint tint] [-sys sys[,...]]
            [-verb] [-w file] [-v] -nav file [-out file] file
```

### Description

Snapshot positioning from a digitized IF data file. The command searches GNSS signals over a short integration window, resolves ms ambiguities in code offsets using a coarse position (or a Doppler-only solution if no coarse position is given), and estimates the receiver position by least-squares from the resolved code offsets.

### Options ([]: default)

- `-ts time`
  - Capture start time in UTC, as `YYYY/MM/DD HH:MM:SS`. [parsed from the file name]
- `-pos lat,lon,hgt`
  - Coarse receiver position (latitude/longitude in deg, height in m). [no coarse position]
- `-ti sec`
  - Time interval between snapshots in s. `0.0` = single snapshot. [`0.0`]
- `-toff toff`
  - Time offset from the start of the IF data in s. [`0.0`]
- `-f freq`
  - Sampling frequency of the IF data in MHz. [`12.0`]
- `-fi freq`
  - IF frequency in MHz. `0` = IQ (zero-IF). [`0.0`]
- `-tint tint`
  - Integration time for signal search in ms. [`20.0`]
- `-sys sys[,...]`
  - Navigation system selection. Each character is one of `G` (GPS), `E` (Galileo), `J` (QZSS), `C` (BeiDou). [`G`]
- `-verb`
  - Enable verbose progress output to stdout.
- `-w file`
  - FFTW wisdom file. [`../python/fftw_wisdom.txt`]
- `-nav file`
  - RINEX navigation data file (required).
- `-out file`
  - Output solution file in RTKLIB solution format. [stdout]
- `-v`
  - Print version and exit.
- `file`
  - Input digital IF data file.


<div class="pagebreak"></div>
<a id="sec-pocket_calib"></a>

## pocket_calib

---
<br>

### Synopsis

```
pocket_calib [-ts time] [-te time] [-f freq] [-v]
             -g geom_file -n nav_file -o out_file
             obs1 obs2 [obs3 ...]
```

### Description

Estimate antenna-array hardware delays (per-element bias relative to CH1) and array attitude (roll / pitch / yaw) by per-epoch EKF using per-element RINEX OBS files and a RINEX NAV file. The receiver position is first estimated from CH1 obs by single-point positioning. The array geometry is read from a body-frame text file. Calibration results are written to a text file consumable by `pocket_trk` / `pocket_sdr.py` for live beam-forming.

### Options ([]: default)

- `-ts time`, `-te time`
  - Start / end time (GPST, `YYYY/MM/DD HH:MM:SS`) to use from the OBS files. [all epochs]
- `-f freq`
  - Frequency index used for calibration (0 = L1, 1 = L2, ...). [`0`]
- `-g geom_file`
  - Array geometry file. One `x y z` (m) per line in body frame (X = right, Y = forward, Z = up). The k-th line is the position of CH(k); the first line is CH1 (reference, must be `0 0 0`).
- `-n nav_file`
  - RINEX navigation data file.
- `-o out_file`
  - Output text file. Stores the estimated `roll/pitch/yaw` and per-CH bias.
- `-v`
  - Print version and exit.
- `obs1 obs2 [obs3 ...]`
  - Per-element RINEX OBS files. The k-th file is the obs file for CH(k). At least 2 elements are required (typical: 4 or 8).


<div class="pagebreak"></div>
<a id="sec-convbin"></a>

## convbin

---
<br>

### Synopsis

```
convbin [option ...] file
```

### Description

Convert receiver-binary log files, RTCM streams, and RINEX files to RINEX OBS / NAV / SBAS files. Pocket SDR's `convbin` is the RTKLIB `convbin` with extensions for RTCM3 array-mode conversion, MSM signal-ID extensions, and a default RINEX version of 3.05.

The full option list is long; only the most-used options are summarized below. For the complete option list and per-format message lists, run `convbin -h`.

### Common Options

- `-r format`
  - Input format. One of `rtcm2`, `rtcm3`, `nov`, `oem3`, `ubx`, `ss2`, `hemis`, `stq`, `javad`, `nvs`, `binex`, `rt17`, `sbf`, `rinex`. Auto-detected from the file extension if omitted.
- `-v ver`
  - Output RINEX version (e.g. `3.05`, `3.04`, `2.11`). [`3.05`]
- `-ts y/m/d h:m:s`, `-te y/m/d h:m:s`, `-ti tint`
  - Start time, end time, observation interval (s).
- `-f n`
  - Number of frequencies to output. [`3`]
- `-y sys`
  - System selection (`G`/`R`/`E`/`J`/`S`/`C`/`I`).
- `-mask code,...`
  - Signal/code masks (`G:1C+5Q`, ...).
- `-o file`, `-n file`, `-g file`, `-h file`, `-q file`, `-l file`, `-b file`, `-i file`, `-s file`
  - Output file paths for OBS, GPS NAV, GLO NAV, SBAS NAV, QZS NAV, GAL NAV, BDS NAV, NavIC NAV, and SBAS message files.
- `-d dir`
  - Output directory.
- `-trace level`
  - Enable RTKLIB trace at the given level.
- `-array`
  - (Pocket SDR extension) RTCM3 array mode. Splits MSM by station ID into per-CH RINEX OBS files. Requires `-r rtcm3`.
- `-xd`, `-xs`
  - (Pocket SDR extensions) Disable Doppler / SNR fields in the output OBS.
- `-h`
  - Print full help (extensive) and exit.
- `-ver`
  - Print version and exit.
- `file`
  - Input file path. Wildcards are *not* expanded on Windows.


<div class="pagebreak"></div>
<a id="sec-str2str"></a>

## str2str

---
<br>

### Synopsis

```
str2str [-in stream[#format]] [-out stream[#format] ...] [options]
```

### Description

Console version of the RTKLIB stream server. Reads from one input stream and forwards to multiple output streams. Optionally converts between formats (e.g. raw → RTCM3) using `-msg`. If `-in` or `-out` is omitted, stdin or stdout is used. To stop, send Ctrl-C (or `SIGINT` for a background process).

The full option list is long; only the most-used options are summarized below. For the complete list, run `str2str -h`.

### Stream Path Forms

- `serial://port[:brate[:bsize[:parity[:stopb[:fctr]]]]][#port]`
- `tcpsvr://:port`
- `tcpcli://addr[:port]`
- `ntrip://[user[:passwd]@]addr[:port][/mntpnt]`
- `ntrips://[:passwd@]addr[:port]/mntpnt[:str]` (output only)
- `ntripc://[user:passwd@][:port]/mntpnt[:str]` (output only, caster)
- `[file://]path[::T][::+start][::xseppd][::S=swap]`

### Format Tags

`#rtcm2`, `#rtcm3`, `#nov`, `#oem3`, `#ubx`, `#ss2`, `#hemis`, `#stq`, `#javad`, `#nvs`, `#binex`, `#rt17`, `#sbf`.

### Common Options

- `-in stream[#format]`
  - Input stream path with optional format tag.
- `-out stream[#format]`
  - Output stream path with optional format tag. May be repeated up to 4 times.
- `-msg "type[(tint)][,type[(tint)]...]"`
  - RTCM message types and per-type output intervals (s).
- `-sta id`
  - Station ID.
- `-opt opt`
  - Receiver-dependent options.
- `-s msec`, `-r msec`, `-n msec`, `-f sec`, `-b str_no`
  - Timeout (ms), reconnect interval (ms), NMEA request cycle (ms), file swap margin (s), back-relay stream number.
- `-c file`, `-c1 file` ... `-c4 file`
  - Input / per-output command file.
- `-p lat lon hgt`, `-px x y z`, `-a antinfo`, `-i rcvinfo`, `-o e n u`
  - Station position (geodetic / ECEF), antenna info, receiver info, antenna offset (ENU).
- `-l dir`, `-x proxy`
  - FTP/HTTP local directory; HTTP/Ntrip proxy.
- `-t level`, `-fl file`
  - Trace level; trace log file path.
- `-h`
  - Print full help and exit.
- `-v`, `-ver`
  - Print version and exit.
