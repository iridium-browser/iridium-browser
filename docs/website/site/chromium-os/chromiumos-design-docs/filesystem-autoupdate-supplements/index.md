---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: filesystem-autoupdate-supplements
title: File System/Autoupdate Supplements
---

[TOC]

This document provides a set of brief Chromium OS design supplements, covering
areas related to the file system and autoupdates.

## Reinstallation

Reinstallation requires a recovery image (on a USB drive or SD card) to be
attached to the system. When the system boots, the user may force reinstallation
by forcing the firmware into recovery mode by pressing a recovery button. The
exact details of how the firmware accomplishes this are outside the scope of
this document.
The recovery image performs the actions described in the following diagram:

![](/chromium-os/chromiumos-design-docs/filesystem-autoupdate-supplements/recovery_image.png)

## Firmware updating

The firmware discussed in this document references a firmware designed by Google
for Chromium OS-based devices. We recommend that devices that support the Google
firmware use that firmware; that lets them take advantage of the boot time,
security, and recovery features discussed in the [Firmware Boot and
Recovery](/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery) and
[Verified Boot](/chromium-os/chromiumos-design-docs/verified-boot) documents.
The firmware is covered in more detail in the [Firmware Boot and
Recovery](/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery)
document, but here is how it will work from the autoupdate perspective.
We plan to generally update the firmware after successful boot of a new image
that contains new firmware to install, but we will be able to force installation
of new firmware before an update is applied if necessary.
The firmware contains a stub that can boot into either of two on-chip boot
loaders (A or B). The stub first tries to boot from A. If A's checksum fails, it
boots B. If B's checksum also fails, it goes into recovery mode.
The firmware updater is a userspace program that runs on system boot. It
performs the following actions:

![](/chromium-os/chromiumos-design-docs/filesystem-autoupdate-supplements/firmware_updater.png)

## Bit rot evasion (corruption on the hard drive)

Note: it's unclear how much we expect to be impacted by drive corruption.
In this scheme, we take advantage of the fact that we could have two identical
system partitions.
Note that this is an early design that may change over time based on feedback.

We use the extra bits in the partition table as follows. Note that we have 8
bits in each byte reserved for the bootable flag. This gives us 7 unused bits
per partition for the boot loader.

![](/chromium-os/chromiumos-design-docs/filesystem-autoupdate-supplements/partition_extra_bits.png)

0x80 Bootable flag: this partition can be booted. This bit being set implies
that it's a good partition to boot from; it's been vetted. 0x40 Tryboot flag:
this partition, which must be bootable, should be booted *this* time, in the
event of multiple bootable partitions being found. 0x20-0x04 Unused. 0x02-0x01
Attempt boot counter, also known as the "remaining_attempts" flag. This takes
precedence over bootable/tryboot. For more information, see the [File
System/Autoupdate](/chromium-os/chromiumos-design-docs/filesystem-autoupdate)
document.

The boot loader flow is then:

![](/chromium-os/chromiumos-design-docs/filesystem-autoupdate-supplements/bootloader_flow.png)

Open issue: We could use a third system partition. This comes at a cost of
space, but lets us *always* have a backup partition ready, even midway through
an autoupdate. If a system partition is 200MB, we might do this. If it's 1 GB,
we might not want to.

## Stacking updates

Updates will be stackable. In other words:

Updates can be applied, one after another as they come out, without a reboot.
So, for example, if you boot into version n, then update the other partition to
n+1 but don't reboot, you're still running n. Then, if n+2 comes out, we will
update from n+1 to n+2 in the other partition, while remaining booted into n.

## Forced reboot

We will be able to force a reboot should we need to apply an emergency security
update.
