---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: frame-rate-test
title: Frame Rate Test
---

An effort to capture frame rate performance on various gestures and content.

[TOC]

## Relevant files

The frame rate test is broken up into two three logical pieces:

1.  Test controller:
    `src/chrome/test/perf/frame_rate/frame_rate_tests.cc`
2.  JS test harness:
    `src/chrome/test/data/perf/frame_rate/head.js`
3.  Test content:
    `src/chrome/test/data/perf/frame_rate/content/ `(controlled via DEPS from
    svn/trunk/deps/frame_rate_content)

## To checkout the content, add theses lines to your .gclient

> Deps
> {
> "name" : "frame_rate_content",
> "url" : "svn://chrome-svn/chrome/trunk/deps/frame_rate/content",
> },

## How to Run the Tests

1.  Build the `performance_ui_tests` binary.
2.  Tests come in two varieties: GPU-only or GPU-neutral.
    *   Run tests that are GPU neutral: `performance_ui_tests
                --gtest_filter=FrameRateTest*`.
    *   Run tests that are GPU accelerated: performance_ui_tests
                --gtest_filter=FrameRateTest\* --enable-gpu

## How It Works

> frame_rate_tests.cc:

> *   Contains the list of tests to run: look for FRAME_RATE_TEST
              macros.
> *   Foreach test:
>     *   Starts up a standalone chrome, loads the page-under-test
>     *   Waits for head.js to indicate that the test has run
>     *   Reads performance results from the page, shoves them out in a
                  way that Chrome dashboard code can understand

> head.js

> *   Registers (and constantly re-registers) a requestAnimationFrame
              callback and derives frame rate from that callback frequency.
> *   If requested, will scroll the page to trigger page updating.
              Useful for web content, less useful for WebGL or Canvas apps.

## Adding New Test Content

Scrolling test content, e.g. content that needs to be scrolled part of the test:

*   Add the content to a new directory &lt;dirname&gt; in
            src/chrome/test/data/perf/frame_rate/content/. Put the top-level
            html inside that directory as test.html
*   Add the following to test.html:
    `<script src="../../head.js"></script>`
*   **Note** that this is an svn managed directory, so once you have
            committed that change,
    bump src/DEPS "frame_rate/content" rev to committed revision.
*   Add a FRAME_RATE_TEST macro to frame_rate_tests.cc for the new
            directory, e.g FRAME_RATE_TEST(&lt;dirname&gt;)

Self-rendering content, e.g. WebGL apps, or content that needs no external
stimulation in order to render:

*   Same steps as scrolling tests, but your customization to the html
            should read:
    `<script src="../../head.js"></script>`
    `<script>`
    `__gestures = {`
    ` stationary: [`
    ` {"time_ms":1, "y":0},`
    ` {"time_ms":5000, "y":0}`
    ` ]`
    `};`
    `</script>`

To debug the test outside of the test harness, open test.html and type
`__start('stationary')` in the javascript console. Once the test runs, type
__calc_results() to see the measured frame rate.

Content additions require codereview, using the standard [contributing
code](/developers/contributing-code) process. Consider nduca@chromium.org and
junov@chromium.org as reviewers.

## Results

### Frame rate tests run on the [GPU waterfall](http://chromegw.corp.google.com/i/chromium.gpu/waterfall).

They also run on the [Perf
waterfall](http://build.chromium.org/p/chromium.perf/waterfall), but they are
not hardware-accelerated there.

    1.  [XP Interactive
                Perf](http://build.chromium.org/p/chromium.perf/builders/XP%20Interactive%20Perf)
    2.  [Mac10.5
                Perf(3)](http://build.chromium.org/p/chromium.perf/builders/Mac10.5%20Perf%283%29)
    3.  [Mac10.6
                Perf(3)](http://build.chromium.org/p/chromium.perf/builders/Mac10.6%20Perf%283%29)
    4.  [Old Mac10.6
                Perf(3)](http://build.chromium.org/p/chromium.perf/builders/Old%20Mac10.6%20Perf%283%29)
    5.  [Linux Perf
                (2)](http://build.chromium.org/p/chromium.perf/builders/Linux%20Perf%20%282%29)
    6.  [Linux Perf
                (lowmem)](http://build.chromium.org/p/chromium.perf/builders/Linux%20Perf%20%28lowmem%29)
    > **([View all six
    > here](http://build.chromium.org/p/chromium.perf/waterfall?branch=&builder=XP+Interactive+Perf&builder=Mac10.5+Perf%283%29&builder=Mac10.6+Perf%283%29&builder=Old+Mac10.6+Perf%283%29&builder=Linux+Perf+%282%29&builder=Linux+Perf+%28lowmem%29&reload=none))**

### You can track the software rendered performance graphically using the [performance dashboard](http://build.chromium.org/f/chromium/perf/dashboard/overview.html). You may also click **\[**results**\]** from the [perf waterfall](http://build.chromium.org/p/chromium.perf/waterfall) to view the graph.

*   Every tab displays results for different content.
*   The trace values, fps and fps_ref, represent the average of all
            gesture results.
*   Click on the graph to see revision information.
    *   **Click "Data" to see results for individual gestures.**

## Gestures

Gestures are simply keyframe animations that are applied to the document scroll.
The animations and their keyframes are stored in the `__gestures` object in
[head.js](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/test/data/perf/frame_rate/head.js?view=markup):

```none
var __gestures = {
  steady: [
    {"time_ms":1, "y":0},
    {"time_ms":5, "y":10}
  ],
  reading: [
    {"time_ms":1, "y":0},
    {"time_ms":842, "y":40},
    ...
    {"time_ms":1170, "y":2373}
  ],
  mouse_wheel: [
    {"time_ms":1, "y":0},
    {"time_ms":164, "y":53},
    ...
    {"time_ms":421, "y":693}
  ],
  ...
};
```

You can either add keyframed animations to this list, or ask head.js to record a
new gesture for you:

1.  Open a webpage to record from (must include
            [head.js](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/test/data/perf/frame_rate/head.js?view=markup)).
2.  Type `__start_recording()` in the javascript console.
3.  Perform any gestures you wish to record.
4.  Type `__stop()` in the javascript console.
5.  Copy the output from `JSON.stringify(__recording)` in the console.
6.  Paste the output in
            [head.js](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/test/data/perf/frame_rate/head.js?view=markup)
            as a new member of `__gestures`.
7.  Copy the formatting from other gestures.
