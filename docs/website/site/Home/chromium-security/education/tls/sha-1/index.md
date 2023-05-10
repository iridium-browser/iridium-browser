---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/education
  - Education
- - /Home/chromium-security/education/tls
  - TLS / SSL
page_name: sha-1
title: A further update on SHA-1 certificates in Chrome
---

We’ve previously made
[several](https://security.googleblog.com/2014/09/gradually-sunsetting-sha-1.html)[
announcements](https://security.googleblog.com/2015/12/an-update-on-sha-1-certificates-in.html)
about Google Chrome's deprecation plans for SHA-1 certificates. This post
provides an update on the final removal of support.

The SHA-1 cryptographic hash algorithm first [showed signs of
weakness](https://www.schneier.com/blog/archives/2005/02/cryptanalysis_o.html)
over eleven years ago and [recent research](https://eprint.iacr.org/2015/967)
points to the imminent possibility of attacks that could directly impact the
integrity of the Web PKI. To protect users from such attacks, Chrome will stop
trusting certificates that use the SHA-1 algorithm, and visiting a site using
such a certificate will result in an interstitial warning.

Release schedule

We are planning to remove support for SHA-1 certificates in Chrome 56, which
will be released to the stable channel [around the end of January
2017](/developers/calendar). The removal will follow the [Chrome release
process](/getting-involved/dev-channel), moving from Dev to Beta to Stable;
there won't be a date-based change in behaviour.

Website operators are urged [to check](https://www.ssllabs.com/ssltest/) for the
use of SHA-1 certificates and immediately contact their CA for a SHA-256 based
replacement if any are found.

SHA-1 use in private PKIs

Previous posts made a distinction between certificates which chain to a public
CA and those which chain to a locally installed trust anchor, such as those of a
private PKI within an enterprise. We recognise there might be rare cases where
an enterprise wishes to make their own risk management decision to continue
using SHA-1 certificates.

Starting with [Chrome
54](https://googlechromereleases.blogspot.com/2016/10/stable-channel-update-for-desktop.html)
we provide the
[EnableSha1ForLocalAnchors](/administrators/policy-list-3#EnableSha1ForLocalAnchors)[
policy](https://support.google.com/chrome/a/answer/187202) that allows
certificates which chain to a locally installed trust anchor to be used after
support has otherwise been removed from Chrome. Features which[ require a secure
origin](/Home/chromium-security/deprecating-powerful-features-on-insecure-origins),
such as geolocation, will continue to work, and SHA-1 client certificates will
still be presented to websites requesting client authentication, however pages
will be displayed as “neutral, lacking security”. Without this policy set, SHA-1
certificates that chain to locally installed roots will not be trusted starting
with Chrome 57, which will be released to the stable channel in March 2017.

Since this policy is intended only to allow additional time to complete the
migration away from SHA-1, it will eventually be removed in the first Chrome
release after January 1st 2019.

As Chrome makes use of certificate validation libraries provided by the host OS
when possible, this option will have no effect if the underlying cryptographic
library disables support for SHA-1 certificates; at that point, they will be
unconditionally blocked. We may also remove support before 2019 if there is a
catastrophic cryptographic break of SHA-1. Enterprises are encouraged to make
every effort to stop using SHA-1 certificates as soon as possible and to consult
with their security team before enabling the policy.
