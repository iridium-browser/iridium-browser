---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
page_name: paint-worklet
title: Paint Worklet
---

Paint Worklet, also known as the [CSS Paint
API](https://developers.google.com/web/updates/2018/01/paintapi), is an effort
to enable web developers deeper control of how elements on their page are
painted. More details can be found in the
[spec](https://www.w3.org/TR/css-paint-api-1/), but in short the API allows
developers to specify a custom JavaScript function for any CSS attribute that
can take a CSS
[&lt;image&gt;](https://developer.mozilla.org/en-US/docs/Web/CSS/image) type.
When the particular part needs to be painted, the JavaScript function is called
with a
[PaintRenderingContext2D](https://www.w3.org/TR/css-paint-api-1/#paintrenderingcontext2d)
that can be drawn into:

paint(ctx, size) { ctx.fillStyle = 'green'; ctx.fillRect(0, 0, size.width,
size.height); }

This functionality first shipped in [Chrome
M65](https://developers.google.com/web/updates/2018/03/nic65).

Off-Thread Paint Worklet

Off-Thread Paint Worklet ([design
doc](https://docs.google.com/document/d/1USTH2Vd4D2tALsvZvy4B2aWotKWjkCYP5m0g7b90RAU/edit?ts=5bb772e1#heading=h.2zu1g67jbavu))
is an ongoing effort (as of 2019/01) to run the developer-provided paint
functions on a different thread than the main rendering loop, and asynchronously
from the standard document lifecycle. The goal is performance isolation, to
allow smooth animation of Paint Worklet-painted elements even when the main
thread is busy. This is considered an architectural improvement project and
should not be visible to web developers (other than in improved performance of
pages that use Paint Worklets).
