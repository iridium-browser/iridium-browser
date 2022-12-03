---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
- - /developers/testing/isolated-testing
  - Isolated Testing
page_name: deterministic-builds
title: Deterministic builds
---

[TOC]

## Summary

Make Chromium's build process to deterministic. Tracking issue:
[crbug.com/314403](http://crbug.com/314403)

**Handling failures on the deterministic bots**

See
<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/deterministic_builds.md#handling-failures-on-the-deterministic-bots>

## Goal

*Improve cycle time and reduce infrastructure utilization with redundant tasks.*

### Sub goals

1.  Reduce I/O load on the Isolate Server by having deterministic
            binaries.
    *   Binaries do not need to be archived if they are already in the
                Isolate Server cache; if the build is deterministic, this
                happens frequently.
2.  Take advantage of native's [Swarming task
            deduplication](https://code.google.com/p/swarming/wiki/SwarmingDetailedDesign#Task_deduplication)
            by skipping redundant test executables.
    *   If an .isolated task was already run on Swarming with the exact
                same SHA-1 and it succeeded, [Swarming returns the cached
                results
                immediately](https://code.google.com/p/swarming/wiki/SwarmingUserGuide#Task_idempotency).

So it's actually a 2x multiplier here for each target that becomes deterministic
and runs on Swarming. Benefits are both **monetary** (less hardware is required)
and **developer time** (reduced latency by having less work to do on the TS and
CI).

We estimate **we'd save around &gt;20% of the current testing load.** This will
result in faster test cycles on the Try Server (TS) and the Continuous
Integration (CI) infrastructure. **Swarming already dedupe 1~7% of the tasks
runtime** simply due to incremental builds.

## Background

*Test isolation* is an on-going effort to generate an exact list of the files
needed by a given unit test at runtime. It enables 3 benefits:

*   Horizontal scaling by splitting a slow test into multiple shards run
            concurrently.
*   Horizontal scaling by running multiple tests concurrently on
            multiple bots.
*   Multi-OS testing of the same binaries. For example, build once, test
            on XP, Vista, Win7, Win8, Win10.

Tracking issue: [crbug.com/98637](http://crbug.com/98637)

*Swarming* is the task distributor that leverage Test isolation to run tests
simultaneously to reduce latency in getting test results.

Normal projects do not have deterministic builds and chromium is one example. *A
deterministic build is not something that happens naturally*. It needs to be
done. *Swarming knows the relationship between an isolated test and the result
when run on a bot with specific features.* The specific features are determined
by the requests. For example the request may specify bot OS specific version,
bitness, metadata like the video card, etc.

Google internally uses many tricks to achieve similar performance improves at
extremely high cache rates:
\[[link](http://google-engtools.blogspot.com/2011/08/build-in-cloud-how-build-system-works.html)\]

> *Building the whole action graph would be wasteful (...), so we will skip
> executing an action unless one or more of its input files change compared to
> the previous build. In order to do that we keep track of the content digest of
> each input file whenever we execute an action. As we mentioned in the previous
> blog post, we keep track of the content digest of source files and we use the
> same content digest to track changes to files.*

## Non Goals

Making the build deterministic is not a goal in these conditions:

*   Content-addressed build. A CAB can be built out of a deterministic
            build but this is not what this effort is about.
*   Making the build deterministic on official builds; at least not in
            the short term. Reproducible builds do have security benefits so it
            can be considered an eventual extension of the project.

## Testing plan

Enforced by bots on the public waterfall on Android, Linux, and Windows; FYI
bots on Mac. See also
<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/deterministic_builds.md>.

## Method

## Documented at <http://blog.llvm.org/2019/11/deterministic-builds-with-clang-and-lld.html>

## OS Specific Challenges

Each toolset is non-deterministic in different ways so the work has to be redone
on each platform.

### Windows

Tracking issue: <https://crbug.com/330260>

Builder:
<https://ci.chromium.org/p/chromium/builders/luci.chromium.ci/Windows%20deterministic>

*   Mostly done. See bug for status.

### macOS

Tracking issue: [crbug.com/330262](http://crbug.com/330262)

Builder:
<https://ci.chromium.org/p/chromium/builders/luci.chromium.ci/Mac%20deterministic>

*   Mostly done. See bug for status.

### Linux

Tracking issue: [crbug.com/330263](http://crbug.com/330263)

Builder:

<https://ci.chromium.org/p/chromium/builders/luci.chromium.ci/Deterministic%20Linux>

<https://ci.chromium.org/p/chromium/builders/luci.chromium.ci/Deterministic%20Linux%20%28dbg%29>

*   Linux is DONE!

### Android

Tracking issue: [crbug.com/383340](http://crbug.com/383340)

Builder:

<https://ci.chromium.org/p/chromium/builders/luci.chromium.ci/Deterministic%20Android>

<https://ci.chromium.org/p/chromium/builders/luci.chromium.ci/Deterministic%20Android%20%28dbg%29>

*   Android is DONE!

### iOS

Tracking issue: [crbug.com/383364](http://crbug.com/383364)

Builder:
<http://build.chromium.org/p/chromium.swarm/waterfall?builder=IOS%20deterministic%20build>

*   Not investigated yet.

## Example workflow on Windows

```none
# Do a whitespace change foo.cc to force a compilation.
echo "" >> foo_main.cc
# It recreates the same foo.obj than before since the code didn't change.
compile foo_main.cc -> foo_main.obj
# This step could be saved by a content-addressed build system, see "extension of the project" below.
link foo_main.obj -> bar_test.exe
# The binary didn't change, it is not uploaded again, saving both I/O and latency.
isolate bar_test.exe -> isolateserver
# Swarming immediately return results of the last green build, saving both utilization and latency.
swarming.py run <sha1 of bar_test.isolated>
```

## Extension of the project

Getting the build system to be content addressed as described in the Google
reference above. This is a secondary task. It is out of scope for this project
but is a natural extension. This saves on the build process itself at the cost
of calculating hashes for each intermediary files. This will be piling on the
fact that the build is itself deterministic. This is not worth doing before the
build is itself deterministic and this property is enforced. This will save
significantly on the build time itself on the build machines.
