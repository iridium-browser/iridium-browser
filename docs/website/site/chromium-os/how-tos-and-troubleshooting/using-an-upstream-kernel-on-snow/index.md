---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: using-an-upstream-kernel-on-snow
title: Using an Upstream Kernel on Chrome OS
---

[TOC]

## **Introduction**

This page aims to document how to use an Upstream Kernel on Chrome OS. It's
aimed at helping people with upstreaming for the [Samsung ARM Series 3
Chromebook](http://www.samsung.com/us/computer/chrome-os-devices/XE303C12-A01US)
(AKA snow) and uses that in many examples. However, it's useful in other cases,
too.

In general this documentation assumes that you've got the Chrome OS source tree
synced down and have a build environment (and chroot) setup. It assumes you've
built packages and an image for your board (like `BOARD=daisy` for the snow
board). See the [Developer Guide](/chromium-os/developer-guide) if you haven't
done that.

## Obtain source code and recent patches

The whole point here is to use the upstream kernel, so these instructions will
direct you to linux-next. ...but linux-next is pretty darn bleeding edge. If you
get stuck and things won't boot, you could always try a known-good state. In
fact, many of the needed patches have now landed on even landed on the main
linux tree now. See the section below "Patches relevant to the Samsung ARM
Chromebook" for the last known good state.

The easiest way to get source code is to just add a remote git server to your
normal kernel tree, fetch things, and then use patchwork to apply some patches.

Here's some steps to do that and start a new work branch on linux-next. WARNING:
The fetch might be slow.

```none
cd ~/trunk/src/third_party/kernel/files
git remote add linuxnext git://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git
git fetch linuxnext
git checkout -b my_branch linuxnext/master
```

## Installing pwclient

Now that you've got the source code you should apply a few patches. In this case
we'll use `pwclient` to get things out of
[patchwork.kernel.org](http://patchwork.kernel.org).

First we need to install `pwclient`.

One way to install the patchwork client in the cros chroot is via its ebuild:

```none
sudo emerge pwclient
```

Alternatively, you can install the latest pwclient directly from kernel.org:

```none
sudo bash -c "curl 'https://patchwork.kernel.org/pwclient/' > /usr/local/bin/pwclient"
sudo chmod +x /usr/local/bin/pwclient
```

Next, setup the `.pwmclientrc` for the LKML project on patchwork.kernel.org:

<pre><code>
<b># The following will clobber any existing ~/.pwclientrc you have (!)</b>
curl 'https://patchwork.kernel.org/project/LKML/pwclientrc/' &gt; ~/.pwclientrc
</code></pre>

## Patches relevant to the Samsung ARM Chromebook

This section should go away eventually once patches are upstream...

Relevant patch series that I've tested (as of 12/19/12)

*   [SD card write protect
            patches](http://patchwork.kernel.org/bundle/dianders/mmc_wp/)
            **(required)**
*   [i2c bus numbering
            patches](http://patchwork.kernel.org/bundle/dianders/i2c_bus_numbering/)
*   [max77686 basic support in device
            tree](http://patchwork.kernel.org/bundle/dianders/snow_max77686/)
            (misses RTC + 32KHz clocks)

Quick way to apply:

```none
for id in 1966801 1966931 1966871 1966781 1966811; do
  pwclient git-am $id
done
pwclient git-am 1966911
pwclient git-am 1840211
```

Status on linux/master:

*   Tested on 12/18 `ae664dba2724e59ddd66291b895f7370e28b9a7a: Merge
            branch 'slab/for-linus' of
            git://git.kernel.org/pub/scm/linux/kernel/git/penberg/linux`
    *   Needed revert of `1974a042dd15f1f007a3a1a2dd7a23ca0e42c01d`. Fix
                is in discussion upstream.
    *   Applied patches (1823551 1823621 1823561 1823591 1781931 1840211
                1894081)
    *   Boots to command line
*   Tested on 12/19 `74779e22261172ea728b989310f6ecc991b57d62: Merge tag
            'for-3.8-rc1' of git://gitorious.org/linux-pwm/linux-pwm`
    *   Needed revert of `1974a042dd15f1f007a3a1a2dd7a23ca0e42c01d`. Fix
                is in discussion upstream.
    *   Applied patches (1823551 1823621 1823561 1823591 1781931
                1840211)
    *   To fix DEPMOD step when doing emerge-daisy chromeos-kernel-3_8:
                Apply patch 2794701
    *   Boots to command line

Status on linux-next/master:

*   Tested on 12/6: cfc4410f5d3de0f68139ddffe065b9889a41d3c0`: Add
            linux-next specific files for 20121206`
    *   Applied above patches (1823551 1823621 1823561 1823591 1781931
                1840211)
    *   Boots to command line
*   Tested on 12/18: `ff430f8aa011716b8274381e31b0bc8d49d4f4be: Add
            linux-next specific files for 20121218`
    *   Applied patches (1823551 1823621 1823561 1823591 1781931 1840211
                1894081)
    *   Boots to command line

## Building and installing the kernel

Certainly there are lots of ways to build and install the kernel. I'm not going
to copy them all here but I'll just point you at the [Kernel
FAQ](/chromium-os/how-tos-and-troubleshooting/kernel-faq). Specifically read the
[How to quickly test kernel modifications (the fast
way)](http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/kernel-faq#TOC-How-to-quickly-test-kernel-modifications-the-fast-way-)
section carefully.

I would strongly suggest that you boot from a fast SD card rather than directly
messing with what you have on eMMC. That way you can always get back to a
running system. The quick set of steps for that is:

*   Switch to [developer
            mode](/chromium-os/developer-information-for-chrome-os-devices/samsung-arm-chromebook).
*   At the command prompt (VT2), enable USB (and SD card) booting with `crossystem dev_boot_usb=1`.
    Note: the exact method might differ between boards; see the [Developer
    Information for Chrome OS Devices
    page](/chromium-os/developer-information-for-chrome-os-devices) for specific
    details.
*   Insert an SD card with an image that you installed with `[cros
            flash](/chromium-os/build/cros-flash)`.
*   Reboot your Chromebook and press `Ctrl-U` at the BIOS prompt to boot
            from your SD card.

You might also want to look at [Using nv-U-Boot on the Samsung ARM
Chromebook](/system/errors/NodeNotFound).

## The kernel config

One especially weird thing when working with the upstream kernel is that
standard ChromeOS kernel `splitconfig` system isn't in place. The kernel ebuild
used by ChromeOS will realize that and will pick up a backup config from
[chromiumos-overlay](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/eclass/cros-kernel).
On snow it will pick up the `exynos5_defconfig` in particular.

If you need to make changes to the kernel config locally the easiest thing to do
is to just override that config. Here's a set of commands that might suit you:

```none
DEFCONFIG=~/trunk/src/third_party/chromiumos-overlay/eclass/cros-kernel/exynos5_defconfig
cd ~/trunk/src/third_party/kernel/v3.8
cp ${DEFCONFIG} .config
ARCH=arm make menuconfig
# Make your changes
ARCH=arm make savedefconfig
cp defconfig ${DEFCONFIG}
make mrproper
```

If you have changes to the config that you think is relevant to others working
on the upstream kernel then feel free to submit a patch!

## Making sure other configs compile

It's hard to make 100% sure that your change won't break anyone else. ...but
it's usually considered courtesy to make sure that some other boards compile. If
you want to see if the `exynos4_defconfig` compiles you could use a recipe like
this:

```none
cd ~/trunk/src/third_party/kernel/v3.8
ARCH=arm CROSS_COMPILE=armv7a-cros-linux-gnueabi- make exynos4_defconfig
ARCH=arm CROSS_COMPILE=armv7a-cros-linux-gnueabi- make -j16
make mrproper
```

The `exynos4_defconfig` is particularly interesting when you're modifying exynos
code because it is a non-device tree enabled board that shares a lot of code
with the exynos config that we're using (so it's easy to break). It's actually
broken at the git hash I listed above because I didn't run it before submitting
my last patch. :(

## Sending your change upstream

This is documented over at the [Kernel
FAQ](/chromium-os/how-tos-and-troubleshooting/kernel-faq), which will soon
(hopefully) include the use of patman.

## What is known to work (and known to not work)

As of 11/30/12:

Works:

*   Serial over UART3 (if you've got a [servo2 debug
            board](/chromium-os/servo) hooked up). ...though remember that the
            default BIOS will try to disable serial console by mucking with the
            kernel command line.
*   i2c (all busses except the very important i2c bus 4)
*   power button and lid switch
*   EMMC and SD

Known not to work yet:

*   X (so ui will crash a whole lot at bootup)
*   USB 2.0 - Can almost do this with patches posted, expect it real
            soon now.
*   USB 3.0
*   i2c4 (which means no keyboard and no tps65090 PMIC)
*   Display
*   HDMI
*   WiFi
*   max77686 RTC + 32KHz
*   Trackpad
*   Audio

## Musings

A few random thoughts:

*   If you end up flashing a bad kernel a lot, it's a lot easier to use
            the "nv-U-Boot" (formerly known as legacy U-Boot). There's now
            documentation for how to do this at [Using nv-U-Boot on the Samsung
            ARM Chromebook](/system/errors/NodeNotFound). Pay special attention
            to the [Booting from a backup kernel](/system/errors/NodeNotFound)
            section, which is really helpful when dealing with lots of
            nonbooting kernels.
*   If you have access to a servo board (probably only people at Google
            will have these) you also have the option of using
            `cros_bundle_firmware` to load up a kernel. The magic command line
            I've used for this is as follows. Note that this won't get you
            kernel modules, just the main kernel.
    *   `cros_bundle_firmware -b daisy -d exynos5250-snow -w usb
                --kernel /build/daisy/boot/vmlinux.uimg`

## Contributing

Patches are certainly appreciated as are reviews of patches posted upstream. In
general most people on the Chrome OS team wouldn't object to people taking
patches from the Chrome OS kernel tree, adjusting them, and posting them
upstream (check with the author, though, since people may be midway through
doing that themselves).
