---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: chromiumos-architecture-porting-guide
title: Chromium OS Architecture Porting Guide
---

[TOC]

## Introduction

So you decided that the existing cpu architectures (Intel's x86_64/i686 and
ARM's 32-bit arm) platforms weren't up to the task for your next system?
Instead, you opted for some magical and new architecture? What you desire is
possible, but you should buckle up and prepare for a bumpy ride!

## Various Upstream Projects

Keep in mind that Chromium OS is based on a lot of upstream projects that we
have not authored. That means you can't really get by with just porting Chromium
OS and forgetting about the rest.

For example, your architecture must already be supported by (at least):

*   [Binutils](http://sourceware.org/binutils/)
*   [GCC](http://gcc.gnu.org/)
*   [GDB](http://sourceware.org/gdb/)
*   [Glibc](http://sourceware.org/glibc/) (sorry, but alternative C
            libraries are not currently supported e.g.
            [uClibc](http://www.uclibc.org/))
*   [Linux kernel](http://kernel.org/)
    *   Must have
                [CONFIG_MMU=y](http://en.wikipedia.org/wiki/Memory_management_unit)
                (if you don't know what this means, then you most likely have
                one, and can thus disregard)

This is just the tip of the iceberg. If you're missing any of these pieces, then
you've got a lot of work ahead of you. Come visit us once you've gotten those
sorted out.

### Existing Gentoo Support

Chromium OS is based on [Gentoo](http://www.gentoo.org/). Hopefully you've
selected a processor that they already support (they support many already). You
can find the current list here:
<http://sources.gentoo.org/profiles/arch.list>

Find what you're looking for? Great! Life will be much easier and you should
move on to the next section.

Your architecture not listed there? Sorry! You're first going to have to
coordinate with the [Gentoo
maintainers](http://www.gentoo.org/main/en/lists.xml) in porting to your crazy
architecture. Come visit us once you've gotten those sorted out.

## Chromium OS Pieces

More To Come!
