---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/user-experience
  - User Experience
page_name: panels
title: Panels
---

**UI under development. Designs are subject to change.**

[<img alt="image" src="/chromium-os/user-experience/panels/Chrome-OS-Panels.png"
height=189 width=400>](/chromium-os/user-experience/panels/Chrome-OS-Panels.png)

Panels in Chromium OS are used as containers that allow a user to multitask
without leaving the view of their current application. For example, with a music
player and chat in panels, a user can control the playback of their music and
chat with a friend while watching a video or reading a long document in their
main view.

Window manager interactions

Panels are always-on-top, and are not attached to a specific window. New panels
open to the left of existing panels.

Open behavior

Panels are minimized and maximized by clicking on their titlebar. When
minimized, a panel is shifted so that only a few pixels of its titlebar are
visible on-screen (this is the 'minimized' state).

When the user's mouse hits the bottom edge of the screen, any minimized panels
slide up to reveal the text in their titlebars (this is the 'peeking' state).
Clicking on these titlebars will restore the panel to its original size.

If a panel is restored with the mouse cursor at the bottom edge of the screen, a
widget will appear under the user's mouse cursor that will minimize the panel
when clicked. The widget disappears as soon as the user's mouse moves away from
the edge. This allows users to quickly open and close panels.

<img alt="image" src="/chromium-os/user-experience/panels/windowstylespng">

Notifications

If a panel is minimized when its title changes, it bounces up to its 'peeking'
state and potentially flashes its background. This allows for a lightweight
alert system without having fullsize panels onscreen.

Auto-arrange

Panels are right-aligned, and automatically arrange themselves in order to not
overlap. If a user drags a panel to the left away from the main group of panels,
it is pushed to the left of all auto-arranged panels until the user explicitly
reorders it into the auto arranged set. It will attempt to hold the defined
position until it is pushed out of the way.

Large panels

In the case where a panel's width exceeds some predetermined max-width value
('X'), the panel is auto-arranged as if it is has width of 'X'. The contents of
the panel are displayed above or below other panels in MRU order, relying on the
user to open and minimize the panels as they see fit.

Overflow

Multiple options exist for overflow (which is to say that we haven't decided
yet):

*   Only show as many panels that fit - others fall off the screen and
            are inaccessible until the onscreen panels are closed.
*   Horizontal scrolling
*   Add a widget to the left side that contains the overflowed items.

Sidebar

Panels can be dragged to sidebars on the left or right of the screen.

Experiments / Controversial features

*   Ability to detach panels to regular floating 'always on top' windows
            on ChromeOS.
*   Pro: more flexible
*   Con: lets users be in a bad state
*   Bottom-edge click to dismiss.
*   Pro: easier to minimize panels
*   Con: harder to focus bottom-edge panel content (e.g. chatbox)

**Visual Spec**

**[<img alt="image" src="/chromium-os/user-experience/panels/panel-spec.png"
height=274 width=400>](/chromium-os/user-experience/panels/panel-spec.png)**

**Themes**

**Themes will not apply to panels.**
