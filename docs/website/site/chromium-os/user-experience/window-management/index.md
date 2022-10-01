---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/user-experience
  - User Experience
page_name: window-management
title: Window Management / Overview
---

******UI under development. Designs are subject to change.******

### In full screen mode, multiple windows are supported by hotkeys and swiping [gestures](https://sites.google.com/a/google.com/chromeos/for-team-members/user-experience/multitouch) to switch back and forth. Users can jump from tab to tab or from window to window. Alternatively, the Overview allows visual tab switching.

[<img alt="image"
src="/chromium-os/user-experience/window-management/OverviewSketch.png">](/chromium-os/user-experience/window-management/OverviewSketch.png)

### The overview

The overview is available via either gesture and hotkey. It provides a visual
way of switching between windows and tabs.

Windows are exploded into the tabs they contain, and all tabs/windows are
visible in a single 2.5d space. We are exploring a variety of different display
techniques, one illustrated below.

[<img alt="image"
src="/chromium-os/user-experience/window-management/Overview.png"
width=600>](/chromium-os/user-experience/window-management/Overview.png)

This model is largely based on Peter Jin Hong & Elin Pedersen's G.L.I.D.E. Tab
Navigation (
[mocks](http://www.flickr.com/photos/peterheads/sets/72157620266552714/show/with/3651412293/)
| [video](http://www.youtube.com/watch?v=kTXvNdiP-rE) ). We present tabs in a
venetian blind arrangement, ensuring visibility of the top left of every page,
and using perspective to compress the most useful portions of the page into the
available strips. Favicons are presented as an alternative visual aid.

To sample window motion, try the
[prototype](/chromium-os/user-experience/window-management/Overview.app.1.0.0.zip)
(Requires Mac OS X 10.5+ / Safari 4).

### Tab switching overlay

A lightweight version of this may be available for quick alt-tab switching. This
would be sorted by most recently used (versus the ordered, ctrl-tab switcher)
and would potentially include recently closed items from history. When
switching, the contents in the background would swap to match the currently
selected thumbnail/icon.

[<img alt="image"
src="/chromium-os/user-experience/window-management/Chrome-OS-Overlay.png"
width=600>](/chromium-os/user-experience/window-management/Chrome-OS-Overlay.png)
