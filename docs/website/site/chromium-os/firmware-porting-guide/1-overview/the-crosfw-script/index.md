---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/firmware-porting-guide
  - Firmware Overview and Porting Guide
- - /chromium-os/firmware-porting-guide/1-overview
  - 1. Overview of the Porting Process
page_name: the-crosfw-script
title: The crosfw script
---

## Firmware build/test script

crosfw is a python script which automates many of the tasks involved in building
and testing firmware. It includes the following features:

*   Build U-Boot for any board (Chrome OS or upstream)
*   Create a boot image with or without verified boot feature built in
*   Configure boot image to enable console output, secure more,
            read-write (two-stop) mode, etc.
*   Write firmware image to board using USB A-A, SD card or Dediprog
            EM100.
*   Optionally write a magic flasher to flash the image to SPI or MMC
*   Various other options like disassembly, image size, U-Boot execution
            trace support, and so on
*   For ARM and x86 it can be used outside the chroot if you have a
            suitable Linaro toolchain
*   Supports multiple connected [servo boards](/chromium-os/servo) (with
            USB A-A / EM100) and writing firmware automatically to the correct
            one

Best of all, crosfw does not spam you with 800KB of build output, thus making it
possible for you to see the warnings/errors.

crosfw is roughly parallel to using chromeos-u-boot and chromeos-bootimage and
these must have been preivously emerged in order to use crosfw.

## Why use the script?

*   Fast and reliable building, testing and flashing of firmware on any
            Chrome OS-supported device (snow, link, pit, spring, puppy)
*   Supports building without verified boot, and a 'small' image, which
            is must faster to load/flash than the full image - this means faster
            development when initially bringing up a platform

## How to use the script?

To build U-Boot with verified boot for pit, create and image and download over
USB A-A:

```none
$ crosfw -b peach_pit -V
```

Same, but download a magic flasher and flash the SPI

```none
$ crosfw -b peach_pit -VF
```

Mimic chromeos-bootimage and create and flash a secure image with silent
console:

```none
$ crosfw -b peach_pit -VFSC
```

You can build sandbox with:

```none
$ crosfw -b sandbox -w
```

You can also build any upstream config. The output files are in directory
/tmp/crosfw for easy access and you can set a variable in your ~/.crosfw file to
change this. You can also use -a to pass options to cros_bundle_firmware, and -h
to get help.

## Can I rely on this script for testing?

You can develop with this script, but before submitting CLs you must test with
the ebuild system. This script does not necessary track changes there

and so inconsistencies may develop over time.

WARNING: Always emerge and test chromeos-u-boot and chromeos-bootimage before
submitting U-Boot CLs to gerrit!
