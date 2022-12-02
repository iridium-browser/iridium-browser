---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: gesture-recognizer
title: Gesture Recognizer
---

# Gesture Recognizer Overview

This document describes the process by which Touch Events received by a
WindowEventDispatcher are transformed into Gesture Events.

[TOC]

## Touch Events

*   Press
*   Move
*   Release
*   Cancel: fired on preventDefault'ed release, or when we need to
            truncate a touch stream (i.e. when something else takes window
            capture)

## Gesture Events

Gesture Events contain type specific data in their GestureEventDetails object.
See
[here](https://docs.google.com/document/d/1mQFx99xz5jVUHQk2jZTjHPgMjRQd4EUDfdLG0XKOlWI/edit)
for a summary of the typical event sequences.

*   Tap Down
    *   Fired when the first finger is pressed.
*   Show Press
    *   Fired on a delay after a tap down if no movement has occurred,
                or directly before a tap if the finger is released before the
                delay is finished.
    *   Triggers the active state.
*   Tap
    *   Fired after a press followed by a release, within the tap
                timeout window.
    *   Has a "tap_count" field, indicating whether or not this is a
                single, double or triple tap.
*   Tap Cancel
    *   Eventually fired after every Tap Down which doesn't result in a
                tap.
*   Double Tap
    *   Only fired on Android, where it is used for double tap zooming.
*   Tap Unconfirmed
    *   Only fired on Android.
    *   Occurs whenever a tap would occur on Aura.
    *   On Android, tap is fired after a delay, to ensure the current
                gesture isn't a double tap.
*   Long Press
    *   Fired on a delay after a tap down, if no movement has occurred.
*   Swipe
    *   Some number of fingers were pressed, and then at least one
                finger was released while all fingers were moving in the same
                direction.
    *   Contains the direction the fingers were moving in.
*   Scroll Begin
    *   Contains the offset from the tap down, to hint at the direct the
                scroll is likely to be in.
*   Scroll Update
    *   Contains the scroll delta.
*   Scroll End
    *   Fired when a finger is lifted during scroll, without enough
                velocity to cause a fling.
*   Fling Start
    *   Fired when a finger is lifted during scroll, with enough
                velocity to cause a fling.
*   Fling Cancel
    *   If we're in a fling, stop it. This is synthesized and dispatched
                immediately before each tap down event.
*   Pinch Begin
*   Pinch Update
    *   Associated scale.
*   Pinch End
*   Two Finger Tap
    *   Two fingers were pressed, then one was released, without much
                finger movement.
*   Begin
    *   A finger was pressed. You can figure out how many fingers are
                down now using event.details.touch_points().
*   End
    *   A finger was released. You can figure out how many fingers are
                down now using event.details.touch_points().

## Terminology

*   Touch Lock:
    *   The association between an existing touch and its target.
*   Touch Capture:
    *   If a window is capturing touches, new touches will be targetted
                at the capturing window.
*   Touch Id:
    *   The global id given to each touch. Contiguous starting from 0
                for each display.
*   Rail Scroll:
    *   A scroll which is locked to only move vertically or
                horizontally.
*   Fling:
    *   When scrolling, if the user releases their fingers, but the
                scroll should continue, this is known as a fling.

# Finding a Target

## Possible ways of establishing a target

1.  If a touch is already locked to a target, keep it.
2.  If a window is capturing touches, target that window.
3.  If it's outside the root window, target the root window.
4.  If it's near another touch, use the other touch's target.
5.  Otherwise, use whichever window the touch is above.

In the following images, a filled in circle represents a touch down, and the
outline of a circle represents a position the touch has moved to. The dotted
green line indicates the window that a touch is targeted at. The rounded
rectangle represents a window, and the outer rectangle represents the root
window.

<table>
<tr>
<td>1. If a touch is already locked to a target, keep it. <img alt="image" src="https://lh6.googleusercontent.com/myjxUQXxaThmV7EUoCfCXkyg0YVCFgdlQKO3gpRdynZ4lSMyJGzLnPdPRDe2qBa_qV6gkKxKn_Ghmv43nVY6SUJr8eb_IOevWjtNvBrCn4OGpm7yo7E" height=214px; width=223px;></td>
<td>2. If a window is capturing touches, target that window. </td>
<td><img alt="image" src="https://lh6.googleusercontent.com/JyqtKa63Qh9yRhlmok8LDLeA2eiKK-CsrKROfmS4SsT7Wign6pkQqMiz0VnPBco5tn3MbGLhe4U--avCCeNB-jSGVUU4YTAJSXkg-ay1KLPrJn6mxwo" height=214px; width=223px;></td>
</tr>
<tr>
<td>3. If it's outside the root window, target the root window.</td>
<td><img alt="image" src="https://lh5.googleusercontent.com/YwO3KZjo5CLMQ-_XNOplU4a9YQtIMi1pLqtOZAcX6RLEnGtJ5D4M0Vxjj83RxPAwd8Hp0-_VCc6DKE9melGnYs1kybpxOB9Xdo-zwCV9-XK_MT-hhpQ" height=214px; width=265px;> </td>
<td>4. If it's near another touch, use the other touch's target. </td>
<td><img alt="image" src="https://lh3.googleusercontent.com/erpsfYRchXsW5q3Al2rSqngixvmYX_q4hjMaSd96Ak1OCQ89jXE6jsdJnF1cFdgNhIH8tIKxYt60o10mqyjeYZ4nx3trlaWhqRkkK61Y61JNmNg6O6I" height=214px; width=222px;></td>
</tr>
<tr>
<td>5. Otherwise, use whichever window the touch is above. <img alt="image" src="https://lh4.googleusercontent.com/rc5RUdxi_QKrsAYU_DF1v9sVmZyAbYzFQDTUrCsiKVshb4dB7338BwT-bPDpUH1btF5UrrzlzkfgCA8I7qXDes0njRZARTW1qC-_fM9vP5R6E7F1LrQ" height=214px; width=223px;></td>
</tr>
</table>

## Complications

### preventDefault’ed Release

When Javascript calls preventDefault on a touch release, we need to make sure
that no gesture events are created, but we also need to ensure that the
GestureRecognizer knows that the associated finger isn’t down any more. To
accomplish this, we turn the touch release into a touch cancel event.

### Touch Capture

When a touch capture occurs, all touches on windows other than the window
gaining capture need to be ignored until they are released. To do this, we fire
a touch cancel for each of these touches.

# Generating Gesture Events

Each touch-id has an associated window (See
GestureRecognizerImpl::touch_id_target_), and each window has an associated
GestureProviderAura.

GestureProviderAura contains a FilteredGestureProvider, which is responsible for
performing gesture detection on both Android and Aura.

# Where To Start

[GestureRecognizerImpl](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/gestures/gesture_recognizer_impl.h&sq=package:chromium)

*   Responsible for mapping from touches to windows, and hosts the
            [GestureProviderAura](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/gestures/gesture_provider_aura.h&q=GestureProviderAura&sq=package:chromium&l=26)
            objects.

[GestureDetector](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/gesture_detection/gesture_detector.cc&q=gesture_dete&sq=package:chromium&l=1),
[ScaleGestureDetector](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/gesture_detection/scale_gesture_detector.cc&sq=package:chromium)

*   Take in touches, and fire callbacks for each gesture.

[GestureProvider](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/gesture_detection/gesture_provider.h&q=GestureProvider)

*   Hosts the
            [GestureDetector](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/gesture_detection/gesture_detector.cc&q=gesture_dete&sq=package:chromium&l=1)
            and
            [ScaleGestureDetector](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/gesture_detection/scale_gesture_detector.cc&sq=package:chromium).
*   Constructs
            [GestureEventData](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/gesture_detection/gesture_event_data.h&q=GestureEventData&sq=package:chromium&l=17)
            objects, which are turned into
            [GestureEvent](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/event.h&q=GestureEvent&sq=package:chromium&l=654)
            objects in
            [GestureProviderAura](https://code.google.com/p/chromium/codesearch#chromium/src/ui/events/gestures/gesture_provider_aura.h&q=GestureProviderAura&sq=package:chromium&l=26).
