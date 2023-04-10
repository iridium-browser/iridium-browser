---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: tab-strip-mac
title: Tab Strip Design (Mac)
---

This document describes the design of the tab strip on Mac OS X including tab
layout, tab drag and drop, and the tab animations.

## Classes

The following classes make up the most important pieces of the implementation of
the tab strip:

#### TabContents

The cross-platform representation of the contents of a tab. There is a
one-to-one correspondence between open tabs and a TabContents object.

#### TabStripModel

The cross-platform model for the contents of the tab strip. There is one
TabStripModel per Browser object. It contains the logic for manipulating
addition and removal of tabs, as well as more complicated features such as
"close all tabs to right". Individual tabs are referenced by index,
\[0..count()\].

#### TabView

The NSView responsible for drawing the curved edges of a tab. Also contains all
the logic for dragging a tab within and between windows.

#### TabController

The NSViewController for managing a single tab in the tab strip. It coordinates
a close box, a title and an "icon view", a client-specified NSView that can be
changed, for example, to show a favicon when the page is at rest and a spinner
view when the page is loading. The default is a NSImageView.

#### TabContentsController

The NSViewController for managing the web contents of an individual tab,
coordinating the native NSViews for a specific TabContents.

#### TabStripController

The Objective-C controller, Mac specific, for managing the interaction between
the TabStripModel and the Cocoa TabViews that comprise the user interface.
Handles

positioning and layout of the TabViews as well as the animations related to tab
drag and drop and tab opening/closing. The controller is responsible for
switching the visible tab to the correct TabContents/TabContentsController based
on changes either in the TabStripModel or in response to a user clicking in a
specific TabView. The controller handles changes from the tab model by
registering a C++ observer.

To match the model with the corresponding views, the controller keeps several
parallel arrays which are normally in sync with the indices of the
TabStripModel, but as described below in the section on animations, they
sometimes are not. It is very important to not confuse the two array indexes and
always use the conversion methods.

#### TabWindowController

The NSWindowController for a window that participates in dragging a tab between
windows. It has the concept of a tab strip and a view that gets switched out
when the tab selection changes. The controller also maintains an "overlay
window" used during tab dragging so that parts of the window frame can be
translucent. It should be reusable in other projects as there is nothing
browser-specific about the API.

## Tab Layout

Tab layout is handled primarily by a single method,
-layoutTabsWithAnimation:renegerateSubviews:. This method performs several
critical tasks:

*   Positioning, sizing, and animating the tabs, including not using all
            available space if rapidly closing tabs
*   Positioning and animating the New Tab Button (NTB), including
            showing and hiding it based on the status of a drag.
*   Handling the view z-ordering so tabs appear with a consistent
            overlap
*   Inserting an empty placeholder space during drag and drop
*   Animating a newly inserted tab

Tab layout can be done both with and without animation. For example, there's no
need to animate tabs before the initial layout has completed or to animate the
first tab in a window. In general, however, any changes caused by user
interaction or model changes are done with animations enabled.

Tabs overlap by a small amount (about 20px) which means z-order becomes
important. If not paid attention to, switching tabs and bringing them forward
over other tabs can cause a haphazard appearance over time as the z-order
becomes inconsistent across the tab strip. In order to address this, the
subviews (TabViews) are removed and re-inserted into the view hierarchy in the
correct z-order based on the regenerateSubviews parameter. Regenerating the
views needs to be done when the tab model or selection changes, but doesn't need
to be done when only the size and position of the tabs change (for instance when
the window is resized).

In order to size and position tabs correctly, the first order of business is to
compute how much space is available which may not be the entire width if the
user is quickly closing tabs. When the user closes a tab using the tab's close
button, tabs to the right should shift left such that the next tab's close
button ends up directly under the mouse. This allows the user to continue
clicking rapidly to close a large number of tabs. A requirement for this to work
is that the tabs do not change width until the user is done (signified by moving
the mouse outside of the tab strip). As a result, during "rapid closure mode"
the amount of available space for determining the width of tabs must remain
consistent and thus the tabs won't change size even though their positions
change during re-layout.

The available width is also affected by (zero or more) pinned (fixed-width)
tabs, as well as compensating for the amount of overlap between tabs. Finally,
space is reserved for both the NTB and the window close/minimize/zoom controls.
After space is allocated to pinned tabs, the remaining space is divided evenly
among the remaining tabs, ensuring that tabs fit within a min and max width.
When tabs get too small, it's the responsibility of the TabController to handle
properly displaying only the elements that fit, such as hiding the title and
only displaying the icon. Note that selected tabs have a slightly larger minimum
width than unselected tabs, allowing enough room to display the close button.

