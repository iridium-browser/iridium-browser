---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: lenovo-chromebook-n20
title: Lenovo Chromebook N20 & N20P
---

[TOC]

## Introduction

This page contains information about the [Lenovo Chromebook N20 &
N20P](https://www.google.com/chromebook/) that is interesting and/or useful to
software developers. For general information about getting started with
developing on Chromium OS (the open-source version of the software on the Chrome
Notebook), see the [Chromium OS Developer Guide](/chromium-os/developer-guide).

<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-chromebook-n20/gallery-LenovoChromebookN20p-1-hinge.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-chromebook-n20/gallery-LenovoChromebookN20p-2-front.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-chromebook-n20/gallery-LenovoChromebookN20p-3-left.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-chromebook-n20/gallery-LenovoChromebookN20p-4-right.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-chromebook-n20/gallery-LenovoChromebookN20p-5-hinge.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-chromebook-n20/gallery-LenovoChromebookN20p-6-side-left.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-chromebook-n20/gallery-LenovoChromebookN20p-7-side-right.jpg"
height=150 width=200>

### Specifications

*   Weight: 2.87 lbs
*   CPU: Baytrail Intel Celeron Dual-Core
            [N2830](http://ark.intel.com/products/81071/Intel-Celeron-Processor-N2830-1M-Cache-up-to-2_41-GHz)
*   RAM: 2GiB DDR3 (Not upgradeable)
*   Display: 11.6" 1366x768
*   Disk: 16GB SSD
*   I/O:
    *   HDMI miniport
    *   1 x USB 2.0
    *   1 x USB 3.0
    *   [SD slot](http://en.wikipedia.org/wiki/Secure_Digital) (SDXC
                compatible)
    *   Headphone/mic combo jack
    *   Camera & mic
    *   Keyboard & touchpad
*   Connectivity:
    *   WiFi: 802.11 a/b/g/n/ac
    *   Bluetooth 4.0
    *   USB ports can handle some Ethernet dongles
*   no [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)
*   Linux 3.10

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

Details for working with developer mode can be found [on this
page](/chromium-os/developer-information-for-chrome-os-devices/acer-c720-chromebook).

## Firmware

This device uses [coreboot](http://www.coreboot.org/) to boot the system. You
can find the source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-clapper-5216.199.B)
in the `firmware-clapper-5216.199.B ` branch.

## What's inside?

**WARNING: Opening the case and fiddling with the stuff inside could easily
brick your system and make it unrecoverable. DO NOT ATTEMPT if you are not
familiar with this process. Really, just don't.**

### Disassembly

Taking apart your Chromebook is **not** encouraged. If you have hardware
troubles, please seek assistance first from an authorized center. There's
nothing inside that you can fix yourself. Be advised that disassembly might
**void warranties** or other obligations, so please consult any and all
paperwork you received first.

Fine. If you **must** risk breaking it for good, at least do it the right way.

*   Note: Lenovo provides
            [documentation](http://download.lenovo.com/consumer/mobiles_pub/lenovo_n20p_chromebook_hmm.pdf)
            for disassembling the device. Please follow that first. These
            directions are a simple fallback.
*   First, acquire the necessary tools:
    *   A small Phillips head screw driver
    *   A [spudger](http://en.wikipedia.org/wiki/Spudger)
*   Close the laptop and flip it over so the bottom is facing up
    *   Remove the 8 screws (two are under the rubber feet near the
                hinge)
*   Flip the laptop over and open it so the keyboard is facing up
    *   Power it off if it powers on
    *   Using the spudger, pry the laptop off starting from the top edge
    *   It uses sticky glue in the middle area, so it might be a little
                difficult
    *   Try to pull it down uniformly to minimize chances of breaking
    *   Detach the keyboard & touchpad cables
*   Remove the 9 screws that were under the keyboard
    *   Use the spudger to remove the upper half of the case and expose
                the motherboard
