---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/get-the-code
  - 'Get the Code: Checkout, Build, & Run Chromium'
page_name: gclient-managed-mode
title: Gclient Managed Mode
---

Managed mode is a **deprecated** feature of gclient. It was conceived to give an
easier workflow for newcomers to git, but turned out to be full of surprising
behaviors. It has thus been deprecated, unmanaged mode is the default, and
existing users of managed mode are encouraged to change to unmanaged mode.

To check which mode you are using, check .gclient (the gclient config file) and
see the value of the "managed" flag. To switch to unmanaged mode from managed
mode:

*   edit your .gclient and set:
    "managed": False,
*   update your repository by using git pull (as described above),
            followed by gclient sync, rather than just gclient sync.

To make the Blink repo also unmanaged, first fetch blink and then:

*   edit your .gclient and set:
    "managed": False,
    "custom_deps": {
    "src/third_party/WebKit": None,
    },
*   update your repository by using git pull in Chromium, then git pull
            in Blink (as described above), followed by gclient sync, rather than
            just gclient sync.

## Reference

*   “[PSA: fetch chromium now uses git unmanaged
            mode](https://groups.google.com/a/chromium.org/forum/#!topic/chromium-dev/n9N5N3JL2_U)”

The main difference between managed mode and unmanaged mode is that in managed
mode, local branches track local master branch instead of origin/master, and
gclient “manages” the branches so that they stay in sync. However in practice
this leads to two master branches (local master and origin/master) that go out
of sync and prevent further uploads/commits. With unmanaged mode you always have
only one master: origin/master. The local master still exists but is not treated
in any special way by the tools.

## What's missing

*   Safesync is not supported for new managed git checkouts yet
            (<http://crbug.com/109191>).
