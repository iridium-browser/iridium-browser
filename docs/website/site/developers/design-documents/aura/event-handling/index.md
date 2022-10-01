---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: event-handling
title: Event Handling
---

A diagram of the architecture of this system:

[<img alt="image"
src="/developers/design-documents/aura/event-handling/arch4.png">](/developers/design-documents/aura/event-handling/arch4.png)

HWNDMessageHandler owns the WNDPROC, and is code that is shared with the
non-Aura windows build. This code is in production and "should work."

On this diagram, the new classes are DesktopRootWindowHostWin(\*\*) and
DesktopNativeWidgetAura. DesktopNativeWidgetAura is a new
views::NativeWidgetPrivate implementation that wraps an aura::Window\* hosted in
a special RootWindow/Host, which is implemented by
DesktopRootWindowHost\[Win|Linux\].

DesktopNativeWidgetAura is cross platform code, DesktopRootWindowHostWin|Linux
is platform-specific.

Basically there are a bunch of functions on these two new classes that need to
be connected together.

I've made a list of these functions here:

<https://docs.google.com/a/google.com/spreadsheet/ccc?key=0AsMZXOiSimBbdGhhTU1xQWo2QUZGRTExdWhBN2toenc>

.. along with some notes I quickly made about what I thought would be a
resolution for that method.

A couple of notes about event handling:

*   Events from the system progress through the diagram above left to
            right, i.e. received at the HWND/XWindow and are processed, maybe
            eventually making it to the Widget (and hence the View hierarchy) on
            the right.
*   Input events in aura must be dispatched by the RootWindow's
            dispatcher for correct behavior, so rather than sending them
            directly from the DesktopRootWindowHostWin to the
            DesktopNativeWidgetAura, we send them into the DRWHW's RootWindow.
            They'll wind up being received by the DNWA's aura::Window via its
            WindowDelegate.
*   Other types of events can perhaps be sent directly from the DRWHW to
            the DNWA.

*   On the other side, the NativeWidgetPrivate interface provides the
            API that views::Widget expects the native widget to perform, and
            this direction goes right-to-left on the diagram above. The typical
            flow here is through DNWA (which implements NWP) and through a
            DesktopRootWindowHost interface (we can add new methods to this as
            needed) which DRWHWin|Linux implements.

*   Sometimes there is a conflict on a method name between one on
            aura::RootWindowHost, which DRWHW|L also implements. In this case
            you can manual RTTI the DRWH to RWH.

*   Some methods in DRWHW are never sent back to the DNWA, e.g. those
            that are windows-specific and are for example completely implemented
            by NativeWidgetWin. I tend to look at NativeWidgetWin for
            inspiration when figuring out this kind of thing.
