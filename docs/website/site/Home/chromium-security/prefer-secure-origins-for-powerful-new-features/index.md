---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: prefer-secure-origins-for-powerful-new-features
title: Prefer Secure Origins For Powerful New Features
---

We (Chrome Security) originally sent this out to various browser development
mailing lists. Here is the canonical location for the original proposal. See
[this link](https://www.w3.org/TR/powerful-features/) for the current public
draft spec.

**This is a living document** — as we learn more, we'll probably need to change
this page.

## Proposal

The Chrome Security team and I propose that, for new and particularly
powerful web platform features, browser vendors tend to prefer to make
the the feature available only to secure origins by default.

## Definitions

“Particularly powerful” would mean things like: features that handle
personally-identifiable information, features that handle high-value information
like credentials or payment instruments, features that provide the origin with
control over the UA's trustworthy/native UI, access to sensors on the user's
device, or generally any feature that we would provide a user-settable
permission or privilege to. Please discuss!

“Particularly powerful” would **not** mean things like: new rendering and layout
features, CSS selectors, innocuous JavaScript APIs like *showModalDialog*, or
the like. I expect that the majority of new work in HTML5 fits in this category.
Please discuss!

“Secure origins” are origins that match at least one of the following (scheme,
host, port) patterns:

*   `(https, *, *)`
*   `(wss, *, *)`
*   `(*, localhost, *)`
*   `(*, 127/8, *)`
*   `(*, ::1/128, *)`
*   `(file, *, —)`
*   `(chrome-extension, *, —)`

This list may be incomplete, and may need to be changed. Please discuss!
A bug to define “secure transport” in Blink/Chromium:
<https://code.google.com/p/chromium/issues/detail?id=362214>

## For Example

For example, Chrome is going to make Service Workers available only to secure
origins, because it provides the origin with a new, higher degree of control
over a user's interactions with the origin over an extended period of time, and
because it gives the origin some control over the user's device as a background
task.

Consider the damage that could occur if a user downloaded a service worker
script that had been tampered with because they got it over a MITM’d or spoofed
cafe wifi connection. What should have been a nice offline document editor could
be turned into a long-lived spambot, or maybe even a surveillance bot. If the
script can only run when delivered via authenticated, integrity-protected
transport like HTTPS, that particular risk is significantly mitigated.

## Background

Legacy platforms/operating systems have a 1-part principal: the user. When a
user logs in, they run programs that run with the full privilege of the user:
all of a user’s programs can do anything the user can do on all their data and
with all their resources. This has become a source of trouble since the rise of
mobile code from many different origins. It has become less and less acceptable
for a user’s (e.g.) word processor to (e.g.) read the user’s private SSH keys.

Modern platforms have a 2-part security principal: the user, and the origin of
the code. Examples of such modern platforms include (to varying degrees) the
web, Android, and iOS. In these systems, code from one origin has (or, should
have) access only to the resources it creates and which are explicitly given to
it.

For example, the Gmail app on Android has access only to the user’s Gmail and
the system capabilities necessary to read and write that email. Without an
explicit grant, it does not have access to resources that other apps (e.g.
Twitter) create. It also does not have access to system capabilities unrelated
to email. Nor does it have access to the email of another user on the same
computer.

In systems with 2-part principals, it is crucial to strongly authenticate both
parts of the principal, not just one part. (Otherwise, the system essentially
degrades into a 1-part principal system.) This is why, for example, both Android
and iOS require that every vendor (i.e. origin) cryptographically sign its code.
That way, when a user chooses to install Twitter and to give Twitter powerful
permissions (such as access to the device’s camera), they can be sure that they
are granting such capability only to the Twitter code, and not to just any code.

By contrast, the web has historically made origin authentication optional. On
the web, origins are defined as having 3 parts: (scheme, host, port), e.g.
(HTTP, [example.com](http://example.com/), 80) or (HTTPS,
[mail.google.com](http://mail.google.com/), 443). Many origins use
unauthenticated schemes like HTTP, WS, or even FTP.

Granting permissions to unauthenticated origins is, in the presence of a network
attacker, equivalent to granting the permissions to any origin. The state of the
internet is such that we must indeed assume that a network attacker is present.

## Thank You For Reading This Far!

We welcome discussion, critique, and cool new features!
