---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: recovery-mode
title: Recovery Mode
---

[TOC]

## Overview

This document describes how recovery mode works in a Chromium-based OS. It
assumes the firmware has already booted in recovery mode, and is now searching
removable media (a USB key or an SD card) for a valid recovery image.

In this document, the term "recovery storage device" (or "RSD") refers to a
removable drive containing Chromium OS recovery software. In contexts other than
that phrase, the term "device" refers to a Chromium OS-based device such as a
netbook.

For background and other relevant information, see the following documents:

*   [Firmware Boot and
            Recovery](/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery):
            how the firmware gets into recovery mode.
*   [Developer
            Mode](/chromium-os/chromiumos-design-docs/developer-mode): how
            developer mode works, including the developer switch.
*   [File
            System/Autoupdate](http://www.chromium.org/chromium-os/chromiumos-design-docs/filesystem-autoupdate):
            how normal updates work.
*   [Firmware Verified
            Boot](/chromium-os/chromiumos-design-docs/verified-boot): how kernel
            images are signed.
*   [Disk Format](/chromium-os/chromiumos-design-docs/disk-format): disk
            drive format, used for both the Chromium OS device and the recovery
            storage device.
*   Boot Process (forthcoming): how kernel images are loaded into RAM
            and executed.

## Design goals

The recovery software can vary by use case to satisfy a variety of different
goals.

For the average user:

*   Only need to support one device model.
*   Download the minimum amount of data.
*   Should be a way to build a recovery device ahead of time; using this
            recovery device for a restore should not require the network.
*   Minimal interaction required.

For the developer:

*   Support loading other operating systems.

For manufacturing or corporate settings:

*   Support a range of device models.
*   Centralized management of images to be pushed; updating the image
            should not require re-flashing many RSDs.

Security:

*   Content stored locally on the storage device should be used in
            preference to that on the network.
*   Use secure transport to network image server.
*   Images are signed (see the [Firmware Verified
            Boot](/chromium-os/chromiumos-design-docs/verified-boot) document)

Simplicity & Robustness:

*   Push complexity from the firmware to the recovery kernel+rootfs,
            where a full Linux-like environment is available.

## Entering recovery mode

For detailed information about entering recovery mode, see the [Firmware Boot
and Recovery](/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery)
document. This section is only a brief summary.

Recovery mode can be triggered:

*   Automatically by the Chromium OS firmware or software, if it
            determines that the system is corrupt and that the backup
            firmware/software is also corrupt.
*   Manually, by the user pressing a Recovery Mode button during boot.

In either case, if a storage device is present and it contains a signed recovery
image, then the firmware doesn't display anything; it assumes the recovery image
will display relevant information and tell the user what is going on.

If a storage device is not present, or does not contain a valid recovery image,
then the firmware needs to get the user started on the recovery process. To do
this, it will display a screen which conveys the following information to the
user:

*   Your netbook has booted into recovery mode.
*   Don't panic!
*   Go to a URL specific to the device model for more instructions:
            (insert model-specific URL here; the exact URL is still under
            development - for example,
            http://chromeos.google.com/recovery/*hardware_model_ID* or
            http://goo.gl/recovery/*hardware_model_ID*)

That information needs to be accessible in many languages, since we won't know
ahead of time what language the user is speaking, and the firmware which
displays the screen is non-interactive (does not have access to the keyboard).
One way to do this is to use just a pictograph and a URL. It is crucial to
instill the need for the user to go to another computer to visit the URL
specified on the screen.

## Recovery website

For devices shipping with Google Chrome OS, the recovery website will be a
Google-hosted website which provides instructions and installers for creating
RSDs.

Instructions will be customized for each device model. For example, if a
particular netbook model does not have wired networking, the instructions for
that model won't suggest plugging in a network cable. The types of valid
external storage devices and their physical locations on the netbook can also be
customized per model.

The recovery website will ask the user to get an appropriate storage device,
then download a recovery installer (see below) to finish the recovery. It should
also download a small config file with the model number of the netbook to be
recovered, so that the recovery installer can use that to fetch the right
recovery data.

## Recovery installer

This is a small program which helps a user create a RSD.

Since the installer needs to run on any computer, we need multiple versions of
it (Windows, Mac, Linux, Chromium OS). The Chromium OS installer will be part of
the default Chromium OS install image, so that any Chromium OS device can be
used to create a RSD for any other Chromium OS device.

The installer will do the following:

1.  Prompt the user to choose a target storage device to use as the RSD.
2.  Verify the storage device is large enough.
3.  Download the appropriate recovery software (recovery kernel +
            recovery root filesystem) and save it to the RSD.
4.  Download the appropriate recovery data (Chromium OS firmware +
            kernel + rootfs + recovery configuration data) and save it to the
            RSD.
5.  Prompt the user to remove the storage device and plug it into the
            netbook.

For more information about the recovery software and data, see the "Types of
Data" section elsewhere in this document.

The following features for the recovery installer are optional for version 1.0,
and may be included in future versions:

*   If the target storage device is big enough for the recovery
            software, but too small to fit both the recovery software and
            recovery data, the installer could help the user set up network
            recovery mode (see below).
*   It could offer to back up the existing contents of the storage
            device before creating the RSD, then restore those contents to the
            storage device after the recovery is done.
*   It could detect the language setting on the source computer, then
            automatically use that language for all displays and prompts during
            recovery.

One potential starting point for a recovery installer is
[UNetbootin](http://unetbootin.sourceforge.net/). It already makes recovery /
installer images for several OSes, and runs on Windows and Linux.

## Types of data on a recovery storage device

The following types of data may be present on a RSD. For information on which
kinds of data appear on each type of RSD, see the "Types of recovery storage
device" section, below.

For more information on GUID partition tables and Chromium OS partition types,
see [Disk Format](/chromium-os/chromiumos-design-docs/disk-format).

### Recovery kernel

This is a Chromium OS kernel partition, signed by the same authority as the
firmware on the device. It boots the netbook into a more usable / interactive
state for the rest of recovery mode.

For Google Chrome OS devices, the kernel is signed by Google. The user will
download the recovery kernel from the Google recovery website.

A different recovery kernel is needed for each processor architecture (x86,
ARM).

Since boot speed is not important, we don't need to optimize the kernel for
variants of each processor architecture; we can compile for the lowest common
feature set.

### Recovery root filesystem

This is a Chromium OS rootfs partition, signed by the same authority as the
recovery kernel.

A recovery root filesystem can support a range of models with compatible
processor architecture; drivers for multiple types of display and storage can be
included. Alternately, if over time the diversity of Chromium OS devices makes
the size of the filesystem prohibitively large, we can produce multiple images,
each applicable to a subset of devices. In the latter case, the model-specific
URL provided by the recovery mode firmware will point the user to the correct
image for that device.

To determine and/or verify which set of recovery data (Chromium OS kernel,
rootfs and firmware) is appropriate for the netbook to be recovered, the
read-only firmware on the netbook will supply an API for the recovery image to
discover the netbook's model number. (See the (forthcoming) System SKU Reporting
Specification.)

### Recovery data

This data partition contains data for the recovery. It may contain rootfs
image(s), firmware image(s), and/or network configuration.

#### Chromium OS kernel+rootfs

This is a full Chromium OS kernel+rootfs image.

All images are signed to protect against accidental or intentional corruption.

*   It may be a Google-signed image provided by the Google Chrome OS
            update server.
*   Alternately, it may be a developer-signed image. See Developer Mode
            below.

Images will have a header that provides the following information:

*   Chromium OS version
*   Signing authority (Google or developer)

If multiple versions of an image are available from the same location (for
example, the data partition on the RSD), the newer version should take
precedence.

Images signed by the same authority as the firmware take precedence over images
signed with other authorities. On a Google Chrome OS device, if both a
Google-signed image and a developer-signed image are available from the same
location (for example, the data partition on the RSD), the Google-signed image
should take precedence. This protects normal users. The developer documentation
on the Chromium OS website will provide instructions for developers to construct
an RSD without a Google-signed image, so that they can load their own OS.

#### Chromium OS firmware

This is firmware for a Chromium OS device.

It has the same information about versions and signing authority in the filename
and/or header as the rootfs, and the same rules of precedence apply if multiple
firmware files are present.

The firmware is likely to be included inside the Chromium OS rootfs.

#### Network configuration

This is configuration data for downloading recovery data over the network. This
includes:

*   order in which to attempt network interfaces (default: wired, then
            wireless)
*   WiFi SSID and WPA key, to support loading recovery data over WiFi
            (no default; if not present and attempting wireless, prompt user)
*   network configuration (gateway, DNS, etc.) (default: use DHCP)
*   hostname or IP address of recovery server (default: Google's update
            server)
*   ssh keys for recovery server, to prevent attackers from presenting a
            simulated server (default: whatever keys we use to sign
            communication with our update server)
*   should the image downloaded from the server be stored to the RSD's
            data partition for future re-use, or not? (default: yes)

## Types of recovery storage device

A recovery storage device is a USB key or SD card containing one or more
recovery kernels and recovery root filesystems, and data to support the
recovery.

(Other kinds of external storage devices may be usable in the future.)

Depending on the contents of the RSD:

*   It may support only one model of netbook, or multiple models.
*   It may load recovery data (rootfs + firmware) from the RSD itself,
            or over the network.

The following subsections describe some typical types of RSDs.

### Single recovery storage device

This is the most common type of RSD. It contains a single bootable recovery
image, and a single set of recovery data. It is self-contained; using it to
recover a device does not require access to the network. This is the type of RSD
created by a recovery installer. It is the least flexible RSD, since it supports
only a single device model.

[<img alt="image"
src="/chromium-os/chromiumos-design-docs/recovery-mode/srjy2ox0dbMkVTv25qW2QdA.png">](/chromium-os/chromiumos-design-docs/recovery-mode/srjy2ox0dbMkVTv25qW2QdA.png)

### Network recovery storage device

This type of RSD loads the Chromium OS software and firmware from a server,
rather than storing it directly on the card.

The recovery software identifies the target device, then downloads matching
recovery data from the server. This allows a single RSD to support multiple
target device models.

The server may be the main Google Chrome OS server provided by Google, or a
local server. This is suitable for a larger corporate, commercial, or
manufacturing environment. Upgrading the recovery data for a netbook model only
requires updating the server, not multiple RSDs. A new model can also be
supported without upgrading the RSDs, as long as the network recovery software
supports the new model's hardware.

[<img alt="image"
src="/chromium-os/chromiumos-design-docs/recovery-mode/sk78RvI_UcGGddwNUBzsAcA.png">](/chromium-os/chromiumos-design-docs/recovery-mode/sk78RvI_UcGGddwNUBzsAcA.png)

Future versions of this specification may allow a PXE boot style implementation,
where the PXE code lives in the recovery kernel instead of the firmware.

## Developer mode

A developer can use recovery mode to load their own OS image onto a Google
Chrome OS device. To do this, all they need to do is make a RSD which contains
developer-signed recovery data in place of the Google-signed recovery data.

See the Developer Mode design document (forthcoming) for more details. Key
points:

*   The device's developer mode switch must be turned on before
            developer-signed images can be loaded.
*   If the kernel on the RSD is signed with a different key than the
            kernel on the destination device, there is a 5-minute delay, during
            which time the user must interact with the screen and keyboard, and
            sound is played. This makes it more difficult to discreetly
            reprogram a device, protecting both users and developers.

For i18n, the developer mode screens must be displayed in the proper language.
If the language is not detected/set by the recovery installer, then recovery
needs to have a language selection screen during startup.

## Network recovery

If appropriate recovery data for the target netbook is not present on the RSD,
the recovery software on the RSD can attempt to download the recovery data from
a server on the network.

To deter spoofing attacks, communication with the server should be done over a
secure channel (such as sftp).

The default behavior is to contact the Google Chrome OS update server, using
wired networking in preference to wireless. This can be overridden via a network
configuration file on the RSD.

Network recovery is useful during manufacturing, to load the initial rootfs onto
the drive. This procedure would look something like this:

1.  As each assembled netbook is ready for programming, a tech
    1.  takes an RSD from a bucket and plugs it into the netbook;
    2.  plugs the netbook into external power (and possibly wired
                network);
    3.  powers on the netbook.
2.  Recovery mode launches, because the netbook's drive has no bootable
            image.
3.  The network configuration file on the RSD points to a manufacturing
            server. The correct image for the netbook is loaded from the server.
4.  When the netbook is done recovering, the tech unplugs the RSD and
            tosses it back in the bucket.

Network recovery is also useful in corporate environments. Tech support can
maintain a single server with all appropriate images; each tech stop location
only needs a few generic RSDs to support all the models of netbooks. To deploy a
new image, only the server needs to be changed.
