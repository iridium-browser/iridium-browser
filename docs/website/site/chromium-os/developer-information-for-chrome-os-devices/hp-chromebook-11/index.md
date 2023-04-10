---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: hp-chromebook-11
title: HP Chromebook 11
---

[TOC]

## Introduction

This page contains information about the [HP Chromebook
11](http://www.google.com/intl/en/chrome/devices/hp-chromebook-11/) that is
interesting and/or useful to software developers. For general information about
getting started with developing on Chromium OS (the open-source version of the
software on the Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

Note: there have been multiple devices released under the moniker "HP Chromebook
11". This page covers the first ARM boards.

### Specifications

*   CPU: [Samsung Exynos 5 Dual
            Core](http://www.samsung.com/global/business/semiconductor/product/application/detail?productId=7668)
            (Cortex A15; 1.7GHz cpu)
*   GPU: [ARM
            Mali-T604](http://www.arm.com/products/multimedia/mali-graphics-hardware/mali-t604.php)
            (Quad Core)
    *   1366x768 [IPS screen](http://en.wikipedia.org/wiki/IPS_panel)
                with 300 nits (spring board)
    *   1366x768 TN panel (skate board)
*   RAM: 2 GiB DDR3
    *   The memory is not upgradable as it is soldered directly to the
                board
*   Disk: 16 GiB SSD (connected to eMMC)
    *   USB expansion ports
    *   No SD slot
*   WiFi: 802.11 a/b/g/n
    *   USB ports can handle Ethernet dongle
*   Power supply: 5.25V <img alt="image"
            src="/chromium-os/developer-information-for-chrome-os-devices/direct-current.svg"
            height=5> ([DC](http://en.wikipedia.org/wiki/Direct_current)) 3.A
            [micro
            USB](https://play.google.com/store/devices/details/Charger_for_HP_Chromebook_11)
*   No [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)
*   [Servo header](/chromium-os/servo): Standard 2x25 / AXK750347G

The first release was the "spring" board. A later revision was named "skate"
with a slightly different BOM.

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

Details for working with developer mode can be found [on this
page](/chromium-os/developer-information-for-chrome-os-devices/samsung-arm-chromebook).

## Firmware

This device uses [Das U-Boot](http://www.denx.de/wiki/U-Boot) to boot the
system. You can find the source in the [ChromiumOS u-boot git
tree](https://chromium.googlesource.com/chromiumos/third_party/u-boot/+/firmware-spring-3824.B)
in the `firmware-spring-3824.B` branch.

There is also firmware in a custom embedded controller (which handles things
like the keyboard), but we won't cover that here as you generally should not
need to modify that. You can find the source in the [ChromiumOS ec git
tree](https://chromium.googlesource.com/chromiumos/platform/ec/+/firmware-spring-3824.B)
(in the spring firmware branch).

## Disassembly

**Skate**

Opening the case might void your warranty. It is also not recommended, since
opening the case lifts the heat sink off the CPU and upon closing the case the
thermal resistance might not fully recover. Especially, it is not recommended to
run the device with the case opened.

But if you must open the case: Remove 11 screws (two of them hidden under the
round rubber feet close to the hinge) and then crack open the case at the seam,
starting on the edge close to the touchpad.
