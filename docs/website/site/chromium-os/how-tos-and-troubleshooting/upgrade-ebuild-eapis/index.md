---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: upgrade-ebuild-eapis
title: Upgrade Ebuild EAPIs HOWTO
---

[TOC]

## Introduction

Gentoo ebuilds have upgraded the API that they have available over time to
include new features and fix bugs in existing behavior. In order to guarantee
that ebuilds written yesterday continue to work with package managers of
tomorrow, the API available to ebuilds has been formalized in the [Package
Manager Spec](http://wiki.gentoo.org/wiki/Project:PMS) (PMS). The short summary
is that ebuilds declare the version of the API that they've been written for by
setting the EAPI variable at the top of the ebuild, and the spec guarantees that
behavioral changes will only occur in newer versions.

You might encounter ebuilds using older EAPIs. We'll provide a quick guide to
cover the common issues you see when upgrading to the latest version. At this
time, that is EAPI 6, but our focus has been on EAPI 5 or newer. This is because
the functionality new to EAPI 6 is not significant enough to warrant forcing an
upgrade and few ebuilds would even utilize it.

Note that while the convention thus far has been to use numbers in the official
spec (like 0, 1, 2, etc...), the EAPI setting is a string, not an integer.

## Common Changes

### EAPI Setting

First, if your ebuild doesn't even have an EAPI setting at the top, then it
defaults to the oldest version of 0. If it does contain a setting, then you
should make sure it is 5.

```none
# Copyright <year> The ChromiumOS Authors.
# Distributed under the terms of the GNU General Public License v2
EAPI="4"
```

This is what the header of your ebuild should look like. The only thing that
comes before the EAPI line is the file's comment block which declares copyright
& licensing (the year & license may vary), then a blank line, then the EAPI
setting.

### Default Source Directory Setting

In EAPI versions before 5, the variable `S` would default to `${WORKDIR}/${P}`.
But if `${P}` did not exist, the ebuild would silently fall back to the work
directory (`${WORKDIR}`). For some ebuilds, this wasn't a problem because they
only installed files out of `${FILESDIR}` or `${SYSROOT}`.

Starting with EAPI 5, the PM longer does this automatic fallback to
`${WORKDIR}`. If you get errors like "`The source directory '...' doesn't
exist`", then you're hitting this problem (i.e. relying on the older silent
fallback behavior).

The easy answer is usually to set `S` to `${WORKDIR}`.This might not be the
right answer (maybe your source lives somewhere else), but frequently this is
OK. So after the `DEPEND`/`RDEPEND` variables and before any of the src_\* or
pkg_\* functions, add:

```none
S=${WORKDIR}
```

### New Function Phases

In EAPI versions starting with 2, some source functions were split up to better
reflect the overall build process. So if you're upgrading from those, here's
what you need to know:

*   If you declare a src_unpack function, move all source
            patching/hacking (like running \`sed\`) to a new src_prepare. This
            often means that src_unpack is deleted entirely as the default of
            unpacking all archives in SRC_URI is sufficient.
*   If you declare a src_compile function, move all configure steps
            (like \`econf\` or \`./configure\`) to a new src_configure. This
            often means that src_compile is deleted entirely as the default of
            just running \`emake\` is sufficient.

Here is an example of splitting src_unpack into src_unpack and src_prepare:

```none
# This is the old (pre-EAPI 4) version.
src_unpack() {
    unpack ${A}
    cd "${S}"
    epatch "${FILESDIR}"/${P}-a-fix.patch
    sed -i 's:old:new:' configure.ac || die
    eautoreconf
}
```

```none
# This is the new (EAPI 4 & 5) version.
# Note that there is no src_unpack at all because the default is `unpack ${A}`.
# If you needed to unpack things in a specific way, you can add your own
# src_unpack function and do just that.
src_prepare() {
    epatch "${FILESDIR}"/${P}-a-fix.patch
    sed -i 's:old:new:' configure.ac || die
    eautoreconf
}
```

```none
# This is the new (EAPI 6+) version.
# Note that there is no src_unpack at all because the default is `unpack ${A}`.
# If you needed to unpack things in a specific way, you can add your own
# src_unpack function and do just that.
src_prepare() {
    default
    eapply "${FILESDIR}"/${P}-a-fix.patch
    sed -i 's:old:new:' configure.ac || die
    eautoreconf
}
```

Here is an example of splitting src_compile into src_configure and src_compile:

```none
# This is the old (pre-EAPI 4) version.
src_compile() {
    econf \
        --enable-foo \
        $(use_enable bluetooth) \
        || die
    emake || die
}
```

```none
# This is the new (EAPI 4+) version.
# Note that there is no src_compile at all because the default is `emake`.
# If you passed additional flags to `emake`, then you can add your own
# src_compile function and do just that.
src_configure() {
    econf \
        --enable-foo \
        $(use_enable bluetooth)
}
```

### Implicit Die Commands

Starting with EAPI 4, all of the helper commands (like emake and doins and dobin
and ...) were updated to automatically call \`die\` when they failed. This means
you no longer have to (and in fact you should avoid doing so). This does not
apply to other commands like \`mv\` or \`sed\` or \`cp\` -- if you run those,
you should still call \`die\`.

Here's an example of converting to these newer EAPIs:

```none
# This is the old (pre-EAPI 4) version.
src_compile() {
    emake USE_EXTRA=1 || die "emake failed"
}
src_install() {
    emake install install-man USE_EXTRA=1 DESTDIR="${D}" || die "emake install failed"
    dodoc *.txt || die "dodoc failed"
    insinto /etc/init
    newins "${FILESDIR}"/package.init package || die "doins failed"
}
```

```none
# This is the new (EAPI 4+) version.
src_compile() {
    emake USE_EXTRA=1
}
src_install() {
    emake install install-man USE_EXTRA=1 DESTDIR="${D}"
    dodoc *.txt
    insinto /etc/init
    newins "${FILESDIR}"/package.init package
}
```

On the rare occasion you expect a command to fail and want to handle it
yourself, you can use the new \`nonfatal\` helper like so:

```none
src_install() {
    if ! nonfatal doins some.bin; then
        # Handle the failed install yourself.
    fi
}
```

### The \`default\` Function

There is now a nifty helper function called `default` that let's you execute the
default commands that a function would do. This saves a bit of code while
letting you still do a bit of additional things. It most commonly comes up in
the `src_install` function when packages use \``emake install DESTDIR="${D}"`\`.
If you need to pass additional flags to `emake` though, then this won't help.

Starting with EAPI 6, the `src_prepare` function must always call `default`. If
you forget, the build will usually fail for you automatically :).

```none
# This is the old (pre-EAPI 4) version.
src_install() {
    emake install DESTDIR="${D}" || die
    doman man/*.1 || die
    insinto /etc
    doins "${FILESDIR}"/some.conf || die
}
```

```none
# This is the new (EAPI 4+) version.
src_install() {
    default
    doman man/*.1
    insinto /etc
    doins "${FILESDIR}"/some.conf
}
```
