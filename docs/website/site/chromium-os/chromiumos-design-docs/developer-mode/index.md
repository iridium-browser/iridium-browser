---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: developer-mode
title: Developer Mode
---

[TOC]

# Introduction

Google Chrome OS devices must be:

*   Secure - users must be protected from attackers. This protection
            should reasonably extend to inexperienced or naive users.
*   Open - developers must be able to install their own builds of a
            Chromium-based OS, or another operating system (e.g., Ubuntu).
            Developers should not be significantly impaired from using the full
            capabilities of the device.

These two requirements are somewhat at odds. Locking down a device to protect
users makes it less useful for developers. Openings for developers to run
modified software easily can also be exploited by attackers.

This document describes:

*   Vulnerabilities to which Google Chrome OS devices may be exposed.
*   Use cases we must allow for normal users and developers.
*   A proposed implementation of developer mode that provides both
            openness for developers and security for users.
*   How the implementation protects from vulnerabilities while allowing
            required use cases.

Note that this document is specifically about the developer mode in Google
Chrome OS devices, not about other Chromium OS-based devices. It's focused on
developers who want to install a different operating system on a device that
initially shipped with Google Chrome OS.

# Vulnerabilities

The following vulnerabilities have been considered in this design.

## Attacker with complete physical access

An attacker with physical possession of the device over an extended period. The
attacker has access to tools including a soldering iron.

The attacker can remove the EEPROM containing our read-only firmware and replace
it with compromised firmware which does not perform signature checks.
Alternately, the attacker could simply replace the embedded controller's EEPROM
with code implementing a keylogger.

Based on YouTube videos of speed disassembly and assembly of computers and
consumer electronics devices, we believe that a practical attack could be
mounted in as little as 5 minutes.

This attack is unlikely to be made against a large number of devices.

Subcases:

*   Evil maid: The attacker modifies a targeted user's netbook so that
            it captures passwords, etc. The attack can be reversed by the
            attacker given a second possession of the device, leaving no
            evidence of the attack.
*   Malicious return: The attacker modifies a device then returns it to
            a store. It could act as a keylogger, phoning home at a later date
            with credentials of the next purchaser.
*   Device swap: The attacker purchases the same model of netbook as the
            targeted user. When the targeted user is distracted, a replacement
            netbook is swapped for the user's netbook.

We cannot prevent this attack.

## Attacker with casual physical access

An attacker with short-term access to a device—for example, a netbook briefly
left unattended at a conference or coffee shop.

The attacker may be able to insert a USB key and reboot the device, but cannot
make extensive physical modifications. The attacker may be able to unscrew a
panel and flip a switch.

Subcases:

*   The attacker attempts to quickly install malicious software on a
            temporarily-unattended machine.
*   The attacker walks down a row of machines at a trade show or store,
            installing malicious software on them (password sniffer, keylogger,
            etc).
*   A store employee attempts to install malicious software on a large
            number of computers for sale.

We should protect against this attack by making it as costly as the physical
attack, in terms of time required and noticeability to passers-by.

## Attacker with no physical access

An attacker with no physical access attempts to alter the device to persistently
run modified software. This is the most dangerous attack, due to its ability to
affect a large number of devices.

We must prevent this attack. That is, it should not be possible to persistently
control a device without first having physical access to it.

## Developer borrows a device

A developer buys or borrows a device, installs their own software, then returns
the device without reinstalling Google Chrome OS.

The custom software installed is unlikely to be intentionally malicious. The
primary risk is that developer images may not autoupdate, so the software on the
device will not be patched to close vulnerabilities. This risk has manifested
recently in smartphones with user-modified ('jailbroken') operating systems.

Subcases:

*   Return to store: The store may boot the device to make sure it's not
            broken, but will not generally open the case to check for hardware
            modification or switch state. The next user buys the device and
            starts using it. The first time the user boots the device, they're
            likely to be right in front of it, so will notice (but may not
            understand) a warning screen.
