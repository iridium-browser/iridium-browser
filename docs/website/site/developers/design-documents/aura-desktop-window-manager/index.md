---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: aura-desktop-window-manager
title: Aura (obsolete)
---

*This document is still good for a high level overview, with contact
information, but many technical details are now obsolete; see the main [Aura
index](/developers/design-documents/aura) for more details.*

### Project Goals

The goal is to produce a new desktop window manager and shell environment with
modern capabilities. The UI must offer rich visuals, large-scale animated
transitions and effects that can be produced only with the assistance of
hardware acceleration.

Other constraints and goals:

*   Cross platform code, should be able to build and run on Windows (and
            maybe eventually other platforms), even if we don't have an initial
            product need for them.
*   Scalable performance characteristics depending on the target
            hardware capability.
*   Provide the foundation of a flexible windowing system and shell for
            Chrome and ChromeOS on a variety of form factors.

Notable non-goals for the initial launch of this system include:

*   Multiple monitor support. (This was added later, early 2013)
*   Software rendering mode or any kind of remote desktop capability to
            the device.
*   NPAPI plugin support. This will never be required. Pepper plugins
            only will be supported.

### UI Design

*Owner: Nicholas Jitkoff (alcor@) (UX) and Kan Liu (kanliu@) (PM)*

### Quick Chrome UI Implementation Backgrounder

Chrome UI for Chrome on Windows and Chrome OS is built using the Views UI
framework that lives in src/views. The contents of a window is constructed from
a hierarchy of views. View subclasses allow for implementation of various
controls and components, like buttons and toolbars. Traditionally Chrome has
used a mix of hand-rolled controls for aspects of its user interface where a
custom look is desired, such as the browser toolbar and tabstrip, and native
controls provided by the host platform where a more conventional look is
desired, such as in dialog boxes and menus. When run on Windows, the Win32 API
provides for native controls in the form of HWNDs, and on ChromeOS, the Gtk
toolkit is used to provide native controls.

A view hierarchy is hosted within a Widget. A Widget is a cross-platform type,
and relies on a NativeWidget implementation that is specific to each host
environment to perform some duties. The NativeWidget implementation is actually
the connection to the host environment. For example, on Windows a
NativeWidgetWin wraps a HWND (via the WindowImpl class), receives Windows
messages for event handling, painting, and other tasks. In the Gtk world, a
NativeWidgetGtk wraps a GtkWidget and responds to signals. The NativeWidget is
responsible for translating platform-specific notifications into cross platform
views::Events and other types that the rest of Views code can respond to in a
platform-independent fashion.

The Chrome UI was originally written for Windows, and so despite the relatively
platform-neutral nature of the View hierarchy and much of the views code,
Win32-isms did creep in. The philosophy on the Chrome team has always been "let
not the perfect be the enemy of the good," so pathways to shorter-term success
have been emphasized. The Mac and Desktop-Linux ports of Chrome pursued a
different strategy for UI, more aggressively using the native toolkits offered
on those platforms (Cocoa and Gtk), so at the start of the Chrome OS project
there was still some considerable Win32 influence in Views code. Many of those
Win32-isms have been augmented by ifdef'ed Gtkisms.

The reliance on platform widget systems has posed a problem though in that it
prevents hardware acceleration of elements of the UI and arbitrary
transformation of UI controls. The platform native frameworks are also peculiar
in a number of ways, sharing constraints that are not relevant to desktop Chrome
or Chrome OS. Before long a desire to eradicate our usage of them grew strong
enough to begin work on doing so. An effort was spun up spanning several teams
to start by removing Gtk usage in the Views frontend code. This has become one
of the major sub-projects required for the Aura work described here.

### Platform Native Control (aka Gtk/HWND) Elimination

*Owner: Emmanuel Saint-Loubert-Bié (saintlou@)*

Gtk/HWND use is pervasive. It is used everywhere from the NativeWidget
implementations that host the View hierarchy down to individual dialog boxes.
Here are examples of work that has been done to eliminate their use:

*   Converted the Options UI to WebUI. The Options dialog boxes were
            massive platform-native constructs that used many Gtk widgets.
            Replacing the whole thing with a WebUI implementation has meant many
            fewer controls.
*   Converted other dialogs to WebUI. In general, if something shows in
            a tab or in something resembling what could be a window.open() popup
            (top level window), it is a candidate for conversion to WebUI. While
            Options was the largest conglomeration of native controls, there is
            a long tail of other smaller dialogs that contribute to our reliance
            on Gtk.
*   Written Views-based implementations of some native controls, like
            Textfield. Some places do not suit conversion to WebUI - e.g. the
            browser frame window itself. In these cases we have to write new
            Views-only (often referred to as "pure views" in code) versions of
            controls like the Textfield.
*   Written Aura-based implementations of the RenderWidgetHostView.
            Traditionally the RenderWidgetHostView has been a HWND or GtkWidget,
            and is used as a parent for windowed NPAPI plugins. Since we are
            only supporting Pepper plugins going forward, we did not need a
            native window to parent NPAPI plugins and a synthetic implementation
            could be done.

While many of the major areas have been successfully tackled, this area remains
a work in progress.

### Hardware Accelerated Rendering/Compositor

*Owner: Antoine Labour (piman@)*

At the onset of this project Chrome was using two compositors - the compositor
used by WebKit to hardware accelerate CSS transitions, and a "Browser
Compositor" run in the UI thread of the browser process, used to implement Views
transformations like whole-screen rotations.

