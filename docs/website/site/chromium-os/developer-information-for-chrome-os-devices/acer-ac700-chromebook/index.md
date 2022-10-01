---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: acer-ac700-chromebook
title: Acer AC700 Chromebook
---

[TOC]

## Introduction

This page contains information about the [Acer AC700
Chromebook](http://us.acer.com/ac/en/US/content/ac700-home) that is interesting
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

Developer mode on the Acer AC700 has two levels of access, "dev-switch on" and
"dev-mode BIOS". With the first level you enable a command line shell, which
lets you look around inside the GNU/Linux operating system, but does not let you
run your own versions. The second level of access installs a special BIOS
component that provides the ability to boot your own operating systems from
either removable (USB/SD) or fixed (SSD) drives. Both levels of access are
completely reversible, so don't be afraid to experiment.

### Developer switch

The developer switch enables the command line shell. The switch is underneath
the chromebook battery. To enable the developer switch you remove the battery,
use something pointy to move the switch towards the side with the red dot above
it, and reboot. Note: **be gentle**! Some people have reported that the
developer switch breaks easily.
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
src="/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/newback.jpg"
height=300
width=400>](/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/newback.jpg)

### Developer-mode BIOS

If you want to make modifications to the Chrome OS filesystem or boot your own
version of Chromium OS, you'll need to activate the second level of developer
access. You do this by running a special command from the command line shell.
You first log in with the username 'chronos' (if you've set a shell password,
you'll be prompted for it). Then you switch to the 'root' account, and run the
command to install the developer-mode BIOS. For example:

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
*   Insert the removable drive containing your image into the left side
            USB slot.
*   If you use the one on the right, the USB stick's light will flash,
            but the screen will stay black and the machine won't boot.
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
using the recovery mode button. On the bottom of the Acer AC700 on the side
nearest the Enter key, there is a tiny pinhole:

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/back.jpg"
height=300
width=400>](/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/back.jpg)

If you stick a paperclip into this hole, you'll feel it press a button. To force
recovery mode, turn the device off, press this button, and while keeping it
pressed, turn the device on again. This sometimes requires three hands or a bit
of contortion, but you'll know it worked when you see the recovery screen
instead of booting normally.

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
looks like, gaze upon this (click for high res versions):

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/acer-zgb-bottom.jpg"
height=224
width=320>](/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/acer-zgb-bottom.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/acer-zgb-bottm-guts.jpg"
height=182
width=320>](/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/acer-zgb-bottm-guts.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/acer-zgb-top-guts.jpg"
height=230
width=320>](/chromium-os/developer-information-for-chrome-os-devices/acer-ac700-chromebook/acer-zgb-top-guts.jpg)

However, we acknowledge that some people like to tinker. So here's a quick guide
to taking it apart.

#### Access to upgradable/cleanable components

This is very easy to do and gets you access to all the pieces you most likely
care about:

*   The hard drive is easy to remove & replace/upgrade
    *   It is a standard SATA port, the form factor only allows
                [Sandisk's
                pSSD](http://www.ssd.gb.com/Products/SanDisk_Industrial_and_Enterprise/End_Of_Life/pSSD_Solid_State_Drive_(SATA)_P4Model/index.php)
    *   Once you remove the two screws, use the black cover tab to pull
                down & remove the module
*   The memory slots is easy to access
    *   The system uses 204-Pin DDR3 SO-DIMM laptop memory
*   The wifi module is easily removable (two screws)
*   The RTC battery can be replaced

And here's the details:

*   Get a smallish phillips head screw driver -- all screws can be
            removed with just this
*   Shut the system down and close the lid
*   Flip the laptop over so the bottom is facing up and the speakers are
            closest to you
*   Remove the three screws in the middle (on the visible plastic piece)
*   Use the tab on the right side of the plastic piece to pop it off

Check out the high res picture above for overview.

#### Access to the rest of the machine

While it is possible to remove the motherboard entirely, there isn't much of a
point. You can see a high res image of what it looks like above if you just want
that.

If you really want to remove it, then here's how:

*   Remove the 9 screws from the outer edges of the bottom of the case
*   Flip it over and open the display
*   The top & bottom of the case should be slightly separated so you can
            now remove the keyboard
    *   There are no screws holding it in, just tabs along the
                top/bottom
    *   Slowly work it until you can pull it straight up through the top
                half of the case
*   Remove all the visible screws that were under the keyboard (there
            are a bunch)
*   Pull the top half of the case off from the bottom
*   Remove all the visible screws from the motherboard (there are only a
            few)
*   Pop the wires off of the wifi & cellular modem
*   Remove the screws from both screen hinges
    *   Be careful as the LCD wire shouldn't really be removed
*   Disconnect the speakers and led module (bottom left)
*   Pull the motherboard out of the case
