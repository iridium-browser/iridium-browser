---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: samsung-series-5-chromebook
title: Samsung Series 5 Chromebook
---

[TOC]

## Introduction

This page contains information about the [Samsung Series 5
Chromebook](http://www.samsung.com/us/computer/chromebook) that is interesting
and/or useful to software developers. For general information about getting
started with developing on Chromium OS (the open-source version of the software
on Chrome OS devices), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

## Entering Developer Mode

You might want to enter developer mode if you're following the instructions in
the [Chromium OS Developer Guide](/chromium-os/developer-guide), or if you just
want to get access to a shell on your device to [poke
around](/chromium-os/poking-around-your-chrome-os-device).

**Caution: Modifications you make to the system are not supported by Google, may
cause hardware, software or security issues and may void warranty.**

Developer mode on the Samsung Series 5 has two levels of access, "dev-switch on"
and "dev-mode BIOS". With the first level you enable a command line shell, which
lets you look around inside the GNU/Linux operating system, but does not let you
run your own versions. The second level of access installs a special BIOS
component that provides the ability to boot your own operating systems from
either removable (USB/SD) or fixed (SSD) drives. Both levels of access are
completely reversible, so don't be afraid to experiment.

### Developer switch

The developer switch enables the command line shell. The switch is behind a
little door on the right-hand side of the chromebook. To enable the developer
switch you open the door, use something pointy to move the switch towards the
back of the device, and reboot. Note: **be gentle**! Some people have reported
that the developer switch breaks easily.
The first time you reboot after turning the developer switch on, your chromebook
will:

*   ***************Show a scary warning that its software cannot be
            trusted, since a command line shell is enabled (press Ctrl-D or wait
            30 seconds to dismiss).***************
*   ***Erase all personal data on the "stateful partition" (i.e., user
            accounts and settings - no worries, though, since all data is in the
            cloud!).***
*   ******Make you wait between 5 and 10 minutes while it erases the data.******

The erase and wait steps only happen when you first enable the developer switch,
to help prevent someone from quickly reimaging your device while you're away
from the keyboard. Successive boots will:

*   ***************Show the same scary warning (press Ctrl-D or wait 30
            seconds to dismiss).***************
*   *********Continue to boot only Google-signed images, and only from
            the SSD.*********

At this point, verified boot is still active but because a command line shell is
enabled, your system is **NOT** secure. Refer to [Poking around your Chrome OS
Notebook](/chromium-os/poking-around-your-chrome-os-device) to see how to access
the command line shell. The message displayed at the shell itself should tell
you how to set your own password to protect shell access and make your system
secure again.
Here's a photo showing the location of the developer switch:

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/b.jpg"
height=171
width=400>](/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/b.jpg)

### Developer-mode BIOS

If you want to make modifications to the Chrome OS filesystem or boot your own
version of Chromium OS, you'll need to activate the second level of developer
access. You do this by running a special command from the command line shell,
available after the device boots to the 'select language' screen by pressing
Ctrl-Alt-"→" (Ctrl-Alt-F2). You first log in with the username 'chronos' (if
you've set a shell password, you'll be prompted for it). Then you switch to the
'root' account, and run the command to install the developer-mode BIOS. For
example:

localhost login: **chronos**
chronos@localhost $ **sudo bash**
localhost chronos # **chromeos-firmwareupdate --mode=todev**

### What's going on here?

[Verified boot](/chromium-os/chromiumos-design-docs/verified-boot) is the
process by which Chrome OS ensures that you are running only the software that
shipped with your chromebook. The process starts with the read-only BIOS, which
is built into the device at the factory and can't be modified without
disassembly (please don't try that; you'll void your warranty). The read-only
BIOS verifies one of two read-write BIOSes (there are two so we can provide
updates if we have to with less risk of failure) and continues execution there.
The read-write BIOS then verifies one of two (same reason) kernels and executes
that, and the kernel verifies its root filesystem as each block is read off the
SSD.
The normal read-write BIOSes will only boot Google-signed kernels, and only from
the SSD. When you run the chromeos-firmwareupdate command above, you are
replacing the primary read-write BIOS with a different one that will allow any
self-signed kernel (refer to the [Chromium OS Developer
Guide](/chromium-os/developer-guide)) to boot from either a removable device (by
pressing Ctrl-U at the scary boot screen) or from the SSD (press Ctrl-D or wait
30 seconds).

## How to boot your own image from USB

*   Follow the steps above to turn on the developer switch and to
            install the developer-mode BIOS.
*   Build a Chrome OS image using the steps in the [Chromium OS
            Developer Guide](/chromium-os/developer-guide). It does not need to
            be a recovery image.
*   Insert the removable drive containing your image into either USB
            slot.
*   Reboot, and when you see the blue scary boot screen, press Ctrl-U.
*   It should boot your image. If for some reason it doesn't think your
            image is valid it will just beep once instead.

## How to install your own Chromium OS image on your SSD

If you follow the full instructions from the [Chromium OS Developer
Guide](/chromium-os/developer-guide), you will eventually end up with a bootable
USB drive containing your image. You can boot that image directly from the USB
drive as described above. Since it's your personal image, it should have shell
access enabled. Log in as user "chronos" and run

```none
/usr/sbin/chromeos-install
```

That will wipe the SSD and install your image on it instead. When you reboot, it
should attempt to boot your version. You'll still continue to see the scary boot
screen at every boot, of course, as long as you are in developer mode and have
the developer-mode BIOS installed, so you'll need to press Ctrl-D or wait 30
seconds to boot.

## Leaving Developer Mode

