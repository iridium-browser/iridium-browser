---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: acer-c720-chromebook
title: Acer C720 & C720P & C740 Chromebook
---

[TOC]

## Introduction

This page contains information about the [Acer C720
Chromebook](https://www.google.com/intl/en/chrome/devices/acer-c720-chromebook/)
and [Acer C720P
Chromebook](https://www.google.com/intl/en/chrome/devices/acer-c720p-chromebook/)
and [Acer C740
Chromebook](http://us.acer.com/ac/en/US/content/series/chromebook11c740) that is
interesting and/or useful to software developers. For general information about
getting started with developing on Chromium OS (the open-source version of the
software on the Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

### Specifications

*   CPU: Haswell Celeron 2995U. 1.4GHz, dual-core, 2MB Cache
*   RAM: 2GiB or 4GiB DDR3 (Not upgradeable)
*   Display: 11.6" TN 1366x768. 220 nits.
*   Disk: 16GB SSD
            ([NGFF](http://en.wikipedia.org/wiki/Next_Generation_Form_Factor)
            M.2 connector)
*   I/O:
    *   HDMI port
    *   1 x USB 2, 1 x USB 3
    *   [SD slot](http://en.wikipedia.org/wiki/Secure_Digital) (SDXC
                compatible)
    *   Headphone/mic combo jack
    *   Camera & mic
    *   Keyboard & touchpad
    *   Touchscreen (C720P model)
*   Connectivity:
    *   WiFi: 802.11 a/b/g/n
    *   USB ports can handle some Ethernet dongles
    *   [Servo header](/chromium-os/servo): Standard 2x25 / AXK750347G
*   [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)

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

### Legacy Boot

This device includes the SeaBIOS firmware which supports booting images directly
like a legacy BIOS would. Note: the BIOS does not provide a fancy GUI for you,
nor is it easy to use for beginners. You will need to manually boot/install your
alternative system.

Like USB boot, support for this is disabled by default. You need to get into
Dev-mode first and then run:

```none
sudo crossystem dev_boot_legacy=1
```

and reboot once to boot legacy images with Ctrl-L.

### Leaving

To leave Dev-mode and go back to normal mode, just follow the instructions at
the scary boot screen. It will prompt you to confirm.

If you want to leave Dev-mode programmatically, you can run `crossystem
disable_dev_request=1; reboot` from a root shell. There's no way to enter
Dev-mode programmatically, and just seeing the Recovery screen isn't enough -
you have to use the three-finger salute which hard-resets the machine first.
That's to prevent a remote attacker from tricking your machine into dev-mode
without your knowledge.

## Troubleshooting

### Legacy Boot Doesn't Work

Sometimes it's possible to break the SeaBIOS install in the flash (sometimes
doing innocuous things like tweaking the GBB flags). If you do get into such a
situation:

*   Check that dev_boot_legacy is set to 1 when you run crossystem
    *   If it isn't, then see the normal Legacy Boot section above
    *   if it is, then see below

You can safely reset the copy of SeaBIOS in your flash by running (as root):

```none
# chromeos-firmwareupdate --sb_extract /tmp
# flashrom -w /tmp/bios.bin -i RW_LEGACY
```

## Running Chromium OS

Before you start fiddling with your own builds it is strongly recommend to
create a recovery USB stick or SD card. As long as you don't disable hardware
write protect on the system & EC firmware, you can get your machine back into
working order by entering Recovery Mode and plugging in your recovery image. You
can create a recovery image from Chrome OS by browsing to
[chrome://imageburner](javascript:void(0);) or follow instructions for other OS
on the [Chrome OS help
center](https://support.google.com/chromebook/answer/1080595?hl=en) site.

You can build and run Chromium OS on your Acer C720 (versions R32 and later).
Follow the [quick start guide](/chromium-os/quick-start-guide) to setup a build
environment. The board name for the Acer C720 is "peppy". Build an image and
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
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-falco_peppy-4389.B)
in the `firmware-falco_peppy-4389.B` branches.

## Disclaimer

**Caution: Modifications you make to your Chromebook's hardware or software are
not supported by Google, may compromise your online security, and may void your
warranty....now on to the fun stuff.**

## What's Inside?

Taking apart your Chromebook is **not** encouraged. If you have hardware
troubles, please seek assistance first from an authorized center. Be advised
that disassembly might void warranties or other obligations, so please consult
any and all paperwork your received first. If you just want to see what the
inside looks like, gaze upon this ([high-res
version](/chromium-os/developer-information-for-chrome-os-devices/acer-c720-chromebook/c720-chromebook-innards.png)):

<img alt="c720 innards"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c720-chromebook/c720-chromebook-annotated-innards.png">

1.  CPU
2.  RAM
3.  System firmware. 8MB SPI Flash.
4.  NGFF (M.2) SSD
5.  Battery enable switch
6.  Battery enable screw
7.  Write-protect screw
8.  [Servo debug header](/chromium-os/servo) (probably missing on your
            machine)
9.  NGFF (M.2) WWAN connector (probably missing on your machine)

Notes:

*   While it is possible to replace the NGFF (M.2) SSD, compatibility is
            not guaranteed. Reliability and power use may vary. If you install
            Chrome OS using a recovery image, the recovery process will
            partition the drive to use the full capacity.
*   The battery enable switch and battery enable screw are a safety
            mechanism to prevent the Acer C720 from being powered by the battery
            while the cover is removed. Either the swich must be pressed OR a
            screw inserted into #6 to power the device from the battery.
*   The debug header (probably missing) has access to all sorts of
            useful things like a serial UART from the EC and PCH, JTAG for the
            EC, and SPI for the system firmware.
*   The NGFF (M.2) WWAN connector (probably missing on WiFi models) has
            enabled USB + PCIe lines and connections to the SIM card reader.
*   The NGFF (M.2) WWAN connector cannot be connected to a SATA device
            as its SATA pins are not connected to the PCH.