*   Return to user: It may take several boots for the user to actually
            be present at the device during boot, but it's very likely the user
            eventually sees a warning screen.

We should protect against these cases.

## Other vulnerabilities

There are other potential data and device vulnerabilities, including:

*   Data corruption caused by cosmic rays, aborted auto-updates,
            defective drives, etc.
*   Theft of a device.

These are outside the scope of this document since they do not relate to
developer mode.

# Use cases to allow

The following use cases have been considered in this design.

## Normal user

A normal user generally runs only Google Chrome OS on their device. They are not
generally interested in running any other operating system on the device.

When faced with recovery mode, many users are likely to return the device to the
store or call tech support.

The most naive users may not understand the significance of warning screens. We
should not be asking the user to make uninformed or cryptic decisions.

We must allow this use case.

## Developer

A developer builds Chromium OS or their own OS and wants to install it on a
device.

The developer has access to developer tools, the target device, and any
documentation which came with the device. They are experienced enough to use a
screwdriver to open a panel, but most will not be willing to use a soldering
iron to modify the device or void their warranty.

The developer expects similar performance while using their own OS as they have
while using Google Chrome OS. That is, boot time and the boot process should not
be significantly impaired from a normal device.

It is acceptable for the initial install of a developer OS to be slow, as long
as it is not painfully slow. Subsequent installs should be fast, so that a
developer can do rapid development.

The developer should be able to go back easily to running Google Chrome OS, at
which point they should have access to automatic updates. This case also covers
responsible developers who buy or borrow a device, try it out with their own OS,
then run recovery mode to restore the device before returning it.

Subcases:

*   Chromium OS developer: They will use the device in a manner similar
            to Google Chrome OS, perhaps with some added capability (shell
            access, etc). This is a common and valuable developer use case;
            these developers contribute back to the Chromium OS project.
*   Interactive developer: The developer uses the device like a normal
            laptop.
*   Remote developer: The developer uses the device as a media server,
            set-top box, or some other use case where they do not have direct
            access to the keyboard and primary display.

We must allow these use cases. If we don't, it is likely developers will hack
holes in verified boot to enable them.

## User installs prebuilt OS distribution (other than Google Chrome OS)

This is much the same as the developer use case. The difference is that the user
did not build the install. This is probably more common than developer installs,
particularly for use cases such as media servers or set-top boxes.

We should allow this use case.

## Manufacturer

Normally, read-only firmware is fixed, and rewritable firmware is signed by
Google. This introduces additional signing steps and delay into board bringup
and development.

A vendor involved in manufacturing a device (OEM, ODM, BIOS vendor, component
vendor) must be able to develop and debug firmware, including the
normally-read-only parts. For example, the vendor must be able to fix bugs in
recovery mode.

The vendor is capable of disassembling a device and making hardware
modifications involving soldering

It is acceptable for modifying a device to disable [write
protection](/chromium-os/firmware-porting-guide/firmware-ec-write-protection) to
be slow, as long as it is not painfully slow. Subsequent installs should be
fast, so that the vendor can do rapid development.

It is not necessary for the modifications to be reversible easily. For example,
removing the modifications can require re-disassembling the device and
re-soldering components.

We must allow this use case, to reduce the time and cost of producing devices
for Google Chrome OS.

# Implementation

The following sections describe the implementation we will use. This design
supports developers while protecting normal users, and requires no changes to
the typical device manufacturing process.

## Developer switch

The device will have an on/off developer switch which is not easily accessible
by a normal user.

The switch is reversible (that is, it can be turned back off once turned on). It
is turned off by default. It must be turned on physically; there must be no way
to turn this switch on without physically manipulating the device.

Approved implementations for this switch:

*   A switch underneath a screw cover.
*   A switch underneath/behind the battery, such that the battery must
            be removed to access the switch.
*   Pressing Control+D at the recovery screen, where the device uses the
            approved keyboard recovery mechanism \[1\].
*   Pressing Control+D at the recovery screen followed by pressing the
            recovery button, where devices have a physical recovery button.

Other implementations must be approved by Google in advance.