Tab layout is effectively a big for-loop that walks the array of TabControllers,
laying them out in order from left to right with a bit of overlap. The size and
positions are set on the corresponding TabView, or its animator if animations
are requested. In they are, there is also some additional pre-flighting to avoid
redundant calls to the animator when the view isn't moving. Once the tabs have
been positioned, layout places the NTB at the far right, showing or hiding based
on if a drag is in progress.

During drag and drop, there's one more component to tab layout: the placeholder.
A "placeholder tab" is an empty space representing where the tab would go were
it to be dropped at the mouse location. Obviously, there can only be one of
these at a time. If there is a placeholder tab (see below for how it's
instantiated), leave a blank space corresponding to the width of the tab being
dragged before moving on to positioning the next tab. Note that this width can
be different than the width of tabs in the current window if the dragged tab
comes from another window. Even though a wider placeholder might cause tabs at
the far right to spill off the edge, it makes for a much improved user
experience as the gap can be better associated with the dragged tab.

## Tab Animations

There are 3 causes of animation in the tab strip:

*   Tabs moving or resizing in response to model changes or the window
            resizing
*   A new tab
*   A closing tab

Moving and resizing is described above in the section on layout, handled simply
by sending the new frame size and position to the TabView's animator. In
addition, handling a new tab is actually just a special case of layout. When a
tab is identified as being new (new tabs are not visible by default), its
TabView is made visible and positioned just below the tab strip. Then, as a part
of normal layout, its animator is told the final location and it "submarines"
into place.

In general, there is a one-to-one correspondence between TabControllers,
TabViews, TabContentsControllers, and the TabContents in the TabStripModel. In
the steady-state, the indices line up so an index coming from the model is
directly mapped to the same index in the parallel arrays holding the views and
controllers. This is also true when new tabs are created (even though there is a
small period of animation) because the tab is present in the model while the
TabView is animating into place. As a result, nothing special need be done to
handle "new tab" animation.

This all goes out the window with the "close tab" animation. The animation kicks
off in -tabDetachedWithContents:atIndex: after receiving the notifiation that
the tab has been removed from the model. The simplest solution at this point
would be to remove the views and controllers as well, however once the TabView
is removed from the view list, the tab z-order code takes care of removing it
from the tab strip and there will be no animation. That means if there is to be
any visible animation, the TabView needs to stay around until its animation is
complete. In order to maintain consistency among the internal parallel arrays,
this means all structures are kept around until the animation completes. At this
point, though, the model and our internal structures are out of sync: the
indices no longer line up. As a result, there is a concept of a "model index"
which represents an index valid in the TabStripModel. During steady-state, the
"model index" is just the same index as the parallel arrays (as above), but
during tab close animations, it is different, offset by the number of tabs
preceding the index which are undergoing tab closing animation. Therefore, the
caller needs to be careful to use the available conversion routines when
accessing the internal parallel arrays (e.g., -indexFromModelIndex:). Care also
needs to be taken during tab layout to ignore closing tabs in the total width
calculations and in individual tab positioning (to avoid moving them right back
to where they were).

In order to prevent actions being taken on tabs which are closing, the TabView
itself gets marked so it no longer will send back its select action or allow
itself to be dragged. In addition, drags on the tab strip as a whole are
disabled while there are tabs closing.

When the CAAnimation for the tab close completes, it notifies its delegate,
TabCloseAnimationDelegate. The delegate messages the TabStripController to
remove the tab from all internal data structures and the indices are once again
all back in sync.

## Tab Drag and Drop

## Tab dragging is one of the more complex aspects of the tab strip and its related classes, but most of the work is actually done within TabView, not the tab strip's controller. There are three basic phases to the dragging, handled by TabView's mouseDown:, mouseDragged:, and mouseUp:, but the main driver is a loop within mouseDown: that runs until the user releases the mouse button. mouseDown: handles both drags and regular clicks on a TabView. A simple click sends the view's action to its target, which selects the appropriate tab by updating the model. While the mouse button is down but is moving, the TabView repeatedly calls mouseDragged:. Once the mouse button is released, it calls mouseUp: to clean up and reset for next time.

The first order of business when dragging a tab is determining if the user wants
to move the entire window or if they want to drag one tab among many. The
heuristic used here assumes that if there is only one tab in a window and there
are no other windows in which this tab can be dropped, dragging the tab acts as
if the user is dragging the background title bar. This simply extends the area
in which the user can reposition the entire window.

If the user is dragging a tab with the potential to drop it somewhere else, they
can either reposition the tab within the same source window, or they can put the
tab into a different destination window. Clearly, if there is only one tab in
the source window, the former can not be the case, and it is assumed the
destination will be in a different window.

