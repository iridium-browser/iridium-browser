---
breadcrumbs: []
page_name: irc-support-faq
title: IRC Support FAQ
---

## What's the latest version?

You can find out from <https://omahaproxy.appspot.com/viewer>, which displays
the latest versions of each channel for each platform directly from the
autoupdate service.

## Where can I find old Chrome binaries?

Chrome binaries are only available through the installer (available from
<https://www.google.com/chrome>) and the autoupdate system. There is no official
source of Chrome binaries older than the current stable channel, because:

1.  There are frequently unpatched security bugs in older versions,
            since patches are only backported to current stable/beta/dev at the
            time of patching.
2.  The support burden of supporting older channels, which often have
            bugs that are fixed in later channels, is very heavy.

If you are a developer looking to build an old version to bisect a bug of check
for a regression, please see the [developers
page](http://www.chromium.org/developers).
