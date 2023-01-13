---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: tabtastic-2-requirements
title: Tabtastic 2 Requirements
---

This page is a collection of requirements for the third generation Chrome
tabstrip implementation.

*   live tab contents during dragging
*   full size dragged representations
*   frame fade transition during detach/attach
*   dragged representation places tab at relevant position in tabstrip
            (from jar)
*   ability to resume drag in tabstrip after attach
*   ability to resume drag in tabstrip after move from overflow UI
*   overflow UI capability
*   multiselect for drag and close
*   independent continuous animations (View::GetAnimator)
*   generic TabStrip baseclass that does not deal with TabContentses
*   BrowserTabStrip subclass that interacts with TabStripModel (and
            knows about TabContentses)
*   testing for Layout
*   general testable interface (View-Controller separation)
*   tests for drag and drop using testable interface
*   windows 7 integration for drag and drop targets at screen edges in
            lieu of the current Dock system that we use on Vista and below.

P2s/3s:

*   pinned tabs
*   tab coloring

Implementing a system capable of supporting all of this will require some tweaks
to TabStripModel and a replacement of the existing Tab/TabRenderer/TabStrip/etc
system.

We should avoid adding new features/capabilities to the current tabstrip until
this system is implemented
