---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: rendering-architecture-diagrams
title: Rendering Architecture Diagrams
---

The following sequence diagrams illustrate various aspects of Chromium's
rendering architecture. The first picture below shows how Javascript and CSS
animations are scheduled using the requestAnimationFrame callback mechanism.

[<img alt="image"
src="/developers/design-documents/rendering-architecture-diagrams/chromium-request-anim-frame.png"
height=300
width=400>](/developers/design-documents/rendering-architecture-diagrams/chromium-request-anim-frame.png)

After the Javascript callback is executed or the CSS animations have been
updated, the web contents view generally needs to be redrawn. The following
(simplified) diagrams show the code execution flow during a repaint in the
non-composited software rendering, composited software rendering and (threaded)
composited GPU rendering modes. Note that the newer [multithreaded ("impl-side")
rasterization mode](/developers/design-documents/impl-side-painting) is not
shown below.

<table>
<tr>
<td>Non-composited SW rendering</td>
<td> Composited SW rendering</td>
<td> Composited GPU rendering</td>
</tr>
<tr>

<td><a
href="/developers/design-documents/rendering-architecture-diagrams/chromium-non-composited-sw-rendering.png"><img
alt="image"
src="/developers/design-documents/rendering-architecture-diagrams/chromium-non-composited-sw-rendering.png"
height=165 width=200></a></td>

<td><a
href="/developers/design-documents/rendering-architecture-diagrams/chromium-composited-sw-rendering.png"><img
alt="image"
src="/developers/design-documents/rendering-architecture-diagrams/chromium-composited-sw-rendering.png"
height=400 width=243></a></td>

<td><a
href="/developers/design-documents/rendering-architecture-diagrams/chromium-composited-hw-rendering.png"><img
alt="image"
src="/developers/design-documents/rendering-architecture-diagrams/chromium-composited-hw-rendering.png"
height=400 width=245></a></td>

</tr>
</table>
