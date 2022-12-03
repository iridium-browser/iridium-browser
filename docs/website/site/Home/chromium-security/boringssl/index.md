---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: boringssl
title: BoringSSL
---

We have used a number of patches on top of OpenSSL for many years. Some of them
have been accepted into the main OpenSSL repository, but many of them don’t mesh
with OpenSSL’s guarantee of API and ABI stability and many of them are a little
too experimental.

But as Android, Chrome and other products have started to need some subset of
these patches, things have grown very complex. The effort involved in keeping
all these patches (and there are more than 70 at the moment) straight across
multiple code bases is getting to be too much.

So we’re switching models to one where we import changes from OpenSSL rather
than rebasing on top of them. The
[result](https://boringssl.googlesource.com/boringssl/) (at the time of writing)
is used in Chrome on Android, OS X, and Windows. Over time we hope to use it in
Android and internally too.

There are no guarantees of API or ABI stability with this code: we are not
aiming to replace OpenSSL as an open-source project. We will still be sending
them bug fixes when we find them and we will be importing changes from upstream.
Also, we will still be funding the Core Infrastructure Initiative and the
OpenBSD Foundation.

But we’ll also be more able to import changes from LibreSSL and they are welcome
to take changes from us. We have already relicensed some of our prior
contributions to OpenSSL under an ISC license at their request and completely
new code that we write will also be so licensed.

(Note: the name is aspirational and not yet a promise.)
