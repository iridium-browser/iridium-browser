---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: chromebook-pixel
title: Chromebook Pixel (2013)
---

[TOC]

## Introduction

This page contains information about the [Chromebook Pixel
(2013)](http://www.google.com/chromebook/pixel) that is interesting and/or
useful to software developers. For general information about getting started
with developing on Chromium OS (the open-source version of the software on the
Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

### Specifications

*   CPU: Intel Core-i5 3427U (dual-core 1.8 GHz)
*   GPU: Intel HD Graphics 4000
    *   2560x1700 screen
    *   Mini DisplayPort
*   RAM: 4 GiB DDR3 (Not upgradeable)
*   Disk: 32 or 64 GiB SSD
    *   SD & USB expansion slots
*   WiFi: 802.11 a/b/g/n
    *   USB slot can handle Ethernet dongle
    *   LTE on some models
*   Bluetooth 3.0
*   No [Kensington Security
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
virtualized.

To invoke Recovery mode, you hold down the ESC and Refresh (F3) keys and press
the Power button for at least 200ms (until the keyboard backlight comes on). If
you don't hold it for long enough, then it won't work.

To enter Dev-mode, you first invoke Recovery, and at the Recovery screen press
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

### Installing Linux to the SSD

See the excellent write up by David Miller here:
<http://vger.kernel.org/~davem/chromebook_pixel_linux.txt>

## Troubleshooting

### Won't boot? Power button does nothing?

Make sure you don't have your Pixel stacked on top of another Pixel (or possibly
other laptop), as the sensor for detecting the screen closed will activate from
the magnet in the device below, preventing booting.

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

### Firmware Event Log

The Pixel firmware saves an event log to read-write flash that can be useful for
troubleshooting your device.

The event log is based on SMBIOS Type 15 Event Log format, but uses a number of
OEM events to provide additional information. The mosys application that is part
of Chromium OS can be used to read and decode the log by running **mosys
eventlog list** as the root user in Chrome OS if the device is in developer mode
or opening **chrome://system** and looking for the **eventlog** entry in
normal/verified mode.

Mosys can also be compiled on other Linux distributions, here are instructions
for Ubuntu that assume basic build and source control tools are installed.
Unfortunately these same instructions do not work on Fedora because it does not
provide static libraries for things like UUID.

```none
# install libuuid headers and static libs
sudo apt-get install uuid-dev
# build flashmap library dependency
git clone https://chromium.googlesource.com/chromiumos/third_party/flashmap/
cd flashmap
make
cd ..
# build mosys and link statically against flashmap
git clone https://chromium.googlesource.com/chromiumos/platform/mosys.git
cd mosys
make defconfig
make EXTRA_CFLAGS="-I ../flashmap/lib -static" FMAP_LINKOPT="-I ../flashmap/lib -L ../flashmap/lib -lfmap"
# run mosys to print event log
sudo ./mosys eventlog list
# example output...
221 | 2013-03-05 08:31:45 | ACPI Wake | S5
222 | 2013-03-05 08:31:47 | Chrome OS Developer Mode
223 | 2013-03-05 09:01:03 | Kernel Event | Clean Shutdown
224 | 2013-03-05 09:01:03 | ACPI Enter | S5
225 | 2013-03-05 09:01:09 | System boot | 362
226 | 2013-03-05 09:01:09 | EC Event | Power Button
227 | 2013-03-05 09:01:09 | ACPI Wake | S5
228 | 2013-03-05 09:01:09 | Wake Source | PCI PME | 0
229 | 2013-03-05 09:01:09 | Wake Source | Internal PME | 0
230 | 2013-03-05 09:01:10 | Chrome OS Developer Mode
```

## Firmware

This device uses [coreboot](http://www.coreboot.org/) and [Das
U-Boot](http://www.denx.de/wiki/U-Boot) to boot the system. You can find the
source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-link-2695.B)
and the [Chromium OS u-boot git
tree](https://chromium.googlesource.com/chromiumos/third_party/u-boot/+/firmware-link-2695.B)
in the `firmware-link-2695.B` branches.

## What's inside?

**WARNING: Opening the case and fiddling with the stuff inside could easily
brick your system and make it unrecoverable. DO NOT ATTEMPT if you are not
familiar with this process. Really, just don't.**

### Disassembly

Taking apart your Chromebook is **not** encouraged. If you have hardware
troubles, please seek assistance first from an authorized center. There's
nothing inside that you can fix yourself. Be advised that disassembly might
**void warranties** or other obligations, so please consult any and all
paperwork you received first. If you just want to see what the inside looks
like, gaze upon this (click for a high res version):

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/chromebook-pixel/link-bottom-guts.jpg"
height=239
width=320>](/chromium-os/developer-information-for-chrome-os-devices/chromebook-pixel/link-bottom-guts.jpg)

Fine. If you **must** risk breaking it for good, at least do it the right way.

*   First, acquire the necessary tools:
    *   1 small flat head screw driver
    *   1 small Phillips head screw driver
    *   1 [suction
                cup](http://www.amazon.com/TEKTON-5652-4-Inch-Suction-Puller/dp/B000NPR3FW/)
        *   No, not one like you use in the shower
        *   You can probably make do with a 2", but a 3" or 4" one would
                    be much better
        *   Multiple suction cups won't really help either (so 2" + 2"
                    != 4")
*   Shut the system down and close the lid
*   Flip the laptop over so the bottom is facing up and the hinge is
            facing away from you
*   Using a flat head screwdriver, pop the four rubber feet off,
            starting from the edge of the case
*   Using a Phillips head screw driver (PH0), remove the 4 screws under
            the rubber feet
*   Stick the suction cup onto the bottom of the case
    *   Center it with respect to the sides (left/right)
    *   Place it roughly 1 centimeter from the front (not the hinge)
                side
    *   The handle should be parallel with the left/right sides
*   Put one hand on the hinge to hold it down (so that you are not
            touching the case itself)
*   Rotate the suction cup as follows:
    *   The handle edge closest to the hinge pulls up
    *   The handle edge closest to the front pushes down
    *   The side snaps should disengage
*   After the side snaps disengage, you should be able to **gently**
            pull the bottom off
    *   Continue applying the rotation force as you do, increasing
                slightly if it does not disconnect
*   If the front edge feels like it isn't coming off, it's probably due
            to the glue
    *   Once the back and side snaps have been disconnected, you can
                gently pivot the panel up to force the glue to disconnect
    *   The glue is the semi-reusable type you might find with credit
                cards in the mail, so it might sort-of restick itself when you
                close it up, but it's never going to be as good as new. That's
                one of the reasons we suggested you not do this.
*   When you put the bottom back, the hinge-side clips should go in
            first. Don't just jam it on and press down. Sheesh.

#### Firmware Write Protect

It's the screw between the USB connector and the battery.

#### Servo Header

Pixel uses a 1x42 [servo header](/chromium-os/servo) (now obsolete).
