---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: mac-plugins
title: Mac Plugins
---

## Overview

This document provides an overview of the Mac-specific aspects of implementation
of Chromium’s plugin hosting. Except where noted, Chromium for Mac uses the same
[overall architecture](/developers/design-documents/plugin-architecture) as
Windows and Linux.

## Platform API Handling

Much of the complexity of the Mac plugin host implementation is in the drawing
and event handling discussed below. There are a few specific areas common to all
of them, relating to certain platform APIs that would not work correctly without
special handling. Use of platform APIs in plugin is discouraged due to this, but
some are very common and have no supported NPAPI equivalent, so must be
supported.

### Plugin-Created Windows

Because plugins are in their own process, the window layer and activation for
windows created by plugins is not correct. To work around this, Chromium uses
DYLD_INSERT_LIBRARIES to hook Carbon window creation and activation calls, and
swizzling to hook Cocoa window creation and activation calls. Chromium attempts
to manage the process activation to match the window activation, so that plugin
windows show on top of browser windows.

#### Full-Screen Windows

Full-screen windows are detected as a special case of plugin-opened windows, and
cause the browser to request or relinquish full screen mode as appropriate,
since this must be done from the foreground process.

#### Limitations

*   Complex plugin and browser window layering, in the case where a
            plugin creates multiple windows, may not be correct. In practice,
            this doesn’t appear to be a problem.
*   Modal windows don’t currently prevent interaction with background
            windows, only bringing background windows forward.
*   There may be specific calls not caught by the current
            interposing/swizzling.

### Cursors

As with full-screen mode, cursors must be managed from the foreground process.
As a result, several Cocoa cursor calls are hooked, and the cursor information
handed off to another process.

#### Limitations

*   Not all cursor operations are currently handled.

## Drawing and Event Handling

## There are two event models and four drawing models on the Mac, and Chromium supports all but the oldest of each. The specific details of each implementation are explained below.

### Carbon Events

This event model is no longer support

### Cocoa Events

The Cocoa event model doesn’t provide a window reference to the plugin, so the
out-of-process implementation is mostly straightforward. However, there are two
wrinkles:

#### IME Support

Cocoa event model IME has extremely specific semantics, which did not mesh
cleanly with WebKit’s IME at the time the code was written. As a result, plugin
IME is handled explicitly in the browser process, with messages sent back and
forth through the renderer process. In the future, it might be possible to move
that support into Blink itself.

#### NPN_ConvertPoint

NPN_ConvertPoint requires that browsers be able to map plugin coordinates to
screen and window coordinates, so the plugin host is forced to track the
location of the browser window. This function is poorly spec’d, however, and
it’s unclear how it should behave in the presence of CSS transforms, so the
implementation may need to move to one where the query is passed into the
renderer and/or browser process, and answered there, instead of window location
being tracked by the plugin process.

### QuickDraw

This drawing model is no longer supported.

### CoreGraphics

CoreGraphics support is straightforward: plugins are provided a graphics port
pointing to the shared buffer. Since plugins are only allowed to draw during a
draw event in this model, tearing is not an issue.
Unlike Windows, where the page background must be provided to the plugin in
order to allow the plugin to composite, CoreGraphics-based drawing preserves
transparency data. As a result, transparent plugins on the Mac do not have the
same double-buffer and synchronous drawing requirements that they do on Windows.

### Core Animation

Since the purpose of the Core Animation model is to provide hardware accelerated
drawing, the normal plugin drawing system is bypassed (as with windowed plugins
on other platforms). Instead, an IOSurface is shared between the plugin and gpu
processes, and the plugin’s CALayer is drawn into that surface using CARenderer.
On the gpu side, the contents of the IOSurface are composited into the page
using a hardware-accelerated drawing mechanism.
Because the IOSurface approach requires explicit action on the browser side when
drawing is necessary, but CARenderer doesn’t provide a way to know when drawing
is necessary, drawing is driven by a high-frequency timer on the plugin side. As
a result, the invalidating Core Animation model described below was developed as
an alternative. Use of the original Core Animation model in Chromium is
discouraged.

#### Limitations

*   The timer adds a small amount of CPU overhead.

### Invalidating Core Animation

The invalidating Core Animation model is identical to the Core Animation model,
except that the plugin informs the plugin host when drawing is necessary; as a
result, the timer is not needed.

### Future Plans

#### 64-Bit

A number of plugins on the Mac are still 32-bit only; because Safari and Firefox
both support 32-bit plugins out of process in 64-bit mode, there has not been
strong incentive for plugin vendors to release 64-bit plugins. Because of this,
any 64-bit Mac Chromium release will likely need to support 32-bit plugins.