When the developer switch is off, only code signed by Google will run on the
device. Any other code will cause the device to reboot into recovery mode. This
protects against remote attack.

Note 1: The approved keyboard recovery mechanism contains a Google-designed
circuit that provides assurance that the executing firmware is the read-only
version, and that the user is physically present.

## Mode transition wipe

On the transition between dev and normal modes, there will be a 5 minute delay
during which the OS will wipe the stateful partition of the device. The delay
and wipe occur on the first boot after the dev switch is enabled.

The goal of this delay is to increase the difficulty of drive-by attacks to the
same level as physically compromising the device. (We could optionally play an
audio track—perhaps something along the lines of "Help, help, I'm being
repressed!"—or require user interaction during this delay, to make it more
noticeable if an attacker is attempting to compromise a device in a public
place.)

Note that the delay protects both normal users and developers; an attacker
trying to change the image on a developer's machine (for example, at a Linux
conference, where Chromium OS machines are likely to be common) will still incur
the delay.

## Recovery button always triggers recovery mode

The device will have a mechanism through which it always boots in [recovery
mode](http://www.chromium.org/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery),
regardless of the position of the developer mode switch. This provides a path
for users and developers to get back to Google Chrome OS, with all its safety
and autoupdates.

Approved implementations for the recovery button are:

*   Physical recovery button that must be held down on boot.
*   Google approved keyboard controlled recovery circuitry, whereby a
            user can hold three keys to get to recovery.

## Boot errors always trigger recovery mode

The firmware always checks the kernel signature at boot time. If the signature
is invalid, then the firmware checks the backup kernel. If both kernels are
invalid, the firmware runs recovery mode.

Google will provide a utility for developers to sign their kernels based on a
self-generated certificate.

Developer kernel signatures are not tied to a single target device; that would
prevent users from installing prebuilt OS distributions. In developer mode, the
cryptographic signature is cannot verified since the user’s public key is not
contained in the firmware; only the signing structure and checksums are used to
“validate” a self-signed kernel.

## Recovery boot image

The recovery boot image is Google-signed software on a removable drive. The
recovery boot image is what copies new firmware and software from the removable
drive to the fixed drive when booted in [recovery
mode](http://www.chromium.org/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery).
Recovery mode firmware will load only Google-signed software from the removable
drive. Those kernels contain their own initramfs, which copies the recovery
image onto the fixed drive.

If the developer switch is off, the recovery boot image will refuse to copy
anything but Google-signed software to the device. This protects against
drive-by attacks; the attacker must have time to get to the developer switch.
This also protects normal users from a remote attacker who stores a malicious
payload on the user's SD card and then attempts to reboot into recovery mode to
install that payload.

If the developer switch is on, the behavior of the recovery image depends on the
keys used to sign the current firmware/kernel/image and the new
firmware/kernel/image to be installed.

*   If the new key is Google-generated, there is no delay. This is a
            user trying to get back to Google Chrome OS.
*   If the new key matches the current key, there is no delay. This is a
            developer updating their own system.
*   If the new key is not Google-signed, and is different than the
            current key, there is a 5-minute delay before copying the payload to
            the fixed disk.

During the delay, the recovery image will display the developer agreement and
instructions on how to reinstall Google Chrome OS if you decide you don't like
the image you're installing. The goal of this delay is to increase the
difficulty of drive-by attacks to the same level as physically compromising the
device. (We could optionally play an audio track—perhaps something along the
lines of "Help, help, I'm being repressed!"—or require user interaction during
this delay, to make it more noticeable if an attacker is attempting to
compromise a device in a public place.)

Note that the delay protects both normal users and developers; an attacker
trying to change the image on a developer's machine (for example, at a Linux
conference, where Chromium OS machines are likely to be common) will still incur
the delay.

## Boot time warning screen

When the device boots normally in developer mode (that is, not in recovery
mode), it displays a warning so that ordinary users won't accidentally end up in
the wrong operating system.

At boot time, if the developer switch is on and the kernel is signed, but not by
Google, then a warning screen will be displayed by the firmware. The UI varies
slightly with the device, but may show a message something like this:

**WARNING!**
**YOUR COMPUTER IS NOT RUNNING GOOGLE CHROME OS!**
**PRESS THE SPACE BAR TO FIX THIS.**

This screen is intended to be a little scary; for example, it should use colors
and symbols designed to indicate danger / badness. It must support i18n.

If the user follows the on-screen instructions, the device will either revert to
normal mode or reboot into recovery mode. The on-screen instructions typically
involve pressing SPACE or RETURN, since these are the most common keys users
press when trying to get past a screen without reading it.

Upon seeing this screen, non-developer users will either return to normal mode,
run recovery mode, call tech support, or give up and return the device. Any of
these outcomes protects the user's data.

If the user presses Control+D, the device will bypass the warning screen waiting
period and continue booting the developer-signed code. Note that there is no
indication on the warning screen that Control+D is a valid option, so naive
users won't be tempted to press it. The Control+D sequence will be documented on
the Chromium OS developer website (you're reading it now), where it is easy for
developers to discover. We use the Control modifier so that accidentally bumping
the keyboard will not trigger it. For interactive developers, this reduces the
pain threshold of the warning screen to a single keypress.

If the user presses a key other than Control+D, space bar, Enter, or Esc, no
action is taken. This reduces the annoyance level for developers; accidentally
pressing D+Control or Control+S does not reboot into recovery mode.

After 20 seconds of displaying the warning screen, the device should beep. This
will alert a nearby user to look at the screen and see the warning. Interactive
developers are likely to press Control+D before this time, so will not be forced
to endure the beep.

If the user waits 30 seconds, the device will continue booting the
developer-signed code. This is necessary to support the remote developer subcase
above. It does mean that a naïve user could keep ignoring this screen. This is a
reasonable tradeoff, given that requiring a keypress invites developers to hack
verified boot.

In addition, there are bios flags that can be set to enable a user to trigger
other interactions, including but not limited to:

*   dev_boot_usb - this enables a device to boot from USB when a user
            presses the Control+U keys at the boot time warning screen.
*   dev_boot_legacy - this enables a device to boot from SeaBIOS when a
            user presses the Control+L keys at the boot time warning screen.

## Manufacturer hardware modifications

The device can be modified to disable the write protection on the firmware. This
modification should involve partial device disassembly and soldering, and will
void the device warranty. Disassembling the device far enough to make this
modification should require destroying a tamper-resistant seal, so that it's
obvious the device has been disassembled.

Once this modification is performed, the manufacturer can reprogram all the
firmware.

On a device where write protection is enabled in hardware (for example, an
EEPROM with a write protect enable line), this modification may take the form of
removing a resistor which pulls the write protect enable line active, and/or
adding a resistor to another set of pads to pull the write protect enable line
to the disabled state.

On a device where write protection is enabled in software (for example, a NAND
flash with a write-once-per-boot instruction which prevents further write
instructions that boot), this modification may take the form of an additional
input GPIO whose state is controlled by a resistor. The boot stub firmware will
read the GPIO state before writing the boot instruction; if the GPIO is
asserted, the boot stub firmware will not issue the write instruction to the
NAND flash.

Alternately, the EEPROM, which is normally soldered directly to the motherboard,
may be replaced with a socketed EEPROM to facilitate reprogramming via an
external programmer.

To prevent devices with disabled write protection from being shipped
accidentally to consumers, final manufacturing tests should verify that the
modifications have not been made. That is, the tests should attempt to write to
the read-only portion of the firmware and/or verify that a write protection
GPIO, if any, is asserted.

Note that a sufficiently advanced developer willing to void the warranty on
their device may be able to make the hardware modifications to disable write
protection and replace the normal Google Chrome OS firmware with their own
firmware. Such a developer would also be able to desolder, remove, and rewrite
the EEPROM chip with their own firmware, so including manufacturer support does
not represent a significant reduction in security.

# How this design addresses vulnerabilities and use cases

## Attacker with complete physical access

We cannot prevent this attack.

## Attacker with casual physical access

The attacker must have sufficient control of the machine to turn on the
not-easily-accessible developer switch.

Since the attacker's code is signed with a different key than the existing
software on the device, the recovery image will delay installing the new image.
The attacker must wait around near the device to interact with the machine
during this time. This raises the risk of discovery for the attacker.

If the attacker has sufficient control of the device that they're not worried
about unscrewing a panel, removing the battery, and interacting with the device
for 5 minutes, that's tantamount to complete physical access. As with the
previous case, an attacker with that level of access cannot be prevented.

## Attacker with no physical access

If there is a vulnerability in Google Chrome, the attacker may be able to obtain
temporary control over the device. Users could potentially enter their
credentials into this compromised device and run modified/unsafe code.

The attacker will not be able to maintain long-term persistent control over the
device. When the device next reboots, the verified boot process will detect the
modified software and trigger recovery mode (if the developer mode switch is
off) or a warning screen (if the dev mode switch is on).

## Developer borrows a device

If the developer is a nice developer (and they remember to restore the device),
they run recovery mode on the device before returning it.

If the developer returns the device without running recovery mode or turning the
developer switch back off, the device will show a warning screen at boot time.

Stores should be advised to boot returned Google Chrome OS machines. If they do,
they'll notice the warning screen and can refuse the return (at which point the
developer will turn off the developer switch and run recovery mode and then
successfully return the device).

If the store does not check a returned device, it could be sold to a user while
still containing developer software. The first time a new user boots their
device they're likely right in front of it (after all, they'll want to see our
5-second boot time) and will see the warning screen. At that point they'll
likely either return/exchange the device, call tech support, or fix it
themselves.

