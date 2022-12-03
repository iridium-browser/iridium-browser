---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: llvm-clang-build
title: Chrome OS build with LLVM Clang and ASAN
---

This page describes steps to build ChromeOS with Clang or
[ASAN](/developers/testing/addresssanitizer) (which is Clang-based). Address
Sanitizer ([ASAN](/developers/testing/addresssanitizer)) is a fast memory error
detector based on compiler instrumentation.

Note that currently we can reliably build ChromeOS Chrome with Clang. Other
Chrome OS modules are TBD.

If you are interested in use of Clang or ASAN, please subscribe to the "page
changes" to be notified when the process below is improved. Please send feedback
about this process to Denis Glotov and/or chromium-os-dev mailing list.

1. Have your chroot set up and all packages built. Just as in [Chromium OS
Developer Guide](/chromium-os/developer-guide).

2. Specify what you want to build with: either clang only or clang + ASAN.

```none
export USE="$USE clang asan"
```

3. Build ChromeOS Chrome as usual.

```none
emerge-$BOARD chromeos-chrome
```

Note that we have builders that use Clang/ASAN and runs auto tests. See more
here: [Sheriff FAQ: Chromium OS ASAN bots](/system/errors/NodeNotFound).
