---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-guide
  - Chromium OS Developer Guide
page_name: beaglebone
title: Building for Beaglebone
---

Building for beaglebone uses the standard build tools, with only minor
adjustments to the options.

#### Build options

Run `cros build-packages` with these options:

```bash
cros build-packages --board=beaglebone --no-withtest --no-withautotest \
    --no-withfactory
```

Assign a password to the 'chronos' account with the standard commands:

```none
  ./enable_localaccount.sh chronos
  ./set_shared_user_password.sh
```

Run `cros build-image` with these options:

```bash
cros build-image --board=beaglebone base
```

Put the image onto a micro-SD card with a command similar to this:

```none
  cros flash usb:// ../build/images/beaglebone/latest/chromiumos_base_image.bin
```

#### Connecting to the serial console

1.  Plug the USB cable into an Ubuntu system.
2.  Run these commands on the Ubuntu system.

```none
sudo modprobe ftdi_sio vendor=0x0403 product=0xa6d0
sudo screen /dev/ttyUSB1 115200
```

**Note:** In some cases, the tty device may be `/dev/ttyUSB0` instead.

#### Booting Chrome OS on the Beaglebone

Plug the micro-SD card into the beaglebone, and apply power. With the beaglebone
**black**, hold down **S2** button on board before applying power. Attach to the
beaglebone console, hit return, and type 'boot' at the U-Boot prompt.

**Note:** Power the beaglebone using the 5V barrel connector. USB power will
usually work but will fail when doing an operation with a high power draw (e.g.
update_engine).
