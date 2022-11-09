---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: 64-bit-support
title: Building 64-bit Chromium
---

### Linux

*   64-bit builds will be the default when compiling on 64-bit Linux.

### Mac OS X

*   Run the following command prior to building: build/gyp_chromium
            -Dtarget_arch=x64 -Dhost_arch=x64

### Windows

*   The default GN build will match that of the host OS (so 64-bit
            Windows will give 64-bit builds). The default GYP build is 32-bits
            on all Windows variants.
*   To force 64-bit building in GYP, run the following command prior to
            building: python build\\gyp_chromium -Dtarget_arch=x64 *and* use
            either the Release_x64 or Debug_x64 build directory with Ninja
    (e.g. "ninja -C out\\Debug_x64")
