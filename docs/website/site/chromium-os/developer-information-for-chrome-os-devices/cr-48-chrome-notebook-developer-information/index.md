---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: cr-48-chrome-notebook-developer-information
title: Cr-48 Chrome Notebook Developer Information
---

[TOC]

## Introduction

This page contains information about the [Cr-48 Chrome
Notebook](http://www.google.com/chromeos/pilot-program-cr48.html) that is
interesting and/or useful to software developers. For general information about
getting started with developing on Chromium OS (the open-source version of the
software on the Chrome Notebook), see the [Chromium OS Developer
Guide](/chromium-os/developer-guide).

## Entering Developer Mode

You might want to enter developer mode if you're following the instructions in
the [Chromium OS Developer Guide](/chromium-os/developer-guide), or if you just
want to get access to a shell on your device to [poke
around](/chromium-os/poking-around-your-chrome-os-device). To get your device
into Developer Mode, you'll need to flip the developer switch to the "Developer
Mode" position.

Caution: Modifications you make to the system are not supported by Google, may
cause hardware issues and may void warranty.

The first time a Chrome Notebook boots in Developer Mode after leaving Normal
Mode it will:

*   ***************Show a scary warning that its software cannot be
            trusted, since verified boot is disabled (press Ctrl-D or wait 30
            seconds to dismiss).***************
*   ***Erase all personal data on the "stateful partition" (i.e., user
            accounts and settings - no worries, though, since all data is in the
            cloud!).***
*   ******Make you wait between 5 and 10 minutes to while it erases the data.******
*   *********Boot from any self-signed image on its SSD, negating the
            security of verified boot.*********

The erase and wait steps only happen when you first switch to Developer Mode, to
help prevent someone from quickly reimaging your device while you're away from
the keyboard. Successive boots in Developer Mode will only:

*   ***************Show a scary warning that its software cannot be
            trusted, since verified boot is disabled (press Ctrl-D or wait 30
            seconds to dismiss).***************
*   *********Boot from any self-signed image on its SSD, negating the
            security of verified boot.*********

Entering Developer mode is easy:

1.  Remove the battery.
2.  Peel off the sticker that hides the developer switch (see image 1).
3.  Flip the developer switch towards the battery connector (see image
            2).
4.  Put the battery back in.
5.  Turn the device back on.
6.  Press Ctrl-D at the scary warning screen.
7.  Wait 5-10 minutes and any saved information on your device will be
            erased.
8.  Congratulations, enjoy hacking in Developer Mode!

Here are some pictures that might help:

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/Mario_DevSwitchTape.png"
height=225
width=400>](/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/Mario_DevSwitchTape.png)

**Image 1: Location of Developer Switch**

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/Mario_DevSwitchOn.png"
height=225
width=400>](/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/Mario_DevSwitchOn.png)

**Image 2: Developer Switch in "Developer Mode" position.**

---

## Getting the recovery kernel

If you are trying to install your own disk image onto your Cr-48 Chrome Notebook
(maybe you're following the [Chromium OS Developer
Instructions](/chromium-os/developer-guide)), you need the recovery kernel. You
can find it at:
<https://dl.google.com/dl/edgedl/chromeos/recovery/mario_recovery_kernel.zip>
(this did not work for me, and I had to follow [these
instructions](/chromium-os/extracting-a-recovery-kernel-from-a-recovery-image)
instead).

---

## Leaving Developer Mode

