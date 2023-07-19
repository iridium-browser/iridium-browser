---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/moblab
  - MobLab
page_name: overview-of-moblab
title: Overview of MobLab
---

## \*\*\*DEPRECATED, PLEASE VISIT
https://www.chromium.org/chromium-os/testing/moblab \*\*\*
(20170714)

[TOC]

## Introduction

When qualifying a new board for release, the test engineering team for Chrome OS
performs the following tasks:

*   Test and qualify firmware
*   Test the Final Shipping Image (FSI) and Auto-Update (AU) images
*   Add repeatable qualifications for components to the Approved Vendor
            List (AVL)

Done manually, these tests take human testers multiple days to complete. While
the tests cannot be performed in parallel, each one can be automated. To
accelerate board bring-up and ensure quality, the Chrome OS team is in the
process of automating the tests so that partners can run them on site.

Testing at the partner site has numerous benefits for both partners and Google:

*   Shortens the feedback loop for bugs found with automated testing.
*   Enables testing on revisions of boards that Google does not have.
*   Identifies hardware defects before mass production begins.
*   Detects regressions during Final Shipping Image and AU image
            revisions.
*   Enables testing of new hardware components (e.g., replace EOL
            components or evaluate a new variant of an existing SKU, such as a
            touchscreen version).
*   Provides a channel for partners to write, run, and add tests to the
            Google framework.

## What is MobLab?

### Required Hardware

MobLab is a test system in a box: a customized Chrome OS image loaded onto a
Chromebox. The MobLab tests are the same tests that the Chrome OS team at Google
runs in the Chrome OS lab. A few customizations have been added to help
partners, like extra processes to streamline interactions with the test suite.

The hardware specs for the supported Moblab platform as of Oct 1, 2015 are
below. If your setup does not use the hardware below, our team will not be able
to assist in troubleshooting.

<table>
<tr>

<td><b>Hardware </b></td>

<td><b>Specification</b></td>

<td><b>Details </b></td>

</tr>
<tr>

<td>Chromebox </td>

<td>ASUS CN62 with Core i3 processor</td>

<td><a href="https://www.asus.com/Chrome-Devices/ASUS_Chromebox_CN62/specifications/">https://www.asus.com/Chrome-Devices/ASUS_Chromebox_CN62/specifications/</a></td>
</tr>
<tr>

<td>USB-to-ethernet adapter</td>

<td>LINKSYS USB3GIG USB 3.0 GIGABIT ETHERNET ADAPTER</td>

<td>---</td>

<td> Apple USB Ethernet adapter</td>

<td>==<a href="http://www.linksys.com/us/p/P-USB3GIG/">http://www.linksys.com/us/p/P-USB3GIG/</a>==</td>

<td>OR</td>

<td>==<a href="https://www.amazon.com/Linksys-Ethernet-Chromebook-Ultrabook-USB3GIG/dp/B00LIW8TBG/ref=sr_1_1?ie=UTF8&qid=1491807967&sr=8-1&keywords=linksys+usb+ethernet+adapter">https://www.amazon.com/Linksys-Ethernet-Chromebook-Ultrabook-USB3GIG/dp/B00LIW8TBG/ref=sr_1_1?ie=UTF8&qid=1491807967&sr=8-1&keywords=linksys+usb+ethernet+adapter</a>==</td>

<td>---</td>

<td> <a href="http://www.apple.com/tw/shop/product/MC704FE/A/apple-usb-ethernet-adapter">http://www.apple.com/tw/shop/product/MC704FE/A/apple-usb-ethernet-adapter</a></td>
</tr>
<tr>

<td>Internal SSD</td>

<td>Transcend 128 GB SATA III 6Gb/s MTS400 42 mm M.2 SSD Solid State Drive TS128GMTS400</td>

<td><a href="http://www.amazon.com/Transcend-MTS400-Solid-State-TS128GMTS400/dp/B00KLTPUU0">http://www.amazon.com/Transcend-MTS400-Solid-State-TS128GMTS400/dp/B00KLTPUU0</a> </td>

</tr>
<tr>

<td>Switching Hub (optional for multiple DUTs)</td>

