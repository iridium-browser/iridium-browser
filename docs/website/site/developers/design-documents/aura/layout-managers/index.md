---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: layout-managers
title: Layout Managers
---

LayoutManager is a convenient abstraction that separates messy layout heuristics
from the other particulars of a Window.

[<img alt="image"
src="/developers/design-documents/aura/layout-managers/LayoutManager.png">](/developers/design-documents/aura/layout-managers/LayoutManager.png)

A LayoutManager implementation attached to an Aura Window can control the layout
(bounds, transform, and even other properties) of that Window's children.

A LayoutManager is typically attached to a window at creation/initialization
time. It is notified when:

*   a child window added or removed, or the visibility of a child
            changes. This type of event may cause other child windows to be
            re-laid out.
*   the size of the bound Window changes, which may cause the children
            to be re-laid out to accommodate the new viewport size.

A LayoutManager implementation can be used for everything from simple behavior
like keeping the application launcher docked to the bottom of the screen to
complex layout heuristics for multiple windows across several virtual
workspaces.
