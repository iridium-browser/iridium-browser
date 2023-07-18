---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/obsolete
  - Obsolete Documentation
page_name: using-eclipse-to-debug-chrome-on-chrome-os
title: Using Eclipse to debug Chrome on Chrome OS
---

**DEPRECATED**

*jamescook says: I learned on 2012-03-08 that our Eclipse workflow is no longer
expected to work.*

*rkc says: The remote debugging workflow for Chrome on ChromeOS changed a while
ago - the linux_disable_pie flag was nuked; instead cmtice@ wrote a script that
makes remote debugging chrome very simple. Details on how to use the script are
at
<https://www.chromium.org/chromium-os/developer-guide#TOC-Remote-Debugging>*

*The Catch: The script will only work on a test image for now - this is because
only the test image has the ssh server running by default and copies over the
test keys which allow authentication-less connections to the device; something
the script depends on.*

*Our debugging from eclipse solution is in an unknown state - if anyone can
test/fix it, a lot of developers on the team would be quite delighted :)*
