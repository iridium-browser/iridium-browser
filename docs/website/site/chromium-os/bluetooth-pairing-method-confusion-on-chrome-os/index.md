---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: bluetooth-pairing-method-confusion-on-chrome-os
title: Bluetooth Pairing Method Confusion on Chrome OS
---

Bluetooth Pairing Method Confusion on Chrome OS

## (CVE-2020-10134)

# Vulnerability Impact

Method Confusion is a vulnerability in the Bluetooth (BT) specification, which
allows an attacker to sit in the middle of a pairing operation between BT
accessory and host. Once pairing is complete, all data could be intercepted
and/or modified in transit by the attacker. For example, if Method Confusion is
used while pairing a BT keyboard, an attacker can read all of a user’s
keystrokes (including passwords and other sensitive information that is entered
via the keyboard) or inject keystrokes on the user’s behalf.

# Vulnerability Description

BT hosts and accessories use a variety of authentication mechanisms during
pairing. The host and accessory do not mutually authenticate that they used the
same method. In particular, the specifics of the Numeric Comparison (NC), and
the Passkey Entry (PE) authentication mechanisms, mean that an attacker with
physical proximity can use their device to insert themselves into the
connection. By exploiting NC with the first target device, the attacker could
leverage PE authentication with the other target device to use the confirmation
number as the passkey.

# Chrome OS Response

Chrome OS is exploring user interface changes to help a user discern if the
pairing process is secure as a short term mitigation strategy. Longer term,
Chrome OS is discussing specification changes and technical fixes with the BT
community. Users who are especially concerned about this should not pair new
devices in public settings, where an adversary could be seated within close
proximity.

Affected Devices

All Chrome OS devices and versions are impacted.