To leave developer mode, simply flip the developer-mode switch back to the OFF
position and reboot. One of two things will happen. If your chromebook still has
a valid read-write normal-mode BIOS, Google-signed kernel, and an unmodified
Chrome OS root filesystem, then that's what will boot and you'll be back running
the official Chrome OS image. Or, if you've modified any part of the verified
boot chain so that a full verified boot process isn't possible, you'll be
dropped into [recovery mode](http://www.google.com/chromeos/recovery). That will
require you to create a bootable USB key to restore your chromebook to its
fresh-from-the-factory state. That's annoying, but not dangerous. As long as you
haven't taken the device apart, you shouldn't be able to permanently break
anything.
In either case, all personal information will be wiped from the device during
the transition.

### One other thing to try first

When the developer switch is on, the BIOS is not updated by any automatic Chrome
OS updates. If you don't think you changed anything but you still end up in
recovery mode, it may be that you've haven't applied a pending firmware update.
Turn the developer switch back on, reboot, and from a root shell run

```none
chromeos-firmwareupdate --mode tonormal
```

That will restore the primary read-write BIOS to normal mode, which may restore
the verified boot process. Turn the developer switch off again and reboot. If
you still end up in recovery mode, you'll just have to use the recovery process
to fix it.

## How to use the Recovery Mode button

Recovery mode is a special boot operation in which the BIOS will:

*   Refuse to boot from the SSD
*   Prompt you to insert a recovery USB drive
*   Only boot a Google-signed image from the USB drive

You will encounter recovery mode when the BIOS is unable to find a valid kernel
to boot, either because the SSD has become corrupted or (more likely) because
you modified all the kernel partitions while in developer mode and have switched
back to normal mode. While in developer mode, you will be presented with the
scary boot screen at every boot. Pressing SPACE or RETURN will take you to
recovery mode.

You can also force your chromebook into recovery mode (even in normal mode) by
using the recovery mode button. On the bottom of the Samsung Series 5, on the
corner nearest the developer switch, there is a tiny pinhole:

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/cc.jpg"
height=300
width=400>](/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/cc.jpg)

If you stick a paperclip into this hole, you'll feel it press a button. To force
recovery mode, turn the device off, press this button, and while keeping it
pressed, turn the device on again. This sometimes requires three hands or a bit
of contortion, but you'll know it worked when you see the recovery screen
instead of booting normally.

## How to hard-reset the EC

You should never have to do this. If you think you need to and haven't been
specifically instructed to do so by Google or Samsung, please contact one of
those companies to tell them why.

The Samsung Series 5, like most portable computers, has a small embedded
controller ("EC") inside it that controls things like battery charging, LEDs,
fans, and so forth even when the device is turned off. The EC runs anytime that
power is available, even battery power. In the extremely rare and unusual case
that the EC needs to be reset, the only certain way is to remove power. But
since the Samsung Series 5 battery is not removable, there is a battery
disconnect button for this purpose. Unplug the AC, flip the device over, and use
a paperclip to gently press the battery reset button through the hole on the
back of the chromebook. Hold it down for a few seconds, then release it.

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/alex_reset.jpg"
height=300
width=400>](/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/alex_reset.jpg)

This process turns the battery off. It won't turn on again until you've
connected the AC power cord and the EC has booted. After that, things should
work normally again. You may have to press the power button once or twice, since
the EC may take a moment or two to fully reboot.

## Firmware

See the [H2C firmware
page](/chromium-os/developer-information-for-chrome-os-devices/h2c-firmware) for
more details.

## What's inside?

**WARNING: Opening the case and fiddling with the stuff inside could easily
brick your system and make it unrecoverable. DO NOT ATTEMPT if you are not
familiar with this process.**

### Disassembly

Taking apart your laptop is **not** encouraged. If you have hardware troubles,
please seek assistance first from an authorized center. Be advised that
disassembly might void warranties or other obligations, so please consult any
and all paperwork your received first. If you just want to see what the inside
looks like, gaze upon this (click for a high res version):

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/alex-bottom-guts.jpg"
height=236
width=320>](/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/alex-bottom-guts.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/alex-bottom-guts-no-battery.jpg"
height=236
width=320>](/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/alex-bottom-guts-no-battery.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/alex-top-guts.jpg"
height=190
width=320>](/chromium-os/developer-information-for-chrome-os-devices/samsung-series-5-chromebook/alex-top-guts.jpg)

However, we acknowledge that some people like to tinker. So here's a quick guide
to taking it apart:

*   First, acquire the necessary tools:
    *   1 phillips head screw driver
        *   A PH0 should handle most screws
        *   A J0 might be better for the screws holding the battery (if
                    you want to remove it)
    *   1 wide flat head screw driver, or a butter knife
*   Shut the system down and close the lid
*   Flip the laptop over so the bottom is facing up
*   Remove the three visible screws from the bottom of the case
*   Remove the rubber feet from the bottom of the case
    *   Works better if you have fingernails as you can slide them under
                the edge and then peel up
    *   Consider sticking them to the case near their original holes so
                they still function as grips on tables
*   Remove the four screws that were under the rubber feet
*   Starting at one of the front corners, use your fingernails to pry
            apart the case slightly
*   As you pry it apart, wiggle it a bit so the plastic clips separate
            cleanly rather than break apart
*   Work your way from one front corner to the other front corner
*   Work your way along the sides towards the hinge
*   Once only the hinge side is still together, you can pull up
            carefully and then pull the bottom away from the hinge

You'll see that only one of the components are replaceable:

*   The hard drive is a 16GiB mSATA

A now-obsolete 40-pin [servo connector](/chromium-os/servo) may be present on
some boards.
