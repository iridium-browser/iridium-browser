---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: profiling-in-chromeos
title: Profiling in Chromium OS
---

[TOC]

**Warning: The profiling infrastructure in Chromium OS is new and experimental. You may hit bugs.**

The [perf_events](https://perf.wiki.kernel.org/index.php/Main_Page) (aka perf)
profiling tools can be used to collect profiles in Chromium OS. This page
describes how to run perf on standalone programs, or on autotests.

## Setup

`dev-utils/perf` should be installed by default in the chroot host (NOTE: it may
be in /usr/local/sbin instead of /usr/sbin):

```none
$ /usr/sbin/perf --version
perf version 2.6.38.1720.g564bb7e
```

It should also be installed by default on the target board when building a
developer image for Chromium OS. If it isn't, you can emerge it manually:

```none
$ sudo emerge-${BOARD} perf
```

Some things to note:

1.  You should also make sure you have debug symbols in the chroot,
            ideally for all packages. Otherwise perf report will not show
            function names. Debug symbols are installed by default when you
            emerge a package. Look in `/build/${BOARD}/usr/lib/debug`.
2.  The build-id of the debug symbols must match the binaries installed
            on the Chromium OS device, otherwise the reports will be inaccurate.
            This could happen if you re-emerge a package without copying it onto
            the device which you are profiling on.
3.  The version of perf you are using is tied to the kernel version. If
            you use kernel-9999, use perf-9999. If you use kernel-next, use
            perf-next. Otherwise your results wont be reliable.

## Profiling Autotests

Profiling autotests can be done using run_remote_tests.sh. There are two options
you should be interested in:

```none
  --profiler: What profiler to use? Example: cros_perf.
  --profiler_args: Arguments to pass to the profiler. Example: options=\"-e instructions -g\"
```

For example, to get a profile of AesThroughput using the default perf options,
simply do:

```none
$ ./run_remote_tests.sh --remote=172.18.221.163 AesThroughput --profiler=cros_perf
```

The perf data file will be placed in the autotest results directory when the
test completes:

```none
>>> Details stored under /tmp/run_remote_tests.WBFE
$ ls /tmp/run_remote_tests.WBFE/platform_AesThroughput/platform_AesThroughput/profiling/iteration.1/     
perf.data
```

You can also pass custom arguments to perf. E.g. to collect instruction events
and call graph information:

```none
$ ./run_remote_tests.sh --remote=172.18.221.163 AesThroughput --profile --profiler_args="options=\"-e instructions -g\""
```

Some things to note:

1.  By default this collects system-wide profiles, which is usually what
            you want. You can filter by thread id or binary when you produce the
            report.
2.  perf is invoked immediately prior to the autotest run_once() method
            and is killed immediately after the method completes. Everything
            executed in this method will be included in the profile.

## Profiling Standalone Binaries

SSH to your Chromium OS device and run` perf record`:

```none
$ sudo perf record -g -a -e cycles -- sleep 5
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.186 MB perf.data (~8115 samples) ]
```

You can replace sleep with any binary you want to profile. Use` perf help
record` to see the various options for recording data.

Now copy the data file back to your chroot:

```none
$ scp chronos@chromeos-box:/tmp/perf.data .
```

## Generating Profile Reports

Use `perf report` to display the profile:

```none
$ /usr/sbin/perf report -i perf.data --symfs /build/${BOARD}/ --kallsyms /build/${BOARD}/boot/System.map-3.3.0
```

You can pass --tui for a more interactive interface (buggy), or --stdio for text
output. Use `perf help report` to see the various options for reporting data.

## Bugs?

File it: <http://crbug.com/new>, cc: sonnyrao.
