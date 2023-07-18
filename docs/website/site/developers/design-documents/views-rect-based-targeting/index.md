---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: views-rect-based-targeting
title: Rect-based event targeting in views
---

Author: Terry Anderson (tdanderson at chromium)

**Problem**

Views (tabs, tab close buttons, bookmark star, etc.) formerly only supported
point-based event targeting, which meant that targeting for gestures only
considered the center point of the touch region and not its size. Treating
gesture events in the same manner as mouse events when determining the event's
target can lead to a poor experience for touchscreen users because a touch
region can overlap many more possible targets than a mouse cursor and it is
difficult to ensure that the center of the touch region is over the intended
target (especially if that target is small, such as the tab close button). The
goal of this feature is to add support for rect-based event targeting
("fuzzing") by using a heuristic to determine the most probable target of a
gesture having its touch region represented by a rectangle.

**Solution**

This heuristic is implemented in **View::GetEventHandlerForRect()** with the
idea that small targets are more difficult to touch reliably than large targets,
so small targets should get priority when determining the most probable target
of a gesture. The heuristic works by recursively looking for candidate targets
among the descendants of **this** which the touch region overlaps by at least
60%. Among all such candidates, we return the target which has its center
point\[1\] closest to the center point of the touch region. If no candidates for
rect-based targeting exist, we instead return the target that would have been
returned if point-based event targeting were used with the center point of the
touch region.

Point-based targeting still uses **View::GetEventHandlerForPoint(gfx::Point)**,
but this function has been made non-virtual and simply calls into the virtual
function **GetEventHandlerForRect(gfx::Rect)** with a unit rect (i.e., having
size 1x1). The implementation of **View::GetEventHandlerForRect()** and its
overrides has preserved the existing behavior of point-based targeting in this
case. Otherwise, if called with a non-unit rect, the overrides of
**GetEventHandlerForRect()** call directly into
**View::GetEventHandlerForRect()**, with two exceptions mentioned in the "Future
work" section below.

See
[r232891](https://src.chromium.org/viewvc/chrome?view=revision&revision=232891)
for the implementation details.

**Future work**

The algorithm does not seem to perform as well for touchscreens which do not
report their own radius values (as compared to touchscreens which do); see
[issue 315795](https://code.google.com/p/chromium/issues/detail?id=315795).
Furthermore, rect-based event targeting is not yet implemented for
**AutofillDialogViews** or **NotificationView**.

**Metrics**

UMA metrics have been added to the **Ash.GestureTarget** histogram to give a
rough estimation of how difficult it is for users to successfully tap on views
elements (see
[r209263](https://src.chromium.org/viewvc/chrome?revision=209263&view=revision)).
These metrics specifically focus on successful taps (where the tap resulted in a
user-visible effect) and unsuccessful taps (where the tap had no visible effect)
on the tabstrip and surrounding regions.

Successful taps counted:

    GESTURE_TABSWITCH_TAP: A tap on a currently unselected tab (i.e., the tap
    was responsible for selecting the tab).

    GESTURE_TABCLOSE_TAP: A tap on a tab's close button.

    GESTURE_NEWTAB_TAP: A tap on the new tab button.

    GESTURE_FRAMEMAXIMIZE_TAP: A tap on the frame maximize button.

    GESTURE_MAXIMIZE_DOUBLETAP: A double-tap on the browser window resulting in
    a maximize or restore action.

Unsuccessful taps counted:

    GESTURE_TABNOSWITCH_TAP: A tap on a currently selected tab.

    GESTURE_FRAMEVIEW_TAP: A tap on the space to the right of the tabstrip, left
    of the tabstrip, or in between tabs\[2\].

    GESTURE_ROOTVIEWTOP_TAP: A tap on the top edge of a browser window.

The effectiveness is computed as the ratio of successful taps to unsuccesful
taps.

This measurement is not perfect because it does not take into account cases
where a tap was classified as successful but had an unintended result (for
example, tapping with the intent of closing a tab but instead having a new tab
open). Measuring these types of cases would require one or both of the
following:

    Adding new metrics that track pairs of actions which occur with a very small
    delay in between. For example, a tap on a tab close button followed almost
    immediately by a Ctrl-Alt-T to restore the most recently closed tab may
    indicate that closing the tab was not the user’s intent.

*   Conducting a user study where participants execute a set of tasks
            (e.g., “close the second tab”, “open a new tab”) and we keep track
            of how many taps were required to accomplish each one.

**Footnotes**

\[1\] Originally the distance to the *center line* (the line that remains after
repeatedly removing 1px borders from the rectangle) of the view was used in
order to prevent bias against wide/tall rectangular targets. But in
[r241955](https://src.chromium.org/viewvc/chrome?revision=241955&view=revision)
this was changed to instead use the center point to fix an edge case and also
because wide/tall targets would never actually be candidates for rect-based
targeting due to their size relative to a touch region.

\[2\] Each double-tap to maximize or restore the window contributes 2 to the
value of this count, and each single tap also contributes 2 to the value of this
count. Thus when interpreting the results of the data, the value of
GESTURE_FRAMEVIEW_TAP first needs to be adjusted as (GESTURE_FRAMEVIEW_TAP - (2
\* GESTURE_MAXIMIZE_DOUBLETAP)) / 2.
