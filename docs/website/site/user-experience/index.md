---
breadcrumbs: []
page_name: user-experience
title: User Experience
---

This section describes the motivations, assumptions, and directions behind
Chromium and Chromium OS's user interface design.<br>
Its goal is to explain the current design in a way that further work can be
developed in-style, or so that our assumptions can be challenged, changed, and
improved.

<div class="two-column-container">
<div class="column">

![Chrome](/user-experience/Chrome.png)

## Chrome Features

### Window
[Window Frame](/user-experience/window-frame) |
[Tabs](/user-experience/tabs) |
[Throbber](/user-experience/tabs/throbber) |
[Toolbar](/user-experience/toolbar) |
[Omnibox](/user-experience/omnibox)

### Browsing
[Bookmarks](/user-experience/bookmarks) |
[History](/user-experience/history) |
[New Tab Page](/user-experience/new-tab-page)

### Additional UI
[Downloads](/user-experience/downloads) |
[Status Bubble](/user-experience/status-bubble) |
[Find in Page](/user-experience/find-in-page) |
[Options](/user-experience/options) |
[Incognito](/user-experience/incognito)

[Notifications](/user-experience/notifications) |
[Infobars](/user-experience/infobars) |
[Multiple Chrome Users](/user-experience/multi-profiles)

### Appearance
[Visual Design](/user-experience/visual-design) |
[Resolution Independence](/user-experience/resolution-independence) |
[Themes](https://developer.chrome.com/docs/extensions/mv3/themes/)

### Accessibility
[Keyboard Access](/user-experience/keyboard-access) |
[Touch Access](/user-experience/touch-access) |
[Low-Vision Support](/user-experience/low-vision-support) |
[Screen reader support](/user-experience/assistive-technology-support)

### UI text
[Write strings](/user-experience/ui-strings) |
[Write message descriptions](/developers/design-documents/ui-localization#TOC-Use-message-meanings-to-disambiguate-strings)

## UX themes

### Content not chrome

*   In the long term, we think of Chromium as a tabbed window
    manager or shell for the web rather than a browser application. We
    avoid putting things into our UI in the same way you would hope that
    Apple and Microsoft would avoid putting things into the standard
    window frames of applications on their operating systems.
*   The tab is our equivalent of a desktop application's title bar;
    the frame containing the tabs is a convenient mechanism for managing
    groups of those applications. In future, there may be other tab
    types that do not host the normal browser toolbar.
*   Chrome OS: A system UI that uses as little screen space as
    possible by combining apps and standard web pages into a minimal tab
    strip: While existing operating systems have web tabs and native
    applications in two separate strips, Chromium OS combines these,
    giving you access to everything from one strip. The tab is the
    equivalent of a desktop application's title bar; the frame
    containing the tabs is a simple mechanism for managing sets of those
    applications and pages. We are exploring 
    [three main variants](/chromium-os/user-experience/window-ui) for 
    the window UI. All of them reflect this unified strip.
*   Chrome OS: Reduced window management: No pixel-level window
    positioning, instead operating in a full-screen mode and exploring
    new ways to handle secondary tasks:
    *   Panels, floating windows that can dock to the bottom of the
        screen as a means of handling tasks like chat, music players, or
        other accessories.
    *   Split screen, for viewing two pieces of content side-by-side.

#### Light, fast, responsive, tactile

*   Chromium should feel lightweight (cognitively and physically) and fast.

#### Web applications with the functionality of desktop applications

*   Enhanced functionality through HTML 5: offline modes, background
    processing, notifications, and more.
*   Better access points and discovery: On Chromium-based browsers,
    we've addressed the access point issue by allowing applications to
    install shortcuts on your desktop. Similarly, we are using
    [pinned tabs](/chromium-os/user-experience/tab-ui) and
    search as a way to quickly access apps in Chromium OS.
*   While the tab bar is sufficient to access existing tabs, we are
    creating a new primary [access point](/chromium-os/user-experience/access-points)
    that provides a list of frequently used applications and tools.

#### Search as a primary form of navigation

*   Chromium's address bar and the Quick Search Box have simplified
    the way you access personal content and the web. In Chromium OS, we
    are unifying the behavior of the two, and exploring how each can be
    used to make navigation faster and more intuitive.

</div>
<div class="column">

![image](/user-experience/ChromeOS.png)

## Chrome OS Features

Note: UI under development. Designs are subject to change.

### Primary UI
[Window UI Variations](/chromium-os/user-experience/window-ui) |
[Window Management](/chromium-os/user-experience/window-management) |
[Pinned Tabs](/chromium-os/user-experience/tab-ui) |
[Apps Menu](/chromium-os/user-experience/access-points) |
[Panels](/chromium-os/user-experience/panels)

[UI Elements](/user-experience/toolbar/#ui-elements) |
[Gestures](/user-experience/multitouch) |
[System Status](/chromium-os/user-experience/system-status-icons)

### Core Applications
[Settings](/chromium-os/user-experience/settings) |
[Content Browser](/chromium-os/user-experience/content-browser) |
[Open/Save Dialogs](/chromium-os/user-experience/opensave-dialogs) |
[Shelf](/chromium-os/user-experience/shelf)

### Devices
[Form Factors](/chromium-os/user-experience/form-factors) |
[Resolution Independence](/user-experience/resolution-independence)

## Video and Screenshots

The implementation, the concept video, and the screenshots are presenting
different UI explorations. Expect to see some variation.

![image](/chromium-os/user-experience/Concept2.jpg)

<p>
<img alt="image" src="/chromium-os/user-experience/sdres_0000_Basic.png" height=112 width=200>
<img alt="image" src="/chromium-os/user-experience/sdres_0001_App-Menu.png" height=112 width=200>
<img alt="image" src="/chromium-os/user-experience/sdres_0002_Panels.png" height=112 width=200>
</p>

</div>
</div>
