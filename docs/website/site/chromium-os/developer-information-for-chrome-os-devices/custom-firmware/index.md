---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: custom-firmware
title: Custom Firmware
---

[TOC]

The Chrome OS firmware is **always** verified as signed by Google. Developer
mode allows you to safely fiddle with the disk and operating system, but the
boot process depends on unmodified firmware. There is no provision for modifying
the firmware without physically taking the device apart. Modifying your firmware
can result in permanent damage.

This page talks about how to use your own custom firmware without damaging the
official firmware on your device, by pretending that your custom firmware is a
custom kernel.

## Firmware as kernel

The normal-mode [boot process](/chromium-os/chromiumos-design-docs/disk-format)
is roughly this: RO firmware verifies and launches the RW firmware, RW firmware
verifies and launches the kernel, the kernel verifies the rootfs as it's read
from the disk.

In developer mode, the RW firmware doesn't verify that the kernel is signed by
Google, just that it looks like a correctly signed kernel image. It doesn't
matter **who** signed it, so you can [build and boot your
own](/chromium-os/developer-guide) kernel and rootfs. On later models, you can
also enable booting a custom Chromium OS kernel from USB.

The ChromeOS firmware doesn't actually look at the kernel, it just looks for a
[ChromeOS-specific
header](/chromium-os/chromiumos-design-docs/verified-boot-data-structures) that
describes it. So if your custom kernel isn't actually a Linux kernel, but is a
custom build of U-Boot that was packaged as though it was a Chromium OS kernel,
the firmware will happily load it into RAM and jump to it. What happens next is
up to you.

### Warning

This has been known to work (more or less), but it's pretty annoying and
difficult to debug. Most of the time when something goes wrong, you just see the
scary boot screen, you press Ctrl-D or Ctrl-U to boot your magic image, and ...
the machine silently hangs or reboots. Sometimes it reboots into recovery mode
but power cycling restores it. Sometimes you have to run the recovery process to
restore it. You shouldn't be able to damage anything, but you can spend a lot of
time trying to get it right.

That said, the general process is fairly straightforward. Of course, it helps
**a lot** if you've [built and booted your own](/chromium-os/developer-guide)
Chromium OS image at least once so that you're familiar with the development
environment.

Google pushes frequent updates to the running Chrome OS system so that over time
it just gets better. However, since the initial portion of the BIOS is
read-only, only the read-write part of the BIOS can be updated and the
requirement that it interoperate with the RW portion limits the amount of change
that can be applied. In addition, updating BIOS can be a scary proposition
because uncaught bugs could require users to run the recovery process. That
means that significant changes to the BIOS are only made by introducing new
ChromeOS devices. As of this writing, there are three major BIOS designs in
existence, and each type loads and runs the kernel in different ways.

#### **H2C**

The first generation of Chromebooks (Cr-48, Alex, ZGB) uses a BIOS called "H2C".

*   **H2C is UEFI-based, but with a lot of stuff cut out to speed up the
            boot process.**
*   **The BIOS is a 64-bit executable, but the ChromeOS kernel is
            launched in 32-bit mode. Our kernel has to be specially modified to
            handle this difference.**
