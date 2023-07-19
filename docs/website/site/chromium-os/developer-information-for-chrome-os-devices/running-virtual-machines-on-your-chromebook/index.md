---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: running-virtual-machines-on-your-chromebook
title: Running virtual machines on your Chromebook
---

[TOC]

## Introduction

The Chromebooks with Intel processors are fast. I've replaced my Macbook Air
with a Chromebook, and run the standard Chrome OS software on VT01, and virtual
machines on VT02. I have booted both Windows and different versions of Linux and
the 9front version of Plan 9.

I currently use a custom build of Qemu. It's a bit hard to get Qemu built in the
Chrome OS build system at present, so I've got a directory containing Qemu, its
libraries and BIOS files, and scripts to chroot to that directory and run Qemu.
Access to devices, where needed, is provided via bind-mounts. The setup sounds a
bit kludgy but works well for me; nevertheless, we welcome improvements. What
we'd most prefer is to get this patch series into Chrome OS, so we have qemu as
part of a "real" build.

<https://gerrit.chromium.org/gerrit/#/c/15445/>

FWIW, this particular instance of qemu was built on arch Linux, lost, sadly,
when my Air was stolen.

### Background

The firmware on Chrome OS devices will clear the VMX bits during boot. This
means that support is disabled, but it is not locked such that runtime cannot
change things. This keeps things secure during initial boot, but doesn't lock
out people from enabling things themselves in the kernel. Otherwise, they'd have
to resort to modifying the firmware and that's always a tricky proposition (make
a mistake and you have a brick).

When the Chrome OS kernel boots up, it will look for the `disablevmx=[on|off]`
option on the kernel command line. If it is set to `off`, then VMX support will
be enabled. For all other situations, we disable VMX and lock the bits so they
cannot be turned back on. This keeps the system secure.

Current Chrome OS systems all ship with KVM disabled. That means you need to
currently build a custom kernel yourself in order to get KVM support.

### Board Specific Notes

Be aware that on earlier Chrome OS devices, the firmware contained bugs such
that they locked VMX support during power on. It's known to affect:

*   [Series 5
            Chromebook](/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook)
*   \*[Samsung Series 5
            550](/chromium-os/developer-information-for-chrome-os-devices/samsung-sandy-bridge)
*   \*[Samsung Series 3
            Chromebox](/chromium-os/developer-information-for-chrome-os-devices/samsung-sandy-bridge)

For devices marked with a \*, you might be able to restore support by hacking
the firmware. Please see the respective device pages for more details.

## Building Chromium OS w/KVM

To start, you're going to need an image that has the KVM modules. You should
update your sources, then build an image with (at minimum) the USE=kvm option,
viz:

```bash
USE=kvm cros build-image --board=lumpy --no-enable-rootfs-verification \
    --boot-args 'disablevmx=off lsm.module_locking=0'
```

Googlers: I have USB sticks that you can use for this install. Come see me if
you want one. Sorry, can't hand these out yet :-(

Install this image in your favorite manner, either via update engine or usb
stick. Boot the stick as usual.

## Enabling VMX Support

The magic kernel command line option is `disablevmx`. So you want to add
`disablevmx=off` to the kernel command line.

Log in as root.
mount -o remount /
/usr/share/kernel/use_kvm.sh

to test:
modprobe kvm_intel

This will almost certainly get an error. There are a few more steps to make sure
virtual machines can be used.
/usr/share/vboot/bin/make_dev_ssd.sh --save_config /tmp/x

Edit this config and add the line
disablevmx=off lsm.module_locking=0

to the command line. Then
/usr/share/vboot/bin/make_dev_ssd.sh --set_config /tmp/x

Then comes the interesting part. On the laptops, you have to hard-disconnect the
battery. On samsung, you do this by putting a paperclip into the hole on the
underside of the trackpad.

Once that's done, you're going to need to pull down two files:

<https://docs.google.com/open?id=0By47TDljmWaSWEttVGNBbVUwMEU>, which is the
qemu and other bits.

cd to /usr/local; mkdir kvm; cd there and untar this file into it. This creates
a directory called qroot.

Next, you need a virtual machine image; I've set an example up at
<https://docs.google.com/open?id=0By47TDljmWaSX2N6VlpkS21Pd3c>. Uncompress this
in /usr/local/kvm (NOT the qroot directory; the one above it).

Now cd /usr/local/kvm/qroot, and

sh Linux

and it might just work. Please let rminnich@chromium.org know about bugs.

## Checking VMX Support on Unofficial Hardware

If you are trying to run Chromium OS on your own hardware (i.e. not a
Chromebook/Chromebox), you should make sure your system is properly configured
first.

### CPU Support

Make sure your CPU has support for the Intel VMX extensions. Simply look at
/proc/cpuinfo to see if it has the vmx flag:

<pre><code>$ grep '^flags\s*:.* vmx ' /proc/cpuinfo
flags           : ... <b>vmx</b> <b>smx</b> ...
</code></pre>

If you don't, then sorry, but your CPU doesn't support VMX extensions.

### BIOS Settings

Most BIOSs today have an option to enable/disable VMX support at boot, and then
lock any further modifications. They often times default to disabling the VMX
extensions.

You can check at runtime by using the `rdmsr` command from the iotools package:

```none
$ sudo modprobe msr
$ sudo iotools rdmsr 0 0x3a
0x0000000000000001
```

You only care about the lower 3 bits. An explanation of the first few bits:

<table>
<tr>
<td> Bit</td>
<td> Meaning</td>
</tr>
<tr>
<td> 0</td>
<td> Settings are locked</td>
</tr>
<tr>
<td> 1</td>
<td> VMX Extensions</td>
</tr>
<tr>
<td> 2</td>
<td> SMX Extensions </td>
</tr>
</table>

Thus, if the last digit in the output is "1" (or much less unlikely, "8"), your
BIOS has disabled VMX support and locked further modification. You will need to
reboot into your BIOS, find the option, and enable it. Look for the word
"virtualization".

#### Using kvm-ok Helper

The latest versions of QEMU/KVM include a tool called `kvm-ok` which is designed
to perform various sanity checks on the system and see if things will work.
Simply install it (note: it's often included in the "kvm" package in your
distro) and run it:

```none
$ kvm-ok
```
