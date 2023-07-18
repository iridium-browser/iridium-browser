---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: twinkie
title: USB-PD Sniffer
---

[TOC]

## Overview

This page describes a USB-PD sniffing dongle with Type-C connectors. The dongle
can be supported by Chrome devices as part of a USB-Type C implementation.

<img alt="image" src="/chromium-os/twinkie/twinkies-2.png" height=156 width=200>
<img alt="image" src="/chromium-os/twinkie/twinkies-1.png" height=156 width=200>
<img alt="image" src="/chromium-os/twinkie/twinkies-3.png" height=156 width=200>

## Hardware Capabilities

*   Sniffing USB Power Delivery traffic on both Control Channel lines
            (CC1/CC2)
*   Transparent interposer on a USB Type-C connection
*   Monitoring VBUS and VCONN voltages and currents (WARNING: VBUS path
            designed for SPR voltages up to 20V. Not appropriate for use with
            USB PD 3.1 EPR systems when in EPR mode)
*   Injecting PD packets on CC1 or CC2
*   Putting Rd/Rp/Ra resistors on CC1 or CC2

[<img alt="image" src="/chromium-os/twinkie/Twinkie-diagram.png" height=314
width=400>](/chromium-os/twinkie/Twinkie-diagram.png)

Complete schematics, layout, and enclosure are attached at the bottom of this
page.

## **Firmware Source Code**

