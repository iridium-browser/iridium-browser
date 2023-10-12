---
breadcrumbs:
- - /developers
  - For Developers
page_name: creating-v8-profiling-timeline-plots
title: Creating V8 profiling timeline plots
---

For visualization, timeline graphs can be plotted to show where V8 is spending
time. This can be used to find bottlenecks and spot things that are unexpected
(for example, too much time spent in unoptimized code). Data for the plot are
gathered by both sampling and instrumentation. Since the latter distorts the
performance, the plot script attempts to undistort the logged timestamps.

[<img alt="image"
src="/developers/creating-v8-profiling-timeline-plots/Screen%20Shot%202014-09-14%20at%201.27.21%20PM.png">](/developers/creating-v8-profiling-timeline-plots/Screen%20Shot%202014-09-14%20at%201.27.21%20PM.png)

## Profiling in Chrome

### Desktop Chrome

Close all Chrome instances (both Canary and Stable).

To profile a web site in chrome, pass the same flags to V8 using `--js-flags`
and with `--no-sandbox` to enable writing into `v8.log`.

For example:

```
$ ./chrome --no-sandbox --js-flags="--prof --log-timer-events" mail.google.com &
sleep 10; kill $!
```

Given that Gmail is already logged into, this profiles the first 10 seconds
after starting Chrome and loading Gmail, before Chrome is killed.

This will create a v8.log file next to the Chrome binary folder.

If the plotting script fails stating that the logfile is inconsistent, retry
while adding --logfile=v8-%p.log to the --js-flags, which will create a separate
file for each process suffixed with the process' pid.

### Chrome on Android

Since Android has additional security sandboxing, the renderer process will not
be able to write to a file in the application's working directory (even when
--no-sandbox is enabled). You must therefore create a directory which can be
written by any user and instruct V8 to log it's output there.

For example:

```
$ adb shell mkdir /data/local/tmp/v8-logs/
$ adb shell chmod 777 /data/local/tmp/v8-logs/ # [choose adb_content_shell_command_line or adb_chromium_testshell_command_line below depending upon your target]
$ ./build/android/adb_content_shell_command_line --no-sandbox --js-flags=\\"--prof --log-timer-events --logfile=/data/local/tmp/v8-logs/v8.log\\"
```

*Now, after restarting the content shell / chromium test shell, the logs will be available in /data/local/tmp/v8-logs/v8.log and can be retrieved using adb pull.*

## Profiling in d8

## To profile a script in d8, run d8 with the appropriate flags

*   ## `--prof` enables tick based profiling
*   ## `--log-internal-timer-events` enables timing the runtime of V8's
            internal components such as the GC, parser, compiler, etc.
*   ## `--log-timer-events` implies `--log-internal-timer-events`, but
            also times external callbacks.

## For example

## $ out/native/d8 --prof --log-timer-events imaging-darkroom.js

## The log entries will be written into `v8.log`

## Plotting

Find the sure you have the `isolate-***.log `or` v8.log` file created during
profiling. It was created in the folder you launched that command from.

Open
<https://v8.googlecode.com/svn/branches/bleeding_edge/tools/profviz/profviz.html>

*   In the file picker dialog, select the log file you want to view.
*   Hit start and wait a few seconds for the plot to generate
*   You can modify the range selection and hit Start again to replot.

## Interpretation of the plot. A case study.

Let's profile
[Octane's](http://www.google.com/url?sa=D&q=http%3A%2F%2Foctane-benchmark.googlecode.com)
pdfjs benchmark. Due to the nature of the benchmark, having many accesses to
typed arrays, we expect a considerable amount of time being spent in external
callbacks (which implement typed arrays).

Figure 1: Using only `--prof`, the resulting plot only contains the sampling
based data. Note that the granularity isn't really enough to get a good picture
of all this. The colors of the top-most javascript stack frames show that we are
running more and more optimized code.

[<img alt="image"
src="/developers/creating-v8-profiling-timeline-plots/0-prof-only.png">](/developers/creating-v8-profiling-timeline-plots/0-prof-only.png)

*   **GCScavenger**: young space GC
*   **GCCompactor**: old space full GC
*   **Execute**: time spent executing JS code (as opposed to GC, waiting
            on native APIs to return, etc.)
*   **Code kind:** color indicates optimized (green) vs. unoptimized
            (red, “full code”)

Figure 2: Adding the option `--log-internal-timer-events` we still get roughly
the same benchmark score, so we can be sure that the resulting plot has not been
distorted by a lot by instrumentation. We now have a clearer breakdown of the
time spent on this benchmark. We can see that parsing the source takes a
sizeable chunk of time, which also leads to an execution pause. We also see that
at the beginning, a lot of recompiling (optimizing) is going on. However, we do
not see the external callbacks that we are expecting. Without additional
instrumentation, those are incorrectly attributed to V8's execution time.

[<img alt="Figure 2"
src="/developers/creating-v8-profiling-timeline-plots/1-plot-internal.png">](/developers/creating-v8-profiling-timeline-plots/1-plot-internal.png)

Figure 3: Using `--time-timer-events` instead and plotting with `DISTORTION=0`,
we can see that we are indeed spending a lot of time in callbacks. However, the
benchmark score is only a fraction of what it should have been. Comparing this
plot with previous ones, we can see that it is severely distorted by
instrumentation. Heterogeneous sampling ticks, observable by unevenly
distributed white gaps, are yet another indication.

[<img alt="Figure 3"
src="/developers/creating-v8-profiling-timeline-plots/2-distorted.png">](/developers/creating-v8-profiling-timeline-plots/2-distorted.png)

Figure 4: Having the distortion parameter automatically calibrated (plot range
is manually set for easier comparison), we can see that due to the
instrumentation overhead, the benchmark run with instrumentation only executed a
fraction of what would have been without instrumentation. The background to this
is that Octane benchmarks are repeated until a minimum length of run time has
been reached. The un-instrumented run manages to complete more iterations of the
benchmark than the instrumented run, in the same length of run time.

Now that the plot has been undistorted, it almost completely agrees with
previous plots in figures 1 and 2.

[<img alt="Figure 4"
src="/developers/creating-v8-profiling-timeline-plots/3-undistorted.png">](/developers/creating-v8-profiling-timeline-plots/3-undistorted.png)

Figure 5: Zooming into the interesting part of the undistorted plot.

[<img alt="Figure 5"
src="/developers/creating-v8-profiling-timeline-plots/4-autoscale.png">](/developers/creating-v8-profiling-timeline-plots/4-autoscale.png)

## Other resources

*   <http://code.google.com/p/v8/wiki/V8Profiler>
*   <https://developers.google.com/v8/profiler_example>
