---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/packages
  - packages
page_name: cros-flashrom
title: flashrom
---

[TOC]

## Overview

Flashrom is a userspace utility we use primarily to update host (AP/PCH)
firmware, but can also be used to update firmware on other components such as
ECs. it can be run both on the target machine (field updates) or on a user's
workstation to flash via an external programmer such as Servo, Dediprog SF100,
Bus Pirate and more.

Flashrom is an open-source project hosted at <http://flashrom.org>. The version
which is used in Chrome OS devices and in the CrOS SDK is a forked version with
some CrOS-specific modifications. It can be found at the following URL:
<https://chromium.googlesource.com/chromiumos/third_party/flashrom/>

## Basic Usage

This section will cover basic command-line usage. The most commonly used
command-line switches are:

-r | --read read flash and save to file

-w | --write write to flash

-p | --programmer &lt;name&gt;\[:&lt;param\] specify the programmer device and
parameters

The "-p" argument tells us which programmer interface we are targeting. Examples
include "host", "ec", or "ft2232_spi:type=servo-v2" (see section on Servo
below). For a full listing of command-line switches, use -h or --help.

The following commands have to do with
[write-protection](/chromium-os/firmware-porting-guide/firmware-ec-write-protection):

--wp-status show write protect status

--wp-range set write protect range
--wp-enable enable write protection
--wp-disable disable write protection

Some examples of basic usage, these are all run on the device itself:

If you want to read the firmware from the SPI:

$ flashrom -p host -r image.bin

If you want to write a firmware image to the SPI:

$ flashrom -p host -w image.bin

If you want to read the ec image from the EC:

$ flashrom -p ec -r ec.bin

If you want to write an ec image to the EC:

$ flashrom -p ec -w ec.bin

If you just want to update your firmware and EC to the latest version in your
Chrome OS system, ignoring any safety checks, run the below which invokes
flashrom for you:

$ chromeos-firmwareupdate --mode=incompatible_update

**Warning:** when running flashrom on an x86 host, the Intel Management Engine
(ME) firmware region will be read back as 0xff bytes, and writes to the ME
region will be silently discarded. This means that if you make a backup of your
flash contents with flashrom -p host -r backup.bin and then try to restore
backup.bin via servo, the ME region will be overwritten with 0xff bytes and your
system will no longer boot. The ME region can be identified by running dump_fmap
on the firmware image; here is an example from Link:

area: 3

area_offset: 0x00001000

area_size: 0x001ff000 (2093056)

area_name: SI_ME

The /usr/sbin/chromeos-firmwareupdate script contains a copy of the factory BIOS
image (including the ME firmware), if you've already run into this problem.

## **Servo**

Servo has an FT2232 USB &lt;--&gt; SPI interface which Flashrom can use to pass
commands thru to the SPI chip. All you need to do is set the programmer target
(-p ft2232_spi:type=servo-v2) and enable/disable the buffers on the flex cable
using dut-control (from the **hdctools** package).

One thing to watch for is the voltage argument to dut-control. Most SPI chips
operate at either 1.8V or 3.3V. This is set by spi2_vref:ppXXXX where XXXX is
1800 or 3300.

Example for programming a 1.8V SPI chip:

$ dut-control spi2_vref:pp1800 spi2_buf_en:on spi2_buf_on_flex_en:on
spi_hold:off cold_reset:on

$ sudo /usr/sbin/flashrom -p ft2232_spi:type=servo-v2 -w &lt;image to
program&gt;

$ dut-control spi2_vref:off spi2_buf_en:off spi2_buf_on_flex_en:off spi_hold:off
cold_reset:off

To control hardware write-protect on DUT using Servo:

$ dut-control fw_wp_en:on fw_wp_vref:pp1800 # Enable WP override on Servo, this
example assumes 1.8V chip

$ dut-control fw_wp:off # Force WP off

$ sudo /usr/sbin/flashrom -p ft2232_spi:type=servo-v2 --wp-disable # Disable
software WP (--wp-enable and --wp-range could also be done here)

