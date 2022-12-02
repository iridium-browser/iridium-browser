---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: embedding-flash-fullscreen-in-the-browser-window
title: Embedding Flash Fullscreen in the Browser Window
---

# Author: Yuri Wiitala (miu@chromium.org)

# Objective

Enhance the \[Pepper\] Flash fullscreen user experience in Chromium to match the
HTML5 fullscreen experience. This would provide users with a single, consistent
fullscreen-mode experience, with Flash fullscreen gaining the improved
window/desktop management behaviors of HTML5 fullscreen. For example, on Mac,
one cannot switch to other apps from Flash fullscreen, but this action is
intelligently handled in the HTML5 fullscreen case.

Side note: Later on, we seek to improve the fullscreen UX further by introducing
a new "Fullscreen in Tab" concept for tab capture/screencasting
(<http://crbug.com/256870>). While a user is "casting," fullscreen content is
being displayed on a remote screen. Therefore, the local screen should be usable
for other tabs or applications. The changes proposed in this document are a
necessary prerequisite for this.

Background

Currently, Flash fullscreen is implemented in the "content" component where a
raw, platform-dependent fullscreen window is created. Some minimal
focus/keyboard control logic is added to support exit and mouse-lock. The entire
stack assumes the Flash plugin handles the user permissions with respect to
allowing fullscreen and/or mouse-locking, and so blindly allows all requests.

Fullscreen mode is initiated by the render process, which is proxying the
request from the Pepper Flash process. It sends a "create" message followed
immediately by a "show" message. Once the fullscreen window is shown, a resize
message is sent back to the renderer as a signal that painting may begin.

The control flow for entering and exiting Flash fullscreen is as follows:

[<img alt="image"
src="/developers/design-documents/embedding-flash-fullscreen-in-the-browser-window/Design%20Doc-%20Flash%20Fullscreen%20in%20Browser.png.1379119228467.png">](/developers/design-documents/embedding-flash-fullscreen-in-the-browser-window/Design%20Doc-%20Flash%20Fullscreen%20in%20Browser.png.1379119228467.png)

On the other hand, the Browser (user-initiated, "F11") and Tab/HTML5
(renderer-initiated) fullscreen modes are controlled by the browser UI
implementation in the "chrome" component, one abstraction level higher. The
browser uses FullscreenController to manage the window sizing and show
"permissions balloons" for both modes, and handles the transitions of switching
in-between or in-and-out.

To engage fullscreen mode for an HTML5 element, the renderer sends a "toggle"
message to the browser process. Unlike Flash fullscreen, no new widgets are
created. FullscreenController instructs the browser window to expand to fill the
screen, and this causes the tab content view to be resized via the layout
process. Then, the renderer process receives a resize message signalling that
fullscreen mode is on. The originating HTML5 element (e.g., a video element),
will then be resized to fill the content area.

The control flow for entering and exiting HTML5 fullscreen is as follows:

[<img alt="image"
src="/developers/design-documents/embedding-flash-fullscreen-in-the-browser-window/Design%20Doc-%20Flash%20Fullscreen%20in%20Browser%20%281%29.png.1379119292169.png">](/developers/design-documents/embedding-flash-fullscreen-in-the-browser-window/Design%20Doc-%20Flash%20Fullscreen%20in%20Browser%20%281%29.png.1379119292169.png)

Thus, there exist two separate fullscreen control/view implementations in
Chromium that operate independently and at different component layers. There
actually exists a conflict, should a page choose to initiate HTML5 fullscreen
during Flash fullscreen or vice versa.

# Observations and Goals

Fortunately, there are key similarities between the implementations of Flash
fullscreen and HTML5 fullscreen that suggests the former can be merged into the
machinery of the latter. First, renderer-side, both send one or more messages to
the browser process to engage fullscreen mode, and both expect an asynchronous
ViewMsg_Resize message to confirm and receive the fullscreen dimensions. Second,
browser-side, the messages from the renderer are handled by delegating to the
same WebContentsImpl instance to initialize and kick-off. Finally, the browser
process sends one or more ViewMsg_Resize messages to notify the renderer of
"screen size" changes, and a ViewMsg_Close to shut down rendering of the widget.

In the past, developing for the Pepper Flash plugin required overcoming very
temperamental compatibility issues. Since one permeating goal is to ensure Flash
fullscreen does not break, no changes will be made that could be observable by
the renderer process (and, by proxy, the pepper flash plugin). This means no
changes to IPC messaging, with the ordering and content of the messages being
exactly the same as in the current implementation.

If we are to proceed, one UX issue to resolve is the "Press Esc to Exit
Fullscreen" message rendered by the Pepper Flash plugin: Ideally, for
consistency, this would be removed in favor of the balloon pop-up shown by
FullscreenController. If this is not possible, then the balloon pop-up should
not be displayed for Flash.

# Design

A three-part change to the existing design is proposed. First, the Flash
"fullscreen" render widget will become embedded within the browser window. More
specifically, it will replace the normal tab content view in the view hierarchy
and participate in layout like any other widget. The owner and creator of the
widget will remain the same and, from the renderer's perspective, Flash will
still be painting into its own dedicated "fullscreen" widget.

Second, the FullscreenController will be used to manage the the sizing/expansion
of the browser window for all fullscreen modes, including Flash, and will treat
Flash fullscreen in the same way as HTML5 fullscreen. As an embedded widget
within the browser window, the Flash fullscreen widget will become expanded to
fill the entire screen during the layout process. FullscreenController itself
will need only small tweaks: Just as for HTML5 fullscreen, it will be tasked to
decide whether the fullscreen and mouse-lock privileges are allowed, but will
instead treat Flash fullscreen mode as fully-privileged by default.

Finally, WebContentsImpl will be modified to invoke the new "embedded widget"
code paths for startup and shutdown (described in detail below). Use of the new
code paths versus the old will be switchable via a new WebContentsDelegate
method, EmbedsFullscreenWidget(). This method will declare whether the browser
implements embedded Flash fullscreen and whether it is enabled/disabled via
command-line flags.

The control flow for entering and exiting embedded Flash fullscreen is as
follows (additions/changes in red):

[<img alt="image"
src="/developers/design-documents/embedding-flash-fullscreen-in-the-browser-window/Design%20Doc-%20Flash%20Fullscreen%20in%20Browser%20%282%29.png.1379119349348.png">](/developers/design-documents/embedding-flash-fullscreen-in-the-browser-window/Design%20Doc-%20Flash%20Fullscreen%20in%20Browser%20%282%29.png.1379119349348.png)

## Entering Embedded Flash Fullscreen Mode

WebContentsImpl drives this process, being the delegate of the
RenderViewHostImpl which receives the "show" message from the renderer. Instead
of showing the window it created as a raw, fullscreened window, the behavior of
ShowCreatedFullscreenWidget() changes to the following:

    Toggle the Browser into fullscreen mode via the same code path as for HTML5
    fullscreen. This triggers FullscreenController's logic which will cause the
    BrowserWindow to expand and re-layout for full-screen. In addition, the
    "Allow/Exit" balloon pop-up will be shown, if appropriate.

    Invoke WebContentsObserver::DidShowFullscreenWidget() for all observers.
    This will be received by WebView (or TabContentsController for Mac), which
    is the child view within BrowserWindow that contains the tab content.
    WebView will alter its child window hierarchy to embed the Flash fullscreen
    widget within itself and, therefore, within the fullscreened BrowserWindow.
    WebView accesses the Flash fullscreen widget via a new
    WebContents::GetFullscreenRenderWidget() method.

## Exiting Embedded Flash Fullscreen Mode

Exiting must be considered from two possible originators. When the exit is
requested by the Flash widget itself, RenderWidgetHostImpl::Shutdown() will be
invoked in the browser process, which in turn calls
WebContentsImpl::RenderWidgetDeleted(). From here, two steps are taken:

    Toggle the Browser out of fullscreen mode. FullscreenController performs the
    same actions as it would for coming out of HTML5 fullscreen, restoring the
    original size and layout of the BrowserWindow.

    Invoke WebContentsObserver::DidHideFullscreenWidget() for all observers.
    WebView receives this and responds by re-attaching the normal tab content
    view in its child window hierarchy.

When the exit originates from the Browser UI (e.g., user action in the
"Allow/Exit" balloon):

    FullscreenController takes the BrowserWindow out of fullscreen mode, just
    like HTML5 fullscreen mode.

    FullscreenController invokes RenderViewHostImpl::ExitFullscreen(). Again,
    this is just like HTML5 fullscreen mode.

    At this point, RenderViewHostImpl notices a Flash fullscreen widget is
    active, and invokes its RenderWidgetHostImpl::Shutdown() method. As
    described above, shutting down the widget will cause the
    DidHideFullscreenWidget() notification to be sent to WebView, restoring the
    normal tab content view in the browser window.

## Other Considerations

Windows 8 Metro Snap mode: When entering Flash fullscreen mode, the
FullscreenController will no-op. However, WebView will still receive the
DidShowFullscreenWidget() notification, allowing it to correctly embed the Flash
fullscreen widget within itself, all properly laid out within the Metro-Snapped
browser window.

Kiosk (Fullscreen App) mode: Just like Windows 8 Metro Snap mode, the
FullscreenController will no-op and WebView will correctly embed the Flash
fullscreen widget.

Content Shell and non-Chromium applications that use src/content/ as a library:
By default, the non-embed Flash fullscreen code path will be used. The key to
this is the new WebContentsDelegate::EmbedsFullscreenWidget() method, whose
default implementation returns false. The method is only overridden to return
true for the Chromium browser on supported platforms.

# Work Plan

Prototyping/Feasibility of Design: Completed (<http://crrev.com/23656002>).

Implementation for Aura, Windows and Mac; disabled by default, behind feature
flag: M31 (<http://crrev.com/23477051>).

Implementation for GTK: M32 (with M31 as a stretch goal).

Launch goal: Tested, UI review, and enabled by default for M32.
