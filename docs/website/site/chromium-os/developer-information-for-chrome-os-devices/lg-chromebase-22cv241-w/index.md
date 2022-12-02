---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: lg-chromebase-22cv241-w
title: LG Chromebase 22CV241-W
---

[TOC]

## Introduction

This page contains information about the [Lenovo Chromebook N20 &
N20P](https://www.google.com/chromebook/) that is interesting and/or useful to
software developers. For general information about getting started with
developing on Chromium OS (the open-source version of the software on the Chrome
Notebook), see the [Chromium OS Developer Guide](/chromium-os/developer-guide).

<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-1-F-101_White_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-2-F-101_Black_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-3-B-113_White_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-4-B-113_Black_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-5-B-102F_White_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-6-B-102F_Black_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-7-S-103_White_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-8-S-103_Black_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-9-S-102_White_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-10-S-102_Black_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-11-S-101_White_Monitor_from-Sandbox.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lg-chromebase-22cv241-w/gallery-LG-12-S-101_Black_Monitor_from-Sandbox.jpg"
height=150 width=200>

### Specifications

*   Weight: 2.87 lbs
*   CPU: Haswell Intel Celeron Dual-Core
            [2955U](http://ark.intel.com/products/75608/Intel-Celeron-Processor-2955U-2M-Cache-1_40-GHz)
*   RAM: 2GiB DDR3 (1 upgradable slot)
*   Display: 21.5" 1920x1080 16:9
*   Disk: 16GB SSD
*   I/O:
    *   HDMI
    *   3 x USB 2.0
    *   1 x USB 3.0
    *   Headphone/mic combo jack
    *   Camera & mic
*   Connectivity:
    *   WiFi: 802.11 a/b/g/n/ac
    *   Bluetooth 4.0
    *   100/1000 Ethernet
*   [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)
*   Linux 3.8

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

Details for working with developer mode can be found [on this
page](/chromium-os/developer-information-for-chrome-os-devices/acer-c720-chromebook).

## Firmware

This device uses [coreboot](http://www.coreboot.org/) to boot the system. You
can find the source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-monroe-4921.B)
in the `firmware-monroe-4921.B` branch.

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

*   First, acquire the necessary tools:
    *   A small Phillips head screw driver
    *   A large Philips head screw driver
    *   A [spudger](http://en.wikipedia.org/wiki/Spudger)
*   Place the computer screen down on a flat surface
*   Pop the plastic cap with the cable holder off the back by pulling
            from the top
    *   Remove the three exposed screws in the middle
    *   Pull the stand off
*   Remove the one visible screw near the USB ports on the back
    *   Pry the central plastic piece of carefully
    *   Try flexing it and pulling from the bottom part
    *   The spudger might help
*   Remove the three visible screws and detach the cables
*   Starting at the bottom, of the screen, there should be a plastic
            hole
    *   Start prying apart the case from there using the spudger
    *   Work all the way around the top
    *   Pull the back off
*   Remove the visible screws from the metal bracket to expose the
            motherboard
