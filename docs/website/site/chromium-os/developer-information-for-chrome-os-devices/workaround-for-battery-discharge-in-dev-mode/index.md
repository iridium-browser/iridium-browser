---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: workaround-for-battery-discharge-in-dev-mode
title: Workaround for battery discharge in dev-mode
---

[TOC]

## Introduction

[Issue 362105](https://crbug.com/362105) describes a painfully annoying problem
for people who put their Chromebooks in dev-mode so they can use them more like
a standard laptop. The tl;dr version is that if you've completely replaced or
disabled Chrome OS and the battery runs all the way down, it forgets that it was
configured to allow non-Chrome OS images to boot, locking you out of your own
device. If this happens to you, here's how to get back in.

Note: If your machine boots up in Chrome OS, you probably haven't lost anything:

> If you're no longer in dev-mode, enter it again and reboot

> Go into crosh (CTRL-ALT-T)

> Type "shell" to get a shell.

> run sudo crossystem dev_boot_legacy=1

> reboot

> press CTRL-L when you see the scary boot screen

If your machine **doesn't** boot Chrome OS and only gives you the "Chrome OS is
missing or damaged" screen with every boot, then the following may help.

This process is long and annoying, but it's better than losing all your data.

## Impacted Devices

The issue has been resolved in newer devices, but most likely will not be
backported to older ones. Here are all the devices impacted (the project names
are listed; see the [Developer Information for Chrome OS
Devices](/chromium-os/developer-information-for-chrome-os-devices) page for
product names):

*   mario & alex & zgb
*   stumpy & lumpy
*   snow
*   parrot
*   stout
*   butterfly
*   link
*   spring & skate
*   peppy & pepto
*   falco & wolf & leon
*   panther & monroe
*   squawks & quawks & clapper & swanky & kip
*   zako
*   big & blaze
*   veyron

## How to recover

These instructions assume you have another Linux machine to do this on, because
1) I know that works, and 2) I don't have any Mac or Windows machines.

Here's what we're going to do:

### Stay in developer mode

If your Linux data is in a separate disk partition that you've created or taken
over, you still may be okay. But best not to risk it. Certainly **don't** go
through the recovery process, because that will wipe the disk.

### Download a recovery image

If you go to
[www.google.com/chromeos/recovery](http://www.google.com/chromeos/recovery),
you'll find instructions for running a Linux script that will download and
format a USB stick for you. Follow them, but **don't** remove the USB stick
afterwards, and **really** don't insert it in your Chromebook. \[Update: okay,
you used to find instructions there, but they're gone. Look
[here](https://support.google.com/chromebook/answer/6002417) instead. I've filed
a bug.\]

Note: My USB stick shows up as `/dev/sdc`. If yours is something else, use that
instead in the examples below.

### Modify the recovery image so we can mount it

The root filesystem on the recovery image is marked with some unsupported ext2
flags, so that if you try to mount it you'll see a message like this:

<pre><code>$ <b>sudo mount /dev/sdc3 /mnt</b>
mount: wrong fs type, bad option, bad superblock on /dev/loop0,
missing codepage or helper program, or other error
In some cases useful info is found in syslog - try
dmesg | tail  or so
Exit code 32
$
</code></pre>

We need to clear those flag bits so we can make changes to the filesystem. To do
this, we need to know the offset (in bytes) of the beginning of the rootfs
partition. You can use the GNU parted tool to find this out:

<pre><code>
$ <b>sudo parted /dev/sdc</b>
GNU Parted 2.3
Using /dev/sdc
Welcome to GNU Parted! Type 'help' to view a list of commands.
(parted) <b>unit B</b>                                                           
(parted) <b>print</b>                                                            
Error: The backup GPT table is not at the end of the disk, as it should be.
This might mean that another operating system believes the disk is smaller.
Fix, by moving the backup to the end (and removing the old backup)?
Fix/Ignore/Cancel? <b>I</b>                                                      
Warning: Not all of the space available to /dev/sdc appears to be used, you can
fix the GPT to use all of the space (an extra 3115136 blocks) or continue with
the current setting? 
Fix/Ignore? <b>I</b>                                                             
Model: Kingston DT 100 G2 (scsi)
Disk /dev/sdc: 3926949888B
Sector size (logical/physical): 512B/512B
Partition Table: gpt
Number  Start        End          Size         File system  Name        Flags
11      32768B       8421375B     8388608B                  RWFW
 6      8421376B     8421887B     512B                      KERN-C
 7      8421888B     8422399B     512B                      ROOT-C
 9      8422400B     8422911B     512B                      reserved
10      8422912B     8423423B     512B                      reserved
 2      10485760B    27262975B    16777216B                 KERN-A
 4      27262976B    44040191B    16777216B                 KERN-B
 8      44040192B    882900991B   838860800B   ext4         OEM
12      950009856B   966787071B   16777216B    fat16        EFI-SYSTEM  boot
 5      966787072B   968884223B   2097152B                  ROOT-B
 3      968884224B   2269118463B  1300234240B  ext2         ROOT-A
 1      2269118464B  2315255807B  46137344B    ext2         STATE
(parted) <b>quit</b>                                                             
$
</code></pre>

Note that we do **not** want to repair any of the GPT headers. Just answer "I"
(ignore) when asked about them.

The rootfs partition is partition 3, which begins 968884224 bytes into the disk.
That's the magic number we need to remember. (We could just refer to /dev/sdc3
with an offset of 0, but this process works on binary images too.)

