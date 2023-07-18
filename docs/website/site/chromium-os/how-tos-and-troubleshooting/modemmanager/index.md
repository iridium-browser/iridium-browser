---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: modemmanager
title: ModemManager care and feeding
---

Updating modemmanager-next from upstream. ModemManager is an actively-developed
project, and we're currently tracking their git master and working with the
upstream team on new features. As such, we need to integrate code from upstream
with some regularity.

Setup: Have a repo set up as follows:

Check out upstream:

$ git clone git://anongit.freedesktop.org/ModemManager/ModemManager

Add the ChromiumOS repo as a remote:

$ git remote add chromiumos
https://chromium.googlesource.com/chromiumos/third_party/modemmanager-next.git

This repo can be reused for multiple updates.

Each time you want to update from upstream:

$ git fetch

$ git co origin/master

$ git co -b merge-${DATE} # the branch name is arbitrary, I found this pattern
handy

$ git fetch chromiumos

$ git merge chromiumos/master

Resolve any conflicts.

Test that the new source compiles and works. (I generally did this by

moving aside the src/third-party/modemmanager-next directory and

moving this working repository into its place).

Test that dependencies (currently, Shill) compile and work, since

they're sensitive to the MM DBus API.

Push to the chromiumos repo (upstream branch and master)

$ git push chromiumos origin/master:refs/heads/upstream

$ git push chromiumos merge-${DATE}:refs/heads/master