The firmware is located In the Chromium Embedded Controller repository under
*[board/twinkie/](https://chromium.googlesource.com/chromiumos/platform/ec/+/firmware-twinkie-9628.B/board/twinkie/),*
the release version is on the firmware-twinkie-9628.B branch :

<https://chromium.googlesource.com/chromiumos/platform/ec/+/firmware-twinkie-9628.B/board/twinkie/>

### **Building Firmware**

Within your Chromium OS chroot, the syntax is:

```none
For the impatient, here is a very cut-down version of the detailed instructions. 
```

```none
If anything here gives you trouble, refer to the [official documentation](http://www.chromium.org/chromium-os/developer-guide) instead.
```

```none
```

```none&#10;&#10;## Install the prerequisite tools&#10;&#10;```

```none
goobuntu$  	sudo apt-get install git-core gitk git-gui subversion curl
```

```none
goobuntu$  	git clone <https://chromium.googlesource.com/chromium/tools/depot_tools.git>
```

```none
goobuntu$  	export PATH=$(pwd)/depot_tools:$PATH
```

```none&#10;&#10;## Check out the sources&#10;&#10;```

```none
goobuntu$  	mkdir chromiumos && cd chromiumos
```

```none
goobuntu$  	repo init -u https://chromium.googlesource.com/chromiumos/manifest.git \
```

```none
                    --repo-url https://chromium.googlesource.com/external/repo.git \
```

```none
                    -g minilayout -b firmware-twinkie-9628.B
```

```none
goobuntu$  	repo sync
```

```none&#10;&#10;## Enter the chroot&#10;&#10;```

```none
goobuntu$  	cros_sdk
```

```none&#10;&#10;## Install the board-specific compiler toolchain&#10;&#10;```

```none
chroot$  	./setup_board --board=falco
```

```none&#10;&#10;## Check out the EC sources&#10;&#10;```

```none
chroot$  cros_workon-falco start chromeos-ec
```

```none
chroot$  repo sync ../platform/ec
```

```none&#10;&#10;## Build things&#10;&#10;```

```none
chroot$	 cd ../platform/ec
```

```none
chroot$  make BOARD=twinkie -j
```

Alternately, you can use the **pre-built firmware**
*[twinkie_v1.11.19-9e81762f2.bin](https://storage.googleapis.com/chromeos-vpa/twinkie-20171122/twinkie_v1.11.19-9e81762f2.bin)*
attached at the bottom of this page.

### **Flashing Firmware**

The USB-PD dongle behaves as a USB DFU device when the ID pin is grounded on the
USB micro-B connector.

This is done either by plugging an A-A USB cable into an A-to-microB OTG adapter
as shown in the photo below

or by typing the` dfu` command on the USB console (if you already have a recent
firmware, e.g. *v1.11.19*+).

Within your Chromium OS chroot:

```none
./util/flash_ec --board=twinkie
```

or on Ubuntu Linux:

```none
sudo apt-get install dfu-util
```

```none
sudo dfu-util -a 0 -s 0x08000000 -D twinkie_v1.11.19-9e81762f2.bin
```

[<img alt="Programming Twinkie with OTG and A-to-A cables"
src="/chromium-os/twinkie/twinkie_programming.jpg" height=161
width=320>](/chromium-os/twinkie/twinkie_programming.jpg)

if you have entered the DFU mode by using the dfu console command, you need to
use the following command to exit it :

sudo dfu-util -a 0 -s 0x08000000:force:unprotect -D
twinkie_v1.11.19-9e81762f2.bin

else plugging the regular USB cable will do it automatically.

## Using the Integrated Command Line over USB

The USB-PD dongle exports its internal command-line console as a pair of USB
bulk endpoints.

On a Linux system, you get the console as a /dev/ttyUSB*n* device.

On recent systems (kernel v3.19+), this ttyUSB device should be instantiated
automatically, or other systems you can try using the *usbserial* kernel module
:

```none
echo '18d1 500A' | sudo tee /sys/bus/usb-serial/drivers/generic/new_id
```

If this fails, you might need to run `sudo modprobe usbserial` first.

## Using as a PD Packet Sniffer

You can use the opensource [Sigrok](http://sigrok.org) framework to acquire and
decode USB Power Delivery traces with the USB-PD dongle. You can then use
[Pulseview ](http://sigrok.org/wiki/PulseView)to display them.

The patches for the Sigrok hardware driver for the dongle is not in the upstream
packages yet.

The packages below also include the bleeding edge version of Pulseview which has
a convenient "Tabular Decoder Output View".

If your machine has an x86_64 processor, you can try the following experimental
pre-built packages:

- for Ubuntu Focal Fossal (20.04 LTS)

```none
cd $(mktemp -d)
wget https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_20.04/libsigrok4_0.5.2-2+twinkie_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_20.04/libsigrokcxx4_0.5.2-2+twinkie_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_20.04/libsigrokdecode4_0.5.3-1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_20.04/sigrok-cli_0.7.1-1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_20.04/pulseview_0.5.0~git20200910+1acc207a-1_amd64.deb
sudo dpkg -i *.deb
sudo apt-get install -f
```

- for Ubuntu Bionic Beaver (18.04 LTS)

```none
cd $(mktemp -d)
wget https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_18.04/libsigrok4_0.5.2-2+twinkie_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_18.04/libsigrokcxx4_0.5.2-2+twinkie_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_18.04/libsigrokdecode4_0.5.3-1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_18.04/sigrok-cli_0.7.1-1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/ubuntu_18.04/pulseview_0.5.0~git20200910+1acc207a-1_amd64.deb
sudo dpkg -i *.deb
sudo apt-get install -f
```

- for Debian Buster (stable) or Chrome OS Crostini container

```none
cd $(mktemp -d)
wget https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/debian-buster/libsigrok4_0.5.2-2+twinkie_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/debian-buster/libsigrokcxx4_0.5.2-2+twinkie_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/debian-buster/libsigrokdecode4_0.5.3-1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/debian-buster/sigrok-cli_0.7.1-1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/debian-buster/pulseview_0.5.0~git20200910+1acc207a-1_amd64.deb
sudo dpkg -i *.deb
sudo apt-get install -f
```

- for Rodete

```none
cd $(mktemp -d)
wget https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/rodete/libsigrok4_0.5.2-2+twinkie_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/rodete/libsigrokcxx4_0.5.2-2+twinkie_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/rodete/libsigrokdecode4_0.5.3-1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/rodete/sigrok-cli_0.7.1-1_amd64.deb \
    https://storage.googleapis.com/chromeos-vpa/twinkie_20201028/rodete/pulseview_0.5.0~git20200910+1acc207a-1_amd64.deb
sudo dpkg -i *.deb
sudo apt-get install -f
```

If you want to do a full build from sources of sigrok and pulseview with Twinkie
support,

now the USB PD protocol decoder is now upstreamed in libsigrokdecode (and
sigrok-cli / pulseview are working out-of-the-box).

You only need to add the Chromium Twinkie hardware driver in libsigrok : [last
version of the patch applying on
libsigrok-0.5.2](https://github.com/vpalatin/libsigrok/commit/8147e36b8ffa48a23cf898332c351f6a6d85484b)
[plus the VBUS
support](https://github.com/vpalatin/libsigrok/commit/97a7e9cbb3126aa5329b96a2f017e682b9036d8f)

A recipe to do this build is written down in [Build Sigrok and Pulseview from
sources](/chromium-os/twinkie/build-sigrok-and-pulseview-from-sources).

### Capturing traces with the Sigrok tool

```none
sigrok-cli -d chromium-twinkie --continuous -o test.sr
```

### Real-time decoding of USB PD packets while capturing

```none
sigrok-cli -d chromium-twinkie --continuous -P usb_power_delivery:cc=CC1:fulltext=yes -P usb_power_delivery:cc=CC2:fulltext=yes -A usb_power_delivery=text
```

### Displaying and decoding PD traces

```none
pulseview test.sr & 
```

Add the **USB PD** decoder from the Decoders menu, then edit the instantiated
decoder to select the appropriate CC line.

[<img alt="image"
src="/chromium-os/twinkie/pulseview_3packets.png">](/chromium-os/twinkie/pulseview_3packets.png)

[<img alt="image"
src="/chromium-os/twinkie/pulseview_zoom_packet.png">](/chromium-os/twinkie/pulseview_zoom_packet.png)

[<img alt="image"
src="/chromium-os/twinkie/pulseview_VBUS_V.png">](/chromium-os/twinkie/pulseview_VBUS_V.png)

[<img alt="image"
src="/chromium-os/twinkie/pulseview_VBUS_V_A.png">](/chromium-os/twinkie/pulseview_VBUS_V_A.png)

### Experimental VBUS analog traces

As shown on the pictures above, you can try to capture the VBUS analog voltage
and current along with the CCx lines traffic.

This feature is still *experimental and might have negative side effects* on
your PD packets capture !

Capturing VBUS voltage only and CCx traffic:

```none
sigrok-cli  -d chromium-twinkie:analog_channels=1 --continuous -o testvbusV.sr
```

Capturing VBUS voltage and current and CCx traffic:

```none
sigrok-cli  -d chromium-twinkie:analog_channels=2 --continuous -o testvbusVA.sr
```

then you can display the .sr files in Pulseview.

## Using as a Power Sink

1.  Use the tw sink command. Exit sink mode by using the reboot command.
2.  When a power source is detected, the dongle negotiates a power
            contract, activating the green LED for 5V, the red LED for 20V, or
            the blue LED for other voltage.
3.  You can change the maximum negotiated voltage with the following
            command in the dongle USB shell:
    pd 0 dev 12
    This example sets a limit of 12 V.
