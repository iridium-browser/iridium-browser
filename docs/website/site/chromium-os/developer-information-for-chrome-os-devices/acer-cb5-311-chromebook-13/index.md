---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: acer-cb5-311-chromebook-13
title: Acer CB5-311 Chromebook 13
---

[TOC]

## Introduction

This page contains information about the [Acer Chromebook 13
(CB5-311)](https://store.google.com/product/acer_chromebook_13) that is
interesting and/or useful to software developers. For general information about
getting started with developing on Chromium OS (the open-source version of the
software on the Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-cb5-311-chromebook-13/gallery-Acer-Chromebook13-1-front.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-cb5-311-chromebook-13/gallery-Acer-Chromebook13-2-left.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-cb5-311-chromebook-13/gallery-Acer-Chromebook13-3-right.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-cb5-311-chromebook-13/gallery-Acer-Chromebook13-4-side-left.jpg"
height=150 width=200> <img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-cb5-311-chromebook-13/gallery-Acer-Chromebook13-5-side-right.jpg"
height=150 width=200>

### Specifications

*   Dimensions: 328 x 229 x 18.0 mm
*   Weight: 1.5kg (3.31 lbs)
*   CPU: NVIDIA [Tegra Logan
            K1](http://www.nvidia.com/object/tegra-k1-processor.html) quad-core
            Cortex-A15
*   RAM: 2GiB DDR3 (Not upgradeable)
*   Display: 13.3" 1920x1080 16:9 TN 200 nits
*   Disk: 16GB SSD
*   I/O:
    *   HDMI port
    *   2 x USB 3.0
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
virtualized. Our partners don't really like physical switches - they cost money,
take up space on the motherboard, and require holes in the case.

To invoke Recovery mode, you hold down the ESC and Refresh (F3) keys and poke
the Power button.

To enter Dev-mode you first invoke Recovery, and at the Recovery screen press
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

Note: Only CrOS formatted images will boot via USB. Other Linux distros will not
work.

### Legacy Boot

Sorry, but this device does not support a legacy BIOS mode. It has an ARM cpu,
so there is no such mode anyways.

### Leaving

To leave Dev-mode and go back to normal mode, just follow the instructions at
the scary boot screen. It will prompt you to confirm.

If you want to leave Dev-mode programmatically, you can run `crossystem
disable_dev_request=1; reboot` from a root shell. There's no way to enter
Dev-mode programmatically, and just seeing the Recovery screen isn't enough -
you have to use the three-finger salute which hard-resets the machine first.
That's to prevent a remote attacker from tricking your machine into dev-mode
without your knowledge.

## Firmware

This device uses [coreboot](http://www.coreboot.org/) to boot the system. You
can find the source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-nyan-5771.B)
in the `firmware-nyan-5771.B` branch.
