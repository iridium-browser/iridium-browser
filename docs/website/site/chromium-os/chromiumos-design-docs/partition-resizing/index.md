---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: partition-resizing
title: Partition Resizing
---

[TOC]

**Note: This is a proposal that has never shipped.**

## Abstract

Chromium OS will be able to resize partitions without erasing them, easing
delivery of system updates.

## Use case

A big new operating system release is on the way, and it needs more system space
than the current system partitions have. We can push an update that makes the
system partitions bigger (by shrinking the stateful partition); then when the
big new release arrives, enough space is available on the disk.

## Partition layout

A drive currently contains up to four partitions, in the following order:

1.  One partition for state resident on the drive, called the "stateful
            partition." Any space required in the stateful partition is
            pre-allocated by the system. For example, a large file will be
            allocated for the autoupdater to use to store downloaded updates.
2.  An optional swap partition.
3.  Two partitions for the root file system. For more information, see
            the [File
            System/Autoupdate](/chromium-os/chromiumos-design-docs/filesystem-autoupdate)
            document.

In the future, drives may be able to have more partitions, as needed.

## Procedure for resizing partitions

The following diagram shows all the steps that must occur to resize partitions
and leave them in the desired state.
Keep in mind that at any point the system may be unexpectedly rebooted, and the
OS must recover gracefully (in particular, it must not brick). Also, keep in
mind that the OS doesn't allow changing the contents or location of the
partition we booted into.
The following procedure assumes that at the start of the process, the system has
booted into the last partition on the disk. If, instead, the system is currently
booted into the second-to-last partition, then the partition-resizing process
usually waits to begin until the next update or reboot occurs naturally.
However, if the resizing is urgent, the system can copy the A partition into B
and then reboot before starting the illustrated procedure.
If there's a swap partition, it isn't resized.
Note that, at the end of the following process, the A and B partitions are
always the same size as each other.

![](/chromium-os/chromiumos-design-docs/partition-resizing/resize_partition.png)

## When does resizing occur?

Partition resizing is initiated by the system, not by the user.
Some notes:

*   The approach described in this document depends on ext4 being able
            to make the stateful partition smaller.
*   If the stateful partition contains a lot of data, it may not always
            be possible to resize it down to the desired size; in that case the
            system will have to delete state information before resizing.
*   The resizing process currently assumes that the machine can't be
            used while partitions are being resized. Therefore, during the time
            while the system is shrinking the stateful partition, the user will
            have to leave the machine turned on but won't be able to use it. The
            system will display a "working...please wait" message on the screen.
*   Open issue: Will ext4 support resizing partitions while the file
            system is mounted? If it does, then we may be able to allow the user
            to use the machine while resizing partitions.

Note that, to support encrypted home directories, we will already have to have a
"working...please wait" message on the screen at logout when we re-sparsify the
home directory. This would be an appropriate time to resize the stateful
partition file system if needed.
