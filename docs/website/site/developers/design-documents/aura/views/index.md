---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: views
title: Views
---

Views is a user interface framework built on a type called, confusingly, View.
Responsible for providing the content of our Aura windows, most of this code is
also used by Desktop Chrome on Windows. Only the NativeWidget implementation
differs (for now). Views also provides a set of useful reusable controls like
Buttons, Menus, etc.

[<img alt="image"
src="/developers/design-documents/aura/views/Views.png">](/developers/design-documents/aura/views/Views.png)

Views is somewhat analogous to Aura, with a native host (NativeWidget
implementations), a View hierarchy within a RootView that handles event
dispatch.

The primary difference is that this system was designed and built before beng
developed a healthy skepticism of is-a relationships, and so you accomplish
rendering, event handling and layout here by subclassing View, rather than
implementing a delegate interface as in Aura.
