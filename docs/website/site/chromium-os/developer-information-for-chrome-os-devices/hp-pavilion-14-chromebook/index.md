---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: hp-pavilion-14-chromebook
title: HP Pavilion Chromebook
---

[TOC]

## Introduction

This page contains information about the [HP Pavilion
Chromebook](http://www.google.com/intl/en/chrome/devices/hp-pavilion-chromebook.html)
that is interesting and/or useful to software developers. For general
information about getting started with developing on Chromium OS (the
open-source version of the software on the Chrome Notebook), see the [Chromium
OS Developer Guide](/chromium-os/developer-guide).

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

Details for working with developer mode can be found [on this
page](/chromium-os/developer-information-for-chrome-os-devices/acer-c7-chromebook).

## Firmware

This device uses [coreboot](http://www.coreboot.org/) and [Das
U-Boot](http://www.denx.de/wiki/U-Boot) to boot the system. You can find the
source in the [Chromium OS coreboot git
tree](https://chromium.googlesource.com/chromiumos/third_party/coreboot/+/firmware-butterfly-2788.B)
and the [Chromium OS u-boot git
tree](https://chromium.googlesource.com/chromiumos/third_party/u-boot/+/firmware-butterfly-2788.B)
in the `firmware-butterfly-2788.B` branches.

## What's inside?

**WARNING: Opening the case and fiddling with the stuff inside could easily
brick your system and make it unrecoverable. DO NOT ATTEMPT if you are not
familiar with this process.**

### Disassembly

Taking apart your laptop is **not** encouraged. If you have hardware troubles,
please seek assistance first from an authorized center. Be advised that
disassembly might void warranties or other obligations, so please consult any
and all paperwork your received first. If you just want to see what the inside
looks like, gaze upon this (click for a high res version):

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-top-guts.jpg"
height=216
width=320>](/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-top-guts.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-bottom-guts.jpg"
height=198
width=320>](/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-bottom-guts.jpg)

However, we acknowledge that some people like to tinker. So here's a quick guide
to taking it apart:

*   First, acquire the necessary tools:
    *   1 phillips head screw driver (a PH0 should be able to handle
                everything)
    *   1 wide flat head screw driver, or a butter knife
*   Shut the system down and close the lid
*   Flip the laptop over so the bottom is facing up and the hinge is
            facing away from you
*   Remove the battery
*   Remove all screws on D-panel (see pic) w/phillips head (there are
            10)[<img alt="image"
            src="/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-dpanel.jpg"
            height=137
            width=200>](/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-dpanel.jpg)
*   Pry off keyboard using a flat instrument
    *   The keyboard can bend inward and upward to dislodge the tabs
                which are all along the top and bottom of the keyboard
    *   Start at the bottom left corner and work your way up the left
                side
    *   With the left side popped, slide the screw driver along the
                middle underneath and wiggle/pop the tabs out as you go
    *   There is a ribbon cable underneath the right shift key that
                needs to be disconnected from the motherboard

        [<img alt="image"
        src="/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-keyboard.jpg"
        height=150
        width=200>](/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-keyboard.jpg)

*   Remove all screws on C-panel. Note location of
            [servo](/chromium-os/servo) connector (non-standard 2x25 connector),
            circled in blue.

    [<img alt="image"
    src="/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-cpanel.jpg"
    height=150
    width=200>](/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-cpanel.jpg)

*   Disconnect the two ribbon cables and pry off C-panel
    *   You can use the open holes in the metal plate to pull up until
                the edge along the hinge separates
    *   Using the edge along the hinge, pull the top panel away from the
                bottom panel
*   Unit is now disassembled. Note the location of the WP switch.

    [<img alt="image"
    src="/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-top-wp.jpg"
    height=200
    width=150>](/chromium-os/developer-information-for-chrome-os-devices/hp-pavilion-14-chromebook/butterfly-top-wp.jpg)
