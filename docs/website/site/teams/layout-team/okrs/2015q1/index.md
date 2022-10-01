---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/okrs
  - Objectives and Key Results
page_name: 2015q1
title: 2015 Q1
---

List of objectives and key results for the first quarter of 2015.

## Overview

*   Key Result Owner(s) Score
*   [Have telemetry benchmark suite running key_silk_cases monitoring
            layout times with less than 1% variation between runs.](#kr0)
            \[benjhayden\] 0.6
*   [Add 3 real-world pages with lots of layout to key_silk_cases. At
            least two of which are mobile oriented.](#kr1) \[benjhayden\] 0.7
*   [Speed up some key_silk_cases tests.](#kr2) \[*everyone*\] 0.0
*   [Create a Measure API prototype and write-up of lessons
            learned.](#kr3) \[jchaffraix, eae\] 1.0
*   [Support natural layout animations (subpixel layout during
            animation).](#kr4) \[eae\] 0.0
*   [Finish root layer scrolling.](#kr5) \[skobes\] 0.5
*   [Move line layout to LayoutUnit.](#kr6) \[hartmanng, szager\] 0.7
*   [Triage all clusterfuzz asserts and fix 50% of them.](#kr7)
            \[cbiesinger\] 0.2
*   [Have bugs automatically filed for new clusterfuzz asserts.](#kr8)
            \[cbiesinger\] 0.0
*   [Render tree modifications during layout.](#kr9) \[dsinclair\] 0.7
*   [Speed up complex text by a factor of 2.](#kr10) \[kojii, eae\] 1.0

## Details

### Have telemetry benchmark suite running key_silk_cases monitoring layout times with less than 1% variation between runs

Owner(s):benjhayden
Score:0.6

The variation between runs is about 3% overall for tests running with the new
layout_avg telemetry metric. Down from about 10% for existing webkit performance
tests.

The variation between runs is about 3% overall, though avg and variation vary
widely between pages.

It looks like there's a couple hundred ms variation in the average of all pages'
layout_avg, though I'm not sure what the take-home number should be. The average
over all the pages' layout_avg is about 5.4ms after I added Masonry, and the
stddev in that line looks to be about 200ms, which is about 3-4% variation.
Masonry's layout_avg is usually around 70ms, and the stddev in its layout_avg
between runs is about 1-2ms, which is about 3% variation. Several pages have a
layout_avg that is consistently below 1ms, so normal variation can be 100% or
more.

One way to reduce variation would be to rewrite the pages to perform
significantly more layout, i.e. boost the signal above the noise.

Perhaps this OKR should have been about making sure that there aren't any
easily-fixable sources of noise in any page's layout_avg, e.g. non-deterministic
load order. I fixed one issue of that nature. The noise in most of the rest of
the pages looks like expected white noise, with a few pages exhibiting rare
spikes that may warrant further investigation, though nothing as troublesome as
that non-deterministic load order.

I'm rewriting the LayoutMetric, so these numbers will change and expose more
detailed information.

### Add 3 real-world pages with lots of layout to key_silk_cases. At least two of which are mobile oriented

Owner(s):benjhayden
Score:0.7

Added two new tests, Polymer Topeka and Masonry.

### Speed up some key_silk_cases tests

Owner(s):everyone
Score:0.0

We've speed up quite a few of the blink layout perf tests but seemingly none of
the silk cases.

It turns out that all the key_silk_cases tests were already pretty fast when it
comes to layout.

*   [chapter-reflow](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-nexus7v2%2Cchromium-rel-mac9%2Cchromium-rel-win7-gpu-nvidia%2Clinux-release&tests=blink_perf.layout%2Fchapter-reflow&checked=chapter-reflow%2Cchapter-reflow%2Cref%2Cchapter-reflow%2Cref%2Cchapter-reflow%2Cref)
            tests speed up by 5-12% depending on platform.
*   [line-layout](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-nexus7v2%2Cchromium-rel-mac9%2Cchromium-rel-win7-gpu-nvidia%2Clinux-release&tests=blink_perf.layout%2Fline-layout&checked=line-layout%2Cline-layout%2Cref%2Cline-layout%2Cref%2Cline-layout%2Cref&start_rev=318673&end_rev=322609)
            tests 4-15% depending on platform.

### Create a Measure API prototype and write-up of lessons learned

Owner(s):jchaffraix, eae
Score:1.0

Split Measure into Element, Text and Custom Layout measure.

*   jchaffraix has several prototypes and
            [write-up](https://docs.google.com/a/chromium.org/document/d/1hGuLzcY8uFcP4fQw2HiwnpnI6SmygqErvYQMxkObONs/edit)
            for Element measure, with key conclusions about the API and what
            people expect.
*   eae has a proposal and some write-up for text.
*   jchaffraix was involved in Custom Layout but ikilpatrick is the main
            driver.

### Support natural layout animations (subpixel layout during animation)

Owner(s):eae
Score:0.0

No real progress, was deprioritized in favor of performance and code health
work. Questionable whether it makes sense without some sort of layout/style
containment as we cannot give any performance guarantees without it.

### Finish root layer scrolling

Owner(s):skobes
Score:0.5

Main-thread, impl-thread, and script-driven frame-level scrolls now work through
LayerScrollableArea codepath with --root-layer-scrolls. Iframes, position:
fixed, and scroll animations work. Need to expand test coverage and look at
potential interactions with other features (pinch viewport, fractional scroll
offsets, top controls, overhang, slimming paint, scroll customization etc).

### Move line layout to LayoutUnit

Owner(s):hartmanng, szager
Score:0.7

Great progress but not quite there yet. Need to solve the performance issue.
Still need to go through all tests on Windows to determine if any of the changes
are due to regressions or if we just need to rebaseline the tests. Mac and Linux
both close.

### Triage all clusterfuzz asserts and fix 50% of them

Owner(s):cbiesinger
Score:0.2

Clusterfuzz now does produce assertion errors. \[restricted link\] If that link
does not show results, click "Toggle experimental testcases".

Triaged most asserts, but so far only on paper. Not fixed because focused more
on Flexbox. Need to continue in Q2 with bug filing and fixing.

### Have bugs automatically filed for new clusterfuzz asserts

Owner(s):cbiesinger
Score:0.0

Not done because not done triaging. However, when it's time to do this it's just
a matter of asking the clusterfuzz team to do it. All code and logic is already
in place.

### Render tree modifications during layout

Owner(s):dsinclair
Score:0.7

Landed first-letter but the list markers change is stuck on finishing the patch,
especially editing. Dan is pulled in other directions (LayoutObject renaming,
unflaking win bot etc)

### Speed up complex text by a factor of 2

Owner(s):kojii, eae
Score:1.0

Speed up by over 2x at a minimum and up to 40x in some cases.

*   [ArabicLineLayout](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-nexus7v2%2Cchromium-rel-mac9%2Cchromium-rel-win7-gpu-nvidia%2Clinux-release&tests=blink_perf.layout%2FArabicLineLayout&checked=ArabicLineLayout%2CArabicLineLayout%2Cref%2CArabicLineLayout%2Cref&start_rev=318673&end_rev=322609)
            2-2.5x depending on platform.
*   [latin-complex-text](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-nexus7v2%2Cchromium-rel-mac9%2Cchromium-rel-win7-gpu-nvidia%2Clinux-release&tests=blink_perf.layout%2Flatin-complex-text&checked=latin-complex-text%2Cref%2Clatin-complex-text%2Clatin-complex-text%2Cref%2Clatin-complex-text%2Cref&start_rev=318673&end_rev=322609)
            8-40x depending on platform.
*   [vertical-japanese-kokoro-insert](https://chromeperf.appspot.com/report?masters=ChromiumPerf&bots=android-nexus7v2%2Cchromium-rel-mac9%2Cchromium-rel-win7-gpu-nvidia%2Clinux-release&tests=blink_perf.layout%2Fvertical-japanese-kokoro-insert&checked=vertical-japanese-kokoro-insert%2Cvertical-japanese-kokoro-insert%2Cref%2Cvertical-japanese-kokoro-insert%2Cref%2Cvertical-japanese-kokoro-insert%2Cref&start_rev=318673&end_rev=322609)
            2-5x depending on platform.
