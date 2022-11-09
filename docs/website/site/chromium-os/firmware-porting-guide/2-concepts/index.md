---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/firmware-porting-guide
  - Firmware Overview and Porting Guide
page_name: 2-concepts
title: 2. Concepts
---

[TOC]

## Firmware Image

The Chrome OS firmware image has two main sections: Read-Only (RO) and
Read-Write (RW). The RO firmware is set at the factory and cannot be updated
after manufacturing. The RW firmware can be updated during Chrome OS auto-update
(AU).

If a problem is found in RO firmware, Google creates an update and places it in
the RW firmware. During the boot process, the RO firmware checks whether there
is an update in the RW section and, if so, jumps to the RW update to execute the
new boot code.

The RO firmware contains the following code:

*   U-Boot, including the device tree for this system
*   On x86 systems: coreboot
*   Google Binary Block (GBB), which contains the following:
    *   Recovery screen images
    *   Public keys needed to verify the RW firmware
*   Firmware ID (a string with the version number and device type)

The RW firmware contains two sections: A and B. Each section contains the
following:

*   U-Boot, including the device tree for this system (identical to the
            U-Boot images in RO firmware)
*   VBlock, which contains the signatures used to verify the kernel
            before loading and running it
*   Firmware ID
*   Embedded Controller image
*   [Fmap](/chromium-os/firmware-porting-guide/fmap), a data structure
            that describes the layout and contents of the SPI Flash. This
            structure is required by the Flashrom tool.

On an ARM Samsung Chromebook (Snow), the firmware is 4MB: 2 MB for RO firmware
and 2MB for RW firmware. On a Chromebook Pixel (Link), the firmware is 8 MB: 2
MB for coreboot and RO firmware (protected by the

write-protect line), 2 MB for Intel ME firmware (protected by Intel-specific
mechanisms), and 4 MB for RW firmware.

## Bundled Firmware

The cros_bundle_firmware tool is a Python script that uses a set of supplied
binary files and a description of the flashmap layout and creates a firmware
image. When you run the emerge Chrome OS bootimage command, the
cros_bundle_firmware tool runs twice: once to create a verified boot image and a
second time to create a nonverified boot image for development.

## Firmware Development

This section provides practical tips on firmware development.

### USB firmware download

Commonly, APs support booting from many devices--for example, eMMC, an SD card,
serial device. The AP can also act as a USB device to which a connected PC can
send the boot software. This technique is widely used in Chrome OS for firmware
development and test, since it allows the system to boot with the new software
in a few seconds.

Following a special procedure, you can reset the AP in a special mode where it
waits for the boot software to be sent over USB. x86 platforms typically do not
implement this feature, but it is commonly found on ARM processors. If
available, this is the primary method of loading firmware in the development
phase. Typical steps for loading firmware using a USB connection are as follows:

1.  Connect your workstation via USB to the Chrome OS AP.
2.  Press the magic button or special key combination on the PC to start
            the process. (This step can be automated on some platforms.)
3.  The AP waits for the boot loader to be sent over the USB connection.
4.  Type the command cros_write_firmware -w usb to send the firmware
            over the wire.
5.  The AP boots using the new software.

### Device Firmware Update (DFU)

Device Firmware Update (DFU) is a new feature in U-Boot that allows you to send
new firmware to a running U-Boot device over a USB cable. To enable this
feature, specify the following config options for the U-Boot driver:

*   CONFIG_CMD_DFU - turns on the command so that you can type it
*   CONFIG_DFU_FUNCTION - turns on the ability to accept the new
            firmware

This feature allows Chromebooks to update themselves while the Chromebook is
paused at a U-Boot prompt. It is also useful when the AP does not support USB
firmware download.

## Verified Boot

The first few steps are slightly different, depending on whether the system has
an ARM processor or an x86 processor.

### Startup Initialization

On ARM systems:

1.  An ARM system may have a chip-specific initialization routine. For
            example, The Samsung Exynos processor has a BL1 signed image.
2.  The Secondary Program Loaderder (SPL) sets up memory, loads U-Boot
            into memory, and runs it.

On x86 systems:

1.  The system runs Coreboot. This program functions similarly to the
            SPL on ARM systems.
2.  Coreboot sets up memory, then loads and runs U-Boot.

### U-Boot and Embedded Controller

U-Boot performs the device initialization for the system, as follows:

