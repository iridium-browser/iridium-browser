---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/oop-iframes
  - Out-of-Process iframes (OOPIFs)
page_name: oop-iframes-rendering
title: Rendering and compositing out of process iframes
---

# Summary

This document provides a design for rendering and compositing out-of-process
iframe (OOPIF) elements.

## Contents

[TOC]

## Background

### Out-of-process iframes

This is in support of the main OOPIF design work, for which a higher level
description can be found at
<http://www.chromium.org/developers/design-documents/oop-iframes>.

### A very brief primer on Blink rendering and painting

When an HTML resource is loaded into Blink, the parser tokenizes the document
and builds a DOM tree based on the elements it contains. The DOM tree is a
WebCore data structure that consists of objects with types that inherit from
[WebCore::Node](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/WebCore/dom/Node.h).

As each node is added to the DOM tree, the attach() method is called on it which
creates an associated renderer (an object with a type that inherits from
[WebCore::RenderObject](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/WebCore/rendering/RenderObject.h))
and adds it to the render tree. Objects in the render tree roughly correspond to
objects in the DOM tree, but there are many exceptions (some DOM nodes do not
receive renderers, some will have more than one renderer, and many renderers are
created that that do not have corresponding DOM nodes). The render tree is the
primary data structure involved in the layout phase of rendering, during which
size and position are computed for all rendered elements.

During construction or modification of the render tree, some renderers will
receive associated
[WebCore::RenderLayer](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/WebCore/rendering/RenderLayer.h)
objects. Attaching a RenderLayer to a RenderObject indicates that the renderer
has its own coordinate system, and therefore the content of the renderer and its
descendants can (but won’t necessarily) be painted to a separate compositing
layer. Each RenderLayer is added to the RenderLayer tree, which has a sparse
association with the render tree and is the primary data structure used for the
painting phase of rendering.

The diagram below provides a basic illustration of the DOM tree, render tree,
and RenderLayer tree for the following HTML:

&lt;html&gt;

&lt;body&gt;

&lt;iframe src=”<http://www.example.org>”&gt;&lt;/iframe&gt;

&lt;/body&gt;

&lt;/html&gt;

<img alt="image"
src="/developers/design-documents/oop-iframes/oop-iframes-rendering/rendering%20design%20-%20rendering%20trees.png">

More details on rendering, and in particular the layout phase, can be found at
<https://www.webkit.org/blog/114/webcore-rendering-i-the-basics/>.

The details of painting depend on whether accelerated compositing is used. This
design uses the assumption that OOPIFs will always be rendered using GPU
accelerated compositing in Chromium.

When accelerated compositing is in use, some RenderLayers receive separate
compositing layers, or backing surfaces, onto which their bitmaps will be
painted. All compositing layers are provided to the compositor, where they are
used to create the final rendered bitmap. In WebCore, compositing layers are
represented and managed by the
[RenderLayerBacking](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/WebCore/rendering/RenderLayerBacking.h)
class.

RenderLayerBacking in turn corresponds to a GraphicsLayer, the glue between
WebCore rendering and platform compositing, instantiated in our case as
[GraphicsLayerChromium](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/WebCore/platform/graphics/chromium/GraphicsLayerChromium.h).
GraphicsLayers are positioned into yet another sparse tree, this one used by the
compositor.

### A little bit about compositing and the GPU

A GraphicsLayer internally manages a
[WebLayer](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/Platform/chromium/public/WebLayer.h),
which is a compositor container for a shared memory buffer between a renderer
process and the GPU process. During the painting phase, each RenderLayer
instructs all of its associated RenderObjects to paint into the WebLayer buffer.
The GPU process can output the bitmap texture to the screen.

Details of how the different processes synchronize buffer usage and throttle
rendering vary significantly by platform. The GPU often uses a double buffering
system, in which two buffers are shared so that one buffer is being output to
the screen while the other is being painted. When a paint is complete, the
renderer issues a swap buffer command to the GPU to cause it to swap which
buffer is being displayed, and which is available for paint. The renderer sends
an IPC to the browser when a new buffer is available and expects an ACK for
every such message. This ACK contains a previously sent buffer to be reused by
the GPU and a sync point (location in the GPU command buffer), or a NULL buffer,
if nothing is available to be recycled. Sync points are used to make sure the
same buffer isn’t being rendered into and drawn out of. When an embedding
compositor issues the last draw call that uses the child renderer’s image data
to the GPU command buffer, it also inserts a sync point into the command stream
and sends an ACK to the renderer with that number. Rendering into the recycled
buffer is only allowed after the GPU processed all commands up to the sync
point. Rendering is suspended until that happens.

