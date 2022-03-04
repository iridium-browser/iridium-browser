---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: optimizing-energy-consumption
title: Optimizing Energy Consumption
---

# Background Info

Here are two really great talks on optimizing battery life:

    Mac: [Maximizing Battery Life on OS X, WWDC
    2013](https://developer.apple.com/videos/wwdc/2013/#701) is a great
    overview.

    Android, [project volta](https://www.youtube.com/watch?v=KzSKIpJepUw) talk.
    (Google-internal [project
    volta](https://sites.google.com/a/google.com/volta/) [tech
    talk](http://go/volta-tech-talk-video) and
    [slides](http://go/volta-tech-talk))

# Telemetry tests

We use Telemetry benchmarks for testing and analyzing energy usage. See [code
concepts](http://www.chromium.org/developers/telemetry#TOC-Code-Concepts) for
details on benchmarks, measurements, and page sets.

## Measurements

    On desktop, the tab_switching measurement opens a group of tabs and measures
    energy after they are all loaded. All except the last tab are backgrounded.
    You can use the benchmark with a blank page in the foreground to understand
    how background tabs behave, or with a regular page in the foreground to
    focus on the foreground tab.

    On android, our chromium.perf tests and most of our development is on chrome
    shell, which does not support tabs. So we use the power test instead. It
    will load each page in the set, with a 20 second pause, a scroll, and
    another 20 second pause in between each load.

## Page sets

It might be a good idea to [record a custom page
set](http://www.chromium.org/developers/telemetry/record_a_page_set) if you’re
working on a very specific issue, for example below-the-fold animated gif. But
we generally use the below page sets for most of our testing and debugging:

Desktop:

    [typical_25](https://code.google.com/p/chromium/codesearch#chromium/src/tools/perf/page_sets/typical_25.py)
    - This is a set of 25 web pages designed to represent general web browsing.

    [top_10](https://code.google.com/p/chromium/codesearch#chromium/src/tools/perf/page_sets/top_10.py)
    - The Alexa top 10 sites.

    [top_10_mobile](https://code.google.com/p/chromium/codesearch#chromium/src/tools/perf/page_sets/top_10_mobile.py)
    - Mobile versions of the Alexa top 10

    [five_blank_pages](https://code.google.com/p/chromium/codesearch#chromium/src/tools/perf/page_sets/five_blank_pages.py)
    - For debugging background noise that occurs regardless of which page you’re
    on.

    [tough_energy_cases](https://code.google.com/p/chromium/codesearch#chromium/src/tools/perf/page_sets/tough_energy_cases.py)
    - Very complex or energy-heavy pages.

Android:

    [typical_10_mobile](https://code.google.com/p/chromium/codesearch#chromium/src/tools/perf/page_sets/typical_10_mobile.py)
    - 10 typical mobile pages.

## Metrics

Each platform has different techniques for measuring energy usage, and often
there are multiple techniques (hardware devices, software estimation). Below are
the names of the metrics, the platforms they are available on, and how they are
gathered.

    energy_consumption_mwh - This is the energy consumption in milliwatt hours.
    In general, this is a software estimation. You can see all the methods we
    have for measuring energy consumption in the
    [power_monitor](https://code.google.com/p/chromium/codesearch#chromium/src/tools/telemetry/telemetry/core/platform/power_monitor/)
    directory in telemetry, but here is a quick run-down of how the bots on the
    waterfall calculate this:

        Android uses
        [dumpsys](https://code.google.com/p/chromium/codesearch#chromium/src/tools/telemetry/telemetry/core/platform/power_monitor/android_dumpsys_power_monitor.py&q=dumpsys&sq=package:chromium&l=12)
        software estimation on L release and above.

        Except Nexus 10, which uses the built in
        [ds2784](https://code.google.com/p/chromium/codesearch#chromium/src/tools/telemetry/telemetry/core/platform/power_monitor/android_ds2784_power_monitor.py)
        fuel gauge on L release and above.

        Mac OS uses
        [powermetrics](https://code.google.com/p/chromium/codesearch#chromium/src/tools/telemetry/telemetry/core/platform/power_monitor/powermetrics_power_monitor.py)
        on 10.9 and above ([more details
        here](https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/powermetrics.1.html)).

        Windows and Linux use
        [MSR](https://code.google.com/p/chromium/codesearch#chromium/src/tools/telemetry/telemetry/core/platform/power_monitor/msr_power_monitor.py)
        on intel processors with sandybridge or later.

    application_energy_consumption_mwh - This is derived by measuring
    energy_consumption_mwh during a quiescent period before running chrome, then
    subtracting that from the energy_consumption_mwh used during the test run.
    The goal is to find the amount of energy used by chrome and ignore the
    steady state energy used by the OS and background processes. However, if
    there is too much noise this number can be off.

    idle_wakeups_total - This is the total number of idle wakeups across all
    processes. It’s currently only available on Mac through powermetrics, but we
    are working on Linux (bug [422170](http://crbug.com/422170)) and Android
    (bug [422967](http://crbug.com/422967)).
    This number is important because it’s a proxy for energy consumption, and it
    figures heavily in the [Energy Impact](http://support.apple.com/kb/HT5873)
    score in OS X. See the [Maximizing Battery Life on OS
    X](https://developer.apple.com/videos/wwdc/2013/#701) talk for more info
    about how reducing idle wakeups improves battery life.

    board_temperature - Temperature in degrees celsius. Available through MSR on
    Windows/Linux (intel sandybridge and later). Also available on Android
    devices running L release+.

# Developing/Testing Locally

    Choose or record a telemetry page set that matches the use case you want to
    test.

    Run the correct telemetry benchmark for your platform (tab_switching for
    desktop, acceptance test for android). Use the --profiler=trace and
    --pageset-repeat=1 flags; this will output a trace and only repeat the test
    once. Example:
    tools/perf/run_benchmark tab_switching.five_blank_pages \\
    --browser=release --profiler=trace --pageset-repeat=1

    Telemetry outputs a trace file. You’ll see the file name in stdout. Open it
    up in
    [about:tracing](http://www.chromium.org/developers/how-tos/trace-event-profiling-tool).
    Look for places where:

        Chrome is doing unexpected work. For example, a timer that wakes every
        1s on a blank page should not be needed. Removing unneeded timers is a
        great way to reduce idle wakups.

        Work is happening too frequently and could be coalesced. Could two IPCs
        be grouped into one? Doing the same amount of work with fewer IPCs
        reduces idle wakeups.

Comparing traces between two builds that have different energy usage can also be
helpful. You can use the --browser argument in telemetry to easily run the test
on your stable, beta, or dev channel and compare that with a local build
(--browser=release or --browser=debug).

# Verifying Results on Perf Bots

Energy measurements are very sensitive to noise. Sometimes you make a change
locally that drives down idle wakeups and you believe will reduce energy
consumption, but the energy numbers you get are just noise. Sometimes you don’t
have access to the OS or hardware you want to test. The chromium.perf and
tryserver.chromium.perf bots can help!

Perf Try Jobs

You can run tab_switching tests on the perf trybots on the
[tryserver.chromium.perf
waterfall](http://build.chromium.org/p/tryserver.chromium.perf/waterfall). Here
is the [documentation for perf try
jobs](http://www.chromium.org/developers/performance-try-bots). If idle wakeups
numbers are available, they will be in the “memory” tab of the results html
page.

chromium.perf waterfall

Several energy tests are running on the chromium.perf waterfall:

    [tab_switching.five_blank_pages](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac9%2Cchromium-rel-win7-dual%2Cchromium-rel-win7-gpu-ati%2Cchromium-rel-win7-gpu-intel%2Cchromium-rel-win7-gpu-nvidia%2Cchromium-rel-win7-single%2Cchromium-rel-win7-x64-dual%2Cchromium-rel-win8-dual%2Cchromium-rel-xp-dual%2Clinux-oilpan-release%2Clinux-release&tests=tab_switching.five_blank_pages%2Fenergy_consumption_mwh&rev=299488&checked=energy_consumption_mwh%2Cblank_page.html_ref%2Cref%2Cenergy_consumption_mwh%2Cblank_page.html_ref%2Cref)

    [tab_switching.top_10](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac9%2Cchromium-rel-win7-dual%2Cchromium-rel-win7-gpu-ati%2Cchromium-rel-win7-gpu-intel%2Cchromium-rel-win7-gpu-nvidia%2Cchromium-rel-win7-single%2Cchromium-rel-win7-x64-dual%2Cchromium-rel-win8-dual%2Cchromium-rel-xp-dual%2Clinux-oilpan-release%2Clinux-release&tests=tab_switching.top_10%2Fenergy_consumption_mwh&rev=299488&checked=energy_consumption_mwh%2Cref)

    [tab_switching.typical_25](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac9%2Cchromium-rel-win7-dual%2Cchromium-rel-win7-gpu-ati%2Cchromium-rel-win7-gpu-intel%2Cchromium-rel-win7-gpu-nvidia%2Cchromium-rel-win7-single%2Cchromium-rel-win7-x64-dual%2Cchromium-rel-win8-dual%2Cchromium-rel-xp-dual%2Clinux-oilpan-release%2Clinux-release&tests=tab_switching.typical_25%2Fenergy_consumption_mwh&rev=299488&checked=energy_consumption_mwh%2Cref%2Cenergy_consumption_mwh%2Cref)

    [tab_switching.tough_energy_cases](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac9%2Cchromium-rel-win7-dual%2Cchromium-rel-win7-gpu-ati%2Cchromium-rel-win7-gpu-intel%2Cchromium-rel-win7-gpu-nvidia%2Cchromium-rel-win7-single%2Cchromium-rel-win7-x64-dual%2Cchromium-rel-win8-dual%2Cchromium-rel-xp-dual%2Clinux-oilpan-release%2Clinux-release&tests=tab_switching.tough_energy_cases%2Fenergy_consumption_mwh&rev=299488&checked=energy_consumption_mwh%2Cref%2Cenergy_consumption_mwh%2Cref)

    [power.android_acceptance](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-motoe%2Candroid-nexus10%2Candroid-nexus4%2Candroid-nexus5%2Candroid-nexus7v2&tests=power.android_acceptance%2Fenergy_consumption_mwh&checked=core)

You can look at the effects of a submitted change. After [bug
423394](http://crbug.com/423394) is fixed, you will be able to get traces for
some of the runs as well.