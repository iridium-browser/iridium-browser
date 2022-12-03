---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: threadsanitizer-tsan-v2
title: ThreadSanitizer (TSan) v. 2
---

ThreadSanitizer v2 is a synchronization error detector based on compiler
instrumentation. It is also capable of detecting other threading errors like
deadlocks, unjoined threads, destroying locked mutexes, use of async-signal
unsafe code in signal handlers, and others.

You may have heard of the old Valgrind-based version of ThreadSanitizer, but
TSan v2 has a brand new state machine and has nothing to do with Valgrind. The
old ThreadSanitizer is deprecated in Chromium.

ThreadSanitizer v2 is only supported on Linux so far.

### Building Chromium with ThreadSanitizer

Set up a GN build by typing gn gen out/tsan && gn args out/tsan (use a directory
name of your choice). Enter these variables in the resulting editor:

> is_tsan = true

> enable_nacl = false

> is_debug = false

Then build like normal with ninja -C out/tsan base_unittests

In GYP:

> `GYP_GENERATORS=ninja GYP_DEFINES='tsan=1 disable_nacl=1 use_goma=1' gclient
> runhooks`

> `ninja -C out/Release base_unittests`

**Note:** TSan builds with libc++ by default (the `use_custom_libcxx=1` GYP
flag). If your tests fail under TSan, make sure you're not relying on some
unspecified libstdc++ behavior.

export
TSAN_OPTIONS="external_symbolizer_path=third_party/llvm-build/Release+Asserts/bin/llvm-symbolizer"

out/Release/base_unittests --no-sandbox 2&gt;&1 | tee log

Running Chrome may require additional options:

`TSAN_OPTIONS="atexit_sleep_ms=200 flush_memory_ms=2000 $TSAN_OPTIONS"
out/Release/chrome --no-sandbox` 2&gt;&1 | tee log

atexit_sleep_ms is 1 second by default. Some tests waiting for child processes
may fail with such a big timeout.

Tests with big memory footprint may hang your machine, so you need to flush
periodically (flush_memory_ms) and skip heavy tests (like OOM)

flush_memory_ms may lead to false negatives, thus the flushing period should be
chosen carefully.

If TSan fails to restore one of the stacks, try adding history_size=7 to
TSAN_OPTIONS (the amount of memory reserved for the stacks is proportional to
2^history_size, 7 is the maximum value).

**Note:** --no-sandbox is essential if you're running Chrome or tests that
invoke Chrome (browser_tests, content_browsertests etc.).

**Note 2:** due to <http://crbug.com/341805> you may need to run Chrome with
--disable-gpu or use xvfb-run.

**Note 3:** running the test multiple times in a row (--gtest_repeat=5) may
increase the reproducibility of the races.

**Note 4:** the following env variables control the behavior of libnss and
libglib. Setting them is optional in most cases, but may help if you're seeing
strange reports in the library code:

export G_SLICE=always-malloc

export NSS_DISABLE_ARENA_FREE_LIST=1

export NSS_DISABLE_UNLOAD=1

**Note 5:** As of November 2018, the commands above don't symbolize the stack
trace. Pipe the output through the ASAN symbolize script, e.g.:

out/Release/base_unittests --no-sandbox 2&gt;&1 |
tools/valgrind/asan/asan_symbolize.py

### Disabling tests

Unlike Valgrind, ThreadSanitizer v2 doesn't support gtest filter files. Instead
of adding a test name to a blocklist you should disable the test in the code
under #if defined(THREAD_SANITIZER).

### Suppressing race reports

The default ThreadSanitizer v2 suppressions reside in
build/sanitizers/tsan_suppressions.cc and are automatically linked to every
executable in Chromium.

You can supply additional suppressions by adding
`suppressions=/path/to/suppressions.txt` to `TSAN_OPTIONS`.

The examples below refer to plain-text suppressions files, the format of
`tsan_suppressions.cc` is almost the same.

ThreadSanitizer data race report contains two or more stack traces of
conflicting memory accesses (the topmost access is the last one) together with
the thread IDs and the acquired mutexes.

For a global variable involved its name is printed, stack or heap memory
locations are described using the allocation stack trace.

When applicable, the stack traces of thread creations and mutex acquisitions are
also listed.