$ dut-control fw_wp_en:off fw_wp_vref:off # Disable WP override on Servo.

In this example we enable WP override, force the signal "off" (drive it high,
even if /WP is asserted by screw/H1/etc), disable software WP, and then disable
WP override. With software WP disabled we can write to the entire chip
regardless of the hardware WP status.

More details about how to run it with Servo are available
[here](/chromium-os/servo) and [here (for Googlers only)](http://go/cros-servo).

## **Partial Reads and Writes**

Flashrom can be instructed to target only a specific region on the flash chip.
This allows scripts (especially factory installer and autoupdate) to selectively
update regions without spending a lot of time reading or updating the entire
flash memory.

### **Selectively include regions**

To include a region in a read/write operation, use the -i option:

-i | --image &lt;name&gt;\[:&lt;file&gt;\] only access image &lt;name&gt; from
flash layout

The \[:&lt;file&gt;\] argument is optional and has an interesting effect on -r
and -w. If a file is not supplied, flashrom will operate on a file that is the
same size as the targeted flash chip. For example, if you wish to write sections
FOO and BAR from a 4MB file to a 4MB flash chip, you could do: flashrom -p
&lt;programmer&gt; -i FOO -i BAR -w filename.bin

If files are specified as arguments to -i options, flashrom will not require an
argument to -w or -r and instead will work with the files supplied via -i
arguments. So if you have separate files for FOO and BAR which are not part of a
larger image, let's call them foo.bin and bar.bin, you can write them to the
firmware ROM like so: flashrom -p &lt;programmer&gt; -i FOO:foo.bin BAR:bar.bin
-w

One can also use a combination:

*   In the read case, supplying a filename as an argument to "-r" will
            produce a ROM-sized file in addition to files produced for "-i"
            arguments which specify a filename.
*   In the write case, filenames supplied as arguments to "-i" options
            take precedence. This allows the user to supply a ROM-sized file to
            "-w" but also patch its content using files supplied to "-i". for
            example, if you have a standard ROM image but you want to apply
            custom data to a specific region when flashing.

### **Layout**

When -i is used, Flashrom will search for the flashmap (fmap) and compare region
names with the name supplied by the user. If the user wishes to supply a custom
mapping instead of using the flashmap, a layout file can be used:

-l | --layout read ROM layout from &lt;file&gt;

For example, this will write the custom-defined ranges from the supplied
filename to the ROM:

echo "000000:0fffff FOO" &gt; layout.txt

echo "200000:2fffff BAR" &gt;&gt; layout.txt

flashrom -p &lt;programmer&gt; -l layout.txt -i FOO -i BAR -w filename.bin

### **Selective verification of partial writes**

By default, flashrom verifies the result of any destructive operation by reading
and comparing the entire ROM. To only verify portions which have supposedly been
written, --fast-verify can be used:

--fast-verify only verify -i part

This is generally considered safe since a discrepancy in the partially written
portions will indicate that something is likely wrong elsewhere.

TODO(dhendrix): That should probably be renamed "--partial-verify" or something.

### **Speeding Up Writes**

This section describes some shortcuts that can be useful for developers. **Do
not rely on the behavior described below for production code**. It's really only
intended to help developers iterate faster.

Erase and write operations are orders of magnitude slower than reads on typical
flash memory devices. Thus, flashrom figures out which blocks need to be updated
so that it can try and skip as many as possible. The basic steps are:

1.  Read entire ROM to obtain a reference image before any destructive
            operation takes place.
2.  Go thru write algorithm and only erase/write blocks as needed.
3.  Read the ROM again to verify that it matches the supplied file.

The main way to speed things up is to skip the first and last verification
steps.

**Skipping verification**

If you're confident that the bits will land where they need to, you can save
time by skipping the verification step.

-n | --noverify don't auto-verify

**Supplying original ROM content with a file**

This will allow you to skip the initial step of obtaining a reference image:

--diff &lt;file&gt; diff from file instead of ROM

This tells flashrom that the file supplied as an argument to --diff will be used
as a reference image, so it will skip reading the ROM content. This is useful if
you can track exactly what is on the ROM, but can be lead to frustrating bugs if
the contents get out of sync. **Use with caution!**

Example (note: this assumes that you have previously flashed "old.bin"):

flashrom -p ft2232_spi:type=servo-v2 -w new.bin --diff old.bin **&& cp new.bin
old.bin**

## **Power management pitfalls to watch out for**

In short:

*   System must remain running (given).
*   powerd must not tickle the embedded controller (See "Embedded
            Controller Interactions" below).
*   Performance should not be significantly degraded.

Chrome OS devices usually read firmware at least once per boot-up often to
obtain things contained in VPD (e.g. antennae calibration data), populate
entries for chrome://system, and small system maintenance tasks.

## **Embedded Controller interactions**

Embedded Controllers are independent microcontrollers which coexist in a
platform to service some power management routines via ACPI or a other
interface. This often includes handling lid switch events, sensors (such as
ambient light sensor), thermal management, etc.

The way they are designed can make firmware updates difficult. To cut down on
cost, often times they lack internal flash memory and have limited amount of
SRAM so they need to access the firmware ROM to load new routines. To make
matters more complicated some systems use the same firmware ROM for both the
host (PCH, SoC, etc) and the EC.

In rare circumstances where we're updating the EC firmware itself, Flashrom will
pause the EC. However, doing so has highly visible side-effects such as
disabling keyboard, causing the fan to spin up all the way, or dimming the
screen. For more frequent host firmware updates, which can take a long time, we
have historically relied on stopping powerd and then restarting it after we're
done. This avoids the problem of interrupting the EC such that it needs to load
more code, and avoids UI jank that can last up to a minute and cause a user to
try to reboot the system. For historical context, see
[chromium-os:18895](https://code.google.com/p/chromium/issues/detail?id=201998).

#### Shared SPI block diagram

#### Non-shared SPI block diagram

## Performance

We like firmware updates to be done as quickly as possible. This is important
not only to minimize any side-effects the user might see as well as overall
risk, but also to optimize time spent on the assembly line when a device is
being manufactured
([chromium-os:14139](https://code.google.com/p/chromium/issues/detail?id=197236)).

Some older systems had performance issues when flashrom ran if powerd was
enabled. This was due to poor DVFS implementations that could dramatically
affect the behavior of the kernel's scheduler and cause huge delays when running
flashrom.

One side-effect was that flashrom would take an excessively long time to
complete due to bad usleep() behavior
([chromium-os:15025](https://code.google.com/p/chromium/issues/detail?id=198133)).
However, when we disabled powerd we found that the UI would become janky
([chromium-os:19321](https://code.google.com/p/chromium/issues/detail?id=202422)).
Eventually settled on disabling powerd only for operations expected to take a
long time and are destructive (erase/writes).

Now that powerd is smarter, we stopped disabling powerd
([chromium:400641](https://code.google.com/p/chromium/issues/detail?id=400641))
and expect it to avoid doing anything which may degrade performance while
flashrom is running.

## Testing

Recent (ca. 2016) improvements have been made to testing capabilities. A new set
of test scripts has been merged and flashrom is gaining some built-in testing
capabilities.

**Built-in Test**

Built-in tests are currently limited to write-protect testing using the
"--wp-test" option. Flashrom will read the write-protect status and attempt to
overwrite the protected region using all the available sector, block, and chip
erase commands.

**Test Script v2**

The test script is used primarily for regression testing. It accepts two
flashrom binaries as inputs, one which is presumed to be stable ("old") and one
which is to be tested ("new").

The test can be performed on a local host or a remote host with SSH access.
Additionally, an external programmer (such as Servo, Dediprog, etc) can be used
as part of the test setup.

Documentation for test script v2: <https://goo.gl/3jNoL7>

#### Flashrom Test Script v2 ‎‎(master doc)‎‎