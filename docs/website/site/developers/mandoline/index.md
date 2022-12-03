---
breadcrumbs:
- - /developers
  - For Developers
page_name: mandoline
title: Mandoline (deprecated according to https://codereview.chromium.org/1677293002/)
---

<https://codereview.chromium.org/1677293002/><img alt="image"
src="/developers/mandoline/Mandoline.png" height=148 width=150>

Mandoline is a project to explore an alternate to the Content layer in Chrome
using Mojo Shell for application loading & management.

We use the Mojo View and Surface systems for application hierarchy/rendering and
the Mojo HTML Viewer Blink embedding for displaying web content. We have a
simple browser shell (a la content shell).

Mandoline components are not allowed to depend on anything at the content layer
or above in the Chromium source, so where we need to reuse code we refactor into
a component with a content layer and a Mojo interface.

Sub-pages:

*   [Build, Debug & Test instructions](/developers/mandoline/build).

Bugs: [all
open](https://code.google.com/p/chromium/issues/list?can=2&q=label%3AProj-Mandoline&colspec=ID+Pri+M+Week+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)

IRC: #mandoline on freenode
