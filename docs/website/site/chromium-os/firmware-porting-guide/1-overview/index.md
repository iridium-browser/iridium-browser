---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/firmware-porting-guide
  - Firmware Overview and Porting Guide
page_name: 1-overview
title: 1. Overview of the Porting Process
---

[TOC]

## Basic Tasks

U-Boot is a highly portable boot loader, supporting many high-end ARM SoCs. It
contains a vast array of features and drivers to help speed up the porting
process. When porting U-Boot to a new board, follow these basic steps:

1.  Add support for your SoC. U-Boot supports many common SoCs, and in
            many cases, a new SoC is a variant of an existing one that can serve
            as a starting point for the port.
2.  Add a board configuration file and a board implementation file
            containing code specific to your board.
3.  Add or modify any drivers you need for your platform.
4.  Bring in the kernel device tree file if available (or write a new
            device tree).

This guide describes these steps in some detail, with a focus on Chrome
OS-specific implementations.

Note: The following information was written for Tegra in 2011 and is somewhat
out of date.

## Development Flow

Chromium OS uses U-Boot as its boot loader on ARM and x86. This page describes
the development flow, requirements for commits submitted to the project,
upstreaming responsibilities and Chromium OS-specific features.

Before reading this document, please see <http://www.denx.de/wiki/U-Boot> and at
the very least, read:

*   <http://www.denx.de/wiki/U-Boot/DesignPrinciples>
*   <http://www.denx.de/wiki/U-Boot/CodingStyle>
*   <http://www.denx.de/wiki/U-Boot/Patches>
*   and check out the U-Boot mailing list

Except during a rebase period, Chromium OS has a single U-Boot repository,
called ‘u-boot.git’, containing all U-Boot source code. There is no
split between different SOCs, no split for Chromium OS-specific code, no split
at all. We deal with the different types of commits using commit tags.

### Commits

*   Formalized commit structure rules (see ‘Commit Rules’ below)
*   All Chromium OS source is in its own subdirectory (see ‘Chromium
            Source Subdirectory’ below)
*   Encourage compliance with pre-submit checks and reverts
*   Tag every commit to indicate its intended destination
*   Each commit must be producing a source tree which builds and runs
            (bisectability)

### Upstreaming

We plan to upstream most or all of our code. Any code that you write will likely
either be dropped or sent upstream. Please consider when writing code that it
should fit with U-Boot conventions
Let’s call the person who is upstreaming a particular feature the ‘canoe
captain’.

*   The canoe captain (with upstreaming responsibility) lies with:
    *   Committer for patches against upstream code
    *   SOC Vendor for patches against vendor code
*   Clearly mark commits that are destined for upstream according to
            destination
*   Open an ‘upstream needed’ tracking bug when you begin upstreaming
            work. There should be one bug per group of related commits. For
            example ‘upstream I2C driver for Tegra’.
*   **Patches should be sent upstream within one month**
*   Canoe captain is responsible for rebasing those commits which are
            not upstream
*   Code should be accepted upstream in the next merge cycle (i.e.
            within 3 months)
*   Should upstream activity produce the need to adjust or rewrite the
            commit, then this responsibility lies with the canoe captain.

### Code Review

*   Add 2-3 reviewers, including 1-2 original code authors
    *   For vendor code, at least one reviewer or the author should be
                with the vendor company
    *   At least one reviewer should be a Chromium OS U-Boot committer
    *   Vendor code requires an LGTM from the both types of reviewers;
                other code just needs an LGTM from the latter
*   Please first carefully check you have followed the rules on commit
            tags, BUG and TEST
*   Initial reviews should be completed within one day during the
            working week
    *   If not possible, then please add a comment to the commit letting
                the author know there will be a delay, and offer an alternative
*   Rebases and minor don’t need a fresh LGTM. So if you get an LGTM on
            patch 3, and just fix requested nits or do a rebase to get patch 4,
            you can push it.

### Code Refactors

*   We encourage discussion of planned refactors using the issue tracker
            (crbug.com). Those which are in upstream code should be discussed on
            the U-Boot mailing list

### Testing

Most testing is currently manual. It is important that people can repeat your
test steps later without a deep knowledge of your environment, board, commit and
mind.

You may find the
[crosfw](/chromium-os/firmware-porting-guide/1-overview/the-crosfw-script)
script useful for building and testing U-Boot.

**Mandatory testing:**

*   **Build and boot** through to kernel (verified or non-verified) on
            your chosen hardware
    *   Check that you change has not affected operation of peripherals
                related to your change
    *   Add a note that you did this (and the result) in your TEST=
                description (see below)
*   If you change modifies code accessible from another architecture,
            **build** for that architecture too
    *   So if your chosen hardware is ARM, but you are not changing
                ARM-specific code, you should build for x86
    *   Add a note about this to TEST=

Suggested testing:

*   If changing generic code, you should ensure that U-Boot continues to
            build for all targets (run MAKEALL)
    *   Add a note about this to TEST=

### Commit Tags

All commits should have exactly one tag from the following table at the start of
the subject line. The tag indicates the intended destination for the commit.
Note that standard upstream commit tags are encouraged also (i2c, usb, net, lcd,
spi) and these follow the primary tag. Tags are always lower case.

Commits cannot normally span more than one of the paths below, so commits in
multiple areas must be split into different commits. The idea to keep in mind is
that this is like having separate repositories, but with less admin/productivity
overhead.

Where a change requires that (say) Chromium, Tegra and Samsung code must change
at the same time to avoid a build breakage, the ‘split’ tag should be used
first, and the rest in alphabetical order as in ‘split: chromium: exynos: tegra:
fiddle with something’.

