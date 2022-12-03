---
breadcrumbs:
- - /developers
  - For Developers
page_name: tools-we-use-in-chromium
title: Tools we use in Chromium
---

**Ninja - This is a command line build system which is much faster than building
from IDEs such as MSVS or Xcode.**

**gclient - This is a wrapper which lets us act on all the various projects as
one - for instance, we use “gclient sync” and “gclient runhooks” often.**

**GN - This generates our project files to use with Ninja, MSVS, and/or Xcode.
It is run by gclient, we don’t normally call it directly. .gn files take the
place of makefiles or build files from other projects.**

**[GRIT](/developers/tools-we-use-in-chromium/grit)** - Short for "Google
Resource and Internationalization Tool," this is a Google-created tool used by
Chromium and usable by other projects.

**Git - The base repository is using git hosted at
<https://chromium.googlesource.com>.**

**DiffMerge - A graphical tool to view diffs graphically on your own machine
before commiting to your local branch. This can be hooked up to Git and
Subversion (search for instructions on the web). There are other graphical diff
viewers that we use, but this one will work on all main supported platforms
(Win, Mac, Linux)**

**Editor extensions - Emacs has some custom extensions to make chromium
development easier -
<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/emacs.md> (TODO:
as we find others for other editors, add them here!)**
