---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/telemetry
  - 'Telemetry: Introduction'
page_name: profiling
title: 'Telemetry: Profiling'
---

[TOC]

**Warning:** Use these directions at your own risk: --profiler is deprecated and
[platform/tracing_controller.py](https://code.google.com/p/chromium/codesearch#chromium/src/tools/telemetry/telemetry/core/platform/tracing_controller.py)
should be used instead for scenarios requiring long-term support for profiling
data collection.

## Tracing

On all platforms, Telemetry can gather an
[about:tracing](http://www.chromium.org/developers/how-tos/trace-event-profiling-tool)
trace for each page in the page_set. To do so, simply pass the --profiler=trace
flag. For example:

```none
$ tools/perf/run_benchmark --profiler=trace page_cycler tools/perf/page_sets/typical_25.json 
```

Instructions for opening the results will be printed.

## CPU Profiling - Linux / Android and Mac

### Linux / Android

In order for Linux profilers to display call graph information correctly, it is
necessary to first build with profiling=1 (GYP) or enable_profiling = true (GN).
To do so:

```none
$ # Linux (GYP)
$ GYP_DEFINES='profiling=1' gclient runhooks 
$ ninja -C out/Release chrome
$ # Android (GYP)
$ GYP_DEFINES="OS=android profiling=1 release_extra_cflags=-fno-omit-frame-pointer disable_pie=1" \
    gclient runhooks
$ ninja -C out/Release chrome_public_apk
$ # GN
$ gn args out/Profiling
# include "enable_profiling = true"
$ export CHROMIUM_OUTPUT_DIR="$PWD/out/Profiling"
$ ninja -C out/Profiling chrome_public_apk
```

### perf

On Linux and Android, Telemetry can gather a
[perf](https://perf.wiki.kernel.org/index.php/Tutorial) profile for each page in
the page_set. To do so, pass the --profiler=perf flag. For example:

```none
$ tools/perf/run_benchmark --profiler=perf kraken
```

Instructions for opening the results will be printed.

If the page_set you are profiling includes cross-origin navigations, it is
necessary to pass --page-repeat=2 so that renderer process is not swapped out
mid-profile on the second run. For example:

```none
$ tools/perf/run_benchmark --profiler=perf --page-repeat=2 page_cycler tools/perf/page_sets/intl_ar_fa_he.json
```

### You may find it useful to install a copy of perf from your distro's repository and use that instead of the included perfhost binary (e.g. for additional UI options), as long as you use the flags Telemetry suggests.

### VTune

On x86 builds (including Android on x86) Telemetry can collect an [Intel
VTune](https://software.intel.com/en-us/intel-vtune-amplifier-xe) profile for
each page in the page_set. This requires VTune to be installed on your home
system. To collect a profile, pass the --profiler=vtune flag. For example:

```none
$ tools/perf/run_benchmark --profiler=vtune kraken
```

You can then open the results in the VTune GUI. Instructions for opening the
results will be printed.

### gprof

While not explicitly supported by Telemetry. It is possible to gather a
[gprof](http://sourceware.org/binutils/docs/gprof/) profile for the entire
measurement run when running in single process mode. To do so, first build with
profiling enabled, -fno-omit-frame-pointer and without position independent
code:

```none
$ GYP_DEFINES='profiling=1' gclient runhooks
```

```none
$ ninja -C out/Release chrome
```

Then run with CPUPROFILE in the environment and --single-process. For example:

```none
$ CPUPROFILE=/tmp/myprofile tools/perf/run_benchmark --extra-browser-args=--single-process sunspider
```

View the profile like so:

```none
$ cpprof --gv out/Release/chrome /tmp/myprofile
```

### systrace

Telemetry also supports recording an Android
[Systrace](http://developer.android.com/tools/help/systrace.html) while running
measurements:

```none
$ tools/perf/run_benchmark --profiler=android-systrace kraken
```

The resulting trace file can be opened in Chrome by going to
[about:tracing](http://www.chromium.org/developers/how-tos/trace-event-profiling-tool)
and clicking on "Load".

### Mac

On mac, Telemetry can gather an
[Instruments](http://developer.apple.com/library/mac/#documentation/DeveloperTools/Conceptual/InstrumentsUserGuide/Introduction/Introduction.html)
profile for each page in the page_set. To do so, pass the --profiler=iprofiler
flags. For example:

```none
$ tools/perf/run_benchmark --profiler=iprofiler octane
```

Instructions for opening the results will be printed. Once in Instruments, if
symbols are not resolved, go to “File &gt; Re-Symbolicate Document...” and
browse to the \*.dsym files for Chromium Framework and Chromium Helper.

## Memory Profiling - Linux / Android

### Linux / Android (C/C++)

On special instrumented builds of linux Android, Telemetry can gather TCMalloc
heap dumps that can be then fed to [Deep Memory
Profiler](/developers/deep-memory-profiler) . Please check that page for:

    GYP_DEFINES for both platforms.

    linux: environment variables

    android: system properties (note: requires a rooted device)

Run any measurement with --profiler=tcmalloc-heap flags. For example:

```none
$ tools/perf/run_benchmark -v --browser=android-content-shell --profiler=tcmalloc-heap memory.top_25
```

Once the measurement completes, the dump files will be in /tmp/{RANDOM}.

Follow instructions from the [Deep Memory
Profiler](/developers/deep-memory-profiler) to generate the data and graphs.

Note: depending on the page set, there’ll be multiple directories under
/tmp/{RANDOM}, one per page in the set. In each folder, there’ll be a mix of
dumps per browser and renderer processes for that particular page. You may want
to analyse the browser process across the entire page set, rather than just a
particular page. To do so:

    There’s a file called “browser.pid” on each one of the page directories.

    Copy (or move) all dump files for that pid to say, “browser/”.

    Run dmprof on that folder.

### Android Memory Report

Telemetry can gather high level "memory report" for each page in the page_set.
To do so, pass the --profiler=android-memreport flags. At the end of the test,
it will print a link to the HTML page that was generated. This profile gives an
overview of all memory (including mapped files, read-only, dirty, clean pages,
etc..) but without any backtrace like the ones above. It does not require any
instrumented build, as it gathers all information directly from /proc/PID/maps,
etc..

### Android Java

Telemetry can gather “heap profiles” for the java side for each page in the
page_set. To do so, pass the --profiler=java-heap flags. For example:

```none
$ tools/perf/run_benchmark -v --browser=android-content-shell --profiler=java-heap memory.top_25
```

The dump files will then be fetched in /tmp/{RANDOM} in the host, already
converted to “hprof” format which can be used by tools like [Eclipse’s
MAT](http://www.eclipse.org/mat/). [This post
](http://android-developers.blogspot.co.uk/2011/03/memory-analysis-for-android.html)
from the android team contains useful instructions. Please note: android has a
“zygote” process that contains pre-loaded resources for all apps. These
resources will show up in the heap profile: they are actually shared across all
apps, they aren't exclusive to the process being profiled.

## Network Profiling - Linux / Android

### Linux / Android

Telemetry can gather “tcpdump” network captures for each page in the page_set.
To do so, pass the --profiler=tcpump flags. For example:

```none
$ tools/perf/run_benchmark --browser=android-content-shell --profiler=tcpdump memory_measurement memory.top_25
```

The dump files will then be fetched in /tmp/{RANDOM}, and then can be opened in
tools such as [Wireshark](http://www.wireshark.org/).

On android, "tcpdump" binary will be downloaded from [cloud
storage](http://www.chromium.org/developers/telemetry/upload_to_cloud_storage)
and installed in the device automatically.

## "Manual" Profiling - Android

Sometimes it's useful to drive the browser "manually" in order to run a specific
profiler during a regular browsing session. This command:

```none
$ tools/perf/record_android_profile.py --browser=android-chrome --profiler=perf
```

Will launch the browser and the associated profiler and wait until you hit
enter. It will then fetch all profile data from the device and print
instructions on how to open them.

You can also use
[adb_profile_chrome](http://www.chromium.org/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs)
to record a perf profile from a running browser:

```none
$ tools/android/adb_profile_chrome --browser=build --time 5 --perf
```
