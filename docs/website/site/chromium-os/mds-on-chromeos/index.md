---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: mds-on-chromeos
title: Microarchitectural Data Sampling on Chrome OS
---

Microarchitectural Data Sampling on Chrome OS

(CVE-2018-12126, CVE-2018-12127, CVE-2018-12130, and CVE-2019-11091)

# Vulnerability Impact

Microarchitectural Data Sampling (MDS) is a group of vulnerabilities that allow
an attacker to potentially read sensitive data. If Chrome processes are
attacked, these sensitive data could include website contents as well as
passwords, credit card numbers, or cookies. The vulnerabilities can also be
exploited to read host memory from inside a virtual machine, or for an Android
App to read privileged process memory (e.g. keymaster). See below for affected
devices.

# Chrome OS Response

To protect users, Chrome OS 74 disables Hyper-Threading by default. For the
majority of our users, whose workflows are primarily interactive, this mitigates
the security risk of MDS without a noticeable loss of responsiveness. Chrome OS
75 will contain additional mitigations.

Users concerned about the performance loss, such as those running CPU intensive
workloads, may enable Hyper-Threading on a per machine basis. The setting is
located at chrome://flags#scheduler-configuration. The "performance" setting
chooses the configuration that enables Hyper-Threading. The "conservative"
setting chooses the configuration that disables Hyper-Threading.

Enterprises who wish to set Hyper-Threading policy organizationally may use the
enterprise policy named “SchedulerConfiguration.”

## Hyper-Threading Policy Guidance

The decision to disable or enable Hyper-Threading is a security versus
performance tradeoff. With Hyper-Threading disabled, Intel CPUs may experience
reduced performance, which varies depending on the workload. But, with
Hyper-Threading enabled, users could execute code, such as by visiting a website
or running an Android app, that exploits MDS to read sensitive memory contents.

As of May 14th, 2019, Google is not aware of any active exploitation of the MDS
vulnerabilities. Users and customers who process particularly sensitive data on
their Chrome OS devices are nonetheless advised to disable Hyper-Threading as a
measure of caution.

# Vulnerability Description

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
See below for more details.

## Microarchitectural Store Buffer Data Sampling (MSBDS) and Microarchitectural Fill Buffer Data Sampling (MFBDS)

(CVE-2018-1212 and CVE-2018-12130 respectively)

Intel CPUs use microarchitectural data structures known as the fill buffer and
store buffer. The fill buffer contains loaded data pending insertion into the L1
cache. The store buffer contains stored data pending write to the memory
subsystem. Concurrently executing threads, on the same physical CPU core, may
potentially read the contents of prior entries for these buffers by observing
timing side channels when speculatively executed.

## Microarchitectural Load Port Data Sampling (MLPDS)

(CVE-2018-12127)

Load ports are used by the CPUs to perform load operations from memory or I/O.
The bus in the load ports may retain data from old operations, allowing one
process to leak data from another process through speculative execution side
channels.

## Microarchitectural Data Sampling Uncacheable Memory (MDSUM)

(CVE-2019-11091)

Uncacheable memory (UC) is read from RAM without filling the CPU’s cache with a
new line. However, uncacheable memory does still move through the store buffers,
fill buffers, and load ports;allowing data stored in UC regions to still be
leaked via the mechanisms described above.

# Affected Devices

Chrome OS devices with affected Intel CPUs, supported as of May 14th, 2019, are
as follows:

    AOpen Chromebase Commercial

    AOpen Chromebox Commercial

    ASI Chromebook

    ASUS Chromebook C200MA

    ASUS Chromebook C300MA

    ASUS Chromebook Flip C302

    ASUS Chromebox 3

    ASUS Chromebox CN60

    ASUS Chromebox CN62

    Acer C720 Chromebook

    Acer Chromebase 24

    Acer Chromebook 11 (C740)

    Acer Chromebook 11 (C771 / C771T)

    Acer Chromebook 13 (CB713-1W )

    Acer Chromebook 15 (C910 / CB5-571)

    Acer Chromebook 15 (CB3-531)

    Acer Chromebook Spin 13 (CP713-1WN)

    Acer Chromebox

    Acer Chromebox CXI2

    Acer Chromebox CXI3

    Bobicus Chromebook 11

    CTL Chromebox CBx1

    CTL N6 Education Chromebook

    Chromebook 11 (C730 / CB3-111)

    Chromebook 11 (C735)

    Chromebook 14 for work (CP5-471)

    Chromebox Reference

    Consumer Chromebook

    Crambo Chromebook

    Dell Chromebook 11

    Dell Chromebook 11 (3120)

    Dell Chromebook 13 3380

    Dell Chromebook 13 7310

    Dell Chromebox

    Dell Inspiron Chromebook 14 2-in-1 7486

    Education Chromebook

    eduGear Chromebook R

    Edxis Chromebook

    Edxis Education Chromebook

    Google Chromebook Pixel (2015)

    Google Pixelbook

    HEXA Chromebook Pi

    HP Chromebook 11 2100-2199 / HP Chromebook 11 G3

    HP Chromebook 11 2200-2299 / HP Chromebook 11 G4/G4 EE

    HP Chromebook 13 G1

    HP Chromebook 14

    HP Chromebook 14 ak000-099 / HP Chromebook 14 G4

    HP Chromebook x2

    HP Chromebook x360 14

    HP Chromebox CB1-(000-099) / HP Chromebox G1/ HP Chromebox for Meetings

    HP Chromebox G2

    Haier Chromebook 11 G2

    JP Sa Couto Chromebook

    LG Chromebase 22CB25S

    LG Chromebase 22CV241

    Lenovo 100S Chromebook

    Lenovo N20 Chromebook

    Lenovo N21 Chromebook

    Lenovo ThinkCentre Chromebox

    Lenovo ThinkPad 11e Chromebook

    Lenovo Thinkpad X131e Chromebook

    M&A Chromebook

    Pixel Slate

    RGS Education Chromebook

    Samsung Chromebook 2 11 - XE500C12

    Samsung Chromebook Plus (LTE)

    Samsung Chromebook Plus (V2)

    Samsung Chromebook Pro

    Senkatel C1101 Chromebook

    Thinkpad 13 Chromebook

    Toshiba Chromebook

    Toshiba Chromebook 2

    Toshiba Chromebook 2 (2015 Edition)

    True IDC Chromebook

    Videonet Chromebook

    ViewSonic NMP660 Chromebox

    Yoga C630 Chromebook
