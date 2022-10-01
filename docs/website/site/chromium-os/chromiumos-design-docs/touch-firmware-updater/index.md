---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: touch-firmware-updater
title: Touch Firmware Updater
---

[TOC]

## Introduction

Touchpads and touchscreens are among the most sensitive input devices integrated
into computing systems today. The complexity of touch controllers has increased
so that touch devices can be thought of as small embedded devices, with many
features and performance determined by upgradeable firmware. Throughout
development and the lifespan of a Chromebook, firmware updates of the touch
device may occur. This page provides an overview of how Chrome OS handles touch
device firmware updates.

## Bus overview

Current generation Chromebooks use I2C (and the SMBus variant) as the underlying
communication bus between the host system (where Chrome OS and the kernel driver
run) and the touch device controller (where the firmware runs). I2C was chosen
because it provides a good balance between power draw, complexity, and
bandwidth. The bus is also well-supported on ARM based platforms. In addition to
I2C for data, an interrupt line is routed from the trackpad to the host to
signal the kernel driver when it is time to query for new data from the device
during operation (and optionally signal state transitions during a firmware
update).

## Kernel driver

Most of the actual work of moving of firmware bits across the bus is done by the
vendor-specific kernel driver or user-space utility. The driver or utility sends
commands to the device to ready it to accept a new firmware payload. Some
trackpad vendors refer to this mode as a “bootloader” mode. The bootloader is a
last resort in case the more complex operational mode firmware is corrupted. A
trackpad without a valid firmware needs to at least be able to accept a working
payload via an update.

The driver or utility sends the firmware binary over the i2c bus to the device’s
bootloader, using the vendor-specific protocol.

While the protocol from the kernel driver to the touch device for the firmware
update is vendor-specific, [Chrome OS requires that the kernel driver use the
request_firmware hotplug
interface](https://www.kernel.org/doc/Documentation/firmware_class/README) to
expose the same interface to userspace:

At a high level, request_firmware allows the kernel driver to access a file for
read operations at /lib/firmware in the root file system.

The device driver must support the following sysfs attributes for version
management:

*   **firmware_version** or **fw_version** - identifies the current
            version number of the firmware loaded on the device, in the form
            &lt;*major_version*&gt;.&lt;*minor_version*&gt;
*   **product_id** or **hw_version** - a unique string that can identify
            the hardware version in the case that the same driver may be used by
            multiple device variants.

For example, a Chromebook Pixel has two Atmel mXT devices. The hw_version of the
mXT224SL trackpad is 130.1. The hw_version of the mXT1664S touchscreen is 162.0.

Finally, one sysfs attribute must be provided to trigger a firmware update
programmatically from user space:

*   **update_fw** - should be writable by the root user. It should also
            be able to accept a different filename in case more than one device
            uses the driver on the system, and therefore, two different
            firmwares are meant for different targets.

The following example shows how the two Pixel touch devices share the same
driver, but appear as separate instances in sysfs :

echo “maxtouch-tp.fw” &gt; /sys/bus/i2c/devices/1-004b/update_fw echo
“maxtouch-ts.fw” &gt; /sys/bus/i2c/devices/2-004a/update_fw

For examples, see the
[cypress](https://chromium.googlesource.com/chromiumos/third_party/kernel/+/refs/heads/chromeos-3.4/drivers/input/mouse/cyapa.c)
and
[atmel](https://chromium.googlesource.com/chromiumos/third_party/kernel/+/refs/heads/chromeos-3.4/drivers/input/touchscreen/atmel_mxt_ts.c)
drivers in the chromeos-kernel tree.

**Userspace updater**

If the vendor uses I2C-HID kernel driver, they may use a user-space utility to
update the device.

## Userspace organization and scripts

The touch firmware updater consists of a set of scripts and firmware files. The
firmware update script is available in a [Chromium OS source
tree](http://git.chromium.org/gitweb/?p=chromiumos/platform/touch_updater.git;a=tree;f=scripts).

The files used by the touch firmware updater (as seen on the filesystem of a
target system) are organized as follows:

/opt/google/touch/scripts/chromeos-touch-firmware-update.sh
/opt/google/touch/firmware/CYTRA-116002-00_2.9.bin
/lib/firmware/cyapa.bin-&gt;/opt/google/touchpad/firmware/CYTRA-116002-00_2.9.bin

Note that `/lib/firmware/cyapa.bin` is a symlink that links to the current
latest version of firmware elsewhere in the rootfs.

On a production Chrome OS system, the rootfs is protected as a part of Chrome OS
verified boot, so neither the scripts nor the firmware binary can be changed by
the user. Each version of Chrome OS ships with a firmware binary for every touch
device in the system installed in the rootfs. Following an auto-update, on boot
the touch firmware update script probes the device's current firmware version
and may trigger a firmware update if the current version is not compatible.

The name of the firmware binary is used to identify the version of the firmware
binary by the chromeos-touch-firmwareupdate.sh. It should be in the format :
&lt;*product_id*&gt;_&lt;*firmware_version*&gt;.bin

In the above example, the product_id is CYTRA-116002-00, and the version is 2.9.

## Firmware update sequence

The touch firmware update runs after the kernel device driver has successfully
probed the touch device.

The chromeos-touch-update upstart job runs before the Chrome OS UI has started.
It blocks the UI job from starting until a firmware version check and potential
update are completed. The UI job is blocked because the touchpad or touchscreen
are nonresponsive for the duration of the update, and during this process a user
should not be able to interact with the device.

For details, see the
[chromeos-touch-update.conf](http://git.chromium.org/gitweb/?p=chromiumos/platform/touch_updater.git;a=blob;f=scripts/chromeos-touch-update.conf;hb=HEAD)
job.

The updater also runs as a part of the recovery process from a recovery image
booted via USB.

## Error recovery

A typical update requires between 8 and 18 seconds. The firmware update runs
before the Chrome OS UI starts (the user sees the splash screen) to prevent the
user from inadvertently causing the update to fail.

However, it is still possible that a firmware update might not complete properly
if the system shuts down due to low battery. In this case, the operational mode
firmware will be corrupted. The next time the device boots, it will not be
possible to probe the device and have a functional touch device.

For error recovery, it is a requirement for the kernel device driver to
recognize this condition and probe successfully into a bootloader-only mode.
From this state, the driver must be able to perform a forced update to a known
good firmware. This means the update_fw sysfs property must exist and perform
the same task in this error condition as during a normal firmware update.
