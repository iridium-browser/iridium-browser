---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/bugs
  - Security Bugs--
page_name: reproducing-clusterfuzz-bugs
title: Reproducing ClusterFuzz bugs
---

If you've been assigned a ClusterFuzz bug but aren't sure what to do next,
you're in the right place. If you want to help catch more bugs, you should also
read up on [developing new
fuzzers](/Home/chromium-security/bugs/developing-fuzzers-for-clusterfuzz) and
[using ClusterFuzz](/Home/chromium-security/bugs/using-clusterfuzz).

Noticed a problem? File a bug using [this
template](https://bugs.chromium.org/p/chromium/issues/entry?components=Tools%3EStability%3EClusterFuzz).

[TOC]

## Getting started

In the best case scenario, downloading the minimized test case and running it in
chrome (or whatever binary you are testing) will be enough to reproduce the
issue. Usually, you will also need to include the command line flags specified
in the report as well.

When this doesn't work, you can also try downloading the exact build that the
crash was found in via the "build" link in the ClusterFuzz report. If you are
able to reproduce the issue using that, but not in your build, it may be a sign
that you need to enable some kind of memory debugging tool, such as
[AddressSanitizer](/developers/testing/addresssanitizer). The tool that you
should will depend on the type of bug. See the common problems section for more
information.

More complicated cases, such as test cases with gestures, may require you to use
the ClusterFuzz local reproduction script. There are several quirks about that
bot environment that this script is able to emulate which may also help you
reproduce the crash consistently. Download the "local reproduction config" file
from the report, and pass it as the --config argument to the script. If you
haven't installed the script yet, see the instructions below. Unfortunately, it
is Googler-only at the moment :(

If you still aren't able to reproduce the issue, you have a few options. For
certain job types such as
[SyzyASan](https://code.google.com/p/syzygy/wiki/SyzyASanBug) ones, a minidump
may also be available. The report itself also includes some information about
what may have gone wrong. If this is enough to work with, you could attempt a
speculative fix. If there is nothing actionable in the report, simply mark it as
WontFix.

## Common problems

*   Test cases with gestures tend to be very difficult to reproduce. If
            you notice that a test case has gestures (look for the "interaction
            gestures" section at the top of the report), you should try using
            the local reproduction script.
*   Some test cases do not reproduce correctly unless served over HTTP.
            Look for a "requires" section in the report, or a notification that
            the test case requires HTTP on the bug.
*   You may not be able to reproduce all issues in a normal chromium
            build since ClusterFuzz makes heavy use of various memory debugging
            tools. Try using
            [AddressSanitizer](/developers/testing/addresssanitizer),
            [SyzyASan](https://code.google.com/p/syzygy/wiki/SyzyASanBug),
            [MemorySanitizer](/developers/testing/memorysanitizer),
            [LeakSanitizer](/developers/testing/leaksanitizer),
            [ThreadSanitizer](/developers/testing/threadsanitizer-tsan-v2),
            [UndefinedBehaviorSanitizer](/developers/testing/undefinedbehaviorsanitizer),
            or whichever tool is being used in the report that has been assigned
            to you.
*   Sometimes, a crash just isn't reproducible. If a crash is marked as
            unreproducible in ClusterFuzz and you're seeing similar results,
            don't spend too long re-running it. If the information in the report
            isn't enough to work with, don't be afraid to mark the bug as
            WontFix.
*   Some tools, like the IPC fuzzer, have a more complicated setup
            process. In these cases, we try to provide additional help links in
            the report itself to documentation which may be useful.

## Setting up the local reproduction script (Googler only)

If you weren't able to reproduce the issue using only the minimized test case
and command line arguments from the report, you may want to try using the local
reproduction script. It attempts to mimic the environment on the ClusterFuzz
bots as closely as possible, and will attempt to run the test multiple times in
case it's flaky.

See
[goto.google.com/clusterfuzz-repro](http://goto.google.com/clusterfuzz-repro)
for setup details.
