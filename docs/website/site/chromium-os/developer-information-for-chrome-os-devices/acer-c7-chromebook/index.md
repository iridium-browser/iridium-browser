---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: acer-c7-chromebook
title: Acer C7 Chromebook
---

[TOC]

## Introduction

This page contains information about the [Acer C7
Chromebook](http://www.google.com/intl/en/chrome/devices/acer-c7-chromebook.html)
that is interesting and/or useful to software developers. For general
information about getting started with developing on Chromium OS (the
open-source version of the software on the Chrome Notebook), see the [Chromium
OS Developer Guide](/chromium-os/developer-guide).

### Specifications

*   CPU: Intel [Sandy Bridge](http://en.wikipedia.org/wiki/Sandy_Bridge)
            Celeron (might vary on specific model)
    *   Some later models shipped [Ivy
                Bridge](http://en.wikipedia.org/wiki/Ivy_Bridge_(microarchitecture));
                those will use the parrot_ivb board
*   GPU: Intel [Sandy Bridge](http://en.wikipedia.org/wiki/Sandy_Bridge)
            Mobile
    *   11.6" 1366x768 16:9
    *   HDMI port
    *   VGA port
*   RAM: 2 GiB or 4GiB DDR3 (might vary on specific model)
*   Disk: 320 GiB HD or 16 GiB SSD (might vary on specific model)
    *   USB expansion ports
    *   [SD slot](http://en.wikipedia.org/wiki/Secure_Digital) (SDXC
                compatible)
*   Networking
    *   WiFi 802.11 a/b/g/n
    *   Dedicated Ethernet port
    *   USB ports can handle Ethernet dongles
*   Power supply: 19V <img alt="image"
            src="/chromium-os/developer-information-for-chrome-os-devices/direct-current.svg"
            height=5> ([DC](http://en.wikipedia.org/wiki/Direct_current)) 2.15A
            <img alt="image"
            src="/chromium-os/developer-information-for-chrome-os-devices/center-positive-polarity.svg"
            height=12> ([positive polarity
            tip](http://en.wikipedia.org/wiki/Polarity_symbols))
*   [Kensington Security
            Slot](http://en.wikipedia.org/wiki/Kensington_Security_Slot)
*   [Servo header](/chromium-os/servo): 1x50 header (now obsolete)

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

Sorry, but this device does not support a legacy BIOS mode. It predates the
launch of that feature and it is not feasible to produce updates of devices in
the field.

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

This device uses [coreboot](http://www.coreboot.org/) and [Das
U-Boot](http://www.denx.de/wiki/U-Boot) to boot the system. You can find the
source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-parrot-2685.B)
and the [Chromium OS u-boot git
tree](https://chromium.googlesource.com/chromiumos/third_party/u-boot/+/firmware-parrot-2685.B)
in the `firmware-parrot-2685.B` branches.

## What's inside?

**WARNING: Opening the case and fiddling with the stuff inside could easily
brick your system and make it unrecoverable. DO NOT ATTEMPT if you are not
familiar with this process.**

### Disassembly

Taking apart your laptop is **not** encouraged. If you have hardware troubles,
please seek assistance first from an authorized center. Be advised that
disassembly might void warranties or other obligations, so please consult any
and all paperwork your received first. If you just want to see what the inside
looks like, gaze upon this (click for high res versions):

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c7-chromebook/acer-c7-bottom-guts.jpg"
height=221
width=320>](/chromium-os/developer-information-for-chrome-os-devices/acer-c7-chromebook/acer-c7-bottom-guts.jpg)

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c7-chromebook/acer-c7-top-guts.jpg"
height=226
width=320>](/chromium-os/developer-information-for-chrome-os-devices/acer-c7-chromebook/acer-c7-top-guts.jpg)

However, we acknowledge that some people like to tinker. So here's a quick guide
to taking it apart.

#### Access to upgradable/cleanable components

This is very easy to do and gets you access to all the pieces you most likely
care about:

<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c7-chromebook/acer-c7-case-bottom.jpg"
height=277 width=400>

*   The hard drive is easy to remove & replace/upgrade
    *   2.5" SATA 2 or SATA 3 drives should work
    *   try to get a 7mm tall one (9.5mm will fit, just sans padding)
*   The two memory slots are easy to access
    *   The system uses 204-Pin DDR3 SO-DIMM laptop memory
*   The wifi module is easily removable (one screw)
*   The exhaust fan is easily removed (two screws) for cleaning

And here's the details (see image on right for more details):

*   Get a smallish phillips head screw driver -- all screws can be
            removed with just this
*   Shut the system down and close the lid
*   Flip the laptop over so the bottom is facing up and the warranty
            seal is closest to you
*   (Step 1) Break the seal that says "warranty is void if seal is
            broken" -- be aware, you might void your warranty by breaking the
            seal :)
*   (Step 1) Remove the single screw under the seal
*   (Step 2) Remove the battery
    *   (Step 2a) There is a slide near the edge of the battery in the
                upper right section -- put the tip of the screw driver into the
                divot and slide it to the right to unlock the battery
    *   (Step 2b) While the battery is unlocked, pull it out
*   (Step 3) Place your thumbs on the bottom two feet and pull towards
            you
    *   Alternatively, you can pull on the edge where the battery was

Check out the high res picture above for overview.

#### Access to the rest of the machine

While it is possible to remove the motherboard entirely, there isn't much of a
point. You can see a high res image of what it looks like above if you just want
that.<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-c7-chromebook/acer-c7-bottom-screws.jpg"
height=222 width=320>

If you really want to remove it, then here's how:

*   Remove the 18 screws holding it down (does not include the warranty
            screw)
    *   4 screws: one next to each of the rubber feet
    *   11 screws labeled M2x6 (including one on the cooling fan)
    *   1 screw labeled M2x3 (near the memory module)
    *   1 screw on the the cooling fan (kitty corner to the M2x6 one)
    *   1 screw on the wireless module
    *   You do not have to remove other screws (like the ones around the
                cpu)
*   The top part of the case is now held to the bottom by plastic tabs
            around the edge
*   Turn the computer onto its side and slowly pry it apart
    *   Switch between the sides to slowly work it apart
    *   You can press on the center of the motherboard where the empty
                memory slot is to help

#### Firmware Write Protect

It's a jumper next to the CPU under the black plastic. See the pictures above
for more details.
