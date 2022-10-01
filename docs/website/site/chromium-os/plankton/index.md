---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: plankton
title: USB Type-C functional testing board
---

[TOC]

## Overview

The USB Type-C functional testing board, Plankton Raiden board, enables USB
Type-C automated functional testing.

Plankton Raiden board is a multi-layer designed testing board to switch out USB
Type-C input data lane. There are several USB muxes and a DisplayPort (DP)
re-driver with multiple IO ports so that we can switch USB Type-C functions with
huge flexibility. On the board there is also an STM MCU chip for muxes and
switches controller, and FTDI listener to get commands from the outside host.

## Hardware Capabilities

### <img alt="image" src="/chromium-os/plankton/IMG_5169.jpg" height=320 width=271>

> ### Data

> *   DUT as host: USB 3.1 Gen 1
> *   DUT as device: USB 2.0

> ### Video

> *   USB-C to DisplayPort converter
> *   Includes DP redriver to allow 4K @ 60Hz

> ### Power

> *   Source 5V/12V/20V@3A for DUT as sink
> *   Monitor voltage/current on VBUS for DUT as source

> ### CC monitoring

> *   Dual CC connection allows “flip connector” tests
> *   USB PD traffic monitor on both CC lines

## Block diagram

[<img alt="image"
src="/chromium-os/plankton/Plankton_raiden%20block%20diagram.png">](/chromium-os/plankton/Plankton_raiden%20block%20diagram.png)

For schematic and board files, please see the attached zip file at the bottom of
this page.

## Hardware Switches

Set the switches to default setting before flashing EEPROM or firmware.

    SW9 - Enable Debug buttons (switch to DISABLE to enable function, there is a
    layout mistake)

    SW1 - Set Boot0 signal to low (No MCU_BOOT0)

    SW3 - Set to L:CN3 or CN5

    SW14 - Set to L: CN5 to HUB

*   SW17 (in the back) - Set 5V OVP for CN1

In the following session, please power up the board and connect to the host
through USB3.0 Micro B port (CN5).

## EEPROM Programming

For a brand-new board, please program its EEPROM first for setting serial number
and product and vendor ID.

Install ftx-prog, you can download the program from the Github
[here](https://github.com/richardeoin/ftx-prog/archive/master.zip) and unzip to
your home directory.

```none
sudo apt-get install build-essential gcc make libftdi-dev
cd ~/ftx-prog-master
make
```

and then use this command to flash EEPROM. The format of serial number is
"901008-XXXXX", get the last 5 digits from the back of the Plankton Raiden
board.

[<img alt="image"
src="/chromium-os/plankton/Plankton%20Raiden%20Software%20Care%20%26%20Feed.jpg"
height=55
width=400>](/chromium-os/plankton/Plankton%20Raiden%20Software%20Care%20%26%20Feed.jpg)

```none
sudo ftx_prog --old-vid 0x0403 --new-vid 0x18d1 --old-pid 0x6015 --new-pid 0x500c --new-serial-number <serial number> --cbus 0 GPIO --cbus 1 Tristate --cbus 2 GPIO --cbus 3 GPIO --manufacturer "Google Inc" --product "Plankton" --ftprog-strings
```

## Firmware Source Code

The firmware is located In the Chromium Embedded Controller repository under
*[board/plankton/](https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/board/plankton/)*
:

<https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/board/plankton/>

### Building Firmware

Within your Chromium OS chroot, the syntax is:

```none
cd ~/trunk/src/platform/ec
make BOARD=plankton
```

### Firmware image will be located under build/plankton/ec.bin

### If your chroot tree is newly-built, you may encounter an error while "make BOARD=plankton"
### Try to type this command first:

### ```none
~/trunk/src/scripts/setup_board --board=peach_pit --nousepkg
```

### and then try "make BOARD=plankton" again.

### Running Servod

1. Build up local package of servod

```none
cd ~/trunk/src/third_party/hdctools
cros_workon --host start dev-util/hdctools
sudo emerge hdctools
```

2. Run servod for Plankton Raiden board

```none
sudo servod -p 0x500c
```

### Flashing Firmware

1. Open another terminal (you need to leave servod on):
Note: use "cros_sdk --no-ns-pid" to enter chroot, otherwise you will get error
while flashing firmware.

```none
cd ~/trunk/src/platform/ec
```

2. Flash firmware which you built earlier into Plankton Raiden (get port number
in servod message, 9999 in normal):

```none
./util/flash_ec --board=plankton --port=<port number>
```

You may also assign the image file to flash into Plankton Raiden by the --image
argument:

```none
./util/flash_ec --board=plankton --port=<port number> --image=<your/image/file/path>
```

### Debug Buttons and LEDs

If debug buttons are enabled (SW9), you can use buttons to switch board
functions. LEDs are helpful for monitoring current state of Plankton Raiden.

<img alt="image" src="/chromium-os/plankton/buttons.jpeg" height=138 width=400>

<img alt="image" src="/chromium-os/plankton/leds.jpeg" height=217 width=400>

<table>
<tr>

<td>SW6</td>

<td>Source 20 V/3A to Type-C port (while DUT plugs on)</td>

<td>DS8, DS4, and DS9 will be on</td>

</tr>
<tr>

<td>SW12</td>

<td>Source 12 V/3A to Type-C port (while DUT plugs on)</td>

<td>DS8 and DS9 will be on </td>

</tr>
<tr>

<td>SW11</td>

<td>Source 5V/3A to Type-C port (while DUT plugs on)</td>

<td>DS9 will be on </td>

</tr>
<tr>

<td>SW8</td>

<td>Provide power from VBUS to CN1</td>

<td>DS5 will be on </td>

</tr>
<tr>

<td>SW2</td>

<td>Reset MCU</td>

</tr>
<tr>

<td>SW5</td>

<td>Toggle USB3.0 & DP for USB type C port</td>

<td>DS1 is on </td>

<td>- USB3.0</td>

<td>- Debug USB from CN14 to DUT</td>

<td>DS1 is off - DP</td>

</tr>
<tr>

<td>SW7</td>

<td>Enable case close debugging </td>

</tr>
<tr>

<td>SW10</td>

<td>Flip CC</td>

<td>Connect DUT via CC1 -DS3 off</td>

<td>Connect DUT via CC2 -DS3 on</td>

</tr>
<tr>

<td>Plug in type C cable</td>

<td>Communication on CC1</td>

<td>DS2 will blink</td>

</tr>
<tr>

<td>Plug in type C cable</td>

<td>Communication on CC2</td>

<td>DS6 will blink</td>

</tr>
</table>

### Software Control

**Using CLI**

1. Use modprobe to load on ftdi_sio on your Unix host.

```none
sudo modprobe ftdi_sio
echo 18d1 500c | sudo tee /sys/bus/usb-serial/drivers/ftdi_sio/new_id
```

2. Check whether TTY is established. An additional ttyUSBx will appear for
ftdi_sio (if not, you may need to remove and attach PR board again).

```none
ls /dev/ttyUSB*
```

3. Open the command-line interface by Unix console tools (ex. socat, cu).

```none
cu -l /dev/ttyUSBx -s 115200
```

Please refer to the document "Plankton Raiden MCU command table" below for CLI.

**Using dut-control**

1. Inside chroot, open up servod with ftdi_sio on PR board.

```none
sudo servod -p 0x500c
```

2. Use “dut-control” to get all available components of PR board.

```none
dut-control
```
