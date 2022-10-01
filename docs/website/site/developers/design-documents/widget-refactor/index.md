---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: widget-refactor
title: Widget Refactor
---

### Background

A view hierarchy is hosted by a views::Widget subclass. This subclass (e.g.
WidgetWin or WidgetGtk) receives native events from the operating system, cracks
them into views-specific types and propagates them into the view hierarchy.

The current arrangement poses some challenges that make porting to a new native
toolkit problematic:

*   The native Widget implementations are very thick. There is a lot of
            duplicated code that makes fixing bugs cumbersome.
*   Swapping native Widget implementations is problematic because they
            are stateful. The state is meaningful and may not have good analogs
            in other implementations.

### Details

sdfsdf
