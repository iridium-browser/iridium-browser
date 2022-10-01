---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: trace-event-profiling-tool
title: The Trace Event Profiling Tool (about:tracing)
---

When diagnosing performance problems it can be valuable to see what Chrome is
doing "under the hood." One way to get a more detailed view into what's going on
is to use the about:tracing tool.

Tracing records activity in Chrome's processes (see [multi-process
architecture](http://www.chromium.org/developers/design-documents/multi-process-architecture)
for more on what each process is doing). It records C++ or javascript method
signatures in a hierarchical view for each thread in each process. This is a lot
of information, but sifting through it can help identify performance
bottlenecks, slow operations, and events with irregular lengths (leading to e.g.
framerate variation).

[<img alt="image"
src="/developers/how-tos/trace-event-profiling-tool/abouttracing2.png"
height=269
width=400>](/developers/how-tos/trace-event-profiling-tool/abouttracing2.png)

## Getting Started Using about:tracing

1.  [Recording Tracing
            Runs](/developers/how-tos/trace-event-profiling-tool/recording-tracing-runs);
            start with this, it's prerequisite for using about:tracing.
2.  [How to use the Frame Viewer to Bust
            Jank](/developers/how-tos/trace-event-profiling-tool/using-frameviewer);
            read this next, to understand how to diagnose rendering performance
            problems.
3.  [Jank Case Study 1](/developers/rendering-performance-case-study-1);
            then read this, for further examples of how to effectively use
            about:tracing in conjunction with the Dev Tools timeline

Note that to understand what's happening in trace events you'll need a basic
understanding of how the browser works. The above articles provide enough to get
started, but it's recommended to first read at minimum:

*   [The Rendering Critical
            Path](/developers/the-rendering-critical-path) for a little more
            background, and...
*   [Anatomy of
            Jank](/developers/how-tos/trace-event-profiling-tool/anatomy-of-jank)
            for precise explanations of various rendering performance problems

**Further reading:**

*   [A presentation from
            pdr@](https://docs.google.com/a/google.com/presentation/d/1pw9kbUFMD7s9KME8yIsCpCNKaSwjkGa89tt4M5rxIGM/edit)
            on how to debug the graphics stack with tracing
*   [Frame Viewer
            Basics](/developers/how-tos/trace-event-profiling-tool/frame-viewer),
            a short guide for how to navigate the frame viewer view. This is
            more succinct but less informative than [frame viewer
            how-to](/developers/how-tos/trace-event-profiling-tool/using-frameviewer)
            above.
*   [Saving Skia
            Pictures](/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium);
            this is useful if you want to capture isolated SkPictures for the
            Skia team.
*   [Tracking memory
            allocations](https://chromium.googlesource.com/chromium/src/+/HEAD/components/tracing/docs/memory_infra.md)
            with memory-infra tracing

**Even further reading:**

*   [How to Understand about:tracing
            results](/developers/how-tos/trace-event-profiling-tool/trace-event-reading)
            (somewhat out of date; refer to the [Frame Viewer
            how-to](/developers/how-tos/trace-event-profiling-tool/using-frameviewer)
            instead)

## Contributing to about:tracing

Start by perusing the [Tracing Ecosystem
Explainer](https://docs.google.com/a/chromium.org/document/d/1QADiFe0ss7Ydq-LUNOPpIf6z4KXGuWs_ygxiJxoMZKo/edit#)
to understand the various different pieces of code involved.

*   To instrument Chrome and add your own custom traces, see
            [Instrumenting Chromium or Javascript code to get more
            detail](/developers/how-tos/trace-event-profiling-tool/tracing-event-instrumentation).
*   To add functionality to the about:tracing viewer itself, see
            [contributing to
            trace-viewer](https://github.com/google/trace-viewer/wiki/Contributing).
    *   [trace-viewer](https://github.com/google/trace-viewer) lives in
                its own repository on GitHub, not in the Chromium tree.

Please file bugs as you find them! If you find any bugs, please [let us
know](https://github.com/google/trace-viewer/issues/new). You review the [known
bugs](https://github.com/google/trace-viewer/issues) as well.
