---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: multitouch
title: Multitouch
---

**UI under development. Designs are subject to change.**

[TOC]

## Trackpad Multitouch (Indirect manipulation)

### Gesture Types

> [<img alt="image"
> src="/user-experience/multitouch/GesturesImages.png">](/user-experience/multitouch/GesturesImages.png)

*   1 finger: traditional pointing
*   2 finger: traditional scrolling
*   3 finger horiz.: three fingers in a row (usually ring/middle/index)
*   4 finger horiz.: four fingers in a row (usually
            pinky/ring/middle/index) - may be hard to disambiguate horizontal
            swipes on small trackpads, so we will overlap many gestures with the
            three finger variants
*   1 finger edge: one finger moving in and out of the trackpad bounds
    *   May require hardware support.
    *   Vertical gestures may be problematic because of limited space.
*   3 finger group: three fingers in a circle (usually
            thumb/middle/index \[or ring\])
    *   Due to people resting their thumb on the trackpad, many of these
                gestures are problematic, particularly pinch and rotate
                gestures. Swipe gestures are generally pretty usable.
*   4 finger group: four fingers in a circle (usually thumb +
            ring/middle/index)
    *   Problematic as above. Swipe gestures are acceptable.

### Proposed behaviors

> #### Main view

    *   **one finger** — cursor
    *   **two** fingers — content manipulation
        *   Sites can override all two finger gestures passed to the
                    content area
        *   Pinch - defaults to page zoom
        *   Rotate - defaults to nothing
        *   Translate - defaults to scroll
    *   **three** fingers horizontal
        *   tab switch
    *   **three** fingers up — into overview
    *   **three** fingers down — focus panels
    *   **four** fingers up — into overview
    *   **four** fingers down — focus panels
    *   **four** fingers horizontal — window switching
    *   **three** finger group translate — back/forward or tab switch
                (reverse of horizontal's behavior)
    *   **three** finger group rotate — problematic because of two
                finger with anchored thumb
    *   **one** finger left/right edge in — back/forward
    *   **one** finger left/right edge out — forward/back, or sidebar
                reveal
    *   **one** finger top edge in — overview
    *   **one** finger bottom edge in — panels

> #### Overview

    *   **two** fingers horizontal — switch tabs
    *   **two** fingers up/down? — close tab
    *   **three** fingers — switch windows
    *   **four** fingers — window switching
    *   **three** fingers down — out of overview
    *   **four** fingers down — out of overview
    *   **three** or **four** fingers up — logout

> #### Panels

    *   **two** fingers = scroll current panel
    *   **three** fingers horizontal = switch panels
    *   **three** fingers up = back to main view

## Full Multitouch (Direct manipulation)

*   All content area gestures (two and one finger) passed directly to
            web pages
*   Default behaviors for one and two finger gestures as above
*   We may reserve the above 3/4 finger gestures for os needs:
    *   three fingers — window movement/resizing
    *   four fingers — overview
