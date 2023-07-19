---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
- - /chromium-os/how-tos-and-troubleshooting/chromiumos-board-porting-guide
  - Chromium OS Board Porting Guide
page_name: private-boards
title: Creating Private Board Overlays
---

[TOC]

## Introduction

In the main [Chromium OS Board Porting
Guide](/chromium-os/how-tos-and-troubleshooting/chromiumos-board-porting-guide),
we covered how to create a new board overlay for a project. The goal there was
to create a board that'd be published for anyone to build themselves. However,
it is not uncommon for people to want to develop their boards in secret before
launch, or to have a few pieces that never get publicly released. Chromium OS
provides a framework for that too, and this guide covers that.

Note that this guide is not standalone. You should be familiar with the
aforementioned board porting guide as we will not duplicate topics that are in
common.

### Private (forever) Features

Some examples of things that won't be released (whether it be practical or
political or legal):

*   OEM Customizations like default login wallpapers or Chrome
            extensions
*   Scripts used in factory settings to reset low level hardware when
            handling RMAs/etc...
*   Source code to proprietary drivers/firmware/etc...
    *   the public overlay should have a package that installs a
                prebuilt binary though
*   ACLs to private Google Storage buckets

### Private & Public Boards

Private and public overlays do not work in isolation. This means that if you
have both private and public board overlays, they will both be used. This allows
you to have a public overlay that includes as many releasable pieces as possible
(and produce a usable image) while having a private overlay with just the few
additional private-only pieces.

Often times people will start by creating only a private board overlay (which is
a perfectly valid setup), and then once things have been released, create a
public board overlay. This is the recommended workflow and you should keep this
in mind when writing the private overlay -- don't make things harder for the
public release. The goal of this guide is to show how to create a private
overlay that is geared towards being released later on.

## Bare Framework

While public overlays live in src/overlays/overlay-$BOARD/, private overlays
live in src/private-overlays/overlay-$BOARD-private/. The file structure is the
same though.

### src/private-overlays/overlay-$BOARD-private/

Even if you have a public overlay, you will need at least these files (even if
there is no content in them like make.defaults):

```none
overlay-$BOARD/
|-- metadata/
|   `-- layout.conf          # Copy of the public overlay version with updated repo-name
|-- profiles/
|   `-- base/
|       |-- make.defaults    # Private board customizations (USE/LINUX_FIRMWARE/etc...)
|       `-- parent           # Either same as the public overlay, or a link to it
`-- toolchain.conf           # Same as the public overlay
```

Most of these files are one or two lines. Let's go through them one at a time.

#### metadata/layout.conf

This will be the same as the public overlay, except you'll want to add a
"-private" suffix to the "repo-name" field. Here's what the private overlay for
the lumpy board uses:

<pre><code>$ cat metadata/layout.conf
masters = portage-stable chromiumos eclass-overlay
profile-formats = portage-2
repo-name = <b>lumpy-private</b>
thin-manifests = true
use-manifests = strict
</code></pre>

#### profiles/base/make.defaults

Since this will be a super set of the public overlay's make.defaults, use this
form:

```none
# NOTE: Only put private-specific stuff in this file.  If your settings
# can be used publicly, then use overlays/overlay-$BOARD/profiles/base/make.defaults
# instead.  There is no need to duplicate information -- these files stack.
# Initial value just for style purposes.
USE=""
USE="${USE} private-specific-flags"
# These are settings that today live in the private overlay, but may be moved to the public.
USE="${USE} public-flags"
```

#### profiles/base/parent

If you have a public overlay already, then the preference would be to redirect
to that. Here is how the private lumpy overlay does it:

```none
$ cat profiles/base/parent
lumpy:base
```

You can still add private specific settings to the profile by adding files under
profiles/base/.

If you don't have a public overlay, then this will have the same content as if
you had a public overlay (see the public porting guide for more info).

#### toolchain.conf

For now, this should be the same as the public overlay. We plan on making this
incremental too, but haven't yet done so.
