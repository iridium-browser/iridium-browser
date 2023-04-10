---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: lenovo-thinkpad-x131e-chromebook
title: Lenovo Thinkpad X131e Chromebook
---

[TOC]

## Introduction

This page contains information about the [Lenovo Thinkpad X131e
Chromebook](http://www.google.com/intl/en/chrome/education/devices/lenovo-x131e-chromebook.html)
that is interesting and/or useful to software developers. For general
information about getting started with developing on Chromium OS (the
open-source version of the software on the Chrome Notebook), see the [Chromium
OS Developer Guide](/chromium-os/developer-guide).

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

**This device has unique requirements for entering recovery mode!**

You must first remove all power from the system, both the battery and the AC
plug. Then when power is re-applied you have 20 seconds to enter recovery mode
with the ESC+Refresh+Power button sequence.

In this system the removal of power sources will force the Embedded Controller
to restart and stay in read-only code to ensure the recovery mode is secure.

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

Sorry, but this device does not support a legacy BIOS mode.

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
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-stout-2817.B)
and the [Chromium OS u-boot git
tree](https://chromium.googlesource.com/chromiumos/third_party/u-boot/+/firmware-stout-2817.B)
in the `firmware-stout-2817.B` branches.

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

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-thinkpad-x131e-chromebook/stout-inside-top.jpg"
width=320>](/chromium-os/developer-information-for-chrome-os-devices/lenovo-thinkpad-x131e-chromebook/stout-inside-top.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-thinkpad-x131e-chromebook/stout-inside-bottom.jpg"
width=320>](/chromium-os/developer-information-for-chrome-os-devices/lenovo-thinkpad-x131e-chromebook/stout-inside-bottom.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-thinkpad-x131e-chromebook/stout-bare-top.jpg"
width=320>](/chromium-os/developer-information-for-chrome-os-devices/lenovo-thinkpad-x131e-chromebook/stout-bare-top.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/lenovo-thinkpad-x131e-chromebook/stout-bare-bottom.jpg"
width=320>](/chromium-os/developer-information-for-chrome-os-devices/lenovo-thinkpad-x131e-chromebook/stout-bare-bottom.jpg)

### Servo Header

The X131e was not designed to use [servo](/chromium-os/servo). A small number of
debug units have a hand-wired header.
