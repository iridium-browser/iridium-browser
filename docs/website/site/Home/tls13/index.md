---
breadcrumbs:
- - /Home
  - Chromium
page_name: tls13
title: TLS 1.3
---

Chrome enabled TLS 1.3 in Chrome 70. However, due to bugs in some
man-in-the-middle proxies, anti-downgrade enforcement was not enabled. The
problematic proxies in question are duplicating a value in the TLS handshake
from the origin server rather than randomly generating it themselves. Firstly,
this means that they're implementing a slightly different protocol than TLS on
their LAN side, the security properties of which are not clear. Secondly, it
means that these proxies appear to TLS 1.3-enabled clients to be signaling that
a downgrade attack is occurring because they're taking TLS 1.3-based values from
the origin server and using them in a lower-version TLS handshake on the LAN
side.

In Chrome 72, downgrade protection will be enabled for TLS connections that use
certificates that chain to a public CA. This should not affect MITM proxies
since they cannot use publicly-trusted certificates. However, in order to get to
the point where this workaround can be removed, all affected MITM proxies will
need to be updated. The following lists minimum firmware versions for affected
products that we're aware of:

Palo Alto Networks:

*   PAN-OS 8.1 must be ≥ 8.1.4
*   PAN-OS 8.0 must be ≥ 8.0.14
*   PAN-OS 7.1 must be ≥ 7.1.21

Cisco Firepower Threat Defense and ASA with FirePOWER Services when operating in
“Decrypt - Resign mode/SSL Decryption Enabled”
([advisory](https://www.cisco.com/c/dam/en/us/td/docs/security/firepower/SA/SW_Advisory_CSCvj93913.pdf))
:

*   Firmware 6.2.3 must be ≥ 6.2.3.4
*   Firmware 6.2.2 must be ≥ 6.2.2.5
*   Firmware 6.1.0 must be ≥ 6.1.0.7

Administrators can test compatibility by flipping
chrome://flags/#enforce-tls13-downgrade to Enabled

Please report problems on the [administrator's
forum](https://productforums.google.com/forum/#!forum/chrome-admins).
