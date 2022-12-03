---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: blink-coordinate-spaces
title: Blink Coordinate Spaces
---

## #### Blink Coordinate Spaces

## Types of Zoom

There are two types of zoom in Chromium: *Browser Zoom* and *Pinch-Zoom*.

**Browser zoom** is what you get by using Ctrl+/- or the zoom buttons in the
<img alt="image"
src="/developers/design-documents/blink-coordinate-spaces/menu.png" height=15
width=15> menu. It changes the size of a CSS pixel relative to a device
independent pixel and so it will cause page layout to change. Throughout Blink,
Browser-zoom is referred to as “Page Zoom” or “Zoom” more generally.

**Pinch-zoom** is activated by using a pinch gesture on a touchscreen or
touchpad. Throughout Blink and the compositor, pinch-zoom is referred to as
“Page Scale”. Pinch-zoom is performed as a post-rendering step, scaling the
surface the web page is rendered onto. This means it doesn't interact with
layout. Put another way, pinch-zoom doesn't change the relationship between
physical pixels and CSS pixels (the \`window.devicePixelRatio\`) as far as Blink
is concerned, even though the user technically sees a different number of
physical pixels per CSS pixel.

## Types of Pixels

**Physical Pixels:** These are the actual pixels on the monitor or screen.

**Device Independent Pixels (DIPs):** Modern devices sport high density displays
(e.g. Apple’s Retina). In order to keep the UI at a comfortably readable size we
scale it up based on the density of the display. This scaling factor is called
the *device pixel ratio* in web APIs and *device scale factor* in Chromium code.
A UI will always be the same size in DIPs regardless of how many pixels are
actually used to display it. To go from physical pixels to DIPs we divide the
physical pixel dimensions by the device scale factor.

Blink implements UI scaling by applying the device scale factor to the browser
zoom. As such, code in Blink rarely needs to directly account for device scale
factor. Instead, most internal geometry in Blink is done in physical pixels.
When interfacing with web content, geometry is converted to CSS pixels by
dividing by the browser zoom, which combines both the user's zoom preference
(ctrl +/-) and device scale factor.

**CSS pixels:** CSS defines its own pixel type that is also independent of
physical pixels. When there is no Browser-Zoom, Pinch-Zoom, or CSS transforms
applied, CSS pixels and DIPs are equivalent. However, zoom can make CSS pixels
bigger or smaller relative to DIPs.

## Coordinate Spaces

Note that the conversion methods between these spaces (in LocalFrameView) do not
apply browser zoom, they deal in physical pixels. To convert from locations and
sizes expressed in true CSS pixels (as used in web APIs) to Document Content,
FrameView Content, or Frame space you must first multiply by the browser-zoom
scale factor.

**Document**

The coordinate space of the current FrameView's document content, in physical
pixels. The origin is the top left corner of the Frame’s document content. In
Web/Javascript APIs this is referred to as "page coordinates" (e.g.
MouseEvent.pageX) though there it is in CSS pixels (i.e. browser zoom applied).
Because coordinates in this space are relative to the document origin, scrolling
the frame will not affect coordinates in this space.

**Frame**

The coordinate space of the current FrameView in physical pixels. The origin is
the top left corner of the frame. Therefore, scrolling the frame will change the
"frame-coordinates" of elements on the page. This is the same as document
coordinates except that Frame coordinates take the Frame’s scroll offset into
account. In Web/Javascript APIs this is referred to as "client coordinates"
(e.g. MouseEvent.clientX) though there it is in CSS pixels (i.e. browser zoom
applied).

**Root Frame**

The Frame coordinates of the top level (i.e. main) frame. This frame contains
all the other child frames (e.g. elements create frames on a page).

**(Visual) Viewport**

The coordinate space of the visual viewport as seen by the user, in physical
pixels. The origin is at the top left corner of the browser view (Window or
Screen). The difference between Viewport and RootFrame is the transformation
applied by pinch-zoom. This is generally what you'd use to display something
relative to the user's Window or Screen.

**Screen**

The final screen space on the user's device, relative to the top left corner of
the screen (i.e. if we're in a Window, this will include the window's offset
from the top left of the screen). Note that this is in DIPs rather than physical
pixels.

## Web-exposed input co-ordinate spaces

To see exactly how some of the above co-ordinate spaces are exposed to
JavaScript in input events see <https://rbyers.github.io/inputCoords.html>.
