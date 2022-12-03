---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: chromeviews
title: views
---

### Overview and background

Windows provides very primitive tools for building user interfaces. The system
provides a few basic controls and native window containers, but building a
custom user interface is difficult. Since we desired a differentiated aesthetic
for Chromium, we have had to build a framework on Windows to accelerate our
development of custom UI. This system is called views.

views is a rendering system not unlike that used in WebKit or Gecko to render
web pages. The user interface is constructed of a tree of components called
Views. These Views are responsible for rendering, layout, and event handling.
Each View in the tree represents a different component of the UI. An analog is
the hierarchical structure of an HTML document.

At the root of a View hierarchy is a Widget, which is a native window. The
native window receives messages from Windows, converts them into something the
View hierarchy can understand, and then passes them to the RootView. The
RootView then begins propagation of the event into the View hierarchy.

Painting and layout are done in a similar way. A View in the View tree has its
own bounds (often imbued upon it by its containing View's Layout method), and so
when it is asked to Paint, it paints into a canvas clipped to its bounds, with
the origin of rendering translated to the View's top left. Rendering for the
entire View tree is done into a single canvas set up and owned by the Widget
when it receives a Paint message. Rendering itself is done using a combination
of Skia and GDI calls — GDI for text and Skia for everything else.

Several UI controls in Chromium's UI are not rendered using Views, however.
Rather, they are native Windows controls hosted within a special kind of View
that knows how to display and size a native widget. These are used for buttons,
tables, radio buttons, checkboxes, text fields, and other such controls. Since
they use native controls, these Views are also not especially portable, except
perhaps in API.

Barring platform-specific rendering code, code that sizes things based on system
metrics, and so on, the rest of the View system is not especially unportable,
since most rendering is done using the cross-platform Skia library. For
historical reasons, many of View's functions take Windows or ATL types, but we
have since augmented ui/gfx/ with many platform independent types that we can
eventually replace these with.

### Code Location and Info

The base set of classes and interfaces provided by views can be found in the
src/ui/views/ directory. All base views classes are in the "views" namespace.

### Common Widgets

In the views framework:

*   **WidgetWin**: The base class for all Widgets in views. Provides a
            basic child window control implementation. Subclass this directly if
            you are not making a top-level window or dialog.
*   **Window**: A top-level window. A subclass of WidgetWin.

For more information on building dialog boxes and other windowed UI using
Window, CustomFrameWindow, etc, read [views
Windowing](/developers/design-documents/views-windowing).

In the Chromium browser front end:

*   BrowserFrame: A subclass of Window that provides additional message
            processing for the Browser window in Chrome. See [Browser
            Window](/developers/design-documents/browser-window).
*   ConstrainedWindowImpl: A subclass of Window that provides the frame
            for constrained dialogs such as the HTTP basic auth prompt.

### Other approaches

At the start of the project, we began building the Chromium browser using native
windows and the owner-draw approach used in many Windows applications. This
proved to be unsatisfactory, since native windows don't support transparency
natively, and handling events requires tedious window subclassing. Some early UI
elements gravitated towards custom painting and event handling (e.g.
Autocomplete), but this was very ad-hoc based on the circumstance.

Existing UI toolkits for Windows are similarly unsatisfying, with limited widget
sets, unnatural aesthetics, or awkward programming models.

### Limitations/issues

By and large, views makes it relatively easy to build complex custom UIs.
However it has a few rough edges that can be improved over time:

*   The Event types currently are occasionally problematic - they crack
            the native windows message parameters and then discard them.
            Sometimes this information is useful.
*   Some ad-hoc message processing.
*   Mix of native controls that don't work properly until inserted into
            a View hierarchy attached to a Widget with a valid HWND. Many of our
            native controls have API methods that require them to exist within a
            window hierarchy. This means that they cannot be fully initialized
            until they are inserted. The View API will eventually be improved to
            make this clearer ([bug 5191](http://crbug.com/5191)).
*   The base Widget interface itself is somewhat frozen in time. Some
            improvement and consolidation would be worthwhile.
