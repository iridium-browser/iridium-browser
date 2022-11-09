---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/accessibility
  - Accessibility for Chromium Developers
page_name: webkit-accessibility
title: Blink Accessibility
---

If you're working on Blink code, here are some things to know about
accessibility.

In Blink, accessibility is off by default. When an operating system's native
accessibility services access web content, Blink is switched into accessibility
mode, which creates a cache of a tree of AccessibilityObjects. For the most
part, the tree of AccessibilityObjects mirrors the render tree, but there are
some exceptions.

Note: this document needs to be updated to reflect the changes after the WebKit
-&gt; Blink fork.

core/accessibility/ contains a generic implementation of accessibility, and then
the mac/, win/, etc. directories contain platform-specific subclasses or
wrappers for this - classes that implement the platform accessibility API for
each node in the accessible tree. However, Chromium doesn't use any of these
platform-specific ports - instead Chromium uses a thin wrapper, defined in
WebKit/chromium/public/WebAccessibilityObject.h, that shuttles the information
to the browser process. The accessibility APIs are implemented in Chromium's
main browser process.

When adding implementations of new HTML tags, make sure they're handled properly
in WebCore/accessibility - is a new subclass needed? Is there a role that covers
this element?

When adding new features to the shadow DOM of an existing control, make sure the
shadow DOM nodes have proper accessibility annotations, like labels or titles.
Make sure anything you add is keyboard-accessible.
