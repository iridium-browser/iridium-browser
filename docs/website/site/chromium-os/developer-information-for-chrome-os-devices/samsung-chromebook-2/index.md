---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: samsung-chromebook-2
title: Samsung Chromebook 2
---

[TOC]

## Introduction

This page contains information about the [Samsung Chromebook
2](https://www.google.com/intl/en/chrome/devices/chromebooks.html#ss2) that is
interesting and/or useful to software developers. For general information about
getting started with developing on Chromium OS (the open-source version of the
software on the Chrome Notebook), see the [Chromium OS Developer
Guide](http://www.chromium.org/chromium-os/developer-guide).

## Specifications

### Peach Pit (XE503C12)

*   CPU: [Samsung Exynos 5 Octa
            5420](http://www.samsung.com/global/business/semiconductor/product/application/detail?productId=7977&iaId=2341)
            (1.9GHz Quad A15 / 1.3GHz Quad A7)
*   Display: 11.6" 1366x768

### Peach Pi (XE503C32)

*   CPU: [Samsung Exynos 5 Octa
            5800](http://www.samsung.com/global/business/semiconductor/product/application/detail?productId=7977&iaId=2341)
            (2.0GHz Quad A15 / 1.3GHz Quad A7)
*   Display: 13.3" 1920x1080

### Common

*   RAM: 4GB DDR3 (Not upgradeable)
*   Disk: 16 GiB SSD
*   I/O:
    *   HDMI Port
    *   1 x USB 2.0 (still uses XHCI)
    *   1 x USB 3.0
    *   [Micro SD slot](http://en.wikipedia.org/wiki/Secure_Digital)
                (SDXC compatible, has hardware but not software for UHS support)
    *   Headphone/mic combo jack
    *   Camera & mic
    *   Keyboard & touchpad
*   Connectivity:
    *   WiFi: 802.11ac (802.11a/b/g/n compatible)
    *   Bluetooth 4.0
    *   USB ports can handle some Ethernet dongles
    *   [Servo header](/chromium-os/servo): Standard 2x25 / AXK750347G

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

This device uses [Das U-Boot](http://www.denx.de/wiki/U-Boot) to boot the
system. You can find the source in the [Chromium OS u-boot git
tree](https://chromium.googlesource.com/chromiumos/third_party/u-boot/+/firmware-pit-4482.B)
in the `firmware-pit-4482.B` branch.

There is also firmware in a custom embedded controller (which handles things
like the keyboard), but we won't cover that here as you generally should not
need to modify that. You can find the source in the [Chromium OS ec git
tree](https://chromium.googlesource.com/chromiumos/platform/ec/+/firmware-pit-4482.B)
(in the pit firmware branch).

### Precompiled Releases

If you want some precompiled versions to chain load of nv U-Boot, these should
work:

*   [peach pit
            (4482.95.0)](https://storage.cloud.google.com/chromeos-image-archive/peach_pit-firmware/R30-4482.95.0/firmware_from_source.tar.bz2)
*   [peach pi
            (4482.95.0)](https://storage.cloud.google.com/chromeos-image-archive/peach_pi-firmware/R30-4482.95.0/firmware_from_source.tar.bz2)

## What's inside?

**WARNING: Opening the case and fiddling with the stuff inside could easily
brick your system and make it unrecoverable. DO NOT ATTEMPT if you are not
familiar with this process.**

**[Stop.](http://www.youtube.com/watch?feature=fvwp&NR=1&v=Fj3WBfRZ5Nc&t=0m31s)
[Don't.](http://www.youtube.com/watch?feature=fvwp&NR=1&v=Fj3WBfRZ5Nc&t=0m31s)
[Come
back.](http://www.youtube.com/watch?feature=fvwp&NR=1&v=Fj3WBfRZ5Nc&t=0m31s)**

Really. Opening the case will allow you to modify the read-only firmware that
makes recovery possible. If someone from "teh internets" says "You need to
reflash your BIOS", they're almost certainly wrong.

### Disassembly

Taking apart your laptop is **not** encouraged. If you have hardware troubles,
please seek assistance first from an authorized center. Be advised that
disassembly might void warranties or other obligations, so please consult any
and all paperwork your received first. If you just want to see what the inside
looks like, gaze upon this (click for a high res version):
