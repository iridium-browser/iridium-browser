---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: wrench-menu-mac
title: Wrench Menu (Mac)
---

Chrome 6 introduces a new Wrench menu that unifies it with the old Page menu. To
save space and eliminate clutter, the UI leads decided to merge common elements
into button items. Cocoa allows putting custom items in menus using
-\[NSMenuItem setView:\], but none of the typical menu interactions that users
expect are provided. Missing are hover states, menu closing on selection, and
non-sticky mode. Menus on Mac OS X have two modes of interaction; the first is
typical of other platforms: click to open and click to select an item. The other
is non-sticky mode, where the user can click the menu open and, without
releasing, drag to the desired item, and release to select it. Custom buttons
were written to implement all these behaviors, as well as to change their
appearance.

Another feature of the menu is to be able to use the zoom buttons and have the
page update while the menu stays open. The issue in implementing this was that
after the zoom button messages the renderer to zoom, the acknowledgment
(containing the actual zoom level) comes back asynchronously to the I/O thread,
which then forwards the Task to the UI thread's main event loop. When a menu is
running, though, a nested event loop is run that blocks that UI event loop. To
fix this, the callback on the IPC thread is special-cased to post the Task in a
way that both the modal menu loop and, if a menu is not running, the main loop
can process it.

[<img alt="image"
src="/developers/design-documents/wrench-menu-mac/WrenchMenuTaskDispatch.png">](/developers/design-documents/wrench-menu-mac/WrenchMenuTaskDispatch.png)
