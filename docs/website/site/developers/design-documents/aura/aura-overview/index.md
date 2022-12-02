---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: aura-overview
title: Aura Overview
---

[<img alt="image"
src="/developers/design-documents/aura/aura-overview/Aura.png">](/developers/design-documents/aura/aura-overview/Aura.png)

From the perspective of the user, Aura provides Window and Event types, as well
as a set of interfaces to customize their behavior.

The root of a Window hierarchy is the RootWindow. This type is responsible for
event dispatch.

An Aura window hierarchy is embedded within a native window provided by the
underlying platform (XWindow, HWND). This native window is encapsulated in a
RootWindowHost implementation for that specific platform. This host receives
native events from the underlying system, cracks them into aura::Event
subclasses and sends them to the RootWindow for dispatch.

In many ways the Aura architecture mimics that of Views, though it is a little
simpler.

### Event Flow

[<img alt="image"
src="/developers/design-documents/aura/aura-overview/EventFlow.png">](/developers/design-documents/aura/aura-overview/EventFlow.png)

This diagram shows the flow of events through each stage of the Aura system.

1.  The RootWindowHost receives a base::NativeEvent (MSG/XEvent\*) from
            the underlying platform.
2.  The RootWindowHost cracks this message to an aura::Event\* type and
            forwards it to the RootWindow for further processing and dispatch.
3.  Depending on the event type, a target window for event dispatch is
            determined for the event. See notes on dispatch below.
4.  Every registered aura::EventFilter\* from the target window up to
            and including the RootWindow is given an opportunity to pre-handle
            the event. An EventFilter implementation will return true if it
            wants to consume the event and stop further processing. EventFilter
            is a powerful API to intercept events before the target Window gets
            a chance to handle them. It can be used by a client to implement
            features like window movement, modality, and so on.
5.  Finally if no EventFilter intercepts the event, it is passed
            directly to the target window's delegate. The Window itself never
            sees the event directly, since window customization in Aura is done
            entirely through delegate interfaces like EventFilter,
            WindowDelegate, etc.
