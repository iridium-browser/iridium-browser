---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: rendering-benchmarks
title: Rendering Benchmarks (aka Smoothness benchmarks)
---

**## **Deprecated: see** <https://chromium.googlesource.com/chromium/src/+/HEAD/docs/speed/benchmark/harnesses/rendering.md>**

## **Contact: nduca, ernstm****

### Chrome now has an awesome rendering benchmark system for GPU and rendering related benchmarks. It works on all chrome flavors, even android and CrOS, even in their content_shell forms.

### We use the following terminology:

*   ### action: an interaction with a web page that we want to measure.
            e.g. scrolling, or simply waiting for a few seconds
*   ### page set: a collection of web pages and associated actions
*   ### metric: computes high-level statistical measures (e.g. means and
            medians) from raw data (e.g. traces).
*   ### measurement: loads the pages from a page set, executes the
            associated actions, collects raw data, and computes final results
            using a metric.
*   ### benchmark: bundles together a measurement and a page_set

### To run the rendering benchmarks you need:

*   ### A chrome checkout ([get the
            code](/developers/how-tos/get-the-code))
*   ### A chrome build. Just canary or a stable will work. Or download a
            continuous build from
            <http://commondatastorage.googleapis.com/chromium-browser-continuous/index.html>
*   ### python.

### Once you've got these things, you're ready to go. To run our top 25 page set
through our smoothness measurement (which tests scrolling speed for sites that
scroll, or interaction speed for sites that have interactions):

> ./run_benchmark --browser=canary smoothness.top_25

### To measure impl-side painting on important mobile sites:

> tools/perf/run_benchmark --browser=canary smoothness.key_mobile_sites

To measure the key mobile sites on an attached android device:

> tools/perf/run_benchmark --browser=android-chrome smoothness.top_25

### To run the smoothness measurement on a Chrome OS device with IP address $CHROMEBOOK_IP from a host machine with a chromium checkout, do this:

> `tools/perf/run_benchmark --browser=cros-chromeos --remote=$CHROMEBOOK_IP
> smoothness.top_25`

Lets break down this command a bit:

*   **`tools/perf`** is where we keep our rendering benchmarks. It
            contains benchmarks, which are written in Python.
*   **`run_benchmark`** is the script we use to run a specific
            benchmark. Benchmarks are a combination of a "measurement" (e.g.
            numbers to compute) and a list of pages to compute those numbers
            for.
*   **`--browser=canary`** tells the script to use Chrome Canary, if it
            is installed on the system. If you don't have canary \[e.g. you're
            on Linux\] it'll fail and tell you to give it another browser.
    *   `--browser=list` - for all browsers that the script thinks it
                can use. Pass --browser=list -vvv if you're not seeing a browser
                you expect to see.
    *   `--browser=system` - the stable chrome install on your system
    *   `--browser=debug` or `release` - chromium from out/Debug or
                out/Release, if it was found
    *   `--browser=content-shell-debug` - a content shell build found in
                out/Debug
    *   `--browser=android-chrome` - chrome detected on an attached
                android device via adb
    *   `--browser=cros-chrome --remote=$CHROMEBOOK_IP` - chrome running
                on your chromebook
    *   `--browser=exact --browser-executable=<path to build>` - your
                tests will work with any chrome build &gt;= M18!
*   **`smoothness.top_25`** is one of our canned benchmarks to run. If
            you type `./run_benchmark`, you'll see a list of other benchmarks
            that we support. There are a lot, and you can add more. :)
*   **`tools/perf/page_sets/top_25.py`** is a list of 25 pages that get
            a tremendous amount of volume on desktop, and hence are important
            for us to not regress. This pageset is used by a variety of
            benchmarks, from smoothness to thread times. There are other sets of
            pages too, for example `key_desktop_sites, key_mobile_sites,` and
            `tough_scrolling_cases`. Some have hundreds or thousands of sites.
            Some have only a few.
*   If you want to have a benchmark that computes smoothness, but for a
            different pageset, then just copy an existing benchmark's .py file
            and make a new one from it that points at your favorite pageset.

When you run a benchmark, you'll get some output that looks like this:

> Pages: \[http___www.youtube.com\]

> \*RESULT frame_times: frame_times=
> \[16.900000,17.280000,17.340000,16.810000,17.400000,17.500000,17.020000,16.820000,17.070000,17.190000,17.560000,17.670000,17.450000,17.570000,17.270000,17.410000,17.590000,17.530000,17.240000,17.550000,17.190000,17.140000,17.090000,17.500000,17.540000,17.000000,17.300000,17.600000,17.430000,17.070000,17.070000,17.760000,17.090000,16.950000,17.020000,17.040000,16.780000,17.060000,17.700000,17.850000,17.230000,17.090000,17.110000,17.110000,17.610000,17.200000,16.990000,17.180000,17.140000,17.130000,17.430000,17.080000,17.100000,17.100000,17.970000,17.150000,17.600000,17.400000,17.140000,16.920000,17.790000,16.780000,17.440000,16.860000,17.720000,17.700000,17.610000,16.940000,17.200000,16.980000,17.260000,17.310000,17.380000,16.960000,17.000000,17.500000,17.240000,17.170000\]
> ms Avg frame_times: 17.267564ms Sd frame_times: 0.279332ms \*RESULT jank:
> jank= 19.4673 \*RESULT mean_frame_time: mean_frame_time= 17.268 ms \*RESULT
> mostly_smooth: mostly_smooth= 1.0 RESULT telemetry_page_measurement_results:
> num_failed= 0 count RESULT telemetry_page_measurement_results: num_errored= 0
> count