<td>Netgear GS105 Unmanaged Switch, Gigabit ethernet switch</td>

<td>==https://www.amazon.com/NETGEAR-GS105NA-Ethernet-Replacement-Unmanaged/dp/B0000BVYT3/ref=sr_1_1?srs=2529990011&ie=UTF8&qid=1492454601&sr=8-1&keywords=GS105==</td>

</tr>
</table>

**Additional requirement for running CTS. Due to the large size of test results
from CTS tests that can eat up 128GB quickly. We also noticed that upgrading
from 4GB memory to 16GB will reduce the DUT crash rate, especially when running
CTS against x86 DUTs.**

<table>
<tr>
<td> <b>Hardware</b></td>
<td><b>Specification </b></td>
<td><b>Example Details </b></td>
<td> Comment</td>
</tr>
<tr>
<td> External Hard Drive</td>
<td> 1TB USB 3.0</td>
<td> <a href="https://www.amazon.com/Elements-Portable-External-Drive-WDBUZG0010BBK-EESN/dp/B00CRZ2PRM/">https://www.amazon.com/Elements-Portable-External-Drive-WDBUZG0010BBK-EESN/dp/B00CRZ2PRM/</a></td>
<td> please follow instruction<a href="/chromium-os/testing/moblab/setup#TOC-Formatting-external-storage-for-MobLab"> here </a>to label the HD correctly "MOBLAB-STORAGE".</td>
</tr>
<tr>
<td> RAM</td>
<td> 16GB</td>
<td><a href="https://www.amazon.com/gp/product/B00J8U549K/ref=oh_aui_search_detailpage?ie=UTF8&psc=1">https://www.amazon.com/gp/product/B00J8U549K/ref=oh_aui_search_detailpage?ie=UTF8&psc=1</a></td>
</tr>
</table>

Note: All of these components can be purchased from:

Sanny Chiu

Synnex Technology International Corp.

Tel: 886-2-2506-3320 Ext 2033

Mobile : 0919-809-819

email: sannychiu@synnex.com.tw

---

## Network Configuration

This diagram shows a typical configuration of MobLab on a partner’s network:

![](/chromium-os/testing/moblab/overview-of-moblab/image00.png)

MobLab establishes a test subnet to isolate testing activities and machines from
the rest of the corporate network. It handles network setup and configuration
tasks, including requesting addresses and configuring services.

The connection to the corporate network and the Internet enables automatic
updates of the MobLab server image, as well as access to Google Cloud Storage.
The corporate network at the partner site should provide security features
(firewall, VPN, etc.) to protect MobLab and the test subnet from potential
external attacks.

---

## Advantages

This section describes the advantages of setting up MobLab and running tests at
your site.

### Easy to set up and manage

MobLab requires virtually no administrative overhead. Storage, network
communication, and software updates are all managed automatically. The latest
software is used for testing, and versions of the operating system and test
suite are always in sync.

### Full lab feature support

Any Autotest suite or individual test case can be run on a single box. This
includes test suites such as build validation tests (BVT), hardware component
qualification tests, and custom automated tests. MobLab can run any test written
using the Autotest framework.

Depending on the type of component being tested, additional equipment may be
required. For example, a WiFi signal is required for wireless connectivity
tests.

### Scaleable

The Autotest framework enables running across multiple machines and scheduling
tests. Simpler tools and test frameworks only support manually launching tests,
or running them on a single machine. With Autotest, you can also define pools of
multiple hosts, to group machines and set scheduling priorities.

### Common ground

Moblab provides a common platform for test infrastructure. Using the same
hardware, software, and test builds makes communication between partners and
Google more efficient. When issues are found, having a common platform makes it
much quicker and easier to reproduce them.

### Open source project

Community support provides greater security and faster response time for
questions and issues. Source files are available for inspection and
customization, and partner contributions are welcome. The only feature
unavailable to custom builds of MobLab is automatic server updates.

## Summary

With MobLab, partners get a robust, low-maintenance test environment where they
can:

*   Run the same tests in a repeatable fashion as in the Chrome OS lab.
*   View test results.
*   Access build images for projects they are responsible for.
*   Run custom local tests that are not a part of the Chromium OS source
            tree.