```none
WARNING: ThreadSanitizer: data race (pid=22215)
  Write of size 4 at 0x7f8a8f1d99ec by thread T11:
    #0 New v8/src/zone-inl.h:66 (content_browsertests+0x000001568815)
    #1 zone v8/src/zone-inl.h:98 (content_browsertests+0x000001568815)
    #2 v8::internal::Parser::ParseLazy(v8::internal::Utf16CharacterStream*, v8::internal::ZoneScope*) v8/src/parser.cc:743 (content_browsertests+0x000001568815)
  Previous write of size 4 at 0x7f8a8f1d99ec by thread T8: 
    #0 New v8/src/zone-inl.h:66 (content_browsertests+0x0000013e5568)
    #1 zone v8/src/zone-inl.h:98 (content_browsertests+0x0000013e5568)
    #2 v8::internal::HGraphBuilder::CreateGraph() v8/src/hydrogen.cc:951 (content_browsertests+0x0000013e5568)
    #3 DoGenerateCode<v8::internal::FastCloneShallowArrayStub> v8/src/code-stubs-hydrogen.cc:288 (content_browsertests+0x0000012cb218)
    #4 v8::internal::FastCloneShallowArrayStub::GenerateCode() v8/src/code-stubs-hydrogen.cc:355 
 
  Location is global 'v8::internal::Zone::allocation_size_' of size 4 at 7f8a8f1d99ec (content_browsertests+0x0000063c69ec)
```

Each suppression is a one line of the form "suppression_type:pattern". The most
common suppression type is "race", see
<https://github.com/google/sanitizers/wiki/ThreadSanitizerSuppressions> for
other suppression types.

The pattern is matched against:

*   function name/file name/module of each frame in the stack trace of
            each conflicting memory access
*   global variable name (if present)

The pattern may contain the wildcard ('\*') symbol which matches any substring.
'\*' is automatically prepended to each pattern unless it starts with '^'.
'\*' is automatically appended to each pattern unless it ends with '$'.

Good suppressions match a single race report (or a number of reports with a
common root cause), but are unlikely to mask further races in other components.

A suppression must be preceded by a comment (started with a "#") with a crbug
link.

Examples of good suppressions for the above race report:

```none
# Suppresses other races in zone-inl.h as well.
race:v8/src/zone-inl.h
# Effectively the same as "v8/src/zone", but more verbose.
race:v8/src/zone*
# You can also suppress globals.
race:v8::internal::Zone::allocation_size_
# Ok, but won't match calls to zone() from other places.
race:FastCloneShallowArrayStub::GenerateCode
```

Examples of bad suppressions:

```none
# Watch out - may match other functions called "New".
race:^New$
# Function arguments may change over time, better omit them.
race:v8::internal::Parser::ParseLazy(v8::internal::Utf16CharacterStream*, v8::internal::ZoneScope*)
# Same as "*New*", which will match a ton of other functions.
race:New
# Will suppress everything in content_browsertests.
race:content_browsertests
# Too generic.
race:base::MessageLoop::Run
```

More info on the suppressions format is available at
<https://code.google.com/p/thread-sanitizer/wiki/Suppressions>.

The possible suppression prefixes are: "race:" (for data races and
use-after-free reports), "thread:" (for thread leaks), "mutex:", "signal:",
"deadlock:" (for lock-order-inversion reports).

You can also disable interceptors in a particular library using the
"called_from_lib:libfoo.so" suppression prefix.

### Reproducing race reports in tests

Before trying to reproduce a race report in a Chromium test, make sure they are
not suppressed or ignored.

**Suppressions** from build/sanitizers/tsan_suppressions.cc (as well as those
passed via TSAN_OPTIONS) are applied at program runtime. If the race report
matches a line in the suppressions file, TSan does not print that report.

**Ignores** from tools/memory/tsan_v2/ignores.txt are applied at compile time.
If the function name matches a "fun:" line in the ignores file, TSan does not
instrument that function, effectively ignoring all memory accesses (but not
synchronization) in that function. If the source file name matches an "src:"
line, every function in that file is ignored. Note that the tests do not depend
on ignores.txt, so you need to touch all the affected source files manually
before rebuilding (or make a clean build) after any change to `ignores.txt`.

### Debugging with GDB

You can't execute an instrumented binary from GDB, because it maps something in
the place where TSan needs to map its shadow:

```none
FATAL: ThreadSanitizer can not mmap the shadow memory (something is mapped at 0x555555554000 < 0x7cf000000000)
```

However you can attach GDB to a running TSan process. If you have troubles
catching a particular process, try to run with `TSAN_OPTIONS=stop_on_start=1`.

Every subprocess will print the following line:

```none
ThreadSanitizer is suspended at startup (pid 20492). Call __tsan_resume().
```

In order to proceed, you'll have to run `gdb -p 20492` and type `call
__tsan_resume()`.
