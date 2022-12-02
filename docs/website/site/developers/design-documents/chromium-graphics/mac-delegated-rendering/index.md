---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/chromium-graphics
  - Chromium Graphics // Chrome GPU
page_name: mac-delegated-rendering
title: Mac Browser Compositor aka Mac Ubercompositor aka Mac Delegated Rendering
---

On Mac, the terms “browser compositor”, “Ubercompositor”, and “delegated
rendering” refer to the same thing. I will try to use “delegated rendering” to
refer to this change in this document. This document describes the
implementation of delegated rendering on the Mac.

Throughout this document, I will try to color-code the data structures and
functions by the process that they are in. Things in the renderer process are
red, things in the browser process are blue, and things in the GPU process are
green.

Table of Contents:

[TOC]

# Delegated Rendering in Aura (Windows, Linux, Chrome OS)

In Aura, the entire browser window is a single OpenGL surface. Everything that
is drawn in that window (including the tab strip, the address bar, the
min/max/close buttons) is drawn using OpenGL into that single OpenGL surface.
<img alt="image"
src="/developers/design-documents/chromium-graphics/mac-delegated-rendering/aura%20ubercomp.png"
height=614px; width=624px;>

Inside the browser process, inside the aura::WindowTreeHost, there exists a
ui::Compositor (which is a wrapper around a cc::LayerTreeHost). This
ui::Compositor generates, via its cc::LayerTreeHost, the actual OpenGL commands
to be executed in the GPU process, to produce the pixels that appear in that
OpenGL surface, for the whole window.

Inside the renderer process, there exists another cc::LayerTreeHost which
decides what is to be drawn for the web contents area. Instead of outputting
pixels directly (know as “direct rendering”), this cc::LayerTreeHost outputs
instructions for how to draw those pixels (in the form of a list of textured
quads), which it sends the browser process, which adds those quads to the things
it will draw inside its ui::Compositor.

In this sense, the renderer has delegated producing actual pixels to the browser
process, hence “delegated rendering”, as opposed to “direct rendering”. These
concepts of “direct” versus “delegating” are made concrete in the cc::Renderer
implementations -- there exists a cc::DelegatingRenderer for delegated
rendering, and there exists a cc::DirectRenderer for direct rendering (with
cc::GLRenderer for OpenGL-accelerated rendering and a cc::SoftwareRender for
software rendering).

Of note is that the browser process’ compositor is a direct renderer, while the
renderer process’ compositor is a delegating renderer.

Note on power+performance:

In the Aura case, in the initial implementation (I think, this may be a lie),
the renderer process used a direct renderer (rendering to a texture), and then
we would draw the resulting image in the browser process’ direct renderer.

This is bad for performance and power in that it uses up to 2x-3x the memory
bandwidth to draw a single frame -- \[write pixel in renderer\] then \[read
pixels in browser\] then \[write pixels in browser\] versus just \[write pixels
in browser\]. I say 2x-3x, because both of those pipelines often involve a
\[read pixels from tile textures\] stage, which makes it more 2x than 3x (other
work also makes the improvement less dramatic).

# Delegated Rendering on Mac

On Mac, only the web contents part of the browser window is an OpenGL surface
drawn by Chrome. The rest of the window is drawn using Cocoa, the native Mac UI
API.

<img alt="image"
src="/developers/design-documents/chromium-graphics/mac-delegated-rendering/mac%20ubercomp.png"
height=614px; width=624px;>

This is because we don’t (yet) have a way to draw a native-feeling Mac UI using
Aura. There is a project underway to do this (starting with non-browser-window
UI such as task manager and app launcher).

# Where the Actual ui::Compositor Lives on Mac

Recall that in Aura, the aura::WindowTreeHost had the ui::Compositor which would
draw the web contents (among other things). On Mac, we don’t have any such
analogous place to put the ui::Compositor.

Creating and destroying a ui::Compositor is very expensive (you have to set up a
GPU command buffer, among other beasts), and keeping one around isn’t cheap
either.

One option would be that we could just hang the ui::Compositor off of the
RenderWidgetHostViewCocoa (the NSView that displays web contents), but this
would be one-or-more-ui::Compositors-per-tab, which would make creating and
destroying tabs slow, and make tabs bloated.

