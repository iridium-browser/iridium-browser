---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: tp-link-onhub-tgr1900
title: TP-LINK OnHub TGR1900
---

[TOC]

## Introduction

This page contains information about the [TP-LINK OnHub Router
TGR1900](http://www.tp-link.us/products/details/cat-9_TGR1900.html), an
[OnHub](https://on.google.com/hub/) WiFi router. that is interesting and/or
useful to software developers. For general information about getting started
with developing on Chromium OS (the open-source version of the software on the
Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/tp-link-onhub-tgr1900/tplink_black.png"
height=200
width=116>](/chromium-os/developer-information-for-chrome-os-devices/tp-link-onhub-tgr1900/tplink_black.png)
<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/tp-link-onhub-tgr1900/tplink_blue.png"
height=200 width=116> [<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/tp-link-onhub-tgr1900/tplink_top.jpg"
height=200
width=199>](/chromium-os/developer-information-for-chrome-os-devices/tp-link-onhub-tgr1900/tplink_top.jpg)

### Specifications

*   CPU: Qualcomm Krait IPQ8064 (ARM A9) dual core 1.4 GHz
*   RAM: 1 GiB DDR3 (Not upgradeable)
*   Disk: 4 GiB eMMC (Not upgradeable)
*   USB 3.0
    *   1 USB Standard A port
*   Connectivity
    *   WiFi aux 802.11 a/b/g/n/ac 1x1
    *   WiFi 2.4 & 5 GHz 802.11 b/g/n 3x3 with smart antenna
    *   Ethernet WAN 1 port 10/100/1000 Mbps
    *   Ethernet LAN 1 port 10/100/1000 Mbps
*   Size: 7.5in x 4.1in x 4.6in
*   Weight: 1.9lb

## Firmware

This device uses [coreboot](http://www.coreboot.org/) and
[Depthcharge](https://chromium.googlesource.com/chromiumos/platform/depthcharge/)
as a payload to boot the system. You can find the source in the [Chromium OS
coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-storm-6315.B)
and the [Chromium OS depthcharge git
tree](https://chromium.googlesource.com/chromiumos/platform/depthcharge/+/firmware-storm-6315.B)
in the `firmware-storm-6315.B` branches.
