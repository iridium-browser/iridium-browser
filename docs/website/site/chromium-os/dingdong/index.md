---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: dingdong
title: USB Type-C to DP Adapter
---

[TOC]

## Overview

The USB Type-C to DP Adapter connects a USB Type-C plug to a DP receptacle. It
enables users of any Chrome device that implements USB-Type C to connect to a
DisplayPort display.

## Hardware Capabilities

This adapter is an implementation of a USB Type-C to DisplayPort Cable. It
follows the requirements described in Section 3.2 "Scenarios 2a and 2b USB
Type-C to DisplayPort Cables" of the VESA DisplayPort Alt Mode for USB Type-C
Standard. The design is similar to the description in Appendix A.2 of the
Standard and shown in Figure A-2: DisplayPort Video Adaptor Cable Block Diagram.

It provides the following features:

*   DisplayPort protocol support
*   automatic enabling of video out
*   video output with resolutions of up to 4K @ 60 Hz (depending on
            system and monitor capabilities)

![image](/chromium-os/dingdong/DingDong%20Block%20Diagram.png)

For schematics, [click
here](https://docs.google.com/a/chromium.org/viewer?a=v&pid=sites&srcid=Y2hyb21pdW0ub3JnfGRldnxneDo3MWY0NGE4NGM4MjBiYTIy)
or see the attached file.

## Firmware Source Code

The firmware is located In the Chromium Embedded Controller repository under
*[board/dingdong/](https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/board/hoho/)*
:

<https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/board/dingdong/>

### Building Firmware

Within your Chromium OS chroot, the syntax is:

```none
cd ~/trunk/src/platform/ec
make BOARD=dingdong
```

### Flashing Firmware

The firmware is normally flashed via the kernel, as part of the Chrome OS
Auto-Update process. Payloads are located in `/lib/firmware/cros-pd`.

When the adapter is connected to a Chrome device, you can manually flash the
read-write firmware as follows:

```none
ectool --cros_pd flashpd 3 <port> /tmp/dingdong.ec.RW.bin
```

The device must be in developer mode to run the `ectool` command. After running
this command, the firmware is located in
`~/trunk/src/platform/ec/build/dingdong/ec.bin .`

## Connecting to the Adapter

Developers of devices based on this design may need to install an initial image
by putting the STM32F072 into DFU mode. This can be done by asserting PD_BOOOT0
(TP8 in the schematic) and resetting the MCU. It can be helpful to attach a
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

<pre><code>scp ~/trunk/src/platform/ec/build/dingdong/ec.bin root@${IP}:/tmp/.
</code></pre>

where *board-name* is the name of the Chrome device, for example peppy, and
*host-IP-address* is its IP address on the network.

The next step is to connect the adapter to the Chrome device in DFU mode and
write a full image (RO + RW). On the Chrome device, the syntax is:

```none
flash_ec --board=dingdong --image=/tmp/ec.bin
```

## Revision History

810-10117-02: Initial release.
810-10117-03: Power sequencing and timing improvements.
