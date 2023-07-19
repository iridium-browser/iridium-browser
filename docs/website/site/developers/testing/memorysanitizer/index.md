---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: memorysanitizer
title: MemorySanitizer (MSan)
---

MemorySanitizer (MSan) is a tool that detects use of uninitialized memory.

MSan is supported on x86_64 Linux. Additional info on the tool is available at
<http://clang.llvm.org/docs/MemorySanitizer.html>.

MSan in Chromium is unlikely to be usable on systems other than Ubuntu
Precise/Trusty - please see the note on instrumented libraries below.

MSan bots are running on
[chromium.memory.fyi](http://build.chromium.org/p/chromium.memory.fyi/waterfall?builder=Chromium+Linux+MSan+Builder&builder=Linux+MSan+Tests&builder=Chromium+Linux+ChromeOS+MSan+Builder&builder=Linux+ChromeOS+MSan+Tests&reload=none),
[client.webrtc](http://build.chromium.org/p/client.webrtc/builders/Linux%20MSan)
and
[chromium.webkit](http://build.chromium.org/p/chromium.webkit/builders/WebKit%20Linux%20MSAN).
There are also two LKGR builders for ClusterFuzz: [no
origins](http://build.chromium.org/p/chromium.lkgr/builders/MSAN%20Release%20(no%20origins)),
[chained
origins](http://build.chromium.org/p/chromium.lkgr/builders/MSAN%20Release%20(chained%20origins))
(see below for explanation). V8 deployment is ongoing.

Trybots:
[linux_chromium_msan_rel_ng](https://ci.chromium.org/p/chromium/builders/try/linux_chromium_msan_rel_ng),
[linux_chromium_chromeos_msan_rel_ng](https://ci.chromium.org/p/chromium/builders/try/linux_chromium_chromeos_msan_rel_ng).


[TOC]

## Pre-built Chrome binaries

You can grab fresh Chrome binaries for Linux built with MSan
[here](http://commondatastorage.googleapis.com/chromium-browser-msan/index.html).

## How to build and run

To set up an MSan build in GN:

```none
gclient runhooks
gn args out/msan
```

In the resulting editor, set the build variables:

> is_msan = true

> is_debug = false # Release build.

(Note: if you intend to run the Blink web tests with the MSan-instrumented
content_shell binary, you must use out/Release instead of out/msan, because
otherwise the test expectations will not apply correctly.)

(In older versions of Chromium you also had to explicitly set
"use_prebuilt_instrumented_libraries = true". This is now the default if is_msan
is set and can no longer be overridden.)

MSan requires using Instrumented system libraries. Note that instrumented
libraries are supported on Ubuntu Precise/Trusty only. More information:
[instrumented-libraries-for-dynamic-tools](/developers/testing/instrumented-libraries-for-dynamic-tools).

The following flags are implied by is_`msan=true` (i.e. you don't have to set
them explicitly):

*   `v8_target_arch=arm64`: JavaScript code will be compiled for ARM64
            and run on an ARM64 simulator. This allows MSan to instrument JS
            code. Without this flag there will be false reports.

Some common flags may break a MSAN build. For example, don't set
"dcheck_always_on = true" when using MSAN.

If you are trying to reproduce a test run from the [Linux ChromiumOS MSan
Tests](https://ci.chromium.org/p/chromium/builders/ci/Linux%20ChromiumOS%20MSan%20Tests)
build, other GN args may also be needed.
You can look for them via your test run page, under the section "lookup builder
GN args". Add all of them, except the goma_dir.

**Running on gLinux locally**

testing/xvfb.py out/msan/unit_tests --gtest_filter="&lt;your test filter&gt;"
**Running on Ubuntu Trusty**

Run the resulting binaries as usual. Pipe both stderr and stdout through
`tools/valgrind/asan/asan_symbolize.py` to get symbolized reports:

```none
./out/msan/browser_tests |& tools/valgrind/asan/asan_symbolize.py
```

### **Disable OpenGL**

Chrome must not use hardware OpenGL when running under MSan. This is because
libgl.so is not instrumented and will crash the GPU process. SwANGLE can be
used as a software OpenGL implementation, although it is extremely slow. There
are several ways to proceed:

*   --disable-gpu: This forces Chrome to use the software path for
            compositing and raster. WebGL will still work using SwANGLE.
*   `--use-gl=angle --use-angle=swiftshader: This switches Chrome to use SwANGLE
            for compositing, (maybe) raster and WebGL.`
*   `--use-gl=angle --use-angle=swiftshader --disable-gl-drawing-for-tests`: Use this if
            you don't care about the actual pixel output. This exercises the
            default code paths, however expensive SwANGLE calls are replaced
            with stubs (i.e. nothing actually gets drawn to the screen).

If neither flag is specified, Chrome will fall back to the first option after
the GPU process crashes with an MSan report.

### Origin tracking

MSan allows the user to trade off execution speed for the amount of information
provided in reports. This is controlled by the GN/GYP flag `msan_track_origins`:

*   `msan_track_origins=0`: MSan will tell you where the uninitialized
            value was used, but not where it came from. This is the fastest
            mode.
*   `msan_track_origins=1` (deprecated): MSan will also tell you where
            the uninitialized value was originally allocated (e.g. which
            malloc() call, or which local variable). This mode is not
            significantly faster than `msan_track_origins=2`, and its use is
            discouraged. We do not provide pre-built instrumented libraries for
            this mode.
*   `msan_track_origins=2` (default): MSan will also report the chain of
            stores that copied the uninitialized value to its final location. If
            there are more than 7 stores in the chain, only the first 7 will be
            reported. Note that compilation time may increase in this mode.

### Suppressions

MSan does not support suppressions. This is an intentional design choice.

We have a [blocklist
file](https://source.chromium.org/chromium/chromium/src/+/main:tools/msan/ignorelist.txt)
which is applied at compile time, and is used mainly to compensate for tool
issues. Blocklist rules do not work the way suppression rules do - rather than
suppressing reports with matching stack traces, they change the way MSan
instrumentation is applied to the matched function. In addition, blocklist
changes require a full clobber to take efffect. Please refrain from making
changes to the blocklist file unless you know what you are doing.

Note also that instrumented libraries use separate blocklist files.

## Debugging MSan reports

Important caveats:

*   Please keep in mind that simply reading/copying uninitialized memory
            will not cause an MSan report. Even simple arithmetic computations
            will work. To produce a report, the code has to do something
            significant with the uninitialized value, e.g. branch on it, pass it
            to a libc function or use it to index an array.
*   When you examine a stack trace in an MSan report, all third-party
            libraries you see in it (with the exception of libc and its
            components) should reside under
            `out/Release/instrumented_libraries`. If you see a DSO under a
            system-wide directory (e.g. /`lib/`), then the report is likely
            bogus and should be fixed by simply adding that DSO to the list of
            instrumented libraries (please file a bug under
            `Stability-Memory-MemorySanitizer` and/or ping eugenis@).
*   Inline assembly is also likely to cause bogus reports. Consequently,
            assembly-optimized third-party code (such as libjpeg_turbo, libvpx,
            libyuv, ffmpeg) will have those optimizations disabled in MSan
            builds.
*   If you're trying to debug a V8-related issue, please keep in mind
            that MSan builds run V8 in ARM64 mode, as explained below.

MSan reserves a separate memory region ("shadow memory") in which it tracks the
status of application memory. The correspondence between the two is bit-to-bit:
if the shadow bit is set to 1, the corresponding bit in the application memory
is considered "poisoned" (i.e. uninitialized). The header file
`<sanitizer/msan_interface.h>` declares interface functions which can be used to
examine and manipulate the shadow state without changing the application memory,
which comes in handy when debugging MSan reports.

Print the complete shadow state of a range of application memory, including the
origins of all uninitialized values, if any. (Note: though initializedness is
tracked on bit level, origins have 4-byte granularity.)

```none
void __msan_print_shadow(const volatile void *x, size_t size);
```

The following prints a more minimalistic report which shows only the shadow
memory:

```none
void __msan_dump_shadow(const volatile void *x, size_t size);
```

To mark a memory range as fully uninitialized/initialized:

```none
void __msan_poison(const volatile void *a, size_t size);
void __msan_unpoison(const volatile void *a, size_t size);
void __msan_unpoison_string(const volatile char *a);
```

The following forces an MSan check, i.e. if any bits in the memory range are
uninitialized the call will crash with an MSan report.

```none
void __msan_check_mem_is_initialized(const volatile void *x, size_t size);
```

This milder check returns the offset of the first (at least partially) poisoned
byte in the range, or -1 if the whole range is good:

```none
intptr_t __msan_test_shadow(const volatile void *x, size_t size);
```

Hint: sometimes to reduce log spam it makes sense to query
`__msan_test_shadow()` before calling `__msan_print_shadow()`.

The complete interface can be found in
`src/third_party/llvm-build/Release+Asserts/lib/clang/3.6.0/include/sanitizer/msan_interface.h`.
Functions such as `__msan_unpoison()` can also be used to permanently annotate
your code for MSan, but please CC eugenis@ if you intend to do so.

## Reproducing ClusterFuzz Bugs

Because MSan only supports Ubuntu Precise/Trusty and not Rodete, the
[ClusterFuzz reproduce tool](https://github.com/google/clusterfuzz-tools) cannot
reproduce bugs found using MSan (on Rodete).

If you are on Rodete, you can try to reproduce them manually using docker to run
MSan by following [these
instructions](/developers/testing/memorysanitizer#TOC-Running-on-other-distros-using-Docker).
