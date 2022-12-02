---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: dell-chromebook-11
title: Dell Chromebook 11
---

[TOC]

## Introduction

This page contains information about the [Dell Chromebook
11](http://www.google.com/intl/en/chrome/education/devices/chromebooks.html#d11)
that is interesting and/or useful to software developers. For general
information about getting started with developing on Chromium OS (the
open-source version of the software on the Chrome Notebook), see the [Chromium
OS Developer Guide](http://www.chromium.org/chromium-os/developer-guide).

### Specifications

*   CPU: Haswell Celeron 2995U. 1.4GHz, dual-core, 2MB Cache
*   RAM: 2GB or 4GB DDR3 (Not upgradeable)
*   Display: 11.6" 1366x768
*   Disk: 16GB SSD (Not upgradeable)
*   I/O:
    *   HDMI port
    *   2 x USB 3
    *   [SD slot](http://en.wikipedia.org/wiki/Secure_Digital) (SDXC
                compatible)
    *   Headphone/mic combo jack
    *   Camera & mic
    *   Keyboard & touchpad
*   Connectivity:
    *   WiFi: 802.11 a/b/g/n
    *   USB ports can handle some Ethernet dongles
    *   [Servo header](/chromium-os/servo): Standard 2x25 / AXK750347G
*   [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

Details for working with developer mode can be found [on this
page](/chromium-os/developer-information-for-chrome-os-devices/acer-c720-chromebook).

## Running Chromium OS

Before you start fiddling with your own builds it is strongly recommend to
create a recovery USB stick or SD card. As long as you don't disable hardware
write protect on the system & EC firmware, you can get your machine back into
working order by entering Recovery Mode and plugging in your recovery image. You
can create a recovery image from Chrome OS by browsing to chrome://imageburner
or follow instructions for other OS on the [Chrome OS help
center](https://support.google.com/chromebook/answer/1080595?hl=en) site.

You can build and run Chromium OS on your Dell Chromebook 11 (versions R31 and
later). Follow the [quick start
guide](http://www.chromium.org/chromium-os/quick-start-guide) to setup a build
environment. The board name for Dell Chromebook 11 is "wolf". Build an image and
write it to a USB stick or SD card.

To boot your image you will first need to enable booting developer signed images
from USB (or SD card). Switch your machine to Developer mode and get to a shell
by either via VT2 (Ctrl+Alt+F2) and logging in as root or by logging in as a
user (or guest mode), starting a "crosh" shell with Ctrl+Alt+t, and typing
"shell". Now run "sudo crossystem dev_boot_usb=1" and reboot "sudo reboot".

Plug your USB stick or SD card in and on the scary "OS Verification is OFF"
screen hit Ctrl+u to boot from external media. If all goes well you should see a
"Chromium OS" logo screen. If you want to install your build to the SSD, open a
shell and type "sudo /usr/sbin/chromeos-install". Note: This will replace
EVERYTHING on your SSD. Use a recovery image if you want to get back to a stock
Chrome OS build.

Have fun!

## Firmware

This device uses [coreboot](http://www.coreboot.org/) to boot the system. You
can find the source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-wolf-4389.24.B)
in the `firmware-wolf-4389.24.B` branches.

## Disclaimer

**Caution: Modifications you make to your Chromebook's hardware or software are
not supported by Google, may compromise your online security, and may void your
warranty....now on to the fun stuff.**

## What's Inside?

Taking apart your Chromebook is **not** encouraged. If you have hardware
troubles, please seek assistance first from an authorized center. Be advised
that disassembly might void warranties or other obligations, so please consult
any and all paperwork your received first.

The location of the firmware write protection disable screw is indicated by the
red arrow below. Remove the screw, boot the system and run "flashrom
--wp-disable" to enable writing to the write protected regions of flash.

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/dell-chromebook-11/Selection_459.png"
height=231
width=320>](/chromium-os/developer-information-for-chrome-os-devices/dell-chromebook-11/Selection_459.png)
