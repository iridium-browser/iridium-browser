---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: chromium-os-faq
title: Chromium OS FAQ
---

[TOC]

## What's the difference between Chromium OS and Google Chrome OS?

Google Chrome OS is to Chromium OS what Google Chrome browser is to Chromium.

*   **Chromium OS** is the open source project, used primarily by
            developers, with code that is available for anyone to checkout,
            modify, and build.
*   **Google Chrome OS** is the Google product that OEMs ship on
            Chromebooks for general consumer use.

Some specific differences:

*   The two projects fundamentally share the same code base, but Google
            Chrome OS has some additional firmware features, including verified
            boot and easy recovery, which require corresponding hardware changes
            and thus also don't work out of the box in Chromium OS builds.
*   Google Chrome OS runs on specially optimized hardware in order to
            get enhanced performance and security.
*   Chromium OS does not auto-update by default (so that changes you may
            have made to the code are not blown away), whereas Google Chrome OS
            seamlessly auto-updates so that users have the latest and greatest
            features and fixes.
*   Google Chrome OS is supported by Google and its partners; Chromium
            OS is supported by the open source community.
*   Google Chrome OS includes some binary packages which are not allowed
            to be included in the Chromium OS project. A non-exhaustive list:
    *   Adobe Flash
    *   Widevine CDM plugin (to support [HTML5
                EME](https://w3c.github.io/encrypted-media/))
    *   3G Cellular support (but work is on going to address this)
    *   DisplayLink Manager for video over USB (some systems)
    *   Android (ARC++) container for running Android apps
*   Some components are available in both, but as closed source
            binary-only blobs. A non-exhaustive list:
    *   Graphics Libraries (e.g. OpenGL) on ARM platforms
*   Google Chrome ships with its own set of [API
            keys](http://www.chromium.org/developers/how-tos/api-keys) while
            Chromium does not include any
    *   Users are expected to set up their own
*   Google Chrome OS has a [green/yellow/red
            logo](https://www.google.com/intl/en/images/logos/chrome_logo.gif)
            while Chromium OS has a [blue/bluer/bluest](/config/customLogo.gif)
            logo.

## Where can I download Google Chrome OS?

Google Chrome OS is not a conventional operating system that you can download or
buy on a disc and install. As a consumer, the way you will get Google Chrome OS
is by buying a Chromebook that has Google Chrome OS installed by the OEM. Google
Chrome OS is being developed to run on new machines that are specially optimized
for increased security and performance. We are working with manufacturers to
develop reference hardware for Google Chrome OS.

Chromebooks are available for sale now! Check out the [Google Chromebook
site](http://www.google.com/chromebook/) for more information.

## Where can I download Chromium OS?

If you are the kind of developer who likes to build an open source operating
system from scratch, you can follow the developer instructions to check out
Chromium OS, build it and experiment with it. A number of sites have also posted
pre-built binaries of Chromium OS. However, these downloads are not verified by
Google, therefore please ensure you trust the site you are downloading these
from.

Keep in mind that Chromium OS is not for general consumer use.

## I am a hardware manufacturer, who can I talk to about making a Chrome OS product?

The Chrome OS business development team can be reached at
[chromeos-interested@google.com](mailto:chromeos-interested@google.com).
