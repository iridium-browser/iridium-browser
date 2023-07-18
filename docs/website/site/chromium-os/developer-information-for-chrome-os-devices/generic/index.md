---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: generic
title: Generic Chrome OS Device Instructions
---

[TOC]

## Introduction

Unfortunately, a page specific to your device does not yet exist. This page
contains generic information for working with your Chrome OS device that is
interesting and/or useful to software developers. It should hopefully work for
your device, but there are no guarantees.

For general information about getting started with developing on Chromium OS
(the open-source version of the software on the Chrome Notebook), see the
[Chromium OS Developer Guide](/chromium-os/developer-guide).

## Developer Mode

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

An unrelated note: Holding just Refresh and poking the Power button hard-resets
the machine without entering Recovery. That's occasionally useful, but use it
with care - it doesn't sync the disk or shut down politely, so there's a nonzero
chance of trashing the contents of your stateful partition.

### Introduction

Enabling Developer mode (dev mode) is the first step to tinkering with your
Chromebook. With Developer mode enabled you can do things like poke around on a
command shell (as root if you want), install Chromium OS, or try other OS's.
Note that Developer mode turns off some security features like verified boot and
disabling the shell access. If you want to browse in a safer, more secure way,
leave Developer mode turned OFF.

Note: Switching between Developer and Normal (non-developer) modes will remove
user accounts and their associated information from your Chromebook.

Note: "dev mode" and "dev channel" are completely independent features (even if
they have very similar names). If you simply want to switch to the dev channel,
see the [channel switching
documentation](https://support.google.com/chromebook/answer/1086915).

### Entering

Please see our new [Debug Button Shortcuts
document](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/debug_buttons.md)
for more details. It explains different form factors too.

### Leaving

To leave Dev-mode and go back to normal mode, just follow the instructions at
the scary boot screen. It will prompt you to confirm.

If you want to leave Dev-mode programmatically, you can run `crossystem
disable_dev_request=1; reboot` from a root shell. There's no way to enter
Dev-mode programmatically, and just seeing the Recovery screen isn't enough -
you have to use the three-finger salute which hard-resets the machine first.
That's to prevent a remote attacker from tricking your machine into dev-mode
without your knowledge.
