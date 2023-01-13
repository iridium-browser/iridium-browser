---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/user-experience
  - User Experience
page_name: window-ui
title: Window UI Variants
---

**UI under development. Designs are subject to change.**

While working on the UI for Chromium OS, we explored three basic structures for
our main interface: Classic, Compact, and Sidetab.

Each have their own strengths and weaknesses, and while we are still interested
in the variations, our current builds are focused on classic and compact
navigation styles.

---

## Classic navigation

![Classic navigation screenshot](/chromium-os/user-experience/window-ui/navpng)

The most basic navigation style is that of a single maximized Chromium window.
This is the equivalent of the Chromium browser windowing UI in maximized mode.

### Strengths
*   Similarity to Chromium-based browsers

### Weaknesses
*   Address bar is visually scoped to tab; using it to swap tabs and
            launch new tabs makes less sense
*   Address bar is used to show the URL; it is less clear where to
            search from

---

## Compact navigation

![Compact navigation screenshot](/chromium-os/user-experience/window-ui/compact_navpng)

If we take the address bar out of the tab, it can be used as both a launcher
and switcher; the user doesn't have to worry about replacing their active tab.
The current url shows while a site is loading, and can be edited or changed by
clicking on the tab.

### Strengths
*   Saves vertical real estate for the content area
*   Search can be used as launcher and switcher
*   Flexible for larger screens. Users can quickly toggle between this
            and classic navigation
*   Apps can provide a better experience with full control of their
            content area

### Weaknesses
*   Current URL is not always visible
*   Navigation controls and menus are not located within the tab and
            lose context sensitivity
*   Crowded tab-strip

---

## Sidetab navigation

![Sidetab navigation screenshot](/chromium-os/user-experience/window-ui/side_navpng)


By moving the tab-strip to the side, we gain a huge amount of real estate
for tabs. The vertical alignment also allows for date ordering and grouping of
tabs. By moving the address bar out of the tab and above the strip, it can be
used both for navigation as well as search.

### Strengths
*   Saves vertical real estate for the content area
*   Allows for a large number of tabs
*   Allows fire-and-forget tab creation

### Weaknesses

*   Wasted space for users with few tabs
*   Superb for full-screen on devices ~1366px wide, but wasteful on
            larger or smaller displays
*   No clear relationship with Chromium browsers

---

## Touchscreen navigation

![Touchscreen navigation screenshot](/chromium-os/user-experience/window-ui/TouchUI.png)

For touch screens, we provide much larger tab and toolbar targets than on
standard chrome. This UI takes up more screen space, but is ideal for portrait
devices, and can be autohidden to have full-screen content. This treatment could
be used on any edge of the screen, and it may be preferable to use the bottom
edge depending on the device.

### Strengths

*   Large, square targets for navigation.
            [example](/chromium-os/user-experience/window-ui/TouchNav.png)
*   Supports quick navigation between a few common apps

### Weaknesses

*   Wasted vertical space
*   Extremely short app/site titles