Assuming there are more than one tab in the source window, the drag begins in
the mode of tracking a drag within the same source window. As the user drags the
mouse back and forth, the TabView instructs the window's TabWindowController to
insert a placeholder for the given tab at the appropriate mouse location. Doing
so will force a layout which moves the dragged tab to the given location in the
window and the tab will appear to "follow" the mouse as it moves back and forth
with the other tabs getting out of its way should it overlap them too much. The
animation comes for free as a part of the normal layout, described above.

[<img alt="image"
src="/developers/design-documents/tab-strip-mac/dragWithin2.png">](/developers/design-documents/tab-strip-mac/dragWithin2.png)

If at any time the mouse leaves a certain boundary above or below the tab strip,
the dragging code assumes the user wants to "tear" the tab out into its own
window. This breaks out of the previous mode and enters the general mode where a
tab (now a full-fledged window) can be dropped anywhere on the desktop or into
another window. Detaching the tab creates a new Browser, complete with its own
TabStripModel, containing the TabContents associated with the original tab. This
allow it to maintain all its existing state and render processes (complete with
animations!). Creating a new browser creates a new BrowserWindowController and
all of the Cocoa objects that make a normal browser window and is a rather
heavy-weight process. The source window is no longer the original from where the
tab was dragged, but becomes the newly formed window.

In this new mode, the visual appearance of the dragged window changes such that
the background of the title bar is translucent but the tab and all the window
contents are opaque. If the user moves the tab over a tab strip of a potential
destination window, the title bar background disappears entirely to give the
appearance that the tab can be part of the destination window. Also, as the user
moves the mouse over a potential destination window, it will be brought to the
front so its tab strip is not obscured by other windows.

[<img alt="image"
src="/developers/design-documents/tab-strip-mac/translucent.png">](/developers/design-documents/tab-strip-mac/translucent.png)

This change in visual appearance is handled by the TabWindowController via an
"overlay window" which re-parents the views of the tab strip, toolbars, content
area, etc into a new (opaque) floating child window while the source window
behind it becomes translucent. When the drag completes, the moving of views into
the overlay is reversed and everything returns to the actual source window.

When the user releases the mouse, there are three cases to check for:

*   User dropped the tab within the same window without leaving the tab
            strip
*   User dropped the tab in a different window (or possibly the same
            window but tore it off then put it back)
*   User dropped the tab on the desktop

In the first case, there's nothing complicated to do as everything is within a
single TabStripModel. When moving between windows, the TabContents of the
dragged source window is added to the destination window's model at the given
location. Finally, the last case doesn't require anything to be moved around at
all because the TabContents was already moved into the new window when it was
initially torn off. In the latter two cases, the overlay window must be cleaned
up and the window appearance put back to normal. The ordering can be tricky with
shadows and alpha values.

One goal of the design of the tab dragging code was for it to be application
agnostic. None of the classes involved are browser-specific and the TabView
itself only makes calls to the TabWindowController of either the source or
destination windows. Any handling of the Chromium-defined TabContents objects
are performed by the BrowserWindowController, which clearly isn't intended to be
shared. Hopefully one day these classes can move to GTM so they can be more
easily re-used by other applications.

## New Tab

The process to create a new tab begins in the model, arriving at the
TabStripController via its C++ observer bridge. First it creates a new
TabContentsController and stores that in an array parallel to the model. Next,
it creates a new TabController and inserts it into a similar parallel array.
Note that the act of creating a new tab cancels "rapid closure mode", restoring
the available width to the full amount. Finally, the controller broadcasts that
the number of tabs has changed, then returns. This allows various parts of the
user interface (such as the menu bar) to adjust for the new number of tabs in
the key window.

Tab layout is performed at different times depending on if the tab is created in
the foreground or background. If in the background, layout is done immediately
so the new tab appears. However, if the tab is created in the foreground, layout
is deferred until the models notifies the controller to select the new tab. This
is done because tab selection can change the width of (small) tabs and thus
layout always needs to be done on tab selection. Waiting for selection avoids
laying out the tabs twice in a row.

Close Tab

The process to close a tab is mostly the reverse of the "new tab" case, only
that it's handled in multiple stages to allow for the animation to complete (as
described above). The model notifies the controller via its observer bridge that
a tab has been detached. Note that there is a notification that the tab is
closing, but it is not used in this implementation. If the window should remain
open because there are more tabs remaining, being the tab closing animation
which will sync up all the data structures upon completion. If closing the final
tab in a window, the window will be going away on its own so don't bother with
animations and close the tab directly.

Detaching also happens in the drag and drop case where a tab is being moved
between windows.
