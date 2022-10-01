---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/chromium-graphics
  - Chromium Graphics // Chrome GPU
page_name: how-to-get-gpu-rasterization
title: How to get GPU Rasterization
---

Chrome 37 introduced a GPU rasterizer. When enabled, some paint workloads can go
from 100ms/frame to 4-5ms/frame.

It doesn't run for all content, but is enabled as a combination of viewport
settings and a device allow-list.

### How to enable GPU rasterization:

*   **Chrome on Android version &gt;= 54** ([chromium rev
            406004](https://bugs.chromium.org/p/chromium/issues/detail?id=591179#c16))
    *   Define a viewport. (Either as a `<meta>` tag, or in CSS as
                @viewport).
    *   Any viewport definition will work.
*   **Chrome on Android version &lt;= 53:**
    *   Use a meta viewport tag containing `width=device-width` and
                `minimum-scale=1.0. For example:`

```none
<meta name="viewport" content="width=device-width, minimum-scale=1.0">
```

    *   It will *not* trigger if minimum-scale is set to "yes". "1.0" is
                explicitly required.
    *   initial-scale and user-scalable may be set or not. They are not
                considered.
    *   Supported devices:
        *   All devices with Android &gt; 4.4
        *   All devices with Android = 4.4 that support OpenGL ES &gt;=
                    3.0
        *   Nexus devices (except Nexus 10)

*   **WebView:**
    *   Enabled for all documents with the above viewport conditions
                specified in the meta tag.
    *   Also enabled on all documents that *do not* specify any viewport
                in the meta tag.

*   **Chrome on Desktop**
    *   In development (as of August 2015)
    *   We will eventually enable GPU rasterization on all platforms and
                devices where we use the GPU for compositing. You can track
                progress towards this goal [here](http://crbug.com/419521).

To use the experimental hardware rasterizer on all pages, regardless of device
and content: --force-gpu-rasterization or
`chrome://flags/#enable-gpu-rasterization`

## GPU Rasterization Veto:

Note that GPU rasterization can get vetoed based on the content itself. For
example, if page contains many SVGs with non-convex paths (common for SVG
icons), GPU rasterization may get disabled for that page load.

### To see if you have GPU rasterization on:

1.  Check in about:gpu for "GPU Rasterization"
2.  Use the FPS meter to show the GPU status.
    *   To open FPS meter, Open DevTools, hit Esc to open the console
                drawer, select Rendering, and check the FPS Meter
    *   on, on (forced)
    *   off (device) - not supported on the device
    *   off (content) - supported on the device but content is veto'd
                for gpu rasterization
        *   To see the content reason, record a trace using
                    chrome://tracing (using cc) and search for the instant event
                    'GPU Rasterization Veto'. The veto reason will be listed
                    within the Args.
    *   off (viewport) - viewport trigger not available
3.  Take a [frame viewer recording using about
            tracing](http://www.chromium.org/developers/how-tos/trace-event-profiling-tool/frame-viewer).
            Click a frame. It will tell you if GPU raster is on.

## GPU Rasterization Before/After

[<img alt="image"
src="/developers/design-documents/chromium-graphics/how-to-get-gpu-rasterization/gpuraster.png"
height=180 width=320>](http://gfycat.com/MediumSevereHermitcrab)

In <http://gfycat.com/MediumSevereHermitcrab> you can view the before and after
for GPU rasterization.

On the left the content checkerboards because it isn't rasterized fast enough.
On the right, we raster on the GPU and it keeps up with the fling very well.
