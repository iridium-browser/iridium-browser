---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: helper-scripts
title: Helper Scripts
---

[TOC]

This page offers some documentation to extra scripts provided in the src/scripts
directory

## cleanup_loopback_devices

We create a lot of loopback devices in various scripts and there are conditions
where we may leave around some loopback devices. This script provides a
convenient way of unmounting any image directories in your chroot (for your
board) and cleaning up errant loopback devices. Note, this script will not
affect loopback devices / mounts in other chroots or outside the chroot.

## gmergefs

gmergefs is an emerge wrapper that combines sshfs with emerge. It allows a
developer to add a package to a live Chromium OS system using the developer's
development system. The easiest way to use it is to pass a --remote flag that
specifies the IP address of the remote device and pass the name of a package to
install. You will also have to specify the board if you don't have a default one
set up.

gmergefs requires that the host machine have ssh access to the root account of
the remote machine and will prompt for this password when run (will need to edit
sshd_config in /etc/ssh and sudo passwd to change root password). Test images by
default have this enabled (see test section for password). Users of non-test
builds will need to enable ssh access for the root account and set a root
password.

gmergefs also requires you have fuse installed on your host machine (if you have
fuse warnings when entering chroot, it will not work).

*Note: By default gmergefs tries to install packages onto /usr/local of the
remote system. If you are updating packages on the root fs you will need to pass
--noonstatefuldev.*

The following are some sample use cases:

*   **Update base packages**

    ./gmergefs --noonstatefuldev --remote 192.0.0.100 chromeos-base/chromeos

*   **Update developer packages**

    ./gmergefs --remote 192.0.0.100 chromeos-base/chromeos-dev

*   **Add a developer package**

    gmergefs --remote 192.0.0.100 pacakge_name

### Troubleshooting

*gmergefs hangs*: This is caused by sshfs hanging when mounting. Check that you
can ssh as root from within your chroot i.e. ssh -o StrictHostKeyChecking=no
root@your_ip. Also double check that you typed in the ip address correctly in
the remote flag.

*gmergefs has trouble writing to either ld.so.cache or other file using
--noonstatefuldev*: gmergefs doesn't automatically remount your root dir to rw.
Go onto your machine and run sudo mount -o remount,rw /

## mount_gpt_image.sh

The output of the Chromium OS build process is a bootable binary gpt bin file
that you can dd straight onto a usb key and install onto a device. However,
often developers will want to make some modifications after the build process or
make changes once an image is already on a usb key from the developer's
development machine. This script makes it easy to do just that. This script will
mount a bin or device specified in the --from parameter (e.g. /dev/sdc or
../build/images/.../.../chromiumos_image.bin) and mount the whole image under
/tmp/m. Passing the same arguments in addition to -u (--unmount) will reverse
the process. This can be used to emerge new packages onto an already created
image as well as make small mod's or to just see what is in the image. See
mount_gpt_image.sh --help for more information.

## wtf

Displays summary status for every git repository that is in a modified state.
Helpful to easily see what changes you have across the entire source tree. This
tool is found in depot_tools.