These are some key statistics for that page as it scrolled, in the default mode
for that platform. But, lets say you wanted to run chrome with some extra
commandline arguments. For this, `--extra-browser-args` is your friend:

> `tools/perf/run_benchmark --browser=canary smoothness.top_25
> tools/perf/page_sets/top_25.json
> --extra-browser-args="--force-llamas-to-have-parachutes
> --use-rainbows-instad-of-omnibox"`

Fun! Remember, unless you pass `--disable-gpu-vsync`, scrolling goes only as
fast as your screen. So, for screen with 60 Hz refresh, 16.6 is usually a good
thing.

### Smoothness Metrics

These are the most important smoothness metrics:

*   **mean_frame_time**: arithmetic mean of frame times
*   **jank**: absolute discrepancy of frame time stamps, where
            discrepancy is a measure of irregularity. It quantifies the worst
            jank. For a single pause, discrepancy corresponds to the length of
            this pause in milliseconds. Consecutive pauses increase the
            discrepancy. The metric is important because even if the mean and
            95th percentile are good, one long pause in the middle of an
            interaction is still bad.
*   **mostly_smooth**: were 95 percent of the frames hitting 60 fps;
            boolean value (1/0).
*   **frame_times**: list of raw frame times, helpful to understand the
            above 3 metrics
*   **mean_pixels_approximated**: percentage of pixels that was
            approximated (checkerboarding, low-resolution tiles, etc.).
*   **mean_input_event_latency**: time between receiving a touchmove
            event and performing the frame swap for the corresponding scroll
            event.

### Rasterize_And_Record Metrics

### Throughout the metrics, you will see the words *paint*, *record*, and *raster*. These have very precise meanings:

*   **paint:** Time dumping WebKit's rendering structures into the
            compositor's rendering structures in software mode and regular
            compositing modes. This is the time spent to walk the webkit tree
            AND software-rasterize its 2D ops AND any time required to do image
            decodes
*   **record:** (impl-side painting mode only). This is the time to JUST
            walk the WebKit tree and dump it into an SkPicture.
*   **raster:** (impl-side painting mode only). This is the time to
            rasterize SkPictures to tiles. If we had a decode cache miss, will
            include time servicing the image cache miss.

In some sense impl-side painting splits paint into raster + record.

rasterize_and_record measurement calculates the time spent in raster and record.
It automatically enables impl-side painting and only works on platforms that
support this feature.

**How it works**

[Telemetry](/developers/telemetry) performance testing framework

Page scrolling is done by telemetry's "scroll" action,
`tools/telemetry/telemetry/internal/actions/scroll.py`. On chrome, it boots the
browser with `--enable-gpu-benchmarking-extension`, which exposes a
`beginSmoothScrollBy(numPixels, function() { callback })` API to javascript that
simulates scrolling as would be done by the user. We then use it to move a page
down.

Other synthetic gestures, such as pinch-to-zoom or swipe, are available. See the
[Synthetic Gestures in
Chrome](https://docs.google.com/a/chromium.org/document/d/1VZ0FhQVc5e2HA9_mJ3HRQlXylCMgNiEJx9kVXe7yTfE/edit?usp=sharing)
document for a full list of gestures and how they're implemented in Chrome.

The smoothness measurement `tools/perf/measurements/smoothness.py` captures a
trace from this interaction, and extracts the time stamps of frames that were
generated. Specifically, the
`BenchmarkInstrumentation::ImplThreadRenderingStats` and
`BenchmarkInstrumentation::MainThreadRenderingStats` events are analyzed. These
events are issued from `cc/debug/benchmark_instrumentation.cc` based on data
collected through `cc/debug/rendering_stats_instrumentation.h`. The smoothness
metric `tools/perf/metrics/smoothness.py` is then used to calculate the metrics
described above (mean_fram_time, jank, etc.).

Telemetry provides a way to separate out the measurement process from the
interaction process from the actual pages being tested. We then maintain a
number of important lists of web pages (page sets), some synthetic some real, in
`tools/perf/page_sets`, grouped by their kind of importance. `top_25`,
`key_desktop_sites` and `key_mobile_sites` are likely of particular interest to
users.

Telemetry provides a mechanism to very reliably record a web page and then
replay it many times in that exact recorded state. We (Chrome team) cannot make
our recordings public since the assets the recording are the property of the
site owners. However, we have exposed a utility that anyone can use to make
their own recordings:

> tools/perf/record_wpr --browser=system tools/perf/page_sets/top_25.json

This will place a file called `top_25.wpr` in `tools/perf/page_sets/data` that
is an archive of the data required to replay those pages back over-and-over
again without deviation.

## Adding credentials to test live sites that require a logged in user

As part of GPU testing, we often want to measure the performance of a site like
Gmail, or Facebook, that sit behind a login. We do not give out logins for
these, but if you have your own, you can put a `credentials.json` in
`tools/perf/page_sets/data` or `~/.telemetry-credentials` in the style of
`tools/telemetry/examples/credentials_example.json` with the right logins and
telemetry will automatically then login to gmail or facebook for you. Patches
are welcome to add support for other sites as well.
