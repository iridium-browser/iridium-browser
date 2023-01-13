---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-guide
  - Chromium OS Developer Guide
page_name: using-serial-tty
title: Controlling Enabled Consoles
---

[TOC]

## Introduction

The set of enabled consoles (serial tty or virtual terminal (VT)) used to
require a combination of USE flags and custom package installs. With the new
chromeos-base/tty package, all you have to do is set a single variable in your
`make.conf`.

By default, only tty2 is enabled.

## Enabling Specific tty Devices

Each tty device will have an init script in `/etc/init` with a name like
`console-tty1.conf`.

### Temporary Testing

You can declare which console to activate through the `TTY_CONSOLE` expanded
flag.

If you do not want to rebuild the whole system, you can just rebuild
chromeos-base/tty with the right flag:

```none
USE="tty_console_tty1 tty_console_tty2" emerge-${BOARD} chromeos-base/tty
```

### Changing Board Defaults

If you want your overlay to install an extra script by default, add the
following line to your `make.conf`:

```none
TTY_CONSOLE="${TTY_CONSOLE} tty1 ttyS0"
```

## Which tty are available?

The tty available are listed in [chromeos-base/tty
ebuild](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/chromeos-base/tty/tty-0.0.1.ebuild).

The current list of tty's:

```none
ttyAMA{0..5}ttyO{0..5}ttyS{0..5}ttySAC{0..5}tty{0..5}
```
