---
breadcrumbs:
- - /developers
  - For Developers
page_name: accessibility
title: Accessibility for Chromium Developers
---

Not everyone uses a computer the same way you do! If you're developing code for
Chromium, it's important to pay attention to *accessibility -* making sure that
all users get equal access to the product, even if they have a disability or use
some alternative method to access their computer.

Accessibility includes, but is not limited to:

*   Full keyboard accessibility.
*   Support for large fonts and very high zoom levels.
*   Support for *[screen
            readers](/user-experience/assistive-technology-support)* for blind
            users.
*   Support for *magnifiers* for low-vision users.
*   Support for *high-contrast modes* for low-vision users.
*   Support for *voice-control software* for users who cannot type.

More information on how to ensure changes to various parts of the codebase are
accessible:

*   [Windows](/developers/accessibility/windows-accessibility)
*   [Mac](/developers/accessibility/mac-accessibility)
*   [Linux](/developers/accessibility/linux-accessibility)
*   [Views (Windows & Chrome
            OS)](/developers/accessibility/views-accessibility)
*   [HTML](/developers/accessibility/html-accessibility) - also see the
            page on [WebUI accessibility
            audits](/developers/accessibility/webui-accessibility-audit).
*   [WebKit/Blink](/developers/accessibility/webkit-accessibility)

### Chrome OS

Test new features on Chrome OS with various accessibility support enabled - in
particular, try it with spoken feedback enabled, which uses the ChromeVox screen
reader.

*   See [ChromeVox on Desktop
            Linux](/developers/accessibility/chromevox-on-desktop-linux) for
            instructions on how to test ChromeVox on desktop Linux.

### Launch bugs

New features must go through accessibility review. When you create a meta bug
(Type=Meta), you'll automatically get Dev-AccessibilityReview-No. Just add a
comment to the bug when you believe it's ready for review or if you have any
questions about what might be needed.

### Contact Us

Google-internal: use the [Chrome-Accessibility
Group](https://groups.google.com/a/google.com/forum/#!forum/chrome-accessibility).

Public: the [Chromium-Accessibility
Group](http://groups.google.com/a/chromium.org/group/chromium-accessibility) is
a public list for discussion of end-user Chromium accessibility issues.

Bugs: tag it with **Cr-UI-Accessibility**.

For gory details of how Chromium handles web content accessibility in a
multi-process browser, see [Accessibility Technical
Documentation](/developers/design-documents/accessibility).