This script will modify the ext2 flags bits in the rootfs filesystem header so
we can mount it. Copy it into a file and mark it as executable.

```none
#!/bin/bash
is_ext2() {
  local rootfs="$1"
  local offset="${2-0}"
  # Make sure we're checking an ext2 image
  local sb_magic_offset=$((0x438))
  local sb_value=$(sudo dd if="$rootfs" skip=$((offset + sb_magic_offset)) \
                   count=2 bs=1 2>/dev/null)
  local expected_sb_value=$(printf '\123\357')
  if [ "$sb_value" = "$expected_sb_value" ]; then
    return 0
  fi
  return 1
}
enable_rw_mount() {
  local rootfs="$1"
  local offset="${2-0}"
  # Make sure we're checking an ext2 image
  if ! is_ext2 "$rootfs" $offset; then
    echo "enable_rw_mount called on non-ext2 filesystem: $rootfs $offset" 1>&2
    return 1
  fi
  local ro_compat_offset=$((0x464 + 3))  # Set 'highest' byte
  # Dash can't do echo -ne, but it can do printf "\NNN"
  # We could use /dev/zero here, but this matches what would be
  # needed for disable_rw_mount (printf '\377').
  printf '\000' |
    sudo dd of="$rootfs" seek=$((offset + ro_compat_offset)) \
            conv=notrunc count=1 bs=1 2>/dev/null
}
[ -n "$2" ] || exit 1
enable_rw_mount $1 $2
```

Provide the USB device and the byte offset as arguments to the script, and now
we can mount the rootfs from the recovery image:

<pre><code>
$ <b>sudo ./enable_rw_mount.sh /dev/sdc 968884224</b>
$ <b>sudo mount /dev/sdc3 /mnt</b>
$ <b>ls /mnt</b>
bin/     dev/  home/  lib64/       media/  opt/       proc/  sbin/  tmp/  var/
debugd/  etc/  lib/   lost+found/  mnt/    postinst@  root/  sys/   usr/ 
</code></pre>

### Change the chromeos-install script to just fix the flags

The recovery image would normally run the `/usr/sbin/chromeos-install` script,
which would wipe the SSD and restore everything. We don't want that, so we'll
just modify that script to restore the dev-mode flags to allow our custom image
to boot again.

Create the script we want:

<pre><code>
$ <b>cat &gt; /tmp/foo.txt</b>
#!/bin/bash
crossystem dev_boot_legacy=1 dev_boot_usb=1 
sleep 5
exit
<b>^D</b>
$
</code></pre>

Replace the original script:

<pre><code>
$ <b>sudo cp /tmp/foo.txt /mnt/usr/sbin/chromeos-install</b> 
$ <b>ls -l</b> <b>/mnt/usr/sbin/chromeos-install</b>
-rwxr-xr-x 1 root root 76 Jun 16 16:44 /mnt/usr/sbin/chromeos-install*
$ <b>cat</b> <b>/mnt/usr/sbin/chromeos-install</b>
#!/bin/bash
crossystem dev_boot_legacy=1 dev_boot_usb=1
sleep 5
exit
$
</code></pre>

Note that it's executable, and has the content we want. Unmount the USB stick.

<pre><code>
$ <b>sudo umount /mnt</b>
</code></pre>

### Recover the dev-mode flags

Now you can insert this recovery image into your Chromebook. When it goes
through the normal "Verifying the integrity of your recovery media" process, it
may complain that the rootfs is not verified, but it will continue on anyway
because we're in developer mode. It will then proceed to the "System recovery"
step, but don't panic - it's going to run your modified script instead. That
should only take a few seconds. Once it completes, remove the USB stick and when
it reboots, your Ctrl-L and Ctrl-U shortcuts should work again.
