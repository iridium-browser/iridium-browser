---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: status-bubble
title: Status Bubble
---

[TOC]

## Overview

Chromium does not have a status bar - to show the target of a link or page load
status, we use a fixed-width bubble that floats over the content when necessary.

[<img alt="image"
src="/user-experience/status-bubble/status_bubble_hover.png">](/user-experience/status-bubble/status_bubble_hover.png)

## Timing and behavior

The status bubble behavior in the link-hover case is described below.

### Mouseover

1.  On mouse over link, begin Timer A.
2.  When Timer A expires, change the bubble text to the link value and
            begin fading in.
3.  If mouse moves out while fading in, begin fading out.
4.  When fade-in completes, show the bubble at full opacity.

### Mouseout

1.  On mouseout, begin Timer B.
2.  If the user mouses over another link, change the bubble text to the
            value of the new link instantly. Go to step 4 of **Mouseover**,
            above.
3.  Otherwise, if Timer B expires:
    1.  Begin fading out until fade-out completes.
    2.  Or if mouse over again, change content and resume fade-in.

This ensures that the bubble does not flicker in and out as the user's mouse
traverses many links, and allows users to see the targets of multiple links
quickly (compare to a tooltip solution). Timer A should be high enough so that
if the user's cursor doesn't accidentally trigger the bubble in normal
navigation, but low enough so that a user who wants to see a link is not kept
waiting - generally with a slower fade-in, Timer A can be pretty low.

## Size

The status bubble's width is fixed to a percentage of the width of the window -
while this results in link truncation, it prevents resizing as the user moves
the mouse over multiple links. We try to keep the bubble under half the width of
the window so that the bubble feels anchored to a corner, making that corner the
obvious target for the user's eyes.

## (Lack of) Security

Note that the status bubble is [trivial to
spoof](https://garron.net/web/spoof-link/) using a little CSS.

In addition, it is also trivial to swap out the link after the user has hovered
over it and pressed it, but before navigation.

Therefore, the status bubble is just a convenience for users hovering over links
on well-behaved sites, but it is **not** a security surface.