A user who lent their device to a developer will likely notice the warning
screen, or that the login screen looks different. At this point they'll complain
to the developer ("Hey sonny, what did you do to Grandma's computer?"), who can
fix it.

Note that in all of these cases, the developer software is unlikely to be
harmful in itself; the risk is that since developer mode removes some of the
normal-mode protections, the user is more vulnerable to attacks.

## Normal user

With the developer mode switch off, a normal user can only run Google Chrome OS.
Anything else will behave like the accidental corruption case—the device will
either repair itself or trigger recovery mode.

## Developer

A developer can modify the software on the device, installing their own Chromium
OS or other OS. Opening a screw panel and flipping a switch is easy and does not
void the device warranty.

To run an OS which requires legacy BIOS, they can replace the kernel with
software which sets up legacy interrupts and then runs something int19h-like to
load that OS. This is even easier since we're using GUID partition tables; the
legacy OS will only see the partitions described in the legacy MBR which
precedes the GUID partition table, so won't see the dedicated kernel partitions.

A developer using the device interactively needs to press a single key at boot
time to dismiss the warning screen. This is minimally invasive; it delays boot
time by only a second.

A developer using the device remotely will need to wait 30 seconds after reboot
for the warning screen to time out. This is not likely to be a significant
impediment for remote uses; systems like media servers or set top boxes are
rebooted infrequently.

## User installs prebuilt OS distribution (other than Google Chrome OS)

Same as Developer case. The modification to enable developer mode is easy, and
the device is comparably usable.

## Manufacturer

A manufacturer can modify the device hardware to allow reprogramming the entire
firmware. Once the modification has been made, firmware development can proceed
at a rate similar to other unprotected devices.

# Revision History

1.04 (1 May 2014) - Clean up several minor inaccuracies.

1.03 (20 Mar 2014) - Generalized from invalid kernel to "boot errors" trigger
recovery mode.

1.02 (3 Mar 2014) - Changed "unsigned" to "unverified" in kernel discussion.

1.01 (16 Mar 2010) - Clarified language for disabling write protect (previously
known as 'manufacturer mode').

1.0 (15 Mar 2010) - Published to chromium.org.

0.1 (12 Dec 2009) - Initial version.
