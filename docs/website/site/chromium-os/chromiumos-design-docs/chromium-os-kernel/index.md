---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: chromium-os-kernel
title: Kernel Design
---

[TOC]

## Abstract

This document outlines the overall design of the kernel, as it relates to
Chromium OS.

See also the [Frequently Asked Questions
page](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/kernel_faq.md)
for more day-to-day details.

## Background

Chromium OS uses the Linux Kernel. Historically we stayed on a 2.6.32
Ubuntu-based for the first several releases, but have since then moved on to
track the upstream mainline kernel directly, applying our changes for the
features and stability we need on top of it.

## Source code

### Git repository

The Chromium OS Linux kernel will be stored in a gerrit/git repository, hosted
on an externally accessible website. All platforms will build from **one**
single kernel source tree (though there will be multiple binary images).

If necessary, we will create short-term private branches for specific vendor
projects that involve pre-production hardware. These branches will be merged
back into the main kernel as soon as possible, using one code review per CL so
that all code in our master branch is either from upstream or code reviewed. The
intent of these private branches is to enable code review and project status
monitoring of code that cannot be publicly released yet, due to NDA. The intent
is specifically **not** to use these branches to keep changes that are shipping
to customers.

Any code submitted should be done as one Git commit per logical change, with a
good description. (See
[Documentation/SubmittingPatches](/chromium-os/chromiumos-design-docs/chromium-os-kernel/submitting-patches)
in the kernel tree.) We may create temporary public branches for the purpose of
working together on code review and integration.

### Licensing

All Linux kernel code, from Google and from partners, must be released under
GPLv2 and must be released under the [DCO (documented certificate of
ownership)](/chromium-os/chromiumos-design-docs/chromium-os-kernel/dco) as a
signoff of ownership. Each commit will contain the signoff of the code
author/committer and the ACK of the patch approver. Code should be submitted
under the Chromium contributor license agreement.

### Changelog entries

Here's an example of a changelog entry:

```none
CHROMIUM: bibble: a patch to fix everything
Bug: 12345
This patch fixes bug 12345 by checking for a NULL pointer before dereferencing ops->thingy
Signed-off-by: Author <author@example.net>
Signed-off-by: Committer <committer@example.net>
Acked-by: Approver <approver@example.net>
```

All kernel code (including backports from upstream) contributed by Google or
partners will include a classification tag in the first line of the commit log.
The classification will be useful when the maintainer needs to rebase to a newer
kernel; the tag is also needed for proper attribution. The tag is in
ALL_CAPS_UNDERSCORE, is followed by a colon, and is at the beginning of the
first line. The first line from each commit should be a summary.

Whenever possible use the -x flag with git cherry-pick. This appends a line to
the commit message that says which commit this cherry-pick came from. Only do
this in the case that the commit was cherry-picked from a repository that is
easily accessible by the public.

Code contributed by the Chromium community will use the tag CHROMIUM. For
example:

```none
CHROMIUM: bibble: a patch to fix everything
```

Code backported from upstream (Linus's tree) will use the tag UPSTREAM. For
example:

```none
UPSTREAM: bibble: a patch to fix everything
```

Code backported from an upstream maintainer tree will use two tags. For example:

```none
UPSTREAM: WIRELESS: bibble: a patch to fix everything
```

Code ported from a Linux distribution tree or other non-upstream tree will also
use an appropriate tag. For example:

```none
UBUNTU: bibble: a patch to fix everything
```

*or*

```none
YOCTO: bibble: a patch to fix everything
```

Code ported from a patch will use the tag FROMLIST and should include a link to
the list the patch was obtained from. For example:

```none
FROMLIST: bibble: a patch to fix everything
```

```none
```

```none
(am from https://patchwork.kernel.org/patch/0987654/)
```

Code backported that you had to change to make it run with an older kernel
version, including conflict resolutions, will use the tag BACKPORT. For example:

```none
BACKPORT: bibble: a patch to fix everything
```

### Upgrades

The kernel will be upgraded to a new version as soon as practical after a new
version of the upstream kernel is released. We will do this via a Git rebase,
this means we'll keep clean versions of our patches floated on top of the latest
tree. In practice, this will happen approximately every 3-6 months, and
approximately every other kernel versions from upstream. Other, smaller updates
will be done on a continuous basis, such as merging in the -stable kernel
updates.

All third-party vendors are expected to supply updated versions of their code
against every new mainline kernel version (for example, at 3.2, 3.3, and so on)
within 14 days of its release, if the code is not already upstream.

## Supported and unsupported features

### Kernel modules and initial RAM disks

We will support kernel modules, though this feature may be removed at some point
in the future. We will **not** support modules built outside of the main kernel
tree.

We will not support initial RAM disks (initrd) for the general kernel, but will
need them on recovery image kernels as well as for the factory install flow.

### Architectures

Support for i386, ARMv7 and x86_64 is planned.

The tradeoff for using 64 bits is additional power consumption and memory usage,
in return for greater performance.

### Swap

We do not plan to support swap in our initial release, but will conduct a
further feasibility investigation to review this. We are concerned about the
effect of swap on:

*   SSD write cycle lifetime
*   maximum latency

We realize that not having swap will limit how many tabs a user can keep open
and the amount of anonymous memory a process can allocate. Aggregate size of
anonymous memory in the system will be limited to the size of RAM. We may
revisit this decision in future releases.

## Configuration files

We use a custom split config to help keep settings unified across different
architectures. See the [Kernel Configuration
document](/chromium-os/how-tos-and-troubleshooting/kernel-configuration) for
more details.
