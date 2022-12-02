---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/trace-event-profiling-tool
  - The Trace Event Profiling Tool (about:tracing)
page_name: saving-skp-s-from-chromium
title: How to save .skp files from Chromium
---

Skia pictures or ".skp's" are a binary format for the draw commands Chromium
sends to Skia for rasterization. You can view these in the [Skia
debugger](https://skia.org/dev/tools/debugger). To get the SkPicture you have
two options:

**Option 1: save the whole page**

1.  Grab Chrome Canary or a tip-of-tree Chromium Release build and
            launch it with the following flags (for help, see \[1\]):
    --enable-gpu-benchmarking --no-sandbox
2.  Load your favorite page
3.  In the DevTools console, type
            chrome.gpuBenchmarking.printToSkPicture('/path/to/skp_dir/')
4.  In the Skia checkout , run ./out/Debug/debugger
            /path/to/skp_dir/layer_0.skp

**Option 2: save an individual picture**

You can view and export these files from Chrome's
[about:tracing](/developers/how-tos/trace-event-profiling-tool) tool

1.  Grab Chrome Canary or a tip-of-tree Chromium Release build and
            launch it with the following flag (for help, see \[1\]):
    --enable-skia-benchmarking
2.  Navigate to "about:tracing".

    [<img alt="image"
    src="/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/smallabouttracing.png"
    height=162
    width=400>](/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/smallabouttracing.png)

3.  Open a new tab and navigate to your favorite knitting website.

    [<img alt="image"
    src="/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/skpsite.png"
    height=341
    width=400>](/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/skpsite.png)

4.  Go back to the about:tracing tab and press record. Choose the "Frame
            viewer", or check “cc” and “cc.debug.display_items”. Uncheck
            everything else for this demo, then press record.

    [<img alt="image"
    src="/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/checkboxes.png"
    height=341
    width=400>](/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/checkboxes.png)

5.  Go back to your knitting webpage and refresh the page. You can
            scroll around, select text, etc (each generated skpicture will be
            saveable).
6.  Go back to “about:tracing” and press “stop tracing”.
7.  Now for some fun! Use the W, A, S, and D keys like you’re playing a
            video game. Your goal in this game is to find the
            “cc::DisplayItemList” dot that you’d like to save. When you’ve found
            the one, click “Save SkPicture”. Note: you can use the left and
            right arrow keys to cycle through cc::DisplayItemLists. You may
            notice there are two copies of most pictures--this is for low
            resolution (first) and high resolution (second).

    [<img alt="image"
    src="/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/picture.png"
    height=341
    width=400>](/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/picture.png)

8.  You can now open this picture in the [Skia
            debugger](https://sites.google.com/site/skiadocs/developer-documentation/skia-debugger).

    [<img alt="image"
    src="/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/skdebugger2.png"
    height=334
    width=400>](/developers/how-tos/trace-event-profiling-tool/saving-skp-s-from-chromium/skdebugger2.png)

**\[1\] How to launch Chrome with flags on OSX**

> Chrome Canary can be launched with:

> > ./Applications/Google\\ Chrome\\ Canary.app/Contents/MacOS/Google\\ Chrome\\
> > Canary --enable-threaded-compositing --force-compositing-mode
> > --enable-impl-side-painting --enable-skia-benchmarking
> > --allow-webui-compositing

> If you build yourself, make sure to use a Release build and launch with:

> > ./out/Release/Chromium.app/Contents/MacOS/Chromium
> > --enable-threaded-compositing --force-compositing-mode
> > --enable-impl-side-painting --enable-skia-benchmarking
> > --allow-webui-compositing
