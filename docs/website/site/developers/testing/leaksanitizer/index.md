---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: leaksanitizer
title: LeakSanitizer
---

LeakSanitizer (LSan) is a heap leak detector similar to
[HeapLeakChecker](/system/errors/NodeNotFound) or to the leak checker in
[Valgrind/Memcheck](/system/errors/NodeNotFound). LSan can be run on top of
[AddressSanitizer](/developers/testing/addresssanitizer) with no additional
slowdown compared to plain ASan.

See <https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer>
for more information.

## LeakSanitizer bots

LeakSanitizer is enabled on the ASan bots on
[chromium.memory](http://build.chromium.org/p/chromium.memory/console) (both
Linux and ChromeOS, with the exception of the "sandboxed" bot). All ASan trybots
also have LSan enabled.

### My CL has benign (test-only, intentional etc) leaks, how do I suppress them?

You should fix the CL instead. The leaks may be harmless, but letting them
accumulate over time has a negative impact on the stability of our bots. If you
leak memory intentionally, use an in-code annotation instead (see
`base/debug/leak_annotations.h`).

Ideally, you should only land suppressions for leaks in third-party code that
you have no control over. If that is the case, take a look at
build/sanitizers/lsan_suppressions.cc. The syntax is as follows:

`leak:<wildcard>`

The wildcard will be matched against the function/module/source file name of
each frame in the stack trace. Supported special characters are `*`, `^`
(beginning of string) and `$` (end of string).

Please be careful not to make the wildcard too generic. The [TSan
v2](/developers/testing/threadsanitizer-tsan-v2) page has some examples of good
and bad suppressions.

You can supply additional suppressions by adding
`suppressions=/path/to/suppressions.txt` to `LSAN_OPTIONS`.

## Using LeakSanitizer

If you don't want to run LeakSanitizer locally, you can skip this section.

### Building Chromium with ASan + LeakSanitizer

LeakSanitizer is supported on x86_64 Linux only. To build, follow the
[ASan](/developers/testing/addresssanitizer) build instructions and add a
reference to is_lsan. Leak detection must not be enabled at build time, or LSan
will find leaks during the build process and fail. Make sure `ASAN_OPTIONS` is
not set when you build.

In GN, set up an "lsan" build directory (the directory naming is up to you):

> gn args out/lsan

Add the args to the resulting editor:

> is_asan = true

> is_lsan = true

> is_debug = false # Release build.

> enable_nacl = false # Not necessary, but makes things faster.

And then build:

> ninja -C out/lsan base_unittests

### Using LeakSanitizer on top of ASan

At run-time, add `detect_leaks=1` to ASAN_OPTIONS. You will also need to enable
symbolization, which is required for suppressions to work:

ASAN_OPTIONS="detect_leaks=1 symbolize=1
external_symbolizer_path=$SRC/third_party/llvm-build/Release+Asserts/bin/llvm-symbolizer"
out/Release/base_unittests

A typical LSan report looks like this:

`=================================================================`

`==18109==ERROR: LeakSanitizer: detected memory leaks`

`Direct leak of 1024 byte(s) in 1 object(s) allocated from:`

` #0 0x430205 in _Znam _asan_rtl_`

` #1 0xfc0edd in TestBody base/tools_sanity_unittest.cc:83`

` #2 0x10eeef7 in Run testing/gtest/src/gtest.cc:2067`

` #3 0x10f04e1 in Run testing/gtest/src/gtest.cc:2244`

` #4 0x10f1357 in Run testing/gtest/src/gtest.cc:2351`

` #5 0x10fe7b2 in RunAllTests testing/gtest/src/gtest.cc:4177`

` #6 0x10fdcbc in impl testing/gtest/src/gtest.cc:2051`

` #7 0x12b5d16 in Run base/test/test_suite.cc:167`

` #8 0x128eaf6 in main base/test/run_all_unittests.cc:8`

` #9 0x7f18a1dc576c in __libc_start_main
/build/buildd/eglibc-2.15/csu/libc-start.c:226`

` #10 0x444a6c in _start ??:0`

`SUMMARY: LeakSanitizer: 1024 byte(s) leaked in 1 allocation(s).`

Note that LeakSanitizer is incompatible with the sandbox. You must pass
`--no-sandbox` if you want to run browser tests/Chrome with LSan.

#### Using debug versions of shared libraries

(NOTE: the libstdc++ part is no longer required for Chromium, where we now use a
custom libc++ binary for ASan builds.)

Be aware that ASan's fast stack unwinder depends on frame pointers, which are
often missing in release versions of shared libraries. If you want to use the
fast unwinder (enabled by default), you should at least install a debug version
of libstdc++. This worked for us on Ubuntu:

`sudo apt-get install libstdc++6-4.6-dbg`

`LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu/debug" ASAN_OPTIONS="detect_leaks=1
`strict_memcmp=0" out/Release/base_unittests

If you still see incomplete stack traces, you can disable the fast unwinder by
adding `fast_unwind_on_malloc=0` to `ASAN_OPTIONS`.

#### GLib

GLib may not play well with leak detection tools in the default mode. We
recommend to pass `G_SLICE=always-malloc`, especially if you're trying to
reproduce a GLib-related leak.

### Using LeakSanitizer without ASan

LSan can also be used without ASan instrumentation. In the build flags above,
omit the is_asan reference and specify only s_lsan. At runtime, use
`LSAN_OPTIONS` instead of `ASAN_OPTIONS`. Disabling ASan instrumentation can
bring a performance advantage, but be aware that this mode is not as well tested
as the LSan+ASan mode.

In this mode leak detection is enabled by default. When building, you must
explicitly set `LSAN_OPTIONS="detect_leaks=0"` to prevent build errors (see
above).