Instead there is a BrowserCompositorCALayerTree class, which owns the
ui::Compositor and a sub-tree of CALayers which draw the contents of the
ui::Compositor. This class can be recycled across different NSViews as needed.
There is at most one spare instance of BrowserCompositorCALayerTree kept around
for recycling.

When a RenderWidgetHostViewCocoa is made visible, it creates a
BrowserCompositorViewMac, which finds or creates a spare
BrowserCompositorCALayerTree, and binds to that. The binding involves adding the
CALayers of the BrowserCompositorCALayerTree to the CALayer tree backing the
NSView. When the RenderWidgetHostViewCocoa is made invisible, it frees its
BrowserCompositorViewMac, which allows the bound BrowserCompositorCALayerTree to
either hang out and try to be recycled, or delete itself.

There also exists a BrowserCompositorViewMacPlaceholder class, which acts as a
hint that a BrowserCompositorCALayerTree may be needed soon, so keep one around
to recycle.

# Sending Delegated Frames from the Renderer To the Browser

This is the sequence of steps by which a frame from the renderer is send to the
browser compositor, and how the frame is acknowledged.

    When the renderer has a new frame that it wants drawn (maybe the cat is on
    top of the Roomba in this one), it sends a ViewHostMsg_SwapCompositorFrame
    from the content::CompositorOutputSurface in the renderer process to the
    browser process’s RenderWidgetHostImpl.

    The browser process’s RenderWidgetHostImpl then takes the frame information
    out of this message and passes to to its RenderWidgetHostView.

    The RenderWidgetHostViewMac and RenderWidgetHostViewAura have a
    DelegatedFrameHost, which acts as an interface between the
    RenderWidgetHostView’s ui::Compositor and the renderer. The frame
    information is passed to this DelegatedFrameHost.

        The RenderWidgetHostViewAura’s ui::Compositor is the ui::Compositor of
        the RenderWidgetHostViewAura’s aura::Window’s aura::WindowTreeHost (if
        it exists).

        Of note is that the interface between the RenderWidgetHostView and its
        ui::Compositor isn’t as explicit as it could be. The ui::Compositor can
        change at any time, and the DelegatedFrameHost only finds out about this
        by when it asks for the ui::Compositor from the RenderWidgetHostView and
        gets a different value than it got before. There are lots of bugs
        lingering here -- this should be set explicitly (see crbug.com/403011).

        Also of note is that RenderWidgetHostViewAndroid has a verbatim copy of
        a lot of the code in DelegatedFrameHost. We should consider making
        RenderWidgetHostViewAndroid use DelegatedFrameHost.

    The DelegatedFrameHost tells the ui::Compositor to draw a new frame with the
    updated contents.

    When the ui::Compositor does a commit (a step in the process of drawing a
    new frame), DelegatedFrameHost::SendDelegatedFrameAck is called, which sends
    a ViewMsg_SwapCompositorFrameAck to the renderer process'
    CompositorOutputSurface::OnSwapAck, telling the renderer that it can produce
    another frame now (perhaps the cat is wearing a shark costume in this one).

Note that the way that the browser can tell the renderer “hey, you’re producing
frames too fast for me to draw them” is by delaying when it does a commit, in
the last step. This means that if the browser’s ui::Compositor stalls, then the
renderer’s compositor will stop producing delegated frames.

# Drawing Delegated (or any other kind of) Frames in the Browser Using IOSurfaces on Mac

After the ui::Compositor does a commit in the above sequence of events, the
compositor issues a bunch of OpenGL commands to draw things, followed by a
glSwapBuffers. This describes that path.

This only describes the IOSurface and CoreAnimation-based approach.

1.  While drawing a frame, the ui::Compositor is rendering into an
            IOSurface-backed texture
    *   This is via an FBO, allocated and set up by
                ImageTransportSurfaceFBO using IOSurfaceStorageProvider
2.  When done drawing the frame, the ui::Compositor’s
            cc::LayerTreeHostImpl’s cc::OutputSurface will issue a glSwapBuffers
            to the GPU process.
    *   The ui::Compositor’s cc::LayerTreeHost will now be blocked until
                its cc::OutputSurface has its OnSwapBuffersComplete method
                called.
