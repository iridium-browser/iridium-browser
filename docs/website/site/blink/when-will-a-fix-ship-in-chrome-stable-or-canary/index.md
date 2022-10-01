---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: when-will-a-fix-ship-in-chrome-stable-or-canary
title: When will a fix ship in Chrome (stable or canary)?
---

Here's a quick guide on how to track changes as they make their way to releases
of Chrome.

This includes all changes to Chrome, including stuff for HTML, CSS, DOM, &
DevTools.

## On chromestatus.com?

First things first, [chromestatus.com](http://chromestatus.com) was built for
this. If the feature is big, it will probably be there.

On the left you can see all features currently in beta. Those should be in
stable soon.

You can search for other features and view what release they are *Enabled by
default.*

[<img alt="https://www.chromestatus.com/"
src="/blink/when-will-a-fix-ship-in-chrome-stable-or-canary/chromestatus42.png"
width=500>](https://www.chromestatus.com/)

## Not on Chromestatus?

Alright, buckle up. This is fun stuff.

It all comes down to tracking the **Chromium revision number.**

*   First, we're assuming you're looking at the bug somewhere in
            [code.google.com/p/chromium/issues/](https://code.google.com/p/chromium/issues/list)
*   Now, when a commit lands, an automated comment is added to the
            Chromium bug.
*   All commits have a git hash and an incremental revision number.
*   You can see the commit below has **373417**.
*   Take note of this number and follow on…

[<img alt="image"
src="/blink/when-will-a-fix-ship-in-chrome-stable-or-canary/cr-commit-pos.png">](/blink/when-will-a-fix-ship-in-chrome-stable-or-canary/Untitled-5.fw.png)

### Waiting for it to hit Chrome Stable?

Generally it takes ~10 weeks from a commit landing to hit stable.

Take the Chromium revision number (or the git SHA) and drop it into
[storage.googleapis.com/chromium-find-releases-static/](https://storage.googleapis.com/chromium-find-releases-static/index.html)
[<img alt="image"
src="/blink/when-will-a-fix-ship-in-chrome-stable-or-canary/f3a.png">](/blink/when-will-a-fix-ship-in-chrome-stable-or-canary/f3a.png)
Visit ==https://omahaproxy.appspot.com== for the version numbers of the current
releases of Chrome.

**When does version X ship in stable channel?**

*   Chrome ships a new major version to stable generally every 6 weeks,
            but in some cases we may choose to skip a release.
*   [Chromium Development Calendar and Release
            Info](/developers/calendar) contains "Estimated Stable Dates", which
            are roughly 6 weeks apart.

### Waiting for it to land in Canary or an Dev build?

You could use the above technique... but here's the guide for hackers or folks
who don't want to wait for Canary or Dev Channel:

#### Get a fresh Chromium build

*   Get the latest chromium in either of these two ways: [Download
            Chromium](/getting-involved/download-chromium)
    *   These builds are generated every hour, so you should be fine.
*   [Canary](http://www.paulirish.com/2012/chrome-canary-for-developers/)
            is updated every day (at ~4am PST).
*   Either way, open up either and go to about:version

[<img alt="image"
src="/blink/when-will-a-fix-ship-in-chrome-stable-or-canary/chromeversion.png">](/blink/when-will-a-fix-ship-in-chrome-stable-or-canary/chromeversion.png)

*   At the end of the Revision line is the Chromium Rev.
*   As long as that rev number is higher than the one you're interested
            in... you're good!
    *   (Of course this assumes no reverts of either the commit or the
                DEPS roll, but you'll probably have spotted those already)

**Enjoy the fix or feature you've been waiting for!**

Updated Oct 2016

Paul Irish
