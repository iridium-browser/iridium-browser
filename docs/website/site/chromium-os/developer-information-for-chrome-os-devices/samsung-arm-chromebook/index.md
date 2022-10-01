---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: samsung-arm-chromebook
title: Samsung ARM Chromebook
---

[TOC]

## Introduction

This page contains information about the [ARM Samsung Series 3
Chromebook](http://www.google.com/intl/en/chrome/devices/samsung-chromebook.html)
that is interesting and/or useful to software developers. For general
information about getting started with developing on Chromium OS (the
open-source version of the software on the Chrome Notebook), see the [Chromium
OS Developer Guide](/chromium-os/developer-guide).

### Specifications

*   CPU: [Samsung Exynos 5 Dual
            (5250)](http://www.samsung.com/global/business/semiconductor/product/application/detail?productId=7668)
            (Cortex A15; 1.7GHz dual core cpu)
*   GPU: [ARM
            Mali-T604](http://www.arm.com/products/multimedia/mali-graphics-hardware/mali-t604.php)
            (Quad Core)
    *   1366x768 screen & HDMI external connector
*   RAM: 2 GiB DDR3
    *   The memory is not upgradable as it is soldered directly to the
                board
*   Disk: 16 GiB SSD (connected to eMMC)
    *   SD & USB expansion slots
*   WiFi: 802.11 a/b/g/n
    *   USB slot can handle Ethernet dongle
*   Power supply: 12V <img alt="image"
            src="/chromium-os/developer-information-for-chrome-os-devices/direct-current.svg"
            height=5> ([DC](http://en.wikipedia.org/wiki/Direct_current)) 3.33A
            <img alt="image"
            src="/chromium-os/developer-information-for-chrome-os-devices/center-positive-polarity.svg"
            height=12> ([positive polarity
            tip](http://en.wikipedia.org/wiki/Polarity_symbols))
*   No [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)
*   [Servo header](/chromium-os/servo): 1x42 header (now obsolete)

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

and reboot once to boot from USB drives with Ctrl-U. Use the USB port next to
the HDMI connector.

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
tree](https://chromium.googlesource.com/chromiumos/third_party/u-boot/+/firmware-snow-2695.B)
in the `firmware-snow-2695.B` branch.

There is also firmware in a custom embedded controller (which handles things
like the keyboard), but we won't cover that here as you generally should not
need to modify that. You can find the source in the [Chromium OS ec git
tree](https://chromium.googlesource.com/chromiumos/platform/ec/+/firmware-snow-2695.B)
(in the snow firmware branch).

### Boot Sequence

*   power on
*   the CPU will execute u-boot from the read-only on-board SPI flash
*   u-boot will look at the GPT layout on the 16 GiB SSD (connected via
            eMMC)
    *   search for the firmware partition marked active and try to boot
                the u-boot that lives there
*   u-boot will look at the GPT layout
    *   search for the Linux kernel partition marked active and try to
                boot the kernel that lives there
*   Linux kernel boots from its corresponding rootfs partition
*   profit!

### Flash Layout

You can show the SPI flash layout on your device by using [flashmap
utilities](http://flashmap.googlecode.com/). It'll be the same on all devices
though so the example below should be applicable as well

```none
$ flashrom -r bios.bin
$ fmap_decode bios.bin
fmap_signature="0x5f5f464d41505f5f" fmap_ver_major="1" fmap_ver_minor="0" fmap_base="0x0000000000000000" fmap_size="0x400000" fmap_name="FMAP" fmap_nareas="23"
area_offset="0x001af000" area_size="0x00051000" area_name="RO_UNUSED"      area_flags_raw="0x01" area_flags="static"
area_offset="0x00000000" area_size="0x00200000" area_name="WP_RO"          area_flags_raw="0x01" area_flags="static"
area_offset="0x00000000" area_size="0x0019f000" area_name="RO_SECTION"     area_flags_raw="0x01" area_flags="static"
area_offset="0x00000000" area_size="0x00002000" area_name="BL1 PRE_BOOT"   area_flags_raw="0x01" area_flags="static"
area_offset="0x00002000" area_size="0x00004000" area_name="BL2 SPL"        area_flags_raw="0x01" area_flags="static"
area_offset="0x00006000" area_size="0x0009a000" area_name="U_BOOT"         area_flags_raw="0x01" area_flags="static"
area_offset="0x000a0000" area_size="0x00001000" area_name="FMAP"           area_flags_raw="0x01" area_flags="static"
area_offset="0x000aff00" area_size="0x00000100" area_name="RO_FRID"        area_flags_raw="0x01" area_flags="static"
area_offset="0x000b0000" area_size="0x000ef000" area_name="GBB"            area_flags_raw="0x01" area_flags="static"
area_offset="0x001a0000" area_size="0x00010000" area_name="RO_VPD"         area_flags_raw="0x01" area_flags="static"
area_offset="0x00200000" area_size="0x000f0000" area_name="RW_SECTION_A"   area_flags_raw="0x01" area_flags="static"
area_offset="0x00200000" area_size="0x00002000" area_name="VBLOCK_A"       area_flags_raw="0x01" area_flags="static"
area_offset="0x00202000" area_size="0x000edf00" area_name="FW_MAIN_A"      area_flags_raw="0x01" area_flags="static"
area_offset="0x002eff00" area_size="0x00000100" area_name="RW_FWID_A"      area_flags_raw="0x01" area_flags="static"
area_offset="0x00300000" area_size="0x000f0000" area_name="RW_SECTION_B"   area_flags_raw="0x01" area_flags="static"
area_offset="0x00300000" area_size="0x00002000" area_name="VBLOCK_B"       area_flags_raw="0x01" area_flags="static"
area_offset="0x00302000" area_size="0x000edf00" area_name="FW_MAIN_B"      area_flags_raw="0x01" area_flags="static"
area_offset="0x003eff00" area_size="0x00000100" area_name="RW_FWID_B"      area_flags_raw="0x01" area_flags="static"
area_offset="0x003f0000" area_size="0x00008000" area_name="RW_VPD"         area_flags_raw="0x01" area_flags="static"
area_offset="0x003f8000" area_size="0x00004000" area_name="RW_SHARED"      area_flags_raw="0x01" area_flags="static"
area_offset="0x003f8000" area_size="0x00004000" area_name="SHARED_DATA"    area_flags_raw="0x01" area_flags="static"
area_offset="0x003fc000" area_size="0x00004000" area_name="RW_PRIVATE"     area_flags_raw="0x01" area_flags="static"
area_offset="0x003fc000" area_size="0x00004000" area_name="RW_ENVIRONMENT" area_flags_raw="0x01" area_flags="static"
```

You're a good robot and can parse that right? :) Here's a highlight of the parts
you'll care about.

#### Read-Only section (first 2 MiB)

<table>
<tr>
<td> <b>offset</b></td>
<td> <b>size</b></td>
<td> <b>content</b></td>
<td> <b>details</b></td>
</tr>
<tr>
<td> 0 KiB</td>
<td> 8 KiB</td>
<td> BL1</td>
<td> Samsung first stage bootloader</td>
</tr>
<tr>
<td> 8 KiB</td>
<td> 16 KiB</td>
<td> BL2</td>
<td> Samsung second stage bootloader</td>
</tr>
<tr>
<td> 24 KiB</td>
<td> 616 KiB</td>
<td> U-Boot</td>
<td> Main (open source) bootloader</td>
</tr>
<tr>
<td> 536 KiB</td>
<td> 16 KiB</td>
<td> U-Boot env</td>
<td> Embedded runtime environment used by U-Boot</td>
</tr>
<tr>
<td> 640 KiB</td>
<td> 4 KiB</td>
<td> Flashmap</td>
<td> Specification for flash layout and such</td>
</tr>
<tr>
<td> 704 KiB</td>
<td> 956 KiB</td>
<td> GBB</td>
<td> Google Binary Block to hold various flags</td>
</tr>
<tr>
<td> 1664 KiB</td>
<td> 64 KiB</td>
<td> VPD</td>
<td> Vital/Vendor Product Data (part numbers/serial number/etc...)</td>
</tr>
</table>

The U-Boot environment can be found embedded inside the U-Boot image itself.
Normally the flash is read-only which means you can't change it. But if you were
to disable the write protect, you could update the environment to do whatever
you like.

## What's inside?

**WARNING: Opening the case and fiddling with the stuff inside could easily
brick your system and make it unrecoverable. DO NOT ATTEMPT if you are not
familiar with this process.**

**Stop. Don't. Come back.**

Really. Opening the case will allow you to modify the read-only firmware that
makes recovery possible. If someone from "teh internets" says "You need to
reflash your BIOS", they're almost certainly wrong.

### Disassembly

Taking apart your laptop is **not** encouraged. If you have hardware troubles,
please seek assistance first from an authorized center. Be advised that
disassembly might void warranties or other obligations, so please consult any
and all paperwork your received first. If you just want to see what the inside
looks like, gaze upon this (click for a high res version):

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/samsung-arm-chromebook/arm-chromebook-inside.jpg"
height=224
width=320>](/chromium-os/developer-information-for-chrome-os-devices/samsung-arm-chromebook/arm-chromebook-inside.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/samsung-arm-chromebook/snow-bottom-guts.jpg"
height=140
width=320>](/chromium-os/developer-information-for-chrome-os-devices/samsung-arm-chromebook/snow-bottom-guts.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/samsung-arm-chromebook/snow-top-guts.jpg"
height=140
width=320>](/chromium-os/developer-information-for-chrome-os-devices/samsung-arm-chromebook/snow-top-guts.jpg)

However, we acknowledge that some people like to tinker. So here's a quick guide
to taking it apart:

*   Shut the system down and close the lid
*   Flip the laptop over so the bottom is facing up
*   Remove the five visible screws from the bottom of the case
*   Remove the rubber feet from the bottom of the case
    *   Works better if you have fingernails as you can slide them under
                the edge and then peel up
    *   Consider sticking them to the case near their original holes so
                they still function as grips on tables
*   Remove the four screws that were under the rubber feet
*   Starting at the side opposite of the hinge (where the trackpad is),
            use your fingernails to pry apart the case slightly
    *   This is the best starting place as it does not have clips
                holding the top & bottom together -- all other locations do
*   As you pry it apart, wiggle it a bit so the plastic clips separate
            cleanly rather than break apart
*   Work your way evenly out until the whole edge has separated
*   Work your way along the sides towards the hinge (again, evenly
            rather than one side and then the other)
*   Once only the hinge side is still together, you can pull up
            carefully and then pull the bottom away from the hinge
    *   This side has two sets of clips which makes separation from this
                side initially almost impossible to do w/out breaking

Then to reassemble:

*   Start at the hinge side and try to get all the loops merged first
            with the clips
    *   You can pull on the back slighty so that the bottom falls into
                place
*   Once the back is snug, work your way along the edges and click the
            clips back into place
*   The edges should be nice and snug if everything is in the right
            place
*   Put all 9 screws back where you found them

#### Firmware Write Protect

It's the screw next to the USB 3.0 connector (see the pictures above for more
details). There should also be a conductive piece of plastic attached to the
screw hole.
