---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: cellular-testing
title: 'Cellular: Testing'
---

The cellular team uses autotests for end-to-end testing of the cellular
components. There are a few separate suites:

1.  suite_Cellular: You should run this before pushing a CL that affects
            part of the cellular stack. Tests here can run on real hardware and
            shouldn't take too long.
2.  control.network_3g: This is what the hardware test lab runs "every
            so often". This runs on real hardware and can take a long time if
            needed.
3.  BVT (the build verify test): runs nothing (yet). We need to fix this
            by making some of our tests appropriate for running in a VM. This
            should run quickly.
