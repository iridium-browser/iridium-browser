---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: toolbar
title: Toolbar
---

A tab's toolbar is designed to show the basic information related to that tab.
We strongly seek to avoid clutter here, and generally start by assuming that we
wouldn't include something unless it had a reasonably high usage level.

[TOC]

## UI Elements

### Back / Forward

Standard browser back and forward buttons. You can access session history in
each direction by clicking and holding or clicking and dragging downwards.

### Reload

A standard reload button. Ctrl and Shift do not modify this button's behavior.

### Home

When a user has enabled the home button, it appears here. The home button does
not appear by default because in a multi-tab world, we believe that the
bookmarks bar is a better place for navigational items.

### Bookmark

See [Bookmarks](/user-experience/bookmarks). The star may be dragged as a URL to
create links to the current page in specific locations (Desktop, Bookmarks Bar).

### Address Bar

See [Omnibox](/user-experience/omnibox).

### Go/Stop

The go and stop buttons are typically not frequently used in browsers. However,
they are frequently essential (for example, the go button is useful in
mouse-only situations). These buttons are combined so that while a navigation is
in progress, the button shows a stop icon, otherwise it shows a go button.
If the navigation state changes while the user's mouse is over the button and
the state change is not a result of a user action, the button will not change.
It also protects against double-clicks.

### Page Menu

This is generally equivalent to "File / Edit / View", and contains things
contextually related to the current tab.

### Chromium Menu

This is the "Tools / Options" menu, and contains items related to the browser.
Technically it should be located outside of the tab bounds, but there was no
satisfying way of doing so.

## Future Work

*   Middle or modifier-clicking on UI elements should open the resulting
            page in the appropriate tab or window.
*   Customization

## Visual Design

Chromium's buttons are currently 25 x 27 pixels, so the icon centerline will be
on a pixel row.
Typically we've gone for icons that feel engraved into the button - as the
button itself is already a third level of depth away from the desktop, we didn't
want to add another.
We aimed to make our buttons feel like hair-trigger switches, where the pushed
state felt like it was only a pixel or two away, though never got this feeling
right (the difference between the pushed and hover states wasn't pronounced
enough).
