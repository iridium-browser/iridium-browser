---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
- - /chromium-os/how-tos-and-troubleshooting/debugging-tips
  - Chromium on Chromium OS Debugging Tips
page_name: host-file-access
title: Host File Access
---

[TOC]

Getting access to files on your host from the target over a network is very
useful. This page lists the ways that this is possible.

## USB stick (sneakernet)

You can put files (typically a full ChromiumOS image build with
`cros build-image`) onto a USB stick, insert it into the target, mount the
device and access the files from there.

## Using sshfs

Since you can use ssh in user space, and we have a 'fuse' module to permit user
space filesystems, we can use sshfs to mount directories:

<pre><code>
# Make sure fuse is loaded
sudo modprobe fuse
# Create location to mount build directory
mkdir /tmp/chrome
# Mount your dev directory
sshfs <i>username</i>@<i>machine</i>:<i>/path/to/chrome</i> /tmp/chrome
# Take a look
ls /tmp/chrome
</code></pre>

You can hope for about 5MB/s over sshfs on a Cortex-A9 with USB Ethernet.

## Using NFS

[Please see
here](/chromium-os/how-tos-and-troubleshooting/network-based-development) for
full details.
NFS requires a lot more setup but has additional features. There are two ways to
use NFS from your target:

*   NFS root - use an NFS mount as your root file system
*   NFS mount - boot as normal from a USB stick / SSD / eMMC, but then
            later mount a directory on your host machine

Each option has advantages and disadvantages which will be noted below. Both
require a certain amount of setup on both the host and target. In NFS terms the
host will be the **server** (containing the files which we want to export) and
the target will be the **client**.

Transfer speed with NFS is faster than sshfs since CPU overhead is less, but is
limited by the USB interface if you are using a 10/100 USB Ethernet dongle. With
a Gigabit USB3 dongle it should operate much faster than ssh.

## NFS Mount

It is also possible to boot from USB or eMMC, but then later mount a filesystem
on your host. This requires some user space utilities to enable portmap, etc.
Please add documentation here if you do this.
