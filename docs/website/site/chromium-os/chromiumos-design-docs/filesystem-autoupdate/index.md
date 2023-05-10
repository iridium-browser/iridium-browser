---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: filesystem-autoupdate
title: File System/Autoupdate
---

[TOC]

## Abstract

This document describes the Chromium OS file system and the autoupdate system.

*   We expect regular updates to Chromium OS to add new functionality,
            improve system performance, and enhance the user experience.
*   The autoupdate mechanism aims to provide seamless and secure updates
            to the latest version of Chromium OS. No user interaction is
            required.
*   Updates usually come in the form of deltas (i.e. only the parts of
            the system that changed are downloaded) which are downloaded to a
            backup boot partition. Upon reboot, the backup partition becomes the
            primary.
*   In case there is a problem with the update, the system can revert to
            using the previous partition.
*   One way in which we secure Chromium OS is by keeping the partition
            containing the OS and other system files read-only. All temporary
            user state (such as browser caches, cookies, etc) is stored on a
            separate partition.

## Goals

The autoupdate system has the following goals:

*   Updates are delta-compressed over the wire.
*   Delta updates are quick to apply.
*   Root file system is mounted read-only (another mount point is
            read/write for storing state).
*   Device can automatically revert to previous installed version of the
            OS if the newly installed OS fails to boot properly.
*   Updates are automatic and silent.
*   Updates should be small.
*   System protects users from security exploits.
*   System never requires more than one reboot for an update.
*   There's a clear division between the storage locations on the drive
            for system software and for user data.
*   The new update must be set bit-for-bit from the server, since we
            will be signing each physical block on the system partition. (For
            more information, see the [Verified
            Boot](/chromium-os/chromiumos-design-docs/verified-boot) design
            document.)

## Partitions

A drive currently contains at least these partitions:

*   One partition for state resident on the drive (user's home
            directory/Chromium profile, logs, etc.)—called the "stateful
            partition."
*   Two partitions for the root file system.

### Root file system

Only one of the two partitions designated for the root file system will be in
use at a given time. The other will be used for autoupdating and for a fallback
if the current partition fails to boot.
While a partition is in use as the boot partition, it's read-only until the next
boot. Not even the autoupdater will edit the currently-booted root file system.
We will mount the stateful partition read-write and use that for all state that
needs to be stored locally.

During autoupdate, we will write to the other system partition. Only the updater
(and apps running as root) will have access to that partition.

## **The update process**

### Successful boot

The update process relies partly on the concept of a "successful boot." At any
given point, we will be able to say one of the following things:

*   This system booted up correctly.
*   This system booted up incorrectly.
*   The system is currently attempting to boot, so we don't know yet.

We consider a boot successful if the updater process can successfully launch.
Once a system has booted successfully, we consider the other root partition to
be available for overwriting with an autoupdate.

### Limiting the number of boot attempts

An updated partition can attempt to boot only a limited number of times; if it
doesn't boot successfully after a couple of attempts, then the system goes back
to booting from the other partition. The number of attempts is limited as
follows:
When a partition has successfully been updated, it's assigned a
remaining_attempts value, currently 6. This value will be stored in the
partition table next to the bootable flag (there are unused bits in the GPT that
the boot loader can use for its own purposes). The boot loader will examine all
partitions in the system; if it finds any partition that has a
remaining_attempts value &gt; 0, it will decrement remaining_attempts and then
attempt to boot from that partition. If the boot fails, then this process
repeats.
If no partitions have a remaining_attempts value &gt; 0, the boot loader will
boot from a partition marked bootable, as a traditional boot loader would.

### Diagram

Here's a diagram of the boot process:

<img alt="image"
src="/chromium-os/chromiumos-design-docs/filesystem-autoupdate/ChromeOSFilesystemAutoupdate.png">

## Supplements

For a detailed design into how delta updates are generated and applied, see:
[Autoupdate Details](/chromium-os/chromiumos-design-docs/autoupdate-details).
For supplementary information about related material, see the
[Filesystem/Autoupdate
supplements](/chromium-os/chromiumos-design-docs/filesystem-autoupdate-supplements)
document.

## **Other notes**

*   All updates directly overwrite the new partition and require no
            temporary space.
*   We download the update over HTTPS, requiring a server certificate
            signed by the one Certification Authority that issues OS provider's
            SSL certificates.
*   Updates are hashed and checksummed. The update servers may sign
            updates as well.
