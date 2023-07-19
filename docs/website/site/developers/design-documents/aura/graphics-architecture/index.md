---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: graphics-architecture
title: Graphics Architecture
---

### Overview

[<img alt="image"
src="/developers/design-documents/aura/graphics-architecture/ChromeGraphicsArchitecture.png">](/developers/design-documents/aura/graphics-architecture/ChromeGraphicsArchitecture.png)

Each Aura Window owns a corresponding compositor layer. The layer tree
corresponds roughly to the window tree (though Views also supports layers, so a
single Aura Window can have nested Layers created by Views within that Window).

If the Window's layer has a texture that texture is drawn by the compositor.
Aura is transitioning to using the Chromium Compositor ("CC"). At time of
writing CC is part of WebKit, but there is a nascent effort to extract it into
its own library within Chromium.

The Compositor maintains two trees of layers, one on the UI thread and one
(optionally) on the Compositor thread. This improves performance for animations
and interactive events like scrolling. See CC documentation for more
information. The CC library talks to the GPU process via command buffer to do
the final rendered output.

### Pre-Layer Tree Unification World

At the time of writing we have two layer trees, one for the UI and one for
content from the renderer process. The renderer process uses WebLayer directly,
while the UI wraps it in a class ui::Layer in ui/compositor, which provides some
additional utilities useful to Aura and Views.

Once CC is extracted into a standalone library we will explore replacing
ui::Layer functionality with direct use of CCLayer.

### Paint Scheduling/Draw Flow

Throughout these documents and in the source code you will see references to
paint and draw. Paint is an operation performed mostly by the Views system to
update the contents of the windows... this is a typically a software operation
to update the contents of a Skia canvas. Draw is an operation performed by the
compositor to draw the contents of layers' textures to the screen, including
potentially compositing some of them against each other.

A draw can be triggered by two sources: the underlying native window owned by
the RootWindowHost can be sent a redraw notification from the system (e.g.
WM_PAINT or Expose), or an application event can trigger a redraw by calling
ScheduleDraw() on any number of framework objects (Compositor, RootWindow, etc).
In the latter case a task is posted that calls Draw().

Layer also supports a method SchedulePaint(), which is exposed through Window
and also NativeWidgetAura and used by Views when some sub-region of the layer is
to be marked invalid. The layer stores the current invalid rect which is a union
of all invalid rects specified since the last validation.

When the compositor draws, it performs a depth-first walk of the layer tree, and
if any of the layers it encounters has an invalid rectangle, it is asked to
repaint its contents. The compositor does this via
WebKit::WebContentLayerClient, implemented by ui::Layer. The layer asks its
delegate to repaint via a call of OnPaintLayer() which takes a Skia canvas sized
to the invalid rect. In Aura, the delegate is the Window, and the Window asks
its delegate to repaint. In production this is often a NativeWidgetAura, a Views
type. As mentioned earlier, Views can also have layers directly, and as such a
View can also be a layer's delegate.

Once the draw is complete the invalid region is validated so no further paint
notifications are sent until explicitly requested by a subsequent invalidation.