The compositor uses a
[TextureLayer](https://code.google.com/p/chromium/codesearch#chromium/src/cc/layers/texture_layer.h)
to encapsulate textures that need to be shared between different processes. It
also provides a management facility for TextureLayer in
[TextureMailbox](https://code.google.com/p/chromium/codesearch#chromium/src/cc/resources/texture_mailbox.h),
which conveys ownership of the texture between processes. One process can make a
texture available to another process by using the glProduceTextureCHROMIUM GPU
command buffer API for the appropriate mailbox, and then the next process takes
ownership of the texture via the glConsumeTextureCHROMIUM GPU command buffer API
on the same mailbox.

Most of this information, and much much more, can be found at
<http://www.chromium.org/developers/design-documents/gpu-accelerated-compositing-in-chrome>.

### The Übercompositor

The
[ubercompositor](https://docs.google.com/a/chromium.org/document/d/1ziMZtS5Hf8azogi2VjSE6XPaMwivZSyXAIIp0GgInNA/edit)
is being developed to improve rendering performance on Aura, by integrating
compositing of UI and web content in the browser process. Under the new
architecture, child compositors in each rendering process supply compositor
frames to the parent compositor which are then composited into the screen for
display.

Each CompositorFrame contains a set of quads and associated texture references
that are generated by a child compositor in a renderer process. Internally,
TextureMailboxes are used to transfer textures to the browser process.

Whereas in the existing hardware path compositing mode the renderer sends a
SwapBuffer message to the GPU to instruct it to display its uploaded texture to
screen, in the ubercompositor architecture the renderer sends an IPC message
directly to the browser containing the compositor frame. The parent compositor
then interacts with the GPU process via IPC in order to do the final
compositing.

## Rendering OOPIFs

### Overview

The full security advantage of OOPIFs is very difficult to achieve when the
compositing of a browser tab’s contents is done by a renderer process, because
the ability to use a texture in compositing implies read access to the pixels of
that texture (see the Cross Site Texture Stealing section below). Therefore the
primary design assumes the availability of ubercompositor. A prototype has been
built using conventional hardware path compositing, and we are maintaining that
parallel design with the expectation that we will want OOPIFs to be available on
platforms without ubercompositor availability, despite the significantly reduced
security properties.

The following list enumerates the requirements for OOPIFs to paint:

    The browser must enable accelerated compositing whenever an OOPIF is being
    rendered.

    New browser process facilities must pass messages from the embedding
    renderer process to the iframe renderer process for sizing information, and
    from the iframe renderer process to the embedding renderer process with
    information on texture availability for compositing.

    New renderer process facilities must receive identifying texture information
    (i.e. a texture mailbox name or ubercompositor frame) and associate it with
    a WebLayer that is attached to the FrameView object associated with the
    OOPIF’s Frame object.

    RenderFrameBase, the WebCore renderer associated with frame and iframe
    elements, must be aware when it contains out-of-process rendered content and
    in that case receive its own RenderLayer and RenderLayerBacking.

    the RenderLayerBacking associated with RenderFrameBase must set the contents
    of its graphics layer to the WebLayer.

When using hardware path compositing without ubercompositor, the renderer
process for the out-of-process frame creates a TextureMailbox. A
RenderWidgetHostViewSubframe object in the browser process passes the mailbox
name to the top-level frame renderer. This is illustrated in the diagram below.
The right half of the diagram is mainly existing functionality that will require
few changes. The left half contains some new and modified classes as well as new
IPC messages.

[<img alt="image"
src="/developers/design-documents/oop-iframes/oop-iframes-rendering/rendering%20design%20-%20texture%20routing.png">](/developers/design-documents/oop-iframes/oop-iframes-rendering/rendering%20design%20-%20texture%20routing.png)

When using the ubercompositor, the renderer process for the subframe creates a
compositor frame (a concept entirely distinct from HTML frames), which can
contain many texture mailboxes. It passes the compositor frame directly to the
browser process, which then relays it to the top-level frame renderer. The
top-level frame renderer cannot access the textures that are stored in GPU
memory, but can pass all of its compositor frames, including those obtained from
the subframe renderer process, back to the browser process for the
ubercompositor to composite them.

[<img alt="image"
src="/developers/design-documents/oop-iframes/oop-iframes-rendering/rendering%20design%20-%20compositor%20frame%20routing.png">](/developers/design-documents/oop-iframes/oop-iframes-rendering/rendering%20design%20-%20compositor%20frame%20routing.png)

### Browser process changes

Relevant changes to the representation of pages and frames in the browser
process are described at
<http://www.chromium.org/developers/design-documents/oop-iframes>.

IPC messages must be routed from the subframe renderer to the top-level frame
renderer to notify it of changed texture buffers or compositor frames. Also, IPC
messages must be routed from the top-level frame renderer to the subframe
renderer with control information such as resizing.

Each ViewHost_\* message will be passed through the RenderWidgetHostView for its
associated RenderFrame. RenderWidgetHostView will use the WebContents to send
the message to the destination RenderFrame (for messages originating from a
subframe renderer, this is the top-level RenderFrame; for messages destined to a
subframe renderer, this will use GetRenderManager()-&gt;current_host()) and then
dispatch the message to the proper recipient.

### Renderer process changes

1. IPCs

RenderView registers handlers for the new ViewMsg_BuffersSwapped IPC message (in
hardware path compositing) and ViewMsg_CompositorFramesSwapped (for
ubercompositor). This handlers lazily instantiate a SubframeCompositorHelper
object and proxies most of the handling to the helper. The new IPC messages
convey information such as size, TextureMailbox name, compositor frame data, and
GPU routing information for the texture with the swapped out frame contents that
was rendered by another process.

2. SubframeCompositorHelper

SubframeCompositorHelper is a new class that is responsible for managing the
TextureLayer and DelegatedRendererLayer information associated with a swapped
out frame. It is attached to the WebKit::RenderFrame for an out-of-process
frame, and is created upon receipt of a ViewMsg_BuffersSwapped or
ViewMsg_CompositorFramesSwapped IPC message. It performs the following tasks:

    Create a local TextureLayer for a received TextureMailbox name, or a
    DelegatedRendererLayer for a received compositor frame

    Creates a WebLayer containing a TextureLayer or DelegatedRendererLayer, and
    attaches the WebLayer to the FrameView for the swapped out subframe

    Manages the lifetime of the TextureLayer or DelegatedRendererLayer

[<img alt="image"
src="/developers/design-documents/oop-iframes/oop-iframes-rendering/rendering%20design%20-%20ogres%20are%20like%20onions.png">](/developers/design-documents/oop-iframes/oop-iframes-rendering/rendering%20design%20-%20ogres%20are%20like%20onions.png)

3. FrameView and RenderIFrame

WebCore itself requires few modifications related to rendering. The main change
is that for any cross-site iframe element, its associated RenderIFrame must
always have a RenderLayer and RenderLayerBacking created. This can be
accomplished by modifying RenderPart::requiresAcceleratedCompositing() and
RenderIFrame::requiresLayer() to return true. Additionally,
RenderCompositor::requiresCompositingForFrame(), which decides whether to create
a new compositing layer for a frame renderer, must always return true for
OOPIFs.

RenderIFrame, since it inherits from WebCore::RenderWidget, contains a pointer
to a WebCore::Widget. The m_widget on any RenderIFrame should always point to a
FrameView. SubframeCompositorHelper attaches the layer to the FrameView to make
it accessible during rendering: RenderCompositor accesses the WebLayer via
FrameView::platformLayer() and assigns it as the content for the frame’s
RenderLayerBacking. Once this is done, the compositor will display the frame
texture in the correct position.

## Compositing

### Cross-site texture stealing

An important security requirement for OOPIFs is that even though cross-site
frame contents are being composited with the content in the top-level frame, the
top-level renderer process must not be able to read the pixels that are drawn
within the frame. The ability to do this would allow a compromised renderer
process access to cross-site content, which is contrary to [the goals of the
site isolation
project](http://www.chromium.org/developers/design-documents/site-isolation).

We don’t believe this to be a feasible goal without the availability of
ubercompositor. It would be very difficult, given the performance constraints of
compositing, to require that the top-level renderer process be able to composite
using a given texture without being able to write that texture to an accessible
memory buffer. In the ubercompositor model, compositing is done by the trusted
browser process so this constraint no longer .

Access to the texture from a parent renderer process will be prevented by the
GPU process, where a check will be added to the handler of the
glConsumeTextureCHROMIUM command buffer API so that a TextureMailbox can be
consumed only by the renderer process that initially created the texture, and by
the browser process.

## Constraints

### Software path rendering

OOPIFs will not be supported when software path rendering is enabled. Software
compositing, when it becomes available, will allow OOPIFs to function on systems
with graphics driver that do not support hardware-based compositing, but this
will have the same security limitations as OOPIFs rendered under current
hardware path compositing, which does not prevent cross-site texture stealing.

### Compositor availability

Ubercompositor is limited to running on Aura, which currently only runs on
ChromeOS. We expect Aura to be running on Windows and Linux before OOPIFs are
enabled on stable without a flag. Until Aura is also ported to OS X, our
implementation will rely on the current hardware-accelerated compositor and the
software compositor.
