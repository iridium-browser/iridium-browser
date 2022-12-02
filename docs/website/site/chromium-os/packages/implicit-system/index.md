---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/packages
  - packages
page_name: implicit-system
title: Implicit System Dependencies
---

[TOC]

## Introduction

Gentoo has the concept of an [implicit system
dependency](http://devmanual.gentoo.org/general-concepts/dependencies/). The
idea is simply put: there are a number of packages that every system will need
to function at any level (to boot, build, etc...), and rather than have every
other package in the tree depend on them, we stick it in the @system set
instead. Then things in that set can be ignored when declaring a package's
dependency (build and run time).

It's a convenient way of:

*   avoiding lots of circular dependencies
*   duplicating the same basic set of dependencies over and over in
            packages
*   swapping out one set of packages for another (like using "busybox"
            instead of "coreutils" in an embedded system)

Consider this: how useful is a system if you don't have programs like sh, cat,
grep, ls, mv, sed, test? Or a C library? Or to try to compile code, but have no
compiler/assembler/linker? Is it really useful for developers to analyze every
single shell script/line of source that goes into a distribution to see which
tools are run (including every time a package releases a new version)? Or do we
simply state up front: at run time, you may assume that a certain baseline of
libraries/programs exist, and at build time, certain libraries/compilers exist?

The question is obviously rhetorical -- developer time is better spent doing
real work than keeping track of these things.

## Rationale

Gentoo doesn't explicitly define what packages qualify as part of the implicit
system dependency. It alludes to them by talking about the @system set, but then
tempers that requirement by stating not all things in it may be ignored.
Chromium OS refines the concept by splitting them: there are "the implicit
system dependencies" and there is the "@system set". The former concept is what
we want to define, and the latter is merely a (partial) consequence of the
first.

More specifically, we don't want to say "the sys-apps/coreutils package is part
of the implicit system dependencies". Instead, we say "\`mv\` and \`ls\` are
part of the implicit system dependencies". Then, with that declaration in hand,
it is obvious to say the default @system set includes the sys-apps/coreutils
package.

The reason for this is that the coreutils package includes a ton of utilities,
many of which we would not consider part of the implicit system set. While it's
unreasonable to say "if you use \`mv\` or \`touch\` or \`ls\` or \`echo\` or
\`cat\`, you have to depend on coreutils" due to their high prevalence, you can
make a reasonable case that if your program runs \`pinky\`, then you probably
should depend on sys-apps/coreutils. The number of packages that use that tool
is low (if there are any at all).

## Guidelines

### Libraries

The only library packages you may assume exist in the implicit system set are
the C library and runtime libraries provided by the compiler. This isn't just
the main C library, but encompasses all the libraries that
[POSIX](http://pubs.opengroup.org/onlinepubs/9699919799/) documents like:

*   -lc
*   -lcrypt
*   -ldl
*   -lm
*   -lpthread
*   -lresolv
*   -lrt
*   -lutil

For internal runtime libraries that the compiler provides (like -lstdc++ and
-lgcc and -lgcc_s and -lasan and the rest), you should not depend on these
yourself.

For all other third party libraries like -lz or -luuid or -lblkid or -lcrack,
you must depend on the relevant package that provides that library.

### Programs

Typically, if the program you wish to use is defined by
[POSIX](http://pubs.opengroup.org/onlinepubs/9699919799/idx/utilities.html) or
the [FHS](http://www.pathname.com/fhs/), you do not have to depend on it. For
the exact list though, please consult the implementation below.

### Compilation

In order to build from source, you do not have to have build time dependencies
on:

*   compiler (e.g. sys-devel/gcc and the utils/frontends it provides)
*   assembler/linker/archiver/strip/etc... (e.g. sys-devel/binutils)
*   OS headers (e.g. sys-kernel/linux-headers)
*   make (e.g. sys-devel/make)
*   tar (e.g. app-arch/tar)
*   gzip/bzip2 (e.g. app-arch/gzip & app-arch/bzip2)
*   any other runtime package listed in the Programs list above

### Exceptions

Because it wouldn't be a good set of guidelines without exceptions, here they
are.

*   If your package is binary only (no source is available to the
            ebuild), then you should depend on the exact libraries that it is
            linked against (e.g. that will frequently be sys-libs/glibc and
            sys-libs/gcc-libs)

## Implementation

You can find the default implicit system package in the
[chromiumos-overlay](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/virtual/implicit-system).
It documents the various packages it pulls in and gives reasoning as to why each
one exists.
