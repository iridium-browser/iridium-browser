---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: bias-on-chromium-os
title: Bluetooth BIAS on Chrome OS
---

Bluetooth BIAS on Chrome OS

## (CVE-2020-10135)

# Vulnerability Impact

BIAS is a group of vulnerabilities in the Bluetooth (BT) specification which
allow an attacker to impersonate a BT accessory after pairing. This is
considered low severity on Chrome OS as no known features rely solely on the BT
accessory claiming successful authentication.

# Vulnerability Description

The BT standard does not mandate mutual authentication. An attacker’s BT device
can authenticate with the OS and advertise to the host OS that it is
authenticated as an already paired keyboard. If the host OS does not perform a
mutual authentication, but assumes that the BT device’s authentication message
is legitimate, the BT device in question could be a different device than which
was originally paired.

# Chrome OS Response

Chrome OS has audited our BT implementation for any code that makes trust
decisions based on the HCI_Authentication_Complete event. Bluetooth chipsets are
encouraged to enforce mutual authentication. Chrome OS is also working on future
specification changes with the Bluetooth community. Chrome OS includes the patch
for [CVE-2019-9506](https://knobattack.com/), which mitigates the risks posed by
the BIAS attacks.

Affected Devices

All Chrome OS devices and versions are impacted.
