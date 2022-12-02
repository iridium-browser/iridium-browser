---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: containers-update
title: L1TF on Chrome OS (a.k.a. Foreshadow & CVE-2018-3646)
---

# Vulnerability Description

Software running in a virtualized operating system may exploit a bug in the
Extended Page Tables implementation to read arbitrary physical memory. This
allows guest operating systems to access memory contents belonging to the host
OS that are meant to be protected from access. This may allow malicious guests
to steal sensitive data from other applications running on the machine.

# Chrome OS response

The vast majority of Chrome OS users are unaffected given that Chrome OS
currently only uses virtualization for running Linux App Containers (aka
Termina), which is under development and only available in Beta, Dev, and Canary
update channels of Chrome OS. Users who don't use the Linux App Containers
feature don't need to take action at this point.

Chrome OS will not release the Linux apps for Chrome OS feature to the stable
channel before September, at which point users will have received the necessary
mitigations in Chrome OS 69 via auto-updates. No further action by stable
channel users are expected to be necessary. Beta, Dev, and Canary channel users
will receive fixes as soon as possible now that public patches are available.
Beta, Dev, and Canary users concerned about the attack in the meantime may
temporarily stop using Linux App Containers.

# Affected devices and Patch Status

Chrome OS devices with Core Intel CPUs running virtual machines are affected.
All listed devices have fixes in Dev, Canary, Beta and Stable builds.

*   Google Pixelbook
*   HP Chromebox G2
*   Samsung Chromebook Plus (V2)
*   Acer Chromebox CXI3
*   HP Chromebook x2
*   ASUS Chromebox 3
*   ViewSonic NMP660 Chromebox
*   CTL Chromebox CBx1
*   Toshiba Chromebook 2 (2015 Edition)
*   Acer Chromebook 11 (C740)
*   Acer Chromebox CXI2
*   Acer Chromebook 15 (C910 / CB5-571)
*   Acer Chromebox CXI3
*   Dell Chromebook 11

For code names, see our [main device
page](/chromium-os/developer-information-for-chrome-os-devices).