1.  U-Boot calls the VbInit() function to start verified boot.
2.  In the simplest case, the boot consists of one step: running the
            code located in the Read-Only (RO) section of the firmware. During
            this process, U-Boot checks whether there are any updates in the
            Read/Write (RW) section of the firmware. If updates are present,
            U-Boot loads and runs RW firmware.
3.  The main firmware performs a software sync that checks whether the
            EC code needs to be updated. If so, the update is sent over I2C,
            SPI, or LPC to the EC.
4.  Once the EC code has been sync’ed, execution jumps to the EC code in
            the RW firmware.
5.  The kernel is loaded and verified.
6.  The correct device tree file is selected for the system (ARM only).
7.  In addition, the kernel contains command line information that
            U-Boot picks up and passes to the kernel for use during boot. The
            command line tells the kernel which device to boot from (eMMC, SD,
            USB) and also contains the Verity parameters that are passed in to
            the kernel at boot time.
8.  The system boots the kernel, which uses the root hash (contained in
            the Verity parameters) to open the root filesystem.
9.  The system initializes user space and runs X and Chrome.

![Verified boot flow](/chromium-os/firmware-porting-guide/2-concepts/Verified%20Boot%20Flow.png)

### Recovery Mode

If the system encounters an error during verified boot--for example, the RW
image doesn’t verify, the kernel is corrupt, or the signatures don’t match, the
system enters recovery mode. The SPI Flash has a minimum version of the firmware
and kernel that be used to boot the system when normal boot mode fails. In
recovery mode, the system remains in the RO portion of the firmware and displays
a screen that asks the user to insert recovery media to recover the system. The
kernel and the root disk used for recovery are signed by Google.

### Developer Mode

A user can also select developer mode, which allows an unsigned kernel to be
loaded. In this mode, verified boot is turned off so that the user can load
experimental kernels for testing purposes.

## Keys and Signing

Cryptographic keys are inserted into the boot software by the signer program.
The signer inserts three types of keys:

*   Developer keys - public keys that anyone can use
*   Mass production (MP) keys - secret keys
*   Recovery keys - also secret and used by Google in recovery software
            images

**Code for the signer is found in the src/platform/vboot_reference directory.**

During verified boot, the following regions hold signing or hash data:

*   GBB (Google Binary Block) - contains a public key
*   VBlock - contains a public key for the kernel and a key for the
            firmware
*   Kernel command line - contains the verity hash

## Secondary Program Loader (SPL)

The secondary program loader (SPL) is used on ARM systems only. On x86 systems,
this bootstrapping function is performed by coreboot. Not all ARM platforms use
SPL, but its usage is becoming increasingly common.

SPL is a small program (about 10 to 30 Kbytes) that loads into the internal SRAM
of the AP. This program is loaded when the system starts to perform the
following initialization functions:

*   Set up basic clocks and power
*   Start memory running
*   Load U-Boot into SDRAM
*   Switch control to U-Boot code

The config variable `CONFIG_SPL_BUILD` is used in Makefiles and source files to
indicate whether a particular element should be included in the initial SPL
build. This approach enables the same set of source files to be used for both
the SPL build and the U-Boot build. The initial SPL setup should perform the
bare minimum needed before U-Boot takes control. For example, you would not need
to turn on the clock for the LCD or enable USB because those peripherals are not
used before U-Boot executes.

