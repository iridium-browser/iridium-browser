---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: acer-c670-chromebook
title: Acer C670 Chromebook 11
---

[TOC]

## Introduction

This page contains information about the [Acer Chromebook 11
(C670)](https://store.google.com/product/acer_chromebook_11) that is interesting
and/or useful to software developers. For general information about getting
started with developing on Chromium OS (the open-source version of the software
on the Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook/gallery-AcerChromebook11-1-front.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook/gallery-AcerChromebook11-2-front-grey.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook/gallery-AcerChromebook11-4-left-grey.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook/gallery-AcerChromebook11-5-right.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook/gallery-AcerChromebook11-6-right-grey.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook/gallery-AcerChromebook11-7-side-left.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook/gallery-AcerChromebook11-8-side-left-grey.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook/gallery-AcerChromebook11-9-side-right.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c670-chromebook/gallery-AcerChromebook11-10-side-right-grey.jpg"
height=150 width=200>

### Specifications

*   Dimensions: 299 x 202 x 18.65 mm
*   Weight: 1.1 kg (2.43 lbs)
*   CPU: Broadwell Intel Celeron Dual-Core
            [N2830](http://ark.intel.com/products/81071/Intel-Celeron-Processor-N2830-1M-Cache-up-to-2_41-GHz)
*   RAM: 2GiB DDR3 (Not upgradeable)
*   Display: 11.6" 1366x768 16:9
*   Disk: 16GB SSD
*   I/O:
    *   HDMI port
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
*   [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)
*   Linux 3.14

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

Details for working with developer mode can be found [on this
page](/chromium-os/developer-information-for-chrome-os-devices/acer-c720-chromebook).

## Firmware

This device uses [coreboot](http://www.coreboot.org/) to boot the system. You
can find the source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-paine-6301.58.B)
in the `firmware-paine-6301.58.B` branch.

#### Firmware Write Protect

The screw is next to the socket of the wifi card.
