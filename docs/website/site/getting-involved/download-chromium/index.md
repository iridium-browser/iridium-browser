---
breadcrumbs:
- - /getting-involved
  - Getting Involved
page_name: download-chromium
title: Download Chromium
---

You can test Chrome builds or Chromium builds. Chrome builds have the most
infrastructure for analyzing crashes and reporting bugs. They also auto-update
as new releases occur, which makes them a good choice for most uses. [Chrome
Canary](https://tools.google.com/dlpage/chromesxs) is available for Windows and
Mac and autoupdates daily. [Other channels](/getting-involved/dev-channel/) (dev
and beta) are available.

Chromium builds do not auto-update, and do not have symbols. This makes them
most useful for checking whether a claimed fix actually works. Use the following
instructions to find builds:

### Easy Point and Click for latest build:

> Open up <https://download-chromium.appspot.com>

### Easy Script to download and run latest Linux build:

*   <https://github.com/scheib/chromium-latest-linux>

### Not-as-easy steps:

1.  Head to
            <https://commondatastorage.googleapis.com/chromium-browser-snapshots/>
2.  Choose your platform: Mac, Win, Linux, ChromiumOS
3.  Pick the Chromium build number you'd like to use
    1.  The latest one is mentioned in the `LAST_CHANGE` file
4.  Download the zip file containing Chromium
5.  There is a binary executable within to run

### Please [file bugs](https://crbug.com/new) as appropriate.

## Downloading old builds of Chrome / Chromium

Let's say you want a build of Chrome 44 for debugging purposes. Google does not
offer old builds as they do not have up-to-date security fixes.

However, you can get a build of Chromium 44.x which should mostly match the
stable release.

Here's how you find it:

1.  Look in
            <https://googlechromereleases.blogspot.com/search/label/Stable%20updates>
            for the last time "44." was mentioned.
2.  Loop up that version history ("44.0.2403.157") in the [Position
            Lookup](https://omahaproxy.appspot.com/)
3.  In this case it returns a base position of "330231". This is the
            commit of where the 44 release was branched, back in May 2015.\*
4.  Open the [continuous builds
            archive](https://commondatastorage.googleapis.com/chromium-browser-snapshots/index.html)
5.  Click through on your platform (Linux/Mac/Win)
6.  Paste "330231" into the filter field at the top and wait for all the
            results to XHR in.
7.  Eventually I get a perfect hit:
            <https://commondatastorage.googleapis.com/chromium-browser-snapshots/index.html?prefix=Mac/330231/>
    1.  Sometimes you may have to decrement the commit number until you
                find one.
8.  Download and run!

\* As this build was made at 44 branch point, it does not have any commits
merged in while in beta.

Typically that's OK, but if you need a true build of "44.0.*2403*.x" then you'll
need to build Chromium from the *2403* branch. Some
PortableApps/PortableChromium sites offer binaries like this, due to security
concerns, the Chrome team does not recommend running them.