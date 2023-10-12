---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/licensing
  - Licensing in Chromium OS
page_name: building-a-distro
title: Licensing Handling For OS Builders
---

[TOC]

Note: This is not legal advice; if you want that, consult a lawyer. This section
discusses purely technical measures that are available to builders. Also be
aware that this system is not perfect, and may make mistakes. You should not
rely on it to fulfill legal obligations.

## Introduction

The Chromium OS build system provides a method for automatically tracking
licenses used by packages that are built & installed. This allows you to easily
generate an image that only contains code that is under an FOSS license (e.g. a
BSD or GPL variant). It also allows you to set even more restrictive guidelines
such as only building packages that fall under BSD style licenses (thus
filtering out all other licenses such as GPL), or filtering out specific
licenses such as the GPL-3.

This is all done via the ACCEPT_LICENSE make.conf variable.

Note: The implication of ACCEPT_LICENSE is that you are accepting the terms of
every license that ends up in the image. Be sure to review each one that is
listed to verify this is OK for your needs.

## Syntax

The ACCEPT_LICENSE variable is a whitespace delimited list of elements. Each
element takes the form of:

*   an explicit license name (e.g. "GPL-2" or "BSD")
*   a license group name (starts with an "@")
*   a glob that means "all licenses"

Each of these elements can be prefixed with a "-" to mean you want to remove
that specific element from the accepted list.

The list is incremental. That means later values add on to previous ones, so "a
b -a" is equivalent to "b".

### Examples

Accept all licenses except for the EULA group:

```none
 ACCEPT_LICENSE="* -@EULA"
```

Accept only the BSD and GPL-2 license:

```none
 ACCEPT_LICENSE="BSD GPL-2"
```

Accept all Free Software licenses, except for the GPL-3:

```none
 ACCEPT_LICENSE="@FREE -GPL-3"
```

## Default Accepted Licenses

The default set of licenses that Chromium OS accepts are:

```none
$ portageq envvar ACCEPT_LICENSE
* -@EULA -@CHROMEOS
```

This means all licenses that the system knows about, except for ones that
require explicit EULA agreements, and ones in the CHROMEOS group. In practice,
this means all licenses considered Free Software (either officially by the OSI
or FSF, or follow the FSF guidelines), and licenses that allow for binary
redistribution (e.g. firmware for hardware devices).

## Accepting Additional Licenses

Sometimes when you try building for a board, you might run into errors like:

```none
  The following license changes are necessary to proceed:
  #required by virtual/opengles-2-r2, required by chromeos-base/chromeos-0.0.1-r181[opengles], required by chromeos-base/chromeos (argument)
  >=media-libs/mali-drivers-bin-0.45-r96 Google-TOS 
```

This is telling you that the package you're attempting to pull in (either
directly, or by other packages) is using a license that has not been accepted
(by virtue of being listed in the ACCEPT_LICENSE variable). You need to review
this specific license, make sure you're OK with the terms of it, and then either
agree to it, or find another way of building things that do not use this
package.

If you do agree to the license, you can declare that intention in a couple of
ways:

*   update the make.conf file in your board's overlay and set
            ACCEPT_LICENSE in there
*   update the /etc/make.conf.user file in your chroot and set
            ACCEPT_LICENSE in there
*   \`export ACCEPT_LICENSE="...some values..."\` in your shell -- note
            that you'll have to re-export this every time you enter the chroot
            via \`cros_sdk\`
*   If you're running `cros build-packages`, use the `--accept-licenses` flag to
    pass an explicit list.  Note that this flag must be included every
    time you run `cros build-packages`.
