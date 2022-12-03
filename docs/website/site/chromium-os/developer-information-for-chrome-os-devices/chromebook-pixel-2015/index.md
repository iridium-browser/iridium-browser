---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: chromebook-pixel-2015
title: Chromebook Pixel (2015)
---

[TOC]

## Introduction

This page contains information about the [Chromebook Pixel
(2015)](http://www.google.com/chromebook/pixel) that is interesting and/or
useful to software developers. For general information about getting started
with developing on Chromium OS (the open-source version of the software on the
Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

### Specifications

*   CPU: Intel Core-i5 (dual-core 2.2 GHz) or Core-i7 (dual-core 2.4
            GHz)
*   GPU: Intel HD Graphics 5500
    *   2560x1700 screen
*   RAM: 8 or 16 GiB DDR3 (Not upgradeable)
*   Disk: 32 or 64 GiB SSD
    *   SD & USB expansion slots
*   USB 3.0
    *   2 USB Type-C (for charging, display, etc...) ports
    *   2 USB Standard A ports
*   WiFi: 802.11 a/b/g/n/ac
    *   USB slot can handle Ethernet dongle
*   Bluetooth 4.0
*   No [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

An unrelated note: Holding just Refresh and poking the Power button hard-resets
the machine without entering Recovery. That's occasionally useful, but use it
with care - it doesn't sync the disk or shut down politely, so there's a nonzero
chance of trashing the contents of your stateful partition.

### Introduction

Enabling Developer mode is the first step to tinkering with your Chromebook.
With Developer mode enabled you can do things like poke around on a command
shell (as root if you want), install Chromium OS, or try other OS's. Note that
Developer mode turns off some security features like verified boot and disabling
the shell access. If you want to browse in a safer, more secure way, leave
Developer mode turned OFF. Note: Switching between Developer and Normal
(non-developer) modes will remove user accounts and their associated information
from your Chromebook.

### Entering

On this device, both the recovery button and the dev-switch have been
virtualized.

To invoke Recovery mode, you hold down the ESC and Refresh (F3) keys and press
the Power button.

To enter Dev-mode, you first invoke Recovery, and at the Recovery screen press
Ctrl-D (there's no prompt - you have to know to do it). It will ask you to
confirm, then reboot into dev-mode.

Dev-mode works the same as always: It will show the scary boot screen and you
need to press Ctrl-D or wait 30 seconds to continue booting.

### USB Boot

By default, USB booting is disabled. Once you are in Dev-mode and have a root
shell, you can run:

```none
sudo crossystem dev_boot_usb=1
```

and reboot once to boot from USB drives with Ctrl-U.

### Leaving

To leave Dev-mode and go back to normal mode, just follow the instructions at
the scary boot screen. It will prompt you to confirm.

## Troubleshooting

### Won't boot? Power button does nothing?

Make sure you don't have your Pixel stacked on top of another Pixel (or possibly
other laptop), as the sensor for detecting the screen closed will activate from
the magnet in the device below, preventing booting.

### Firmware Event Log

The Pixel firmware saves an event log to read-write flash that can be useful for
troubleshooting your device.

The event log is based on SMBIOS Type 15 Event Log format, but uses a number of
OEM events to provide additional information. The mosys application that is part
of Chromium OS can be used to read and decode the log by running **mosys
eventlog list** as the root user in Chrome OS if the device is in developer mode
or opening **chrome://system** and looking for the **eventlog** entry in
normal/verified mode.

## Firmware

This device uses [coreboot](http://www.coreboot.org/) and
[Depthcharge](https://chromium.googlesource.com/chromiumos/platform/depthcharge/)
as a payload to boot the system. See here for a [detailed description of the
Samus FMAP / firmware flash layout](/chromium-os/firmware-porting-guide/fmap).
You can find the source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-samus-6300.B)
and the [Chromium OS depthcharge git
tree](https://chromium.googlesource.com/chromiumos/platform/depthcharge/+/firmware-samus-6300.B)
in the `firmware-samus-6300.B` branches.

## What's inside?

**WARNING: Opening the case and fiddling with the stuff inside could easily
brick your system and make it unrecoverable. DO NOT ATTEMPT if you are not
familiar with this process. Really, just don't.**

#### Firmware Write Protect

It's the screw (plus a golden washer) between the USB-A connector and the left
speaker.
