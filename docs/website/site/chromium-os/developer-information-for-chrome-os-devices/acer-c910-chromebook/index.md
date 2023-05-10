---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: acer-c910-chromebook
title: Acer C910 Chromebook 15
---

[TOC]

## Introduction

This page contains information about the [Acer Chromebook 15
(C910)](https://store.google.com/product/acer_chromebook_15) that is interesting
and/or useful to software developers. For general information about getting
started with developing on Chromium OS (the open-source version of the software
on the Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c910-chromebook/chromebook_0001s_0000_Acer-Chromebook-15-01-front.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c910-chromebook/chromebook_0001s_0001_Acer-Chromebook-15-02-left.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c910-chromebook/chromebook_0001s_0002_Acer-Chromebook-15-03-right.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c910-chromebook/chromebook_0001s_0003_Acer-Chromebook-15-04-side-left.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c910-chromebook/chromebook_0001s_0004_Acer-Chromebook-15-05-side-right.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c910-chromebook/chromebook_0001s_0005_Acer-Chromebook-15-06-group-black.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c910-chromebook/chromebook_0001s_0006_Acer-Chromebook-15-07-group-white.jpg"
height=150 width=200>

### Specifications

*   Dimensions: 383 x 256 x 24.2 mm
*   Weight: 2.2 kg (4.85 lbs)
*   CPU: Broadwell Intel Celeron Dual-Core
            [3250U](http://ark.intel.com/products/74744/Intel-Core-i3-3250-Processor-3M-Cache-3_50-GHz)
*   RAM: 2GiB or 4GiB DDR3 (Not upgradeable)
*   Display: 15.6" 1920x1080
*   Disk: 16GB or 32GB SSD
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
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-yuna-6301.59.B)
in the `firmware-yuna-6301.59.B` branch.
