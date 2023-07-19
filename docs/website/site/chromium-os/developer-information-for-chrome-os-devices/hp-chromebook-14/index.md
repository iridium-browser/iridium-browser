---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: hp-chromebook-14
title: HP Chromebook 14
---

[TOC]

## Introduction

This page contains information about the [HP Chromebook
14](http://www.google.com/intl/en/chrome/devices/hp-14-chromebook.html) that is
interesting and/or useful to software developers. For general information about
getting started with developing on Chromium OS (the open-source version of the
software on the Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-14/large-1.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-14/large-2.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-14/large-3.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-14/large-4.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-14/large-5.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-14/large-6.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-14/large-7.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-chromebook-14/large-8.jpg"
height=150 width=200>

### Specifications

*   CPU: Haswell Celeron 2995U. 1.4GHz, dual-core, 2MB Cache
*   RAM: 2GiB or 4GiB DDR3 (Soldered down...sorry kids)
*   Display: 14" 1366x768. 200 nits.
*   Disk: 16GB SSD
            ([NGFF](http://en.wikipedia.org/wiki/Next_Generation_Form_Factor)
            M.2 connector)
*   I/O:
    *   HDMI port
    *   1 x USB 2.0, 2 x USB 3.0
    *   [SD slot](http://en.wikipedia.org/wiki/Secure_Digital) (SDXC
                compatible)
    *   Headphone/mic combo jack
    *   Camera & mic
    *   Keyboard & touchpad
*   Connectivity:
    *   WiFi: 802.11 a/b/g/n
    *   USB ports can handle some Ethernet dongles
    *   Bluetooth 4.0
    *   [Servo header](/chromium-os/servo): Standard 2x25 / AXK750347G
*   [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

Details for working with developer mode can be found [on this
page](/chromium-os/developer-information-for-chrome-os-devices/acer-c720-chromebook).

## Firmware

This device uses [coreboot](http://www.coreboot.org/) to boot the system. You
can find the source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-falco_peppy-4389.B)
in the `firmware-falco_peppy-4389.B` branches.

## Disassembly

Opening the case might void your warranty.

If you must open the case: Remove 13 screws, 4 of them hidden under small rubber
plugs (not under the large round rubber feet) and then crack open the case at
the seam, starting on the edge close to the touchpad. See also:
<https://youtu.be/IrIRLbZ4npY>
