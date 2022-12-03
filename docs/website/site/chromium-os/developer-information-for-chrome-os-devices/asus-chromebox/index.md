---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: asus-chromebox
title: Asus Chromebox
---

[TOC]

## Introduction

This page contains information about the Asus Chromebox that is interesting
and/or useful to software developers. For general information about getting
started with developing on Chromium OS (the open-source version of the software
on the Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

### Specifications

*   CPU: Haswell Celeron 2995U. 1.4GHz, dual-core, 2MB Cache OR 1.7 GHz
            Core i3-4010U, quad-core
*   RAM: 2GB (celeron) or 4GB (i3)
*   Display: None
*   Disk: 16GB SSD
            ([NGFF](http://en.wikipedia.org/wiki/Next_Generation_Form_Factor)
            M.2 connector)
*   I/O:
    *   HDMI port
    *   DisplayPort++
    *   4 x USB 3.0
    *   [SD slot](http://en.wikipedia.org/wiki/Secure_Digital) (SDXC
                compatible)
    *   Headphone/mic combo jack
*   Connectivity:
    *   WiFi: 802.11 a/b/g/n
    *   USB ports can handle some Ethernet dongles
    *   Ethernet
    *   [Servo header](/chromium-os/servo): Standard 2x25 / AXK750347G
*   [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

**Introduction**

Enabling Developer mode is the first step to tinkering with your Chromebox. With
Developer mode enabled you can do things like poke around on a command shell (as
root if you want), install Chromium OS, or try other OS's. Note that Developer
mode turns off some security features like verified boot and disabling the shell
access. If you want to browse in a safer, more secure way, leave Developer mode
turned OFF. Note: Switching between Developer and Normal (non-developer) modes
will remove user accounts and their associated information from your Chromebox.

### Entering

To invoke Recovery mode, you insert a paper clip and press the RECOVERY BUTTON
(just above the kensington lock) and press the Power button. Release the
RECOVERY BUTTON after a second.

To enter Dev-mode you first invoke Recovery, and at the Recovery screen press
Ctrl-D (there's no prompt - you have to know to do it). It will ask you to
confirm by pressing the RECOVERY BUTTON again.

Dev-mode works the same as always: It will show the scary boot screen and you
need to press Ctrl-D or wait 30 seconds to continue booting.

### USB Boot

By default, USB booting is disabled. Once you are in Dev-mode and have a root
shell (Ctrl-Alt-F2), you can run:

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
can create a recovery image from Chrome OS by browsing to chrome://imageburner
or follow instructions for other OS on the [Chrome OS help
center](https://support.google.com/chromebook/answer/1080595?hl=en) site.

You can build and run Chromium OS on your Asus Chromebox (versions R32 and
later). Follow the [quick start guide](/chromium-os/quick-start-guide) to setup
a build environment. The board name for the Asus Chromebox is "panther". Build
an image and write it to a USB stick or SD card.

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
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-panther-4920.24.B)
in the `firmware-panther-4920.24.B` branches.
