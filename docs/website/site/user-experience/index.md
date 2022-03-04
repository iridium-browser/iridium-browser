---
breadcrumbs: []
page_name: user-experience
title: User Experience
---

This section describes the motivations, assumptions, and directions behind
Chromium and Chromium OS's user interface design.

Its goal is to explain the current design in a way that further work can be
developed in-style, or so that our assumptions can be challenged, changed, and
improved.

<table>
<tr>

<td><a href="/user-experience/Chrome.png"><img alt="image"
src="/user-experience/Chrome.png"></a></td>

<td>## Chrome Features</td>

<td><b>Window</b></td>

<td><a href="/user-experience/window-frame">Window Frame</a> | <a
href="/user-experience/tabs">Tabs</a> | <a
href="/user-experience/tabs/throbber">Throbber</a> | <a
href="/user-experience/toolbar">Toolbar</a> | <a
href="/user-experience/omnibox">Omnibox</a> </td>

<td><b>Browsing</b></td>

<td><a href="/user-experience/bookmarks">Bookmarks</a> | <a
href="/user-experience/history">History</a> | <a
href="/user-experience/new-tab-page">New Tab Page</a> </td>

<td><b>Additional UI</b></td>

<td><a href="/user-experience/downloads">Downloads</a> | <a
href="/user-experience/status-bubble">Status Bubble</a> | <a
href="/user-experience/find-in-page">Find in Page</a> | <a
href="/user-experience/options">Options</a> | <a
href="/user-experience/incognito">Incognito</a> </td>

<td><a href="/user-experience/notifications">Notifications</a> | <a
href="/user-experience/infobars">Infobars</a> | <a
href="/user-experience/multi-profiles">Multiple Chrome Users</a> </td>

<td><b>Appearance</b></td>

<td><a href="/user-experience/visual-design">Visual Design</a> | <a
href="/user-experience/resolution-independence">Resolution Independence</a> |
Themes</td>

<td><b>Accessibility</b></td>

<td><a href="/user-experience/keyboard-access">Keyboard Access</a> | Touch
Access | <a href="/user-experience/low-vision-support">Low-Vision Support</a> |
<a href="/user-experience/assistive-technology-support">Screen reader
support</a> </td>

<td><b>UI text</b></td>

<td><a href="/user-experience/ui-strings">Write strings</a> | <a
href="/developers/design-documents/ui-localization#TOC-Use-message-meanings-to-disambiguate-strings">Write
message descriptions</a></td>

<td>## UX themes</td>

<td><b>Content not chrome</b> </td>

*   <td>In the long term, we think of Chromium as a tabbed window
            manager or shell for the web rather than a browser application. We
            avoid putting things into our UI in the same way you would hope that
            Apple and Microsoft would avoid putting things into the standard
            window frames of applications on their operating systems.</td>
*   <td>The tab is our equivalent of a desktop application's title bar;
            the frame containing the tabs is a convenient mechanism for managing
            groups of those applications. In future, there may be other tab
            types that do not host the normal browser toolbar.</td>
*   <td>Chrome OS: A system UI that uses as little screen space as
            possible by combining apps and standard web pages into a minimal tab
            strip: While existing operating systems have web tabs and native
            applications in two separate strips, Chromium OS combines these,
            giving you access to everything from one strip. The tab is the
            equivalent of a desktop application's title bar; the frame
            containing the tabs is a simple mechanism for managing sets of those
            applications and pages. We are exploring <a
            href="/chromium-os/user-experience/window-ui">three main
            variants</a> for the window UI. All of them reflect this unified
            strip.</td>
*   <td>Chrome OS: Reduced window management: No pixel-level window
            positioning, instead operating in a full-screen mode and exploring
            new ways to handle secondary tasks:</td>
    *   <td>Panels, floating windows that can dock to the bottom of the
                screen as a means of handling tasks like chat, music players, or
                other accessories.</td>
    *   <td>Split screen, for viewing two pieces of content
                side-by-side.</td>

<td><b>Light, fast, responsive, tactile</b></td>

*   <td>Chromium should feel lightweight (cognitively and physically)
            and fast.</td>

<td><b>Web applications with the functionality of desktop applications</b></td>

*   <td>Enhanced functionality through HTML 5: offline modes, background
            processing, notifications, and more.</td>
*   <td>Better access points and discovery: On Chromium-based browsers,
            we've addressed the access point issue by allowing applications to
            install shortcuts on your desktop. Similarly, we are using <a
            href="/chromium-os/user-experience/tab-ui">pinned tabs</a> and
            search as a way to quickly access apps in Chromium OS.</td>
*   <td>While the tab bar is sufficient to access existing tabs, we are
            creating a new primary <a
            href="/chromium-os/user-experience/access-points">access point</a>
            that provides a list of frequently used applications and tools.</td>

<td><b>Search as a primary form of navigation</b></td>

*   <td>Chromium's address bar and the Quick Search Box have simplified
            the way you access personal content and the web. In Chromium OS, we
            are unifying the behavior of the two, and exploring how each can be
            used to make navigation faster and more intuitive.</td>

<td><a href="/user-experience/ChromeOS.png"><img alt="image"
src="/user-experience/ChromeOS.png"></a></td>

<td>## Chrome OS Features</td>

<td><b>Note: UI under development. Designs are subject to change.</b></td>

<td><b>Primary UI</b></td>

<td><a href="/chromium-os/user-experience/window-ui">Window UI Variations</a> |
<a href="/chromium-os/user-experience/window-management">Window Management</a> |
<a href="/chromium-os/user-experience/tab-ui">Pinned Tabs</a> | <a
href="/chromium-os/user-experience/access-points">Apps Menu</a> | <a
href="/chromium-os/user-experience/panels">Panels</a> </td>

<td><a href="/system/errors/NodeNotFound">UI Elements</a> | <a
href="/user-experience/multitouch">Gestures</a> | <a
href="/chromium-os/user-experience/system-status-icons">System Status
Icons</a></td>

<td><b>Core Applications</b></td>

<td> <a href="javascript:void(0);">Settings</a> | <a
href="/chromium-os/user-experience/content-browser">Content Browser</a> | <a
href="/chromium-os/user-experience/opensave-dialogs">Open/Save Dialogs</a> | <a
href="/chromium-os/user-experience/shelf">Shelf</a> </td>

<td><b>Devices</b></td>

<td> <a href="/chromium-os/user-experience/form-factors">Form Factors</a> | <a
href="/user-experience/resolution-independence">Resolution Independence</a>
</td>

<td>## Video and Screenshots</td>

<td>The implementation, the concept video, and the screenshots are presenting
different UI explorations. Expect to see some variation.</td>

<td><b><a href="http://www.youtube.com/watch?v=hJ57xzo287U"><img alt="image"
src="/chromium-os/user-experience/Concept2.jpg"></a></b></td>

<td><a href="/chromium-os/user-experience/sdres_0000_Basic.png"><img alt="image"
src="/chromium-os/user-experience/sdres_0000_Basic.png" height=112
width=200></a> <a
href="/chromium-os/user-experience/sdres_0001_App-Menu.png"><img alt="image"
src="/chromium-os/user-experience/sdres_0001_App-Menu.png" height=112
width=200></a> <a href="/chromium-os/user-experience/sdres_0002_Panels.png"><img
alt="image" src="/chromium-os/user-experience/sdres_0002_Panels.png" height=112
width=200></a> </td>

</tr>
</table>

<table>
<tr>
</tr>
</table>