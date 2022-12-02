---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: browser-window
title: Browser Window
---

The Chromium browser window is represented by several objects, some of which are
included in this diagram:

[<img alt="image"
src="/developers/design-documents/browser-window/BrowserWindow2.png">](/developers/design-documents/browser-window/BrowserWindow2.png)

## Frame

The frame is the portion of the browser window that includes the title bar,
sizing borders, and other areas traditionally known as the "non-client" areas in
Windows terminology. We supply a frame (called BrowserFrame) that subclasses the
views::Widget class that adds some additional handling on Vista for DWM
adjustments behind the TabStrip, etc.
On Windows Vista, we only use the glass mode when desktop compositing is
enabled; in Classic or Vista Basic modes we use the XP Luna mode. Because the
user is able to toggle the compositing on and off by changing Windows themes, we
need to be able to dynamically change frame mode. For more information about how
this is done in views, see the [views::Widget](goog_1235887298838)[
documentation](/developers/design-documents/views-windowing). The browser window
provides two NonClientFrameView subclasses - GlassBrowserFrameView and
OpaqueBrowserFrameView which are swapped in the BrowserFrame's NonClientView
when DWM is toggled.

Browser View

The BrowserView object contains all of the elements that are common between the
frames that are part of the presentation of the browser window - the tab strip,
the toolbar, the bookmarks bar, and other elements of the UI. When the frame
changes, this View is inserted into the new NonClientView.

The BrowserView implements an interface called BrowserWindow, which the Browser
object uses to interact with the View.

## Browser

This is the core state and command execution component of a Browser window. It
interacts with an abstract Browser Window interface to update the UI. This
allows us to write unit tests that supply a windowless "testing view" and then
execute high-level functionality within the browser from within the unit testing
framework, rather than running UI tests.
