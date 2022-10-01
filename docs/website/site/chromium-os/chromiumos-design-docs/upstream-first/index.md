---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: upstream-first
title: Upstream First
---

## Abstract

This document outlines a process for managing the flow of kernel source patches
between Chromium OS and hardware partners.

## Goals

Our goal with the "Upstream First" policy is to eliminate kernel version
fragmentation.

We know that hardware partners support other devices as well as Chromium
OS-based devices. So rather than having partners target multiple kernels for
multiple devices, we'd like to encourage them to target one kernel: the upstream
kernel.

By having everyone target a single kernel, we hope to avoid duplication of
effort and get everyone (our hardware partners, their partners, and the upstream
community) working together on improving a single set of patches. By working
together, we can build a better kernel and avoid later having to rework changes
that are incompatible with the upstream kernel.

## Accepting patches

A device driver patch must be accepted upstream before it can be accepted into
the Chromium OS kernel. Ideally, the patch will be in Linus Torvalds's kernel
tree before it is accepted into the Chromium OS kernel. However, other options
are also available.

The following list shows options for getting a patch accepted into the Chromium
OS kernel. We encourage all partners to strive to attain option 1.

**Patch options, ordered from high to low preference**

1.  The patch is accepted into Linus Torvalds's upstream tree.
2.  The patch is accepted into the subsystem maintainer tree (such as
            Dave Miller's netdev tree).
3.  The patch is accepted into the the device maintainer tree (such as
            samsung or msm) and a pull request has been sent to Linus Torvalds.

After the patch is accepted into an upstream tree, Google will cherry-pick it
into the Chromium OS kernel. If the patch requires backport work, Google will
work with the vendor to do the backport.

Exemptions are granted on a case-by-case basis.