<table>
<tr>
<td><b>Tag:</b></td>
<td><b>Meaning</b></td>
<td><b>Canoe Captain</b></td>
<td><b>Path</b></td>
</tr>
<tr>
<td>cros:</td>
<td>Local patches, will not conceivably go upstream. Use for verified boot mainly</td>
<td>-</td>
<td>cros/</td>
</tr>
<tr>
<td>tegra:</td>
<td>Tegra SOC, board and driver code</td>
<td>Nvidia</td>
<td>arch/arm/cpu/armv7/tegra\*/</td>
<td> board/nvidia/</td>
<td> drivers/</td>
</tr>
<tr>
<td>exynos:</td>
<td>Samsung SOC, board and driver code</td>
<td>Samsung</td>
<td>arch/arm/cpu/armv7/exynos\*/</td>
<td> board/samsung/</td>
<td> drivers/</td>
</tr>
<tr>
<td>arm:</td>
<td>ARM-generic patches, committer mans the canoe</td>
<td>Committer</td>
<td>arch/arm</td>
<td> (but not in an soc subdir)</td>
</tr>
<tr>
<td>x86:</td>
<td>x86-generic patches, committer mans the canoe</td>
<td>Committer</td>
</tr>
<tr>
<td>upstream:</td>
<td>Pulled in from upstream</td>
<td>-</td>
<td>any</td>
</tr>
<tr>
<td>drop:</td>
<td>Urgent / unclean / factory patches which we will drop on next rebase</td>
<td>-</td>
<td>any</td>
</tr>
<tr>
<td>split:</td>
<td>Indicates a commit which must be split to send upstream</td>
</tr>
<tr>
<td>gen:</td>
<td>Generic (not architecture-specific) commit which should go upstream</td>
<td>Committer</td>
<td>any. Note that when sending upstream this tag is dropped</td>
</tr>
<tr>
<td> config:</td>
<td> Change to a board configuration file</td>
<td> -</td>
<td> include/configs/</td>
</tr>
</table>

#### Pre-upload Checks

Before uploading a commit for review, please do the following checks:

*   The commit is ‘checkpatch’ clean (repo upload checks this)
*   The commit has tags indicated its upstream destination
*   The commit has a valid bug referenced, by a tag on a line by itself
*   The commit has a valid test case.

### BUG=

*   For public bugs: BUG=chromium-os:12435
*   For private bugs: BUG=chrome-os-partner:1234
*   It is possible to use BUG=none in some cases. We should use this
            rarely. Examples where is it useful are when:
    *   The commit is a small clean-up not related to a particular
                subsystem. Examples are fixing a comment or code style
    *   The commit fixes compilation warnings
*   In most other cases, if you cannot find a bug to reference, you
            should file a new public bug and reference that.

### TEST=

This should be detailed enough that another person without particular knowledge
of your code can run the procedure you detail, and verify your commit. The
information should specific. Examples are:

*   TEST=build and run on daisy; run ‘bootp’ command, see that it finds
            an IP address
*   TEST=manual
    . after enabling configuration necessary for this code to compile
    in, added a printout statement in main.c to report the clock
    frequency and to print a few strings at get_tbclk() tick
    intervals. The clock frequency was correctly reported at 1800 MHz
    and the printouts came out one second apart.
*   TEST=boot from SD card; run sd_to_spi; see that it completes; then
            boot from
    SPI and see that the U-Boot timestamp is correct.
*   TEST=build okay and boot on Daisy

Some bad examples of test information are:

*   TEST=tested on daisy (how? need detail on what was done and what
            result as obtained)
*   TEST=none (surely you are least built it, did you run it on any
            board?)
*   TEST=verified that it worked well (how?)

### Chromium Source Subdirectory

Our local code commits can be broken into several areas, but almost all code is
in a ‘cros/’ subdirectory. Here is the current split:

<table>
<tr>
<td><b>Directory</b></td>
<td><b>Used for</b></td>
<td><b>Previously</b></td>
</tr>
<tr>
<td>cros/</td>
<td>All Chromium OS commits</td>
<td>Various</td>
</tr>
<tr>
<td>cros/tegra</td>
<td>Implementation of Chrome OS low-level API on Tegra SOCs</td>
<td>board/nvidia/chromeos</td>
</tr>
<tr>
<td>cros/exynos</td>
<td>Implementation of Chrome OS low-level API on Exynos SOCs</td>
<td>-</td>
</tr>
<tr>
<td>cros/coreboot</td>
<td>Implementation of Chrome OS low-level API on Coreboot</td>
<td>board/chromebook-x86/chromeos</td>
</tr>
<tr>
<td>cros/lib</td>
<td>Chrome OS library functions</td>
<td>lib/chromeos</td>
</tr>
<tr>
<td>cros/vboot</td>
<td>Verified boot exported functions</td>
<td>lib/vbexport</td>
</tr>
<tr>
<td>cros/cmd</td>
<td>Chrome OS commands</td>
<td>common/cmd_....c</td>
</tr>
<tr>
<td>cros/include</td>
<td>Chrome OS headers</td>
<td>include/chromeos</td>
</tr>
</table>

### Other sections in the U-Boot Porting Guide

1. [Overview of the Porting
Process](/chromium-os/firmware-porting-guide/1-overview) (this page)

2. [Concepts](/chromium-os/firmware-porting-guide/2-concepts)

3. [U-Boot Drivers](/chromium-os/firmware-porting-guide/u-boot-drivers)
