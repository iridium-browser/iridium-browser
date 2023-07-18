---
breadcrumbs:
- - /developers
  - For Developers
page_name: speed-hall-of-fame
title: Speed Hall of Fame
---

For some time in 2013-2015, we listed a performance improvement of the week.
This page is now here for historical purposes.

But, feel free to nominate future changes! All Chromium contributors are
eligible. To ensure a change is considered, nominate it to
[perf-sheriffs@chromium.org](mailto:perf-sheriffs@chromium.org).

[TOC]

## 2015

### 2015-02-11

This week, we highlight the performance sheriffing process working as it should
due to reliability improvements. A few weeks ago, Joshua Bell landed a [patch
impacting IndexedDB](http://f27dd1d7e9322388546c613cfed39bafda153859/). The
performance sheriff Oystein Eftevaag filed a [bug for an IndexedDB
regression](http://crbug.com/454622) and the autobisect bot submitted a bisect
job on his behalf. It returned with high confidence that Joshua's patch was to
blame. While the regression was unexpected, Joshua investigated, determined it
was his patch, and then posted a [fix that resolved the performance
regression](https://chromium.googlesource.com/chromium/src/+/05d0eec7715de17e1a9b7b9b16a5a5f37d36fb73).
While we still have plenty of work to do improving reliability and decreasing
latency, successes like these show that our improvements are making progress.
Thank you to Oystein for the find and Joshua for the fix!

### 2015-01-20

This week Balazs Engedy [moved opening the LoginDatabase off of the UI
thread](https://chromium.googlesource.com/chromium/src/+/41c91fbbdcfde75c9058d06ceae816f12699fc2f)
and out of the critical path for opening Chrome. This [shaved hundreds of
milliseconds off startup
times](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-win7-dual%2Cchromium-rel-win7-single%2Cchromium-rel-win8-dual%2Clinux-release&tests=startup.warm.blank_page%2Fmessageloop_start_time&checked=messageloop_start_time%2Cref%2Cmessageloop_start_time%2Cref%2Cmessageloop_start_time%2Cref%2Cmessageloop_start_time%2Cref&rev=312613)
for Windows and Linux, decreasing the amount of time until the browser windows
becomes responsive. Thanks for your help Balazs!

## 2014

### 2014-10-28

This week's improvement of the week goes to Ulan Degenbaev. A few weeks ago we
noticed recurring, long garbage collection activity and a seemingly related 80%
spike in metrics from the field. The V8 team triaged this back an issue with
their idle notification scheme, and Ulan [landed a
fix](https://codereview.chromium.org/662543008) to make things right. The patch
has already been merged to M39 and will go out with the next push. Thanks Ulan!

### 2014-09-17

Last week Mike Klein turned on the [new SkRecord-based backend for
SkPicture](https://chromium.googlesource.com/chromium/src/+/c3d2efb33238a3ee19cc8e21f4d91ef8c55f23c4),
which resulted in [25-30% faster
recording](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-motoe%2Candroid-nexus10%2Candroid-nexus4%2Candroid-nexus5%2Candroid-nexus7v2&tests=rasterize_and_record_micro.key_silk_cases%2Frecord_time&checked=record_time%2Crecord_time%2Crecord_time%2Crecord_time%2Crecord_time&start_rev=293371&end_rev=294785)
on Android devices. Recording is one phase of the painting process, so faster
recording will result in smaller paint times and faster framerates. This new
backend is the result of several months of Mike's work. Thanks Mike!

### 2014-08-20

Last week Oystein Eftevaag [landed a
patch](https://codereview.chromium.org/169043004/) to start the commit upon
receiving the last blocking stylesheet. This resulted in a 25% improvement from
the time the user initiates a request to the time they see the first paint.
Unlike most metrics we quote here, this didn't come from from our devices in the
lab, but from real users in the wild. Great work Oystein!

### 2014-08-06

Last week, Danno Clifford landed a JavaScript [array allocation
optimization](https://code.google.com/p/v8/source/detail?r=22645) targeted at
the Kraken benchmark suite. It improved Chromium's score on the [audio-dft
subtest by
30-70%](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-nexus4%2Cchromium-rel-win7-dual&tests=kraken%2Faudio-dft&checked=core)
and the [benchmark total score by
5-15%.](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-nexus4%2Cchromium-rel-win7-dual&tests=kraken%2FTotal&checked=core)
Danno was so pleased with his work that he immediately left on a well-deserved
vacation. Thanks Danno!

### 2014-07-13

This week, Fadi Meawad [fixed a bug in Chrome's power
monitor](https://codereview.chromium.org/401083002/) on Windows that resulted in
an improvement of 7% of the entire system's battery life while running Chrome.
This particular bug gathered[ over 7,000 stars on the issue
tracker](https://code.google.com/p/chromium/issues/detail?id=153139) - the most
of any bug by [several
thousand](https://code.google.com/p/chromium/issues/list?can=2&q=stars%3A5000).
While these savings are impressive, our power work is nowhere near complete.
Stay tuned for more improvements!

### 2014-06-18

Jakob Kummerow landed [two](https://code.google.com/p/v8/source/detail?r=21816)[
optimizations](https://code.google.com/p/v8/source/detail?r=21834) to V8
specifically targeted at optimizing the
[React](http://facebook.github.io/react/) framework. Thanks for your work!

### 2014-06-11

This week, Emil Eklund [finished up
work](https://codereview.chromium.org/315623002/) on
[DirectWrite](http://msdn.microsoft.com/en-us/library/windows/desktop/dd368038(v=vs.85).aspx)
for Windows. In addition to being a widely requested feature by users and
developers with [over 500 stars on the
bug](https://code.google.com/p/chromium/issues/detail?id=25541), DirectWrite
also has performance considerations. For example, we're seeing about a 7-10%
warm page
[load](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-win7-dual%2Cchromium-rel-win7-single%2Cchromium-rel-win7-x64-dual&tests=page_cycler.intl_ko_th_vi%2Fwarm_times&checked=page_load_time%2Cpage_load_time_ref&start_rev=274147&end_rev=276826)[
time](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-win7-dual%2Cchromium-rel-win7-single%2Cchromium-rel-win7-x64-dual&tests=page_cycler.intl_hi_ru%2Fwarm_times&checked=core&start_rev=274147&end_rev=276826)[
improvement](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-win7-dual%2Cchromium-rel-win7-single%2Cchromium-rel-win7-x64-dual&tests=page_cycler.intl_ar_fa_he%2Fwarm_times&checked=core&start_rev=274147&end_rev=276826)
on pages with non-Latin fonts. Thanks Emil, for your work toward a beautiful and
speedy user experience.

### 2014-05-28

Over the past few weeks, Sami Kyostila landed a
[series](https://src.chromium.org/viewvc/chrome?revision=273530&view=revision)[
of](https://src.chromium.org/viewvc/chrome?revision=273984&view=revision)[
patches](https://src.chromium.org/viewvc/chrome?revision=269094&view=revision)
for increased reliability and decreased cycle time of our performance tests.
Performance improvements are fantastic, but without work like Sami's we would be
flying blind with a bunch of broken tests. Thanks for your hard work Sami!

### 2014-04-30

Simon Hatch[
upgraded](https://src.chromium.org/viewvc/chrome?revision=266629&view=revision)
the capabilities of our bisect bots so that they can now bisect functional
breakages and changes in variance. You can find instructions in the ["tips"
section of the
documentation](http://www.chromium.org/developers/tree-sheriffs/perf-sheriffs/bisecting-performance-regressions#TOC-Tips),
but it's as easy as setting bisect_mode to return_code or std_dev in your bisect
jobs. Simon's work should help us quite a bit in our quest for reliable, stable
benchmarks!

### 2014-04-16

Dale Curtis landed a[
patch](https://src.chromium.org/viewvc/chrome?revision=264741&view=revision)
this week that increases the buffer size for audio streams when appropriate,
reducing both CPU and power usage. For certain media types (such as mp3) the
system-wide power consumption improved by[ up to
35%](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac9&tests=media.tough_media_cases%2Fenergy_consumption_mwh&checked=video.html%3Fsrc_tulip2.mp3_type_audio&start_rev=263589&end_rev=264782),
with about[ 20% savings on
average](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac9&tests=media.tough_media_cases%2Fenergy_consumption_mwh&checked=core&start_rev=263589&end_rev=264782).
This is the first of many great power patches we expect to see now that the
infrastructure is in place. Great work Dale!

### 2014-03-26

This week David Reveman landed a
[patch](http://src.chromium.org/viewvc/chrome?revision=254436&view=revision) to
remove task references from RasterWorkerPool on all platforms. I can't possibly
produce a better summary than his own patch description: "This moves the
responsibility to keep tasks alive while scheduled from the RasterWorkerPool to
the client where unnecessary reference counting can be avoided. The result is a
[~5x improvement in BuildRasterTaskQueue
performance](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-gn%2Candroid-nexus10%2Candroid-nexus5%2Cchromium-rel-mac9%2Cchromium-rel-win8-dual%2Clinux-release&tests=cc_perftests%2Fbuild_raster_task_queue&rev=254610&checked=32_0),
which under some circumstances translate to almost [2x improvement in
ScheduleTasks
performance](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-gn%2Candroid-nexus10%2Candroid-nexus5%2Cchromium-rel-mac9%2Cchromium-rel-win8-dual%2Clinux-release&tests=cc_perftests%2Fschedule_tasks_direct_raster_worker_pool&rev=254610&checked=32_1)."
Thanks David!

### 2014-03-12

Last week Hayato Ito reduced checking if a DOM tree is a descendent of another
from O(N) in the height of the tree of trees to O(1). In smaller trees this
produces[ 2-3x faster event
dispatching](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-nexus5%2Cchromium-rel-mac8%2Cchromium-rel-win7-dual&tests=blink_perf%2FEvents_EventsDispatchingInShadowTrees&rev=256330&checked=core),
but in the deeply nested trees Hayato created he saw more than a 400x
improvement! I'd also like to thank Hayato for the[ fantastic
description](http://src.chromium.org/viewvc/blink?revision=168901&view=revision)
of the patch and its effects in the CL description. Great work!

### 2014-03-05

This week's improvement goes to Chris Harrelson, who [landed a
patch](http://src.chromium.org/viewvc/blink?view=rev&revision=168238) speeding
up CSS descendant selectors by an astounding factor of [20-30x across all
platforms](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-nexus10%2Candroid-nexus4%2Candroid-nexus5%2Candroid-nexus7v2%2Cchromium-rel-mac6%2Cchromium-rel-mac8%2Cchromium-rel-mac9%2Cchromium-rel-win7-dual%2Cchromium-rel-win7-single%2Cchromium-rel-win7-x64-dual%2Cchromium-rel-win8-dual%2Cchromium-rel-xp-dual%2Clinux-release&tests=blink_perf%2FCSS_ClassDescendantSelector&checked=CSS_ClassDescendantSelector&start_rev=254375&end_rev=255079).
Though it can be difficult to trust microbenchmark results, this change is
expected to save 90% or 50ms of style recalc time from expand animations. One
more step in the direction of silky smooth web apps!

### 2014-02-26

This week's improvement belongs to Kentaro Hara, whose[ V8 bindings
patch](http://src.chromium.org/viewvc/blink?view=revision&revision=167879)
resulted in a 20-30% improvement in[
several](https://chromeperf.appspot.com/report?rev=253452&masters=ChromiumPerf&bots=android-gn%2Cchromium-rel-mac8%2Cchromium-rel-win8-dual%2Clinux-release&tests=blink_perf%2FBindings_insert-before&checked=core)[
bindings](https://chromeperf.appspot.com/report?rev=253452&masters=ChromiumPerf&bots=android-gn%2Cchromium-rel-mac8%2Cchromium-rel-win8-dual%2Clinux-release&tests=blink_perf%2FBindings_append-child&checked=core)[
benchmarks](https://chromeperf.appspot.com/report?rev=253452&masters=ChromiumPerf&bots=android-gn%2Cchromium-rel-mac8%2Cchromium-rel-win8-dual%2Clinux-release&tests=blink_perf%2FBindings_node-list-access&checked=core)
across all platforms. Great work!

### 2014-02-19

This week Oystein Eftevaag landed a
[patch](https://codereview.chromium.org/157963002) that allows faster first
paints in the lack of pending stylesheet loads. This produced a [Speed
Index](https://sites.google.com/a/webpagetest.org/docs/using-webpagetest/metrics/speed-index)
improvement of about 5% on Android, and a radically faster Google Search loading
time -- beginning to show the page a full 2.5s sooner! Oystein's work also
unlocked the possibility for several further performance enhancements, so stay
tuned for more progress.

### 2012-02-12

This week it was impossible to choose between three truly epic improvements! So
they’ll have to share the winnings. First, Daniel Sievers
[dropped](https://codereview.chromium.org/148243011) Windows cold message loop
start time from [~4s to
~1s](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-win7-dual&tests=startup.cold.blank_page%2Fmessageloop_start_time&checked=messageloop_start_time%2Cref&start_rev=247437&end_rev=250669),
returning us to pre-Aura levels. David Reveman pwnd some compositing benchmarks
in a [series](https://codereview.chromium.org/154003006)[
of](https://codereview.chromium.org/131543014)[
patches](https://codereview.chromium.org/144463012), improving them [several
fold](https://chromeperf.appspot.com/report?rev=249602&masters=ChromiumPerf&bots=android-nexus5%2Cchromium-rel-mac9%2Cchromium-rel-win7-single&tests=cc_perftests%2Fschedule_alternate_tasks&checked=2_1_0)
across platforms. Last but not least, Camille Lamy shaved [a couple hundred
milliseconds](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-gn%2Candroid-nexus10%2Candroid-nexus4%2Candroid-nexus5%2Candroid-nexus7v2&tests=page_cycler.top_10_mobile%2Fwarm_times&checked=page_load_time)
off some page loads in the top 10 mobile sites suite by
[moving](https://codereview.chromium.org/88503002/) unload event handling off of
the critical path.

### 2012-02-05

This week's improvement has been over a year in the making. Toon Verwaest
succeeded in reducing code duplication and complexity by removing call inline
caches from the V8 codebase. The result was the deletion of over 10,000 lines of
code and several
[unexpected](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac6%2Cchromium-rel-mac8%2Cchromium-rel-mac9%2Cchromium-rel-win7-dual%2Cchromium-rel-win7-single%2Cchromium-rel-win7-x64-dual%2Cchromium-rel-win8-dual%2Cchromium-rel-xp-dual%2Clinux-release&tests=dromaeo.jslibeventprototype%2Fjslib&rev=248514&checked=jslib%2Cref)[
perf](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac8%2Cchromium-rel-mac9&tests=blink_perf%2FCSS_ClassDescendantSelector&rev=248527&checked=CSS_ClassDescendantSelector%2Cref)[
improvements](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac8%2Cchromium-rel-mac9&tests=blink_perf%2FLayout_SimpleTextPathLineLayout&rev=248527&checked=Layout_SimpleTextPathLineLayout%2Cref)
- not to mention the improvement to the developers' lives who would have had to
maintain that code. Thank you Toon for all your hard work!

### 2014-01-22

This week’s improvement is in our ability to measure. We’re now receiving the
first [energy consumption
metrics](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac9&tests=smoothness.top_25%2Fenergy_consumption_mwh&rev=247255&checked=energy_consumption_mwh%2Cref)
on the Mac 10.9 perf bots (with Android soon to follow). This enables us to
begin optimizing while avoiding regressions. Huge thanks to Jeremy Moskovich,
Elliot Friedman, and the Chrome Infra team for getting our first energy
benchmark up and running!

### 2014-01-08

Just before the holidays, the on-duty performance sheriff Victoria Clarke
noticed significant [startup time performance
regressions](https://code.google.com/p/chromium/issues/detail?id=330285); in
some cases, the regression was as much as 350%. Victoria traced it back to an
innocuous-looking [change in the password
manager](http://src.chromium.org/viewvc/chrome?view=revision&revision=241930).
Upon revert our [startup
metrics](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-win8-dual&tests=startup.warm.blank_page%2Fopen_tabs_time&checked=open_tabs_time%2Cref&start_rev=241294&end_rev=243265)
recovered completely. Thanks Victoria!

## 2013

### 2013-12-18

Simon Hatch landed a
[patch](http://src.chromium.org/viewvc/blink?view=revision&revision=163110) that
prioritizes the loading of visible images. This is a huge user win in perceived
loading time, but the initial attempt also unexpectedly introduced
[several](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-win7-single&tests=blink_perf%2FLayout_fixed-grid-lots-of-data&checked=Layout_fixed-grid-lots-of-data&start_rev=238327&end_rev=241268)[
large](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac8&tests=page_cycler.typical_25%2Fcold_times%2Fpage_load_time&checked=page_load_time&start_rev=238426&end_rev=241090)[
performance](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac8&tests=blink_perf%2FLayout_floats_50_100&rev=238938&checked=Layout_floats_50_100)[
regressions](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-win7-gpu-intel&tests=robohornet_pro%2FTotal&rev=238982&checked=Total)
which probably would have went to stable in the olden days. Pat Meenan, the
performance sheriff on duty, [tracked them back to Simon’s
patch](https://code.google.com/p/chromium/issues/detail?id=326165). This enabled
Simon to
[revert](http://src.chromium.org/viewvc/blink?revision=163362&view=revision) the
patch and
[reland](http://src.chromium.org/viewvc/blink?view=revision&revision=163563) it
a few days later with all regressions resolved.

### 2013-12-11

More compositor improvements this week, thanks to Vlad Levin. Vlad [introduced
the concept of
TileBundles](http://src.chromium.org/viewvc/chrome?revision=239074&view=revision)
into the compositor, resulting in a [3-4x performance
gain](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-mac8%2Cchromium-rel-win7-dual%2Clinux-release&tests=cc_perftests%2Fupdate_tile_priorities_stationary&rev=239020&checked=perspective)
in updating tile priorities on desktop.

### 2013-12-04

This week we saw a [70-90%
improvement](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-gn%2Candroid-nexus10%2Candroid-nexus4%2Candroid-nexus5%2Candroid-nexus7v2%2Cchromium-rel-mac7-gpu-intel%2Cchromium-rel-win7-gpu-intel%2Cchromium-rel-win7-gpu-nvidia%2Clinux-release&tests=cc_perftests%2Flayer_tree_host_commit_time&rev=237085&checked=heavy_page_page_scale)
in composited layer tree host commit time across all platforms with impl-side
painting. This is due in part to two changes, [one by Adrienne
Walker](http://src.chromium.org/viewvc/chrome?revision=237136&view=revision) and
[one by Eric
Penner](http://src.chromium.org/viewvc/chrome?view=revision&revision=237179).
The former ensures that on a page with many layers, Chrome only spends time
updating ones that have actually changed; the latter optimizes tiling resolution
when scaling. Thanks to you both!

### 2013-11-13

This week’s improvement comes from outside Google: Jun Jiang from Intel landed
an [order of magnitude speed
increase](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=chromium-rel-win8-dual&tests=blink_perf%2FCanvas_draw-dynamic-webgl-to-hw-accelerated-canvas-2d&rev=234806&checked=Canvas_draw-dynamic-webgl-to-hw-accelerated-canvas-2d)
for drawing dynamic WebGL to hardware-accelerated Canvas 2D.

### 2013-11-07

Elly Jones [landed a
fix](https://src.chromium.org/viewvc/chrome?revision=224030&view=revision) that
reduces startup time by 1.5s for most users on Windows. The win was secured by
decreasing the timeout for
[WPAD](http://en.wikipedia.org/wiki/Web_Proxy_Autodiscovery_Protocol) to a
reasonable value.

### 2013-10-31

This week we wanted to recognize the folks working on Aura and the
Ubercompositor, who not only reversed a number of Windows regressions but pushed
them further into solid performance enhancements. Specifically they were able to
improve framerate (as much as double!) on the [blob
demo](http://webglsamples.googlecode.com/hg/blob/blob.html), [deferred
irradiance volume
demo](http://codeflow.org/webgl/deferred-irradiance-volumes/www/), and [WebGL
aquarium](http://webglsamples.googlecode.com/hg/aquarium/aquarium.html). The
changes also affected real-world applications, similarly improving framerate on
properties such as Google Maps.

### 2013-10-23

This week’s top improvement is from a perf sheriff, Prasad Vuppalapu, who
diagnosed a 50% regression in loading Japanese and Chinese web pages on Windows.
The [revert](https://code.google.com/p/chromium/issues/detail?id=304868)
recovered the performance. Holding on to the speed we have is equally as
important as improving our speed in the first place.

### 2013-10-16

This week’s top improvement is from Elliott Sprehn, who landed a massive 93.5%
performance improvement to CSS/StyleSheetInsert. Great work Elliott!
