---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: firmware-porting-guide
title: Firmware Overview and Porting Guide
---

*4/11/2013*

## Firmware Overview

When you turn on an embedded device such as a Chromebook, the firmware is the
first software that runs. It sets up the machine so that it can load the
operating system and then transfers control to the OS. This general pattern
applies to any architecture, but there are differences depending on the exact
system architecture. For example, on ARM SoCs, it is common to have a boot ROM
within the SoC that is capable of booting from various media and may set up
basic clocks and peripherals.

On x86 systems, only the boot system is in SPI flash, and the firmware must deal
with every aspect of setting up the machine. Typically, a first stage firmware
such as Coreboot (not covered in this guide) is used on x86 to deal with these
complexities. On x86, part of the firmware stays resident even after the OS
runs, whereas on ARM and most other architectures, the firmware is no longer in
memory once control transfers to the OS.

### Firmware Tasks on Chrome OS

The firmware sets up the basic peripherals such as the keyboard, display, mass
storage, security chip (TPM), and SPI flash for its own use while it runs the
steps to set up the machine. In Chrome OS, the firmware can be updated, so the
system first checks whether an update exists and, if so, it loads the update and
jumps to execute it. In normal cases, the OS is available and is loaded and
started. If a problem exists (for example, the OS is corrupt), the firmware
enters recovery mode, which allows the user to insert media with a new OS. In
developer mode, users can bypass Chrome OS security, or even the entire OS.

### U-Boot

U-Boot (Universal Boot) is a useful basis for firmware because it has wide
architecture support and contains many drivers and subsystems. Chrome OS uses an
upstream version of U-Boot and adds its own verified boot infrastructure, which
is linked to U-Boot. Many of the drivers and features in U-Boot are used by
verified boot. Some of the advantages of U-Boot are

*   It is a relatively simple boot loader that is easy to modify and
            adjust to various needs.
*   It is popular on Power PC and widely available on ARM.
*   It includes a comprehensive command-line interface and environment
            system that allows for persistent state across reboots.
*   Supports a device tree structure that accommodates hardware
            differences
*   It supports most common files systems, USB, SPI, I2C, displays,
            SD/MMC, SCSI, and NAND flash.
*   It has a large upstream community and active mailing list.

U-Boot was started in 2002 by Wolfgang Denk, who based his work on previous
Power PC development (PPCBoot) by Magnus Damm. Wolfgang Denk is the primary
maintainer of U-Boot source code (see
[www.denx.de](http://www.denx.de/wiki/U-Boot)). U-Boot is available in the Git
tree. Patchwork is used to manage patches to the tree. All development is done
in patches sent to the [u-boot mailing
list](http://lists.denx.de/mailman/listinfo/u-boot).

## U-Boot Porting Guide

This guide describes how to port U-Boot to a new Chrome OS platform. Although it
specifically describes using U-Boot with Chrome OS, the general process,
information, and examples apply to using U-Boot with any Chromium OS. The guide
provides a conceptual framework for U-Boot porting tasks as well as
task-oriented guidance for the porting process.

1. **[Overview of the Porting
Process](/chromium-os/firmware-porting-guide/1-overview)**

*   High-level view of the porting process
*   Development flow

2. **[Concepts](/chromium-os/firmware-porting-guide/2-concepts)**

*   Boot process
*   Verified boot
*   Development flow
*   Firmware image
*   Firmware development
    *   USB firmware download
    *   Device Firmware Update (DFU) - over the air
*   Bundled firmware
*   Secondary program loader (SPL)
*   Flat device tree
*   Keys and signing
*   Environment

3. **[Drivers for Chrome
OS](/chromium-os/firmware-porting-guide/u-boot-drivers)**

*   Audio codec
*   Clock
*   Ethernet
*   GPIO (General Purpose Input/Output)
*   I2C (Inter-IC Communications)
*   I2S (Inter-IC Sound)
*   Keyboard
*   LCD (Liquid Crystal Display)
*   NAND (alternative to SDMMC if supported)
*   Pinmux (Pin Multiplexing)
*   Power
*   PWM (Pulse Width Modulation)
*   SDMMC (Secure Digital Multimedia Memory Card) and eMMC
*   SPI
*   SPI Flash (Serial Peripheral Interface Flash)
*   Timer
*   TMU (Thermal Management Unit)
*   TPM (Trusted Platform Module)
*   UART (Universal Asynchronous Receiver Transmitter)
*   USB host
*   DSTREAM/ DS-5 - debug tools
*   ICE/Trace - debug tools

**Appendix A.** [Using nv-U-Boot on the Samsung
Chromebook](/system/errors/NodeNotFound)