For a number of reasons, it is desirable to unify our compositing efforts here
and provide a single compositor. The primary reason is achieving acceptable
performance on target hardware. It is necessary to have a single compositor and
draw-pass instead of two as we have now. We would also like to unify the layer
trees too at some point, although this was deemed less critical.

The Browser Compositor is implemented as implementations of a ui::Compositor
interface, such as a GL one and a D3D one. Antoine has been proceeding by
writing a new implementation that uses the WebKit-CC compositor. This way the UI
can continue to use the ui::Layer API as its render target. As mentioned, we may
eventually consolidate the API between UI and WebKit.

The compositor is a distinct component in Chrome code, consuming only gfx types,
WebKit (obviously) and other low level components. In the fullness of time the
WebKit compositor will be extracted from WebKit further so that we do not need
to drag all of WebKit into Aura and Views.

[<img alt="image"
src="/developers/design-documents/aura-desktop-window-manager/Chrome%20Graphics%20Infrastructure.png">](/developers/design-documents/aura-desktop-window-manager/Chrome%20Graphics%20Infrastructure.png)

### Aura WM and Shell

Aura

*Owner: Ben Goodger (beng@) and Scott Violet (sky@)*

To allow us to perform large scale window transitions, we need to back Windows
by compositor layers so that we can animate them without redrawing. This led to
the development of a simple window type that supported an API compatible with
(i.e. implementing the other side of the contract expected by) the Views
NativeWidget type. We had initially tried to do this with a View-backed
NativeWidget implementation (called NativeWidgetViews) called the views-desktop.
However we still needed a platform-native widget
(NativeWidgetWin/NativeWidgetGtk) to host the hierarchy. A big challenge was
that pervasive in Chrome code is the concept of a gfx::NativeView/NativeWindow,
which on Chrome OS and Windows was expected to resolve to a GtkWidget or an
HWND. This assumption is also baked into NativeWidgetWin/NativeWidgetGtk and
thus we were presented with many challenges parenting windows properly, since we
could only ever offer the top level (desktop/screen-level window) as a parent to
code that expected a NativeView, rather than a more localized (and probably more
correct) window, because a views::View couldn't be a NativeView.

This, combined with some lingering issues with large View hierarchies led to the
development of the simple aura::Window type. The aura::Window is what we
consider a NativeView/NativeWindow (it typedefs thus). In the Views system, we
have implemented a new NativeWidget targeting this type (NativeWidgetAura) that
returns the bound aura::Window from its GetNativeView() method.

The aura::Window wraps a Compositor Layer. It also has a delegate which responds
to events and paints to its layer.

aura::Windows are similar to Views, only simpler, they are a hierarchy that live
within an aura::Desktop. The aura::Desktop is bound to an aura::DesktopHost,
which is where the real platform-specific code lives. There is a DesktopHost
that wraps an HWND and one that wraps an X window. There is no Gtk in this
world. You can think of this as us having pushed the platform specific code one
layer further away from Views, out to the screen edge (as far as ChromeOS is
concerned). All windows within are synthetic. The DesktopHost window receives
the low level platform events, cracks them to aura::Events, targets them to aura
Windows, which pass them along to their delegates. On the Views side,
NativeWidgetAura is a aura::Window delegate, receives the aura::Event (which it
considers a platform native event), and constructs relevant views::Event types
to propagate into the embedded View hierarchy.

Aura is responsible for the Window hierarchy, event cracking and propagation,
and other basic window functionality (like focus, activation, etc).

Note that despite the fact that Aura is used by Views, it does not actually use
Views itself. It is at a lower level of the onion. Think of it like a raw Win32
HWND or GtkWidget.

#### The Aura Shell and Chrome Integration

*Owner: Zelidrag Hornung (zelidrag@) and David Moore (davemoore@)*

A desktop environment is much more than just basic window types. We needed a
playground to implement the higher level elements of the window manager, such as
constraint-based moving and sizing, shell features such as the persistent
launcher at the bottom of the screen, status areas, etc. Rather than build this
directly into Chrome, which is huge and takes forever to link, we decided to
build this as a separate component. Because it consists of UI components like
the launcher and custom window frame Views, it would need to depend not just on
Aura but also Views.

The product is a shell library (called aura_shell) that (eventually) we can use
in Chrome when built with We also have a test runner, called aura_shell_exe.
This instantiates the shell, and launches a few sample/example windows that
allow us to build out and test functionality. Within the shell, models for
components that would normally be populated with user data (such as apps in the
launcher) come from mocked models. When instantiated in Chrome, the real data is
provided.

The Chrome OS UI team has traditionally worked on many of these features and
people from that team will contribute heavily to this effort.

### Implementation Strategy

Since this is a complex project, there are several sub-efforts. The breakdown
above covers the main areas: Compositor, Gtk-removal, Aura and the Aura
Shell/Chrome Integration.

There is much work to be done, so we're pursuing a lot of it in parallel. While
the two-compositor system in place at the start of the project isn't something
we can put into production, it has let us start building out the Windowing
system while the single compositor work proceeds. Likewise, getting a basic
shell up and running with embedded Views widgets allows shell components like
the launcher to be started while other elements of the window system are being
designed and constructed. Similarly, Web-UI based components like the App List
can be built in Chrome behind a flag independent of any of the rest of this
work.

Since we're offering a new (native) widget system, our approach to implementing
this new UI has been to consider it a new target platform for Chrome, and our
work can be considered another "port".

You can build the code by setting use_aura=1 in your GYP_DEFINES. This should
work from Linux or Windows. This switch should define everything else necessary
to make the components above work.

### Major Revision History

*11/11/2011 - Ben Goodger and James Cook - revisions*

*10/5/2011 - Ben Goodger - initial revision*