Returning to normal mode is as simple as entering developer mode: shut down,
remove the battery, flip the switch, reboot. There are a couple of things to
note, however.
First, assuming you haven't modified anything, the first time you boot in normal
mode after leaving developer mode, the stateful partition will be erased. This
is a much faster erase process than when entering developer mode, usually only
30 or 40 seconds, and only happens with the first boot.
Second, verified boot will be enabled, meaning that only Google-signed images
will be bootable. If you haven't modified the original kernel or rootfs
partitions in any way, you should have no problems. If you've made changes to
the kernel partitions, the Cr-48 will refuse to boot that kernel and will
display a recovery screen. You'll have to [create a recovery USB
drive](http://www.google.com/chromeos/recovery) to restore your Cr-48 to the
factory condition. If you've made changes to the rootfs partition but not the
kernel, the Cr-48 **may** appear to boot normally, but may later suddenly reboot
and/or display the recovery screen. This happens because the kernel verifies the
rootfs as each block is read from the SSD, so it may not encounter a modified
block until sometime later. When it does, it will reboot immediately.

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
You can also force your Cr-48 into recovery mode (even in normal mode) by using
the recovery mode button.
On the bottom of the Cr-48, directly below the ESC key, there is a tiny pinhole:

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/cr48_recovery_button.jpg">](/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/cr48_recovery_button.jpg)

If you stick a paperclip into this hole, you'll feel it press a button. To force
recovery mode, turn the Cr-48 off, press this button, and while keeping it
pressed, turn the Cr-48 on again. This sometimes requires three hands or a bit
of contortion, but you'll know it worked when you see the recovery screen
instead of booting normally.
There is little use for this button other than booting a [recovery USB
drive](http://www.google.com/chromeos/recovery). In developer mode you can run
your own scripts from the recovery USB drive, but in developer mode you can
trigger recovery mode by just pressing SPACE at boot. Still, there it is.

## How to boot your own (non-Chromium OS) image from USB

(Note: This part is outdated: make_developer_script_runner.sh does not exist
anymore)

Sometimes, you just want to have a shell or maybe you want to install another
operating system. If you're in Developer Mode, then you can totally do that.

This tip could actually be useful to someone who wasn't a Chromium OS developer.
However, at the moment the only easy way to get the tools is to follow many of
the steps in the [Chromium OS Developer Guide](/chromium-os/developer-guide). If
you don't care about developing for Chromium OS and just want to get to booting
your own stuff, you should read the developer guide with these thoughts in mind:

1.  The instructions in the [Chromium OS Developer
            Guide](/chromium-os/developer-guide) assume that you're running the
            [Ubuntu Lucid](http://www.ubuntu.com/) distribution of Linux. If
            you're not, you may not be able to follow them.
2.  You should only need to follow the instructions up to the point of
            "Enter the chroot". You don't actually need to build a Chromium OS
            image. You probably also want to choose the "minilayout" when
            downloading the source code, since that will be faster.

The rest of the instructions will assume that you've followed the instructions
enough to make a chroot and that you're currently in the chroot.

### Getting the recovery kernel for your Chrome OS Notebook

You'll need to get a "recovery kernel" for your Chrome OS Notebook in order to
follow these instructions. You can download the an officially-signed Recovery
Kernel for the [Cr-48
Chromebook](/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information)
(AKA Mario) by running the steps below from inside the chroot:

```none
curl https://dl.google.com/dl/edgedl/chromeos/recovery/mario_recovery_kernel.zip > ~/trunk/mario_recovery_kernel.zip
unzip -d ~/trunk ~/trunk/mario_recovery_kernel.zip
RECOVERY_KERNEL=~/trunk/mario_recovery_kernel.bin
```

The instructions below assume that you've set the `RECOVERY_KERNEL` variable as
above.

If you have trouble downloading the recovery kernel, you might be able to
[extract an officially-signed Recovery Kernel from an officially-signed Recovery
Image](/chromium-os/extracting-a-recovery-kernel-from-a-recovery-image) instead.

### Creating a USB disk that will boot a simple script

Using the `make_developer_script_runner.sh` tool, you can create a USB disk that
will run shell script. In this example, we'll show how to create a USB disk with
a script that will leave you running the shell.

#### Create the script

```none
echo -e  '#!/bin/sh\nexec /bin/sh\n' > run_a_shell.sh
```

#### Build a disk image

Now, use make_developer_script_runner.sh. The script will create a file called
`dev_runner_image.bin` that can be placed on a USB disk. Note that these
instructions assume that you've set the RECOVERY_KERNEL variable properly, as
explained above.

```none
./make_developer_script_runner.sh --kernel_image=${RECOVERY_KERNEL} --developer_script=run_a_shell.sh
```

#### Find your USB disk

You should plug a USB disk in to your computer now and wait a few seconds for it
to be detected. You can then run this quick command to use some Chromium OS
tools to detect the disk and find out where it is:

```none
bash -c 'source chromeos-common.sh; 
  echo
  for disk in $(list_usb_disks); 
    do echo /dev/$disk - $(get_disk_info $disk manufacturer) - $(get_disk_info $disk product); 
  done'
```

...assuming your disk was detected, it should print out as a result of that
command. For instance, on my computer, I see this, which tells me that my USB
disk is in `/dev/sdc`:

```none
/dev/sdc - Kingston - DataTraveler G3
```

#### Format your USB disk with the disk image

**==WARNING==: This step will erase all data on your USB disk. Make sure there's
nothing important on it.**
==ALSO==: If you mistype this step (don't put the right /dev/sdX in the
command), it can blow away some other disk on your computer.**

**BE CAREFUL!**

Really, I'm serious. *Be very careful*. If you shoot yourself in the foot with
these instructions, you only get to blame yourself.

OK, now that you've read all of the disclaimers, the command you'll need to
enter is this, replacing `${MY_USB_KEY_LOCATION}` with the /dev/sdX value that
is for your USB disk (see the "Find your USB disk" instruction above).

TODO: You may need to unmount your USB disk before running this instruction if
Ubuntu mounted it for you. Put a set of instructions that tell how to do that.

```none
sudo dd if=dev_runner_image.bin of=${MY_USB_KEY_LOCATION} bs=16M conv=fsync
```

#### Boot your Chrome OS Notebook with your USB image

Now, shut off your Chrome OS Notebook and turn it back on. Since you're in
Developer Mode (right?), you should see the warning. Hit the space bar to enter
recovery mode. Once prompted, insert your USB disk.

Assuming that you haven't reimaged the built-in SSD with a developer-signed
image, you'll need to wait 5 minutes each time you boot up with this disk image.
This is a security precaution, to prevent someone from quickly rebooting your
device from their USB key while you're getting more coffee (see the [Developer
Mode](/chromium-os/chromiumos-design-docs/developer-mode) design document).

---

### Create a USB disk that will boot another operating system

TODO: This should be possible for someone skilled in the art of Linux. The above
instructions tell you how to run any arbitrary Linux program, and (with root
access) you should be able to do pretty much anything you want.

It would be nice to get some good instructions here, though.

---

### Install your own build of Chromium OS

First, download the recovery kernel, as explained above. Then build a Chromium
OS image, as mentioned in the [Chromium OS Developer
Guide](/chromium-os/developer-guide). Once you have the recovery kernel and the
OS image, you'll stitch them together, copy them to a USB drive, and then boot
your system from the USB drive.

Combine the recovery kernel and your OS image, to create a recovery image

You can build your own recovery image using `mod_image_for_recovery.sh`. This
image will have the officially-signed Recovery Kernel (so you can boot from USB)
and will install a self-signed SSD image (so you can boot only with developer
mode). Here's the magic set of steps (assuming that you've got a path to the
recovery kernel in `${RECOVERY_KERNEL}` and that the recovery kernel is a match
for the `${BOARD}` you built):

