---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-information-for-chrome-os-devices
  - Developer Information for Chrome OS Devices
page_name: h2c-firmware
title: Insyde H2O Firmware
---

## Introduction

The first few Google Chrome OS devices used a custom firmware based on a
proprietary base often referred to as "H2C". It's a 64 bit UEFI BIOS from
[Insyde](http://www.insyde.com/) named H2O ("Hardware-2-Operating System"), but
a lot of code/features have been stripped out to significantly speed things up.
Basically, everything not needed to boot Chrome OS was removed.

Unfortunately, this also means the firmware source cannot be released.

## Version Info

The version numbers can be broken down like so:

*   &lt;board&gt;.&lt;Insyde H2O core version&gt;.&lt;Google H2C
            version&gt;.&lt;release number&gt;
    *   the board will be Mario for the CR-48
    *   the H2O version will be three dotted values and really only have
                meaning to Insyde
    *   the H2C (Hardware to Chrome) version is:
        *   Insyde release of 4 digits
        *   the letter 'G'
        *   a number incremented by Google for each release
    *   the release number for the project itself:
        *   Insyde release of 4 digits
        *   letters incremented by Google for each release

Here is an example from the CR-48:

*   Mario.03.60.1120.0038G5.0018d
    *   board: Mario
    *   H2O core version: 03.60.1120
    *   H2C version: 0038G5
        *   Insyde release: 0038
        *   Google release: 5
    *   Project version: 0018d
        *   Insyde release: 0018
        *   Google release: d (4th -&gt; d)
