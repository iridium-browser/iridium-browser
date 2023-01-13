---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: minimuffin
title: 45W USB Type-C charging adapter control board
---

[TOC]

## Overview

The 45W USB Type-C charging adapter control board implements the controls for a
charging adapter for a USB Type-C enabled device.

## Hardware Capabilities

This board provides the following features, to enable USB Type-C based power
charger design:

*   BMC (Biphase Mark Coding) used for USB PD (Power Delivery)
            communication over the CC wire
*   Support 20V/12V/5V@ 2.25A and up to 45W
*   Support OCP (Over Current Protection) / OVP (Over Voltage
            Protection) at adapter's secondary side

### <img alt="image"
src="/chromium-os/minimuffin/45W%20Type-C%20Adapter%20Control%20Board%20150710.gif"
height=259 width=400>

For schematic, see the attached file at the bottom of this page.

## Firmware Source Code

The firmware is located In the Chromium Embedded Controller repository under
*[board/zinger/](https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/board/zinger/)*
:

<https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/board/zinger/>

### Building Firmware

Within your Chromium OS chroot, the syntax is:

```none
cd ~/trunk/src/platform/ec
make BOARD=zinger
```

### Flashing Firmware

The firmware is normally flashed via the kernel, as part of the Chrome OS
Auto-Update process. Payloads are located in `/lib/firmware/cros-pd`.

When the adapter is connected to a Chrome device, you can manually flash the
read-write firmware as follows:

```none
ectool --cros_pd flashpd 4 <port> /tmp/zinger.ec.RW.bin
```

The device must be in developer mode to run the `ectool` command. After running
this command, the firmware is located in
`~/trunk/src/platform/ec/build/zinger/ec.bin`