```none
./mod_image_for_recovery.sh \
    --board=${BOARD} \
    --nominimize_image \
    --kernel_image ${RECOVERY_KERNEL} \
    --image ~/trunk/src/build/images/${BOARD}/latest/chromiumos_image.bin
```

**SIDE NOTE**: If you're interested in creating a test image (used for allowing
Chromium OS to talk to autotest), you can run `cros build-image test` to create
a test image that can be combined with the recovery image:

```bash
cros build-image --board=${BOARD} test
./mod_image_for_recovery.sh \
    --board=${BOARD} \
    --nominimize_image \
    --kernel_image ${RECOVERY_KERNEL} \
    --image ~/trunk/src/build/images/${BOARD}/latest/chromiumos_test_image.bin
```

Copy the recovery image to a USB key

The first step is to insert a USB flash disk (4 GB or bigger) into your build
computer. **This disk will be completely erased, so make sure it doesn't have
anything important on it**. Wait ~10 seconds for the USB disk to register, then
type the following command:

```none
cros flash --board=${BOARD} usb://
```

For more details on using this tool, see the [Cros Flash
page](/chromium-os/build/cros-flash).

When the cros flash command finishes, you can simply unplug your USB key and
it's ready to boot from.

**IMPORTANT NOTE**: To emphasize again, cros flash completely replaces the
contents of your USB disk. Make sure there is nothing important on your USB disk
before you run this command.

