---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/user-experience
  - User Experience
page_name: form-factors
title: Form Factors Exploration
---

**This is a concept UI under development. Designs are subject to change.**

While its primary focus is netbooks, Chrome OS could eventually scale to a wide
variety of devices. Each would have vastly different input methods, available
screen space, and processing power. Below is an illustration of the forms we are
considering along with notes for each:

<img alt="image"
src="/chromium-os/user-experience/form-factors/Form%20Factors.png">

> [<img alt="image"
> src="/chromium-os/user-experience/form-factors/netbook.png">](/chromium-os/user-experience/form-factors/netbook.png)

> Netbook 10-12"

> Because of their small screen resolution, the netbook ui is tailored to one
> web page on the screen at a time. Interaction is primarily via mouse and
> keyboard, and the UI is adapted to this, with primary targets distributed
> along the screen edges. Panels would dock against the bottom of the screen and
> could be moved to the sides as well.

    *   Full Screen, Compact/Classic/Sidebar UI
    *   Omnibox may autohide on devices with limited vertical height
    *   Docking panels
    *   Tabs and Windows

> [<img alt="image"
> src="/chromium-os/user-experience/form-factors/Tablet.png">](/chromium-os/user-experience/form-factors/Tablet.png)

> Tablet 5-10"

> On tablets, the UI would be adjusted to handle larger touch targets. Initial
> explorations have maintained the same basic chrome layout, but enlarged the
> controls. Icons could be placed above tabs to provide larger, square targets.
> Panels would be placed along the bottom edge and could be opened with upward
> dragging motions.

    *   Full screen, [Touch
                UI](/chromium-os/user-experience/window-ui#TOC-Touchscreen-navigation)
    *   Docking panels
    *   Touch panel UI
    *   Tabs only
    *   [High-res display](/user-experience/resolution-independence)
    *   [Visual
                explorations](/chromium-os/user-experience/form-factors/tablet)

> [<img alt="image"
> src="/chromium-os/user-experience/form-factors/laptop.png">](/chromium-os/user-experience/form-factors/laptop.png)

> Laptop 15-17"

> On laptop-sized devices, full screen mode is not suitable for most web pages.
> At this point we would re-introduce multiple windows on screen, using either
> overlapping or tiling windowing systems. Panels would now be able to dock to
> edges *or* float freely on the screen.

    *   Windowed, Classic UI
    *   Overlapping or tiled window management
    *   Floating or docking panels

> [<img alt="image"
> src="/chromium-os/user-experience/form-factors/Desktop.png">](/chromium-os/user-experience/form-factors/Desktop.png)

> Desktop 24-30"

> The desktop UI is similar to the laptop UI, but benefits more from freely
> positioned windows and access points near the cursor. Other potential
> enhancements include magnetic windows/panels that can be moved around with
> each other to create workspaces.

    *   Windowed, Classic UI
    *   Overlapping window management
    *   Floating or docking panels

> [<img alt="image"
> src="/chromium-os/user-experience/form-factors/Display.png">](/chromium-os/user-experience/form-factors/Display.png)

> Display 40-60"

> On large displays or projector screens, the design needs are very similar to
> those of the netbook: full screen content, autohiding chrome, large targets.
> Complex window management is even less important, and it may be sufficient to
> have no windows, only zoomable tabs.

    *   Full screen display
    *   Auto-hiding Omnibox
    *   Docking panels
    *   Split screen
    *   Tabs only
