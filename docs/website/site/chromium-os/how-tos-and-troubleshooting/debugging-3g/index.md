---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: debugging-3g
title: Debugging a cellular modem
---

[TOC]

## Communicating directly with a modem with the AT command set

Most modems support AT commands which control the modem and query for
information. On a Chromium OS device with the developer mode enabled, you can
use the socat command to communicate with the modem. You need a Chromium OS
device with a test image installed (or you can use `gmerge socat` if you are
familiar with the dev server) . For example:

*   Type `CTRL+ALT+T` to enter the crosh shell.
*   Type `shell` to enter the bash shell.
*   Figure you which USB device to use with: `ls /dev/ttyACM*
            /dev/ttyUSB*`
*   Stop modem manager if it is running: `sudo stop modemmanager`
*   Type `sudo socat - /dev/ttyACM1,crnl` or `sudo socat - /dev/ttyUSB0
            `to communicate with the modem.
*   Ensure that the modem is enabled with the command: "`AT+CFUN=1`"

For example:

```none
crosh> shell
chronos@localhost ~ $ sudo stop modemmanager
chronos@localhost ~ $ ls /dev/ttyACM* /dev/ttyUSB*
/dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyUSB2
chronos@localhost ~ $ socat - /dev/ttyUSB1
ATZ
OK
AT+CFUN=1
OK
ATI
Manufacturer: Qualcomm Incorporated
Model: ......
Revision:....
ESN: 0x80d...
+GCAP: +CIS707A, ....
chronos@localhost ~ $
```

Depending on the modem, you can try different AT commands, e.g. "AT+CGMI",
"AT+CGSN", etc.

## A notable exception: Gobi Modems

While modems based on the Qualcomm Gobi chipset support AT commands, Chromium OS
uses the Qualcomm connection management API to control these modems.

## Using minicom

One can also use minicom to communicate with the modem. This program is
available in test images (or you can use `gmerge socat` if you are familiar with
the dev server) .

*   Type `CTRL-ALT-T` to enter the crosh shell.
*   Type `shell` to enter the bash shell
*   Figure you which usb device to use with: `ls /dev/ttyACM*
            /dev/ttyUSB*`
*   Stop modem manager if it is running: `sudo stop modem-manager`
*   Type `sudo minicom -s`
*   Ignore, "WARNING: configuration file not found"
*   Use the down arrow key and enter to choose the menu **Serial port
            setup**
*   Type `A` to edit the **Serial Device**, changing it to
            `/dev/ttyACM1` or `/dev/ttyUSB1`
*   Press enter to return to the top level configuration menu
*   Use the arrow keys and enter to select **Exit** which will exit
            configuration and let you communicate with the modem.
*   Use `CTRL-A x` to exit and` CTRL-A z` for help

## Collecting Additional Logging

Run the following crosh command to enable debug logging in modem manager at
runtime (without restarting modem manager).

```none
crosh> modem set-logging debug
```

On a Chromium OS device with the developer modem enabled, you can restart modem
manager with debug logging enabled.

```none
crosh> shell
chronos@localhost ~ $ sudo restart modemmanager MM_LOGLEVEL=DEBUG
```

## Testing Download Speeds

Chromium images include the sftp client, and also wget and curl for downloading.
If you have a test build, it contains iperf too. You will need to enable
developer mode on the platform before you are able to access the shell.

*   Flip the dev switch to enable developer mode.
*   Reboot the system
*   Wait for system to wipe stateful partition.
*   When booted, press CTRL+ALT+F2 (right arrow) to enter VT2 shell.
*   Use username chronos or root. No password necessary.
*   You can now use
            [sftp](http://www.openbsd.org/cgi-bin/man.cgi?query=sftp&sektion=1),
            [curl](http://curl.haxx.se/docs/manpage.html),
            [wget](http://www.gnu.org/software/wget/manual/wget.html) or
            [iperf](http://iperf.sourceforge.net/) for testing performance.
