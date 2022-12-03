---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: mds
title: Microarchitectural Data Sampling
---

Microarchitectural Data Sampling

# (CVE-2018-12126, CVE-2018-12127, CVE-2018-12130, and CVE-2019-11091)

# Summary

Microarchitectural Data Sampling (MDS) refers to a set of speculative execution
side-channel vulnerabilities which potentially allow results from previous
execution on a core to be observed across security boundaries via
microarchitectural state, on certain Intel CPUs. They are described in [Intel's
announcement](https://www.intel.com/content/www/us/en/security-center/advisory/intel-sa-00233.html),
and referred to as
MSBDS/[CVE-2018-12126](https://cve.mitre.org/cgi-bin/cvename.cgi?name=2018-12126),
MLPDS/[CVE-2018-12127](https://cve.mitre.org/cgi-bin/cvename.cgi?name=2018-12127),
MFBDS/[CVE-2018-12130](https://cve.mitre.org/cgi-bin/cvename.cgi?name=2018-12130),
and
MDSUM/[CVE-2019-11091](https://cve.mitre.org/cgi-bin/cvename.cgi?name=2019-11091).

An attacker successfully exploiting these vulnerabilities could read sensitive
data from other processes running on the system, breaking the isolation between
processes provided by modern operating systems. If Chrome processes are
attacked, these sensitive data could include website contents as well as
passwords, credit card numbers, or cookies.

Chrome, like all programs, relies on the operating system to provide isolation
between processes. Operating system vendors may release updates to improve
isolation, so users should ensure they install any updates and follow any
additional guidance from their operating system vendor in relation to MDS
mitigation.

Some operating system mitigations will also require changes in Chrome which we
shall include in subsequent Chrome releases. Users should ensure their version
of Chrome is [always up to
date](https://support.google.com/chrome/answer/95414?co=GENIE.Platform%3DDesktop).

## Response

The Chrome team investigated various mitigation options Chrome could take
independently of the OS, but none were sufficiently complete or performant.
Users should rely on operating system level mitigations.

### Android

On the Android platform, the vast majority of devices are not affected, as these
issues only apply to some Intel-based systems.

As always, Android users should apply updates for their devices as soon as they
are available from their OEM.

### Chrome OS

Chrome OS has disabled Hyper-Threading on Chrome OS 74 and subsequent versions.
This provides protection against attacks using MDS. [More details on Chrome OS's
response](/chromium-os/mds-on-chromeos).

### macOS

macOS Mojave 10.14.5 [includes MDS
mitigations](https://support.apple.com/en-us/HT210107). These have been adopted
by Chrome and will be included in Chrome 75 which will be released to the Stable
channel on or around the 4th of June.

### Windows

Windows users should apply updates with MDS mitigations as soon as they are
available, and [follow any guidance to adjust system settings if
appropriate](https://portal.msrc.microsoft.com/en-US/security-guidance/advisory/ADV190013).

### iOS

Apple iOS devices use CPUs not known to be vulnerable to MDS.

### Linux

Linux users should apply kernel and CPU microcode updates as soon as they are
available from their distribution vendor, and follow any guidance to adjust
system settings.
