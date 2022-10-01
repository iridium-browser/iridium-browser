---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: hoho
title: USB Type-C to HDMI Adapter
---

[TOC]

## Overview

The USB Type-C to HDMI Adapter connects a USB Type-C plug to an HDMI receptacle.
It enables users of any Chrome device that implements USB-Type C to connect to
an HDMI display.

## Hardware Capabilities

This adapter is an implementation of a USB Type-C DFP_D to HDMI Protocol
Converter. It follows the requirements of section 4.3.1 of the VESA DisplayPort
Alt Mode for USB Type-C Standard. In the VESA standard, refer to Figure 4-13:
Scenario 3a.

It provides the following features:

*   DisplayPort protocol conversion to support HDMI
*   automatic enabling of video out
*   video output with resolutions of up to 4K @ 60 Hz (depending on
            system and monitor capabilities)

![image](/chromium-os/hoho/HoHo%20Block%20Diagram%20%281%29.png)

For schematics, [click
here](https://docs.google.com/a/chromium.org/viewer?a=v&pid=sites&srcid=Y2hyb21pdW0ub3JnfGRldnxneDoyZWUwYmU2NWNiMWMwZTY2)
or see the attached file.

## Firmware Source Code

The firmware is located In the Chromium Embedded Controller repository under
*[board/hoho/](https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/board/hoho/)*
:

<https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/board/hoho/>

### Building Firmware

Within your Chromium OS chroot, the syntax is:

```none
cd ~/trunk/src/platform/ec
make BOARD=hoho
```

### Flashing Firmware

The firmware is normally flashed via the kernel, as part of the Chrome OS
Auto-Update process. Payloads are located in `/lib/firmware/cros-pd`.

When the adapter is connected to a Chrome device, you can manually flash the
read-write firmware as follows:

```none
ectool --cros_pd flashpd 4 <port> /tmp/hoho.ec.RW.bin
```

The device must be in developer mode to run the `ectool` command. After running
this command, the firmware is located in
`~/trunk/src/platform/ec/build/hoho/ec.bin .`

## Connecting to the Adapter

Developers of devices based on this design may need to install an initial image
by putting the STM32F072 into DFU mode. This can be done by asserting PD_BOOOT0
(TP11 in the schematic) and resetting the MCU. It can be helpful to attach a
switch for this purpose on prototype devices.

Installing an image requires the dfu-util tools package on the Chrome device
where the adapter is connected. To deploy dfu-util and copy the chromeos-ec
image from within your Chromium OS chroot, the syntax is:

<pre><code>BOARD=<i>board-name</i>
</code></pre>

<pre><code>IP=<i>host-IP-address</i>
</code></pre>

<pre><code>emerge-${BOARD} dfu-util chromeos-ec
</code></pre>

<pre><code>cros deploy $IP dfu-util chromeos-ec
</code></pre>

<pre><code>scp ~/trunk/src/platform/ec/build/hoho/ec.bin root@${IP}:/tmp/.
</code></pre>

where *board-name* is the name of the Chrome device, for example peppy, and
*host-IP-address* is its IP address on the network.

The next step is to connect the adapter to the Chrome device in DFU mode and
write a full image (RO + RW). On the Chrome device, the syntax is:

```none
flash_ec --board=hoho --image=/tmp/ec.bin
```
