---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: gpu-overdraw-debugging-tool
title: GPU Overdraw Debugging Tool
---

Debugging GPU Overdraw in Chrome

# Intro

When diagnosing performance problems it can be valuable to visualize GPU
overdraw. This can show you where Chrome might be doing more rendering work than
necessary and help you see where you might be able to reduce rendering overhead.

The primary audience for this tool is Chrome developers but it can also be used
to diagnose performance problems with web pages.

# Getting Started

The GPU overdraw feedback tool built into Chrome has been inspired by the [GPU
overdraw debug
feature](https://developer.android.com/studio/profile/dev-options-overdraw.html)
that exists on Android and developers familiar with that feature will feel at
home using this tool to improve the Chrome UI or web pages.

To visualizing GPU overdraw in Chrome navigate to about:flags and enable the
Show overdraw feedback experiment. If you are on Chrome OS then it is also
recommended to disable the Partial swap experiment as that produces output that
is easier to analyze.

When enabled, this tool visualize overdraw by color-coding interface elements
based on how many elements are drawn underneath. The element colors are hinting
at the amount of overdraw on the screen for each pixel, as follows:

True color: No overdraw

Blue: Overdrawn once

Green: Overdrawn twice

Pink: Overdrawn three times

Red: Overdrawn four or more times

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/sKYz4g05kUmYRmx8p5qR50w/image?w=599&h=386&rev=1&ac=1"
height=386 width=599>

Example of GPU overdraw feedback output.

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/s0J4KvzwLwH8-dNJuiC8UOQ/image?w=599&h=435&rev=22&ac=1"
height=435 width=599>

Some overdraw is unavoidable. As you are tuning your UI elements or web page,
the goal is to arrive at a visualization that shows mostly true colors and 1X
overdraw in blue. The calculator UI shown above is an example of undesirable
amount of GPU overdraw.

Examples of undesirable and desirable Debug GPU Overdraw output.

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/shfmR54KvCMtndr2_oPSxDg/image?w=624&h=473&rev=102&ac=1"
height=473 width=624>

# Fixing Overdraw

There are several strategies you can pursue to reduce or eliminate overdraw. If
you are working on the Chrome OS UI then it usually comes down to using fewer
Aura windows with opacity set to TRANSLUCENT. For Chrome apps and web-pages in
general, the following strategies will likely apply:

    Removing unnecessary use of “position:fixed;”.

    Avoid use of 3D transformations and translateZ=0.

    Use { alpha: false } for &lt;canvas&gt; elements unless alpha is needed.

# Tracing

If you are analyzing GPU overdraw for animations or creating automated
performance tests then overdraw feedback in the form of trace events can be
useful. See [The Trace Event Profiling Tool
(about:tracing)](http://www.chromium.org/developers/how-tos/trace-event-profiling-tool)
for more details about how to record tracing runs. Enable the viz.overdraw
tracing category to have Chrome record the amount of overdraw for each frame.
The result is presented as a GPU Overdraw counter that changes over time as
overdraw increase or decrease.

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/sLB42g9KPAjoPQGg8bqA4Sg/image?w=599&h=435&rev=21&ac=1"
height=435 width=599>

The value of the counter is the percentage of overdraw in the last frame
presented. No overdraw for one half the screen and 1X overdraw for the other
half results in a Gpu Overdraw counter value of 50. This metric is only
available when ARB_occlusion_query is available.

*Under construction...*