3.  The command buffer will decode this and end up calling
            ImageTransportSurfaceFBO::SwapBuffers
    *   This will send the IOSurface's ID to the browser process in a
                GpuHostMsg_AcceleratedSurfaceBuffersSwapped message.
4.  Processing of the SwapBuffers in the command stream in the GPU
            process will result in
            BrowserCompositorOutputSurface::OnSwapBuffersComplete being called
            in the browser process (this is done by the GpuControl::Echo
            mechanism).
    *   On non-Mac platforms, this results in the cc::OutputSurface
                being un-blocked, un-blocking the ui::Compositor. This is
                undesirable on Mac, because this is insufficiently sensitive to
                GPU back-pressure.
    *   To avoid this on Mac, the Mac will not call the super-class'
                cc::OutputSurface::OnSwapBuffersComplete inside
                BrowserCompositorOutputSurface::OnSwapBuffersComplete.
5.  The GpuHostMsg_AcceleratedSurfaceBuffersSwapped IPC will be received
            by GpuProcessHostUIShim in the browser process
    *   Of note is that this goes through the RenderWidgetResizeHelper,
                so it will be able to be received even during a live resize.
6.  The GpuProcessHostUIShim will pass this to the static method
            BrowserCompositorViewMac::GotAcceleratedFrame. This will look up the
            BrowserCompositorCALayerTree corresponding to the output surface ID.
    *   This call to
                BrowserCompositorCALayerTree::GotAcceleratedIOSurfaceFrame will
                create (if not created already) an IOSurfaceLayer (a sub-class
                of CAOpenGLLayer) and tell it that it needs to draw via
                -\[IOSurfaceLayer setNeedsDisplay:YES\] or -\[IOSurfaceLayer
                setAsynchronous:YES\].
    *   It will also open the IOSurface ID that was sent from the GPU
                process and create an OpenGL texture backed by it.
7.  At some point the system will call -\[IOSurfaceLayer
            displayInCGLContext\], which is where the IOSurface is actually
            drawn
    *   The OpenGL texture allocated in the above step is drawn here as
                a full-viewport quad
    *   After this, the cc::OutputSurface is un-blocked, allowing the
                ui::Compositor to produce more frames (which, in turn, allows
                the renderer to produce more frames).

Notes on power+performance:

Note that we draw all of the content into the IOSurface-backed FBO, and then
drew that FBO to the screen in the browser process. This is reduction in
performance and increase in GPU power consumption.

It is possible to draw directly to the browser process using the CAContext (aka
CARemoteLayer API), discussed later.

**Mechanism of GPU back-pressure:**

> Note that between steps 6 and 7 above, we told CoreAnimation “hey, we’re a
> CAOpenGLLayer and this is crazy, but we have a new frame, so call our draw
> method maybe”. If the GPU is really busy, it may be more than one vsync
> interval before the draw method is called. It is by this mechanism that
> back-pressure is applied from the GPU to the ui::Compositor, to the renderer.

Draw methods never being called:

This back-pressure mechanism (the delay between steps 6 and 7) can sometimes
mis-fire. Sometimes we just never end up getting the draw method called, just by
a fluke. As a result, there is a DelayTimer set which fires 1/6th of a second
after we ask CoreAnimation to draw us. If we haven’t drawn after this timer
fires, we un-block the cc::OutputSurface anyway.

Synchronous versus asynchronous drawing:

The way that we say “please draw this IOSurfaceLayer or ImageTransportLayer” is
not by calling -\[CAOpenGLLayer setNeedsDisplay\], but rather by calling
-\[CAOpenGLLayer setAsynchronous:YES\]. This results in CoreAnimation asking us,
at (about) every vsync, “hey, do you have content ready for me to draw?”. This
“CoreAnimation pulls from us” rather than “we push to CoreAnimation” is the best
way to get smooth animation.

The drawback is that we get a callback every vsync. If we don’t have any
content, then this is just idle CPU cycles (and it can add up to a lot). To
compensate for this, if we tell CoreAnimation “sorry, we don’t have new content
ready for you” (in the -\[CAOpenGLLayer canDrawInCGLContext\] function) a
certain number of times in a row, we switch to the -\[CAOpenGLLayer
setAsynchronous:NO\] mode.

