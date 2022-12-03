---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: accessibility
title: Accessibility Technical Documentation
---

[TOC]

This page is targeted at Chrome developers, third-party assistive technology
developers, and highly-technical users. For general information about Chrome
accessibility, see the following user experience documents:

*   [Accessibility: Keyboard Access](/user-experience/keyboard-access)
*   [Accessibility: Touch Access](/user-experience/touch-access)
*   [Accessibility: Low-Vision
            Support](/user-experience/low-vision-support)
*   [Accessibility: Assistive Technology
            Support](/user-experience/assistive-technology-support)

**If you're a Chrome developer and you just want to make sure your feature is
accessible without learning all about the gory details, see:**

    **[Accessibility for Chromium Developers](/developers/accessibility)**

For information about how accessibility code is tested, including how to write
new tests or debug test failures, see:

*   [Accessibility Testing](/developers/accessibility/testing)

## Goals

**Chrome should be accessible to all users, including those with special access
needs. While there are a number of different challenges, we believe the
following key areas are the most important:**

*   **Keyboard access**: it should be possible to accomplish any action
            easily without using the mouse.
*   **Low-vision support**: support high-contrast themes, zooming, and
            large font sizes.

    ****Assistive technology support**: Chrome should work with the most popular
    Mac and Windows screen readers, magnifiers, voice control programs, and
    more.**

Improving accessibility support often helps not only users with disabilities,
but also for power users. Whenever possible, we prefer to design Chrome so that
everyone can share the same accessible interface, rather than special modes for
accessibility.

**Volunteers with an interest in improving accessibility are welcome (both for
testing and development). Please join the
[chromium-accessibility](http://groups.google.com/a/chromium.org/group/chromium-accessibility)
list for further information.**

## Accessibility Bug Tracking

We use Chromium's issue tracker to keep track of bugs and feature development.
You can keep track of Accessibility issues using the following links:

*   [All Accessibility
            bugs](https://code.google.com/p/chromium/issues/list?q=Cr%3DUI-Accessibility)
*   [Windows accessibility
            bugs](https://code.google.com/p/chromium/issues/list?q=Cr%3DUI-Accessibility+os%3Dwindows)
*   [Mac accessibility
            bugs](https://code.google.com/p/chromium/issues/list?q=Cr%3DUI-Accessibility+os%3Dmac)

You can also file a [New issue](http://code.google.com/p/chromium/issues/entry).

## How Chrome detects the presence of Assistive Technology

For performance reasons Chromium waits until it detects the presence of
assistive technology before enabling full support for accessibility APIs.

Windows: Chrome calls NotifyWinEvent with EVENT_SYSTEM_ALERT and the custom
object id of 1. If it subsequently receives a WM_GETOBJECT call for that custom
object id, it assumes that assistive technology is running.

Mac OS X: Chromium turns on or off accessibility support based on whether it
sees a client, such as VoiceOver, has set the AXEnhancedUserInterface attribute
on the main application window.

**To override:**

1.  Start Chrome with this flag:` --force-renderer-accessibility`
2.  Or, visit this url to turn it on from within Chrome:
            `chrome://accessibility`

## How Assistive Technology can detect Chrome

Traditionally, assistive technology on Windows used the window class name as a
way to identify a known browser and adapt accordingly. However, this means
assistive technology needs to be updated whenever Chrome needs to change its
window class name, and it also means that other Chrome-based browsers wouldn't
be detected.

If you must use the window class name, please accept any class name that begins
with "Chrome", rather than detecting the current window class name.

As a more robust alternative, Chrome 28 and higher supports
[IAccessibleApplication](http://accessibility.linuxfoundation.org/a11yspecs/ia2/docs/html/interface_i_accessible_application.html),
and it returns strings of this form:

` applicationName: Chrome/vv.xx.yyyy.zz`
` applicationVersion: Mozilla/5.0... Chrome/`vv.xx.yyyy.zz ...
` **toolkitName: Chrome**`
` toolkitVersion: Mozilla/5.0... Chrome/`vv.xx.yyyy.zz ...

Note that there are other Chrome-based browsers. Any Chrome-based browser will
have "Chrome" as its toolkitName, so ideally you should use the toolkit name as
the way to enable custom support for Chrome accessibility.

The applicationVersion and toolkitVersion are the browser user agent, the same
one sent to web servers with each web request. That's a way to get more
information about the exact Chrome version dynamically.

## Accessibility internals

Chrome's multi-process architecture is different from that of any other browser.
For security, the main browser UI is in one process, and web pages are run in
separate renderer processes (typically one per tab). The Renderer processes are
the only ones with a representation of the webpage's DOM and therefore all of
the accessibility information, but the renderer processes are specifically not
allowed to interact with the operating system (sending or receiving events or
messages) - in particular, the renderer processes cannot send or receive
accessibility events.

As a result, Chrome uses a cache of the accessible DOM tree in the browser
process. For performance reasons, this only happens if assistive technology is
detected or if it's explicitly enabled (see above).

For a high-level overview of the implementation of Chrome's accessibility
support, see
[/docs/accessibility.md](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/accessibility.md).

## API support

**Windows:**

*   MSAA/IAccessible (complete)
*   IAccessible2 (complete)
*   ISimpleDOM (mostly complete)
*   IAccessibleEx and UI Automation (very limited)

**Mac OS X:**

*   NSAccessibility (complete)

**Linux:**

*   ATK

**Android:**

*   Android accessibility API (complete)

## Testing

Chrome has a built-in page you can access to view accessibility internals and
toggle accessibility support on or off on a per-tab basis. Just visit this url:

` chrome://accessibility`

On Windows, we recommend testing with a combination of
[AccProbe](http://accessibility.linuxfoundation.org/a11yweb/util/accprobe/) and
[AViewer](http://blog.paciellogroup.com/2013/03/aviewer-2013/) (they each have
their strengths). To use AccProbe, you may need to have a JRE 6 installation and
pass it as a command-line option: (e.g., accprobe.exe -vm
"C:\\jre6\\bin\\javaw.exe") and disable Java auto-updates
(http://www.java.com/en/download/help/javacpl.xml) which will trash your JRE 6
installation.

On Mac, we recommend testing with [Accessibility
Inspector](http://developer.apple.com/library/mac/#documentation/Accessibility/Conceptual/AccessibilityMacOSX/OSXAXTesting/OSXAXTestingApps.html).