*   **The "bootloader stub" is a standard UEFI executable, with source
            in
            [src/third_party/bootstub/](http://git.chromium.org/gitweb/?p=chromiumos/third_party/bootstub.git).
            The purpose of this tiny executable is to**
    *   **Update the kernel cmdline to let the kernel find the correct
                rootfs partition.**
    *   **Obtain the memory map from the BIOS that the kernel will
                need.**
    *   **Switch CPU modes from 64-bit to 32-bit.**
    *   **Jump to the kernel entry point at 0x100000.**
*   **You can replace the bootloader stub with any other UEFI
            application (such as grub2), but it will be broken in two ways:**
    *   **The BIOS' console output functions are disabled, so you can't
                see any output.**
    *   **None of the UEFI-provided disk handles are valid, so you can't
                do any disk IO.**

#### **Coreboot**

The second generation x86 Chromebooks (Stumpy, Lumpy, Parrot) firmware is a
combination of Coreboot and U-Boot.

*   **Coreboot initializes the bare hardware, then invokes a
            "payload".**
*   **The payload is U-Boot, configured and customized to boot using our
            verified boot process without delay.**
*   **Coreboot and U-Boot are both 32-bit executables, as is the kernel
            entry code (even for 64-bit kernels).**
*   **U-Boot updates the kernel cmdline and zeropage table, before
            jumping directly to 0x100000. Any "bootstub" code is ignored.**

**The two nice things about the coreboot BIOS are:**

*   **U-Boot can decide at boot time to enable the console output if it
            needs to.**
*   **The handoff from BIOS to kernel is a simple 32-bit jump. No
            trampoline, no reliance on run-time-resident BIOS functions.**

#### **U-Boot**

The ARM-based Samsung Chromebook (Snow) uses U-Boot alone.

*   The read-only firmware consists of three parts:
    1.  BL1/PRE_BOOT is the chip-specific bootloader provided by
                Samsung.
    2.  BL2/SPL is a very tiny board-specific bootloader derived from
                U-Boot.
    3.  The RO U-Boot then handles the verification and loading of the
                RW firmware.
*   The read-write firmware is also U-Boot.
*   U-Boot passes all the kernel parameters through FDT.
*   The kernel is loaded in RAM at a different location from x86.
*   The kernel is wrapped as a FIT image, not a raw binary.

#### **Depthcharge**

The newest Chromebooks (Haswell and beyond) now use Depthcharge, which is
deployed as a payload from Coreboot effectively replacing U-Boot entirely.
Depthcharge removes reliance on U-Boot and allows for a lighter boot process
with on demand initialization of devices. There is a presentation on Depthcharge
available
[here](/chromium-os/2014-firmware-summit/ChromeOS%20firmware%20summit%20-%20Depthcharge.pdf).

### Producing a "kernel" to boot

**Step one** is to build your custom bootloader.

On x86 coreboot if you're modifying the Chrome OS U-Boot, that may be as simple
as changing `CONFIG_SYS_TEXT_BASE` to the expected kernel load address and
running `emerge chromeos-u-boot` (`cros_workon` first, of course). You'll
probably need to tweak a few other things to enable the vga output or provide an
interactive prompt.

For an ARM Chromebook, you can find detailed instructions on the [Using
nv-U-Boot on the Samsung ARM Chromebook](/system/errors/NodeNotFound) page.

**Step two** is to sign your binary like a Chromium OS kernel:

```none
echo blah > dummy.txt
vbutil_kernel --pack kernelpart.bin \
	--keyblock /usr/share/vboot/devkeys/kernel.keyblock \
	--signprivate /usr/share/vboot/devkeys/kernel_data_key.vbprivk \
	--version 1 \
	--vmlinuz ${MY_BINARY} \
	--bootloader dummy.txt \
	--config dummy.txt \
	--arch arm
KPART=$(pwd)/kernelpart.bin
```

Notes:

1.  **Your binary is specified with the **`--vmlinuz`** argument.**
2.  **We use a `dummy.txt` file for the bootloader and config file args,
            just so `vbutil_kernel` doesn't complain. (TODO: Seems to work
            without this. Is it still needed?)**
3.  **We specify `--arch arm` even if we're building for x86, because
            x86 kernels have a preamble that `vbutil_kernel` will try to remove
            and U-Boot doesn't have that.**
4.  **At some point we should modify `vbutil_kernel` to be more
            accommodating so we don't have to use these tricks. There's a
            [bug](http://code.google.com/p/chromium-os/issues/detail?id=23548)
            open for that, and I'll get to it Real Soon Now.**

**Step three** is to copy that image to the chromebook and try it out.

**chronos# cd /mnt/stateful_partition/**
**chronos# scp YOU@SOMEWHERE:/PATH/TO/kernelpart.bin .**

Install it into partition 4 (assuming we've booted from partition 2):

**chronos# dd if=kernelpart.bin of=/dev/sda4
chronos# cgpt add -i 4 -P 9 -T 1 -S 0 /dev/sda**

**Note that I'm making partition 4 a high priority, but marking it as
unsuccessful and with a tries count of 1. That way, when you reboot, you'll go
back to your previous (working) ChromeOS kernel in partition 2.**

Reboot and if all goes well, your code should run.

An alternate here is to enable USB boot (`crossystem dev_boot_usb=1`) and then
boot from an SD card / USB key that you built/burned with `cros build-image` /
image_to_usb. In that case you can just put your `${KPART}` straight into
partition 2 and boot it with Ctrl-U.

### **Known issues**

There probably are some. Even within a single type of firmware, each Chromebook
has a slightly different version. Testing is done and bugs are fixed right up
until final manufacturing, and development continues on the top of tree
constantly. The standard Linux kernel doesn't generally trust the BIOS very
much, so it often does a lot of its own hardware initialization even on Chrome
OS. That means that some firmware bugs may exist that are masked or cleaned up
by the kernel, where they won't be exposed by testing. Plus, there can be
dependencies between the RO firmware (Coreboot, especially) and the RW firmware
that a custom bootloader may not be aware of.

## I'm not afraid of damaging things

Well, okay then. Here's [a presentation from OSCON 2013](http://goo.gl/jsE8EE)
on how to build and install your own firmware, including the read-only parts.
Good luck.