For more information on SPL and other config options for SPL, see the [SPL
README
file](http://git.denx.de/?p=u-boot.git;a=blob;f=doc/README.SPL;h=4e1cb28800411e0c7f626970bec2326baf45d49c;hb=HEAD).

## Flat Device Tree

**The flat device tree (FDT), or simply device tree, is a simple text file that
contains a data structure describing the hardware components of the system and
how they are connected. This file is compiled into binary format and passed to
the operating system at boot time. The device tree consists of a list of nodes,
each of which represents a device or a bus in the system. These nodes are
grouped hierarchically. (The tree is "flattened" when it is compiled.) The
top-level nodes contain subnodes that are attached to them in some way. For
example, a top-level node could be a peripheral device attached to a bus, and
the subnodes would be the devices connected to that bus.**

**The device tree file is located in the board/*vendor_name*/dts directory.**

### How Is the Device Tree Used?

**Both the kernel and U-Boot use the device tree as the basic *description* of the hardware components and layout. In addition, for Chrome OS, the device tree provides configuration information. The following three nodes provide configuration information to Chrome OS:**
**config - general configuration information**
**chromeos-config - includes information about verified boot**
**flash - specifies the flashmap**

### Sample Device Tree Nodes

Here is an example of nodes for the SPI flash and EC, taken from the device tree
for the Exynos5250 board:

`spi@131b0000 {`

` spi-max-frequency = <1000000>;`

` spi-deactivate-delay = <100>;`

` cros-ec@0 {`

` reg = <0>;`

` compatible = "google,cros-ec";`

` spi-max-frequency = <5000000>;`

` ec-interrupt = <&gpio 174 1>;`

` optimise-flash-write;`

` status = "disabled";`

` };`

`};`

The main node in the block delimited by this excerpt is the SPI peripheral
(labeled spi@131b0000). The braces following this node contain a list of
property/value pairs for the node, and the subnode, which is an additional
device attached to the first device. The spi@131b0000 contains one subnode, the
cros-ec, which has its own set of property/value pairs that describe its
parameters.

### Include Files for Flashmap and Other Config Information

The following include files are found in board/samsung/dts/exynos5250-snow.dts:

/dts-v1/;

/include/ "exynos5250.dtsi" - SoC file; includes nodes for all of the
peripherals in the SoC

/include/ "flashmap-exynos-ro.dtsi" - layout of the read-only portion of the
flashmap\*

/include/ "flashmap-4mb-rw.dtsi" - layout of the 4 MB read/write portion of the
flashmap\*

/include/ "chromeos-exynos.dtsi" - exynos-specific configuration for Chrome OS

/include/ "cros5250-common.dtsi" - common board information for Chrome OS-based
exynos5250 boards

\*For information on the format of the flashmap device tree, see
[cros/dts/bindings/flashmap.txt](https://chromium.googlesource.com/chromiumos/third_party/u-boot/+/d25429efd1b286223caa994796f3e8b7f041659b/cros/dts/bindings/flashmap.txt).

### Additional Resources

This section provides a simple overview of how Chrome OS uses the device tree.
For additional, detailed information on device trees, consult the following
resources:

*   Device tree spec
    *   <https://www.power.org/resources/downloads/Power_ePAPR_APPROVED_v1.0.pdf>
*   Grant Likely paper

    *   <http://lwn.net/Articles/414016/>
*   Useful video

    *   <http://www.youtube.com/watch?v=LSHo3weGFCU>
*   device-tree discuss mailing list
    *   <https://lists.ozlabs.org/listinfo/devicetree-discuss>
*   Motivation paper
    *   <http://ozlabs.org/~dgibson/papers/dtc-paper.pdf>
*   U-Boot fdt config README
    *   <http://git.denx.de/cgi-bin/gitweb.cgi?p=u-boot.git;a=blob;f=doc/README.fdt-control;h=85bda035043497d8d23aed8a436e24d630c60937;hb=HEAD>
*   Other web sites, including
    *   <http://elinux.org/Device_Trees>
    *   <http://devicetree.org/Main_Page>

## Environment

The default environment is defined in the board configuration file (for
example,` /include/configs/smdk5250.h`, which in turn includes `chromeos.h`).
This default environment is compiled into U-Boot and is used whenever Chrome OS
boots in verified mode. The environment consists of a list of name/value pairs
used to hold autoboot settings, selected input and output devices, boot scripts,
the kernel load address, and other basic settings.

The device tree contains a load-environment node that specifies whether the
system is running in verified (secure) or nonverified mode. For verified mode,
the load-environment variable has a value of 0, since the default environment is
used. For nonverified mode, the load-environment variable has a value of 1,
which allows a custom environment to be loaded if available (see next
paragraph).

A custom environment can be stored in the SPI flash, NAND flash, and MMC so that
it persists between boots. In nonverified mode, the most recently used
environment is stored and loaded from SPI flash. Nonverified mode, which enables
you to easily load a custom environment, is a useful feature during development,
since it allows you to boot from the network or from a USB SD card. U-Boot also
provides commands for setting and getting environment variables, which can be
accessed through scripts.

Other sections in *U-Boot Porting Guide:*

1. [Overview of the Porting
Process](/chromium-os/firmware-porting-guide/1-overview)

2. Concepts (this page)

3. [U-Boot Drivers](/chromium-os/firmware-porting-guide/u-boot-drivers)
