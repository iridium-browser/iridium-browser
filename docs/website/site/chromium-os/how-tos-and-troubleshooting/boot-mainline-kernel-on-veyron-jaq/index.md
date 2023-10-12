---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: boot-mainline-kernel-on-veyron-jaq
title: Boot mainline kernel on veyron-jaq
---

## Note: work in progress.

## ChromiumOS

#### Setup

Follow the instructions in the
[Developer Guide](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/developer_guide.md).

## Firmware

Build netboot firmware for veyron to pull kernel image and command line from a
tftp share on the network. Do the following inside the chroot:

**Note:** The following uses a servo to update firmware. I think you can do the
flashrom command on the device itself, but that introduces a brick risk.

```none
cros_sdk # to enter chroot
```

```none
BOARD=veyron_jaq
```

```none
TFTP=<tftpserver>
```

```none
FTP_FOLDER=${BOARD}
```

```none
BOOTFILE=${FTP_FOLDER}/vmlinuz
```

```none
ARGSFILE=${FTP_FOLDER}/cmdline
```

```none
BIOS=/build/${BOARD}/firmware/image.net.bin
```

```none
emerge-${BOARD} chromeos-ec libpayload depthcharge coreboot chromeos-bootimage
```

```none
sudo gbb_utility -s --flags=0x239 ${BIOS}
```

```none
sudo ~/trunk/src/platform/factory/setup/update_firmware_settings.py \
  -i ${BIOS} \
  --tftpserverip=${TFTP} \
  --bootfile=${BOOTFILE} \
  --argsfile=${ARGSFILE}
dut-control warm_reset:on spi2_buf_en:on spi2_buf_on_flex_en:on spi2_vref:pp1800
sudo flashrom -p ft2232_spi:type=servo-v2${SERIAL_SERVO} --verbose -w ${BIOS}
dut-control spi2_buf_en:off spi2_buf_on_flex_en:off spi2_vref:off warm_reset:off
```

## Kernel

#### Setup

Pull down a copy of mainline and use multiv7 defconfig (a .config is also
attached to this page, but will likely be stale by the time of reading)

```none
mkdir ~/s/kernel
```

```none
cd ~/s/kernel
```

```none
git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git .
```

```none
ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make O=.build_multiv7 multiv7_defconfig
```

#### Download the its file attachment to this page and store it in ~/s/kernel/.build_mutliv7

#### Build & Deploy

Compile your kernel

```none
ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make O=.build_multiv7 -j40
```

Create a fit image for veyron-jaq that contains the kernel and the device tree

```none
mkimage -D "-I dts -O dtb -p 2048" -f rk3228-veyron-jaq.its arch/arm/boot/vmlinuz
```

Copy the fit image to your tftp share

```none
scp arch/arm/boot/vmlinuz <tftpserver>:/tftpboot/veyron_jaq
```