This switching the isAsynchronous mode can cause problems when dynamically
changing the content scale of the layer (no idea why, should probably file a
Radar, cause this reproduces with a reduced test case), so, when we have to
change the content scale of the layer, we destroy and re-create the
CAOpenGLLayer.

# Drawing Delegated (or any other kind of) Frames in the Browser Using CAContexts

This is the mechanism by which we can draw directly into a CALayer in the GPU
process, and have the content appear in the browser process.

An API for this was introduced in 10.7 (the CARemoteLayer API), but was broken
in 10.9. The replacement API (CAContext) was reverse-engineered, and appears to
continue to work in 10.10.

The sequence by which frames are drawn using CAContexts is as follows.

1.  While drawing a frame, the ui::Compositor is rendering (via its
            command buffer) into an OpenGL texture.
    *   This is via an FBO, allocated and set up by
                ImageTransportSurfaceFBO using CALayerStorageProvider
2.  When done drawing the frame, the ui::Compositor’s
            cc::LayerTreeHostImpl’s cc::OutputSurface will issue a glSwapBuffers
            to the GPU process.
    *   The ui::Compositor’s cc::LayerTreeHost will now be blocked until
                its cc::OutputSurface has its OnSwapBuffersComplete method
                called.
3.  The command buffer will decode this and end up calling
            ImageTransportSurfaceFBO::SwapBuffers
    *   This will create (if not created yet) a CAContext and
                ImageTransportLayer (a subclass of CAOpenGLLayer), and will tell
                the ImageTransportLayer to draw via -\[ImageTransportLayer
                setNeedsDisplay\] or -\[ImageTransportLayer
                setAsynchronous:YES\].
4.  Processing of the SwapBuffers in the command stream in the GPU
            process will result in
            BrowserCompositorOutputSurface::OnSwapBuffersComplete being called
            in the browser process (this is done by the GpuControl::Echo
            mechanism).
    *   On non-Mac platforms, this results in the cc::OutputSurface
                being un-blocked, un-blocking the ui::Compositor. This is
                undesirable on Mac, because this is insufficiently sensitive to
                GPU back-pressure.
    *   To avoid this on Mac, the Mac will not call the super-class'
                cc::OutputSurface::OnSwapBuffersComplete inside
                BrowserCompositorOutputSurface::OnSwapBuffersComplete.
5.  At some point the system will call -\[ImageTransportSurfaceLayer
            displayInCGLContext\] in the GPU process
    *   The OpenGL texture that was drawn to is drawn here as a
                full-viewport quad
    *   After this, the GPU process will send the CAContext’s ID to the
                browser process in a GpuHostMsg_AcceleratedSurfaceBuffersSwapped
                message.
6.  The GpuHostMsg_AcceleratedSurfaceBuffersSwapped IPC will be received
            by GpuProcessHostUIShim in the browser process
    *   Of note is that this goes through the RenderWidgetResizeHelper,
                so it will be able to be received even during a live resize.
7.  The GpuProcessHostUIShim will pass this to the static method
            BrowserCompositorViewMac::GotAcceleratedFrame. This will look up the
            BrowserCompositorCALayerTree corresponding to the output surface ID.
    *   This call to
                BrowserCompositorCALayerTree::GotAcceleratedCAContextFrame will
                create a CALayer from the CAContext ID, if it has not been done
                already, which will make the frame appear on the screen (if this
                has been done already, then nothing needs to be done to get the
                content to appear on the screen.
    *   Finally, the cc::OutputSurface is un-blocked, allowing the
                ui::Compositor to produce more frames (which, in turn, allows
                the renderer to produce more frames).

# Stalling/Waiting for Frames in the Browser

There are some times that the browser will want to pause for frames from the
renderer. For instance, when resizing, we want to make sure that we do not allow
the window to complete resizing to a new size until it has content for that
size. This is accomplished by pumping a specialized nested run-loop which runs
all tasks posted by the ui::Compositor and handles selected IPCs (the ones
mentioned in this document, among others) from the GPU process and the renderer
process.

This restricted nested run loop is pumped inside the NSView’s setFrameSize in
the WasShown function as well. There is substantial documentation of its
behavior next to the definition of the RenderWidgetResizeHelper class.
