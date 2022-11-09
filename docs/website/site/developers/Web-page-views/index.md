---
breadcrumbs:
- - /developers
  - For Developers
page_name: Web-page-views
title: Web page views
---

Our original architecture did not have any platform separation in the browser
window and tab layer. WebContents contained a RenderViewHost, and that
implemented a great deal of both logic and view-related functionality.

We want to make this architecture cleaner and separate out the view from the
cross-platform logic. This document tracks the progress of this work.

## Current architecture

We have split out a variety of OS-specific views from WebContents. On Windows,
the objects look like this:

[<img alt="image"
src="/developers/Web-page-views/WebContents3.png">](/developers/Web-page-views/WebContents3.png)

The view-related calls from RenderViewHost to WebContentsView are removed from
WebContents completely by using a separate delegate. When RenderViewHost needs
to make a view-related call, it asks its delegate (through the
RenderViewHostDelegate interface) for the "View" delegate. This is implemented
by the WebContentsView.