#### Install your image to the Cr-48's SSD

In developer mode, your Chrome OS Notebook gives you an option to use a recovery
image every time the machine boots. To install your recovery image, do the
following:

1.  Turn your Chrome OS Notebook off.
2.  Turn it back on.
3.  During the boot warning, press space to enter recovery mode.
4.  Wait until prompted to put your USB disk in.
5.  Put the USB disk in.
6.  Wait while the image is copied to the SSD.

If you reboot now, you'll be booting from your image (you may need to wait past
the recovery screen). Congratulations!

**IMPORTANT NOTE**: You **must** stay in Developer Mode to continue booting your
image. Since your image was not signed by the release keys (it's self-signed
image), it will only boot in Developer Mode. If you want to go back to Release
Mode, just copy (/bin/dd) [a recovery
image](http://support.google.com/chromeos/bin/answer.py?hl=en&answer=1080595)
directly to a USB drive, without making any modifications to it.

---

## How to install a different OS on your SSD

There's an example of configuring a Cr-48 to dual-boot Chrome OS and Ubuntu
[here](/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/how-to-boot-ubuntu-on-a-cr-48).

## Troubleshooting

### Battery Doesn't Charge

If your battery is completely drained, sometimes the CR-48 has trouble charging
it again. You might be able to workaround it by doing:

*   Plug the AC adapter in
*   Wait 20 seconds
*   Unplug the AC adapter
*   Wait 20 seconds
*   Plug the AC adapter in
*   Wait 20 seconds
*   ... repeat this cycle 10 times or so ...
*   The battery should hopefully start charging again

## Firmware

See the [H2C firmware
page](/chromium-os/developer-information-for-chrome-os-devices/h2c-firmware) for
more details.

## What's inside?

**WARNING: Opening the case and fiddling with the stuff inside could easily
brick your system and make it unrecoverable. DO NOT ATTEMPT if you are not
familiar with this process.**

### Components

Here is a rundown of the parts that are not soldered down:

*   Power supply: 19.5V <img alt="image"
            src="/chromium-os/developer-information-for-chrome-os-devices/direct-current.svg"
            height=5> ([DC](http://en.wikipedia.org/wiki/Direct_current)) 2.05A
            <img alt="image"
            src="/chromium-os/developer-information-for-chrome-os-devices/center-positive-polarity.svg"
            height=12> ([positive polarity
            tip](http://en.wikipedia.org/wiki/Polarity_symbols))
*   SSD: 16GiB mSATA
*   RAM: one slot for 204-Pin DDR3 SO-DIMM laptop memory

### Disassembly

Taking apart your laptop is **not** encouraged. If you have hardware troubles,
please seek assistance from a friend knowledgeable in the area. If you just want
to see what the inside looks like, gaze upon this (click for a high res
version):

[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/mario-bottom-guts.jpg"
height=231
width=320>](/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/mario-bottom-guts.jpg)
[<img alt="image"
src="/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/mario-top-guts.jpg"
height=102
width=320>](/chromium-os/developer-information-for-chrome-os-devices/cr-48-chrome-notebook-developer-information/mario-top-guts.jpg)

However, we acknowledge that some people like to tinker. So here's a quick guide
to taking it apart:

*   Shut the system down and close the lid
*   Flip the laptop over so the bottom is facing up
*   Pop out the battery using the release switch in the middle of the
            case
*   Remove the visible screws
    *   There should be 10 in total (a line of 3 black, a line of 3
                silver, and a line of 4 silver)
    *   The black screws are M2x2x0.4 while the silver screws are
                M2x3x0.4
*   Remove the two screws under the rubber feet (M2.5x6x0.45)
    *   If you pry them up starting at the edge closest to the hinge,
                you're less likely to strip the glue
*   Using a flat instrument (your nails, a wide screw driver, etc...),
            pry the case apart starting at the side w/the VGA port
    *   Work your way around the front and to the other side
    *   By now, it should be pretty easy to wiggle the whole thing apart

You now have access to the upgradable components.
