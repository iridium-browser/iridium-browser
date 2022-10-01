---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-privacy
  - Chromium Privacy
- - /Home/chromium-privacy/privacy-sandbox
  - The Privacy Sandbox
page_name: covert-tracking
title: covert-tracking
---

## Covert Tracking

Unlike browser resources such as cookies which offer users the ability to
control or clear them, other aspects of browsers can be used to track and
identify users in a way that they cannot control. These convert tracking
mechanisms such as fingerprinting and cache inspection, leverage details of
browsers that people are often not aware of and are hard for individuals to
protect themselves against.
In order to combat these opaque tracking vectors, Chrome is both proposing
several new technologies to the web community to limit fingerprinting surfaces
and taking action to close down these back channels of tracking information.

### Fingerprinting

Fingerprinting is the generation of a unique identifier derived from intrinsic
differences between one user’s device and another’s that are detectable. Stable
fingerprints are particularly dangerous in that they can be used to re-identify
a user across any site indefinitely and users do not have the ability to
manually clear them like cookies can be cleared. Unstable fingerprints are
derived from information which may only be stable for a short period of time,
but can be used to link identities across sites that are visited within that
period of stability

#### Privacy Budget

Our overarching strategy to combat fingerprinting and not impede the development
of the web is the [Privacy Budget
proposal](https://github.com/bslassey/privacy-budget). After conducting a study
to measure the amount of entropy exposed by each surface, we will start to
enforce a limit on entropy collected by each site. Once the limit is reached,
the site will no longer be able to collect any further entropy.

### IP Address

IP Address is a large single source of entropy. We will explore options for
removing it as a reliable source of entropy. [Willful IP
Blindness](https://github.com/bslassey/ip-blindness) is one approach that would
facilitate an application server not receiving an IP address from separate
connection handling infrastructure. This separation would afford for the use of
IP addresses in more positive uses such as combating spam, fraud, and denial of
service while preventing it from being used by the application service to
covertly track a user.

### Removing Passive Fingerprinting Surfaces

Modern browsers expose several bits of information to web sites by default.
Several projects are underway to remove this exposure and introduce alternative
methods by which sites can query only the information they need. Examples
include [obsoleting the User
Agent](https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/-2JIRNMWJ7s)
string in transition to [Client Hints](https://github.com/WICG/ua-client-hints).

### Reducing Entropy From Surfaces

Certain APIs expose more information than is strictly needed for their purpose.
Efforts are underway to systematically identify these APIs and reduce the
information exposed to the minimum required for their motivating use cases.
Examples include the [device
orientation](https://bugs.chromium.org/p/chromium/issues/detail?id=1018180) and
[battery-level](https://bugs.chromium.org/p/chromium/issues/detail?id=661792)
APIs.

## Cache Inspection

A site that wants to transmit its identifier to another site without the use of
cookies can selectively load or not load a series of resources such that they
will be in the HTTP cache of the browser. As a third party embedded on another
site, it can attempt to load those resources in such a way that they will appear
if they are in the cache but will not load from the server if they are not in
the cache. This can create a stable cross-site identifier. Chrome will
[partition the HTTP
cache](https://groups.google.com/a/chromium.org/forum/?utm_medium=email&utm_source=footer#!msg/blink-dev/6KKXv1PqPZ0/3_1nYzrBBAAJ)
using top frame origin in order to prevent using this technique to create the
stable cross-site identifier. Chrome will also [need to partition or disable
other means of storing data in third party
contexts](https://docs.google.com/document/d/1V8sFDCEYTXZmwKa_qWUfTVNAuBcPsu6FC0PhqMD6KKQ/edit).
