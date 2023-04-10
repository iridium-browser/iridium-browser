---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: owp
title: Web Platform Security
---

Developers should have the tools necessary to defend their creations against the
wide spectrum of maliciousness thrown at them on a daily basis. We design and
implement platform-level features that enable a robust defense, and work in
standards bodies to get them in front of as many developers as possible.

[TOC]

## Features we're working on

### Content Security Policy

We've shipped [Content Security Policy Level 2](http://www.w3.org/TR/CSP2/) as
of Chrome 42, and we're starting to put together the vision for the next
iteration of the standard. You can follow along at
<https://github.com/w3c/webappsec-csp>. Firefox has support for most of CSP2 as
well.

### Subresource Integrity

We've shipped [Subresource Integrity](http://www.w3.org/TR/SRI/) as of Chrome
46. This feature should also be shipping in Firefox ~43, which is exciting to
see.

### Upgrade Insecure Requests

We've shipped [Upgrade Insecure
Requests](http://www.w3.org/TR/upgrade-insecure-requests/) as of Chrome 44. This
feature should also be shipping in Firefox 42, which will give us a fairly broad
base of support.

### Mixed Content

We've been steadily tightening our [Mixed
Content](http://www.w3.org/TR/mixed-content/) blocking over the last ~18 months.
The specification has broad approval from other vendors, who are generally
aligning with Chrome's behavior over time.

### Cookies

We have a number of cookie-related proposals floating around that we're building
support for in the IETF's HTTPbis group:

    [Deprecating modification/creation of "Secure" cookies on non-secure
    origins](https://tools.ietf.org/html/draft-west-leave-secure-cookies-along)
    strengthen the security properties of cookies with the Secure attribute.

    [SameSite](https://tools.ietf.org/html/draft-ietf-httpbis-cookie-same-site-00)
    cookies provide a defense against various forms of CSRF

    [Cookie Prefixes](https://tools.ietf.org/html/draft-west-cookie-prefixes)
    might provide a mitigation for the integrity and confidentiality limitations
    of cookies' delivery model.

    [Origin cookies](https://tools.ietf.org/html/draft-west-origin-cookies) are
    a slightly more robust model than the prefix proposal, but also seem less
    likely to be adopted, so. Belts and suspenders.

### Credential Management

We're working with Chrome's password manager team to define an imperative API
for interaction with the browser's stored credentials. The spec is progressing
nicely at <https://w3c.github.io/webappsec-cred» and the feature is mostly
implemented behind `chrome://flags/#enable-credential-manager-api`. >

### Referrer Policy

[Referrer Policy](https://w3c.github.io/webappsec-referrer-policy/) shipped in
Chrome a long time ago, and the new bits we've added to the spec are trickling
out over time.

We need to do some refactoring in order to consistently apply referrer policy
for redirects (basically hoisting the parsing and processing up out of Blink and
into the network stack). Hopefully we'll get that done in Q4 (2015).

### Secure Contexts

[Secure Contexts](https://w3c.github.io/webappsec-secure-contexts/) underlies
our commitment to [prefer secure origins for powerful new
features](/Home/chromium-security/prefer-secure-origins-for-powerful-new-features).
We've shipped the concept of a "secure context" in Chrome, and are working over
time to [deprecate a set of APIs in insecure
contexts](/Home/chromium-security/deprecating-powerful-features-on-insecure-origins).
`getUserMedia()` is unavailable over HTTP as of Chrome 47, and we've got our eye
on geolocation next.

### Clear Site Data

We've specified and implemented a feature to allow developers to clear out the
data a browser has stored for their origin at
<https://w3c.github.io/webappsec-clear-site-data/>.

### Entry Point Regulation

We've specified EPR as a defense against CSRF, and hope to get to a prototype
implementation soonish.(this hasn't made progress in a few years)

## Come work with us!

If any of the above work resonates with you, come help us! We're hiring in
Munich, Germany, which is [a wonderful place to work
indeed](https://www.google.com/about/careers/locations/munich/). There's a
[generic software engineering job
description](https://www.google.com/about/careers/search#!t=jo&jid=43144&), but
the best way to get noticed is probably to ping
[@mikewest](https://twitter.com/mikewest) on Twitter, or drop him an email at
[mkwst@google.com](mailto:mkwst@google.com).
