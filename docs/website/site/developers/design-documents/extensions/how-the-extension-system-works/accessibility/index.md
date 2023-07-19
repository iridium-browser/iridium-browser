---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/how-the-extension-system-works
  - How the Extension System Works
page_name: accessibility
title: Accessibility
---

[TOC]

**### Keyboard navigation**

**Fundamental to keyboard-only and screen reader users is navigational access
and interaction using the keyboard alone. This includes means to set keyboard
focus to toolstrips and Omnibox icons for background page functionality, as well
as any popups, dialogs or other UI elements the extensions might generate. Also
required is a way to discern and trigger any associated behavior/action
connected to these UI elements.**

**Setting keyboard focus to a toolstrip could be provided either through a
keyboard shortcut (much like we have for the
[toolbar](/user-experience/toolbar)), or by unifying access to all of our
toolbars ([toolbar](/user-experience/toolbar),
[infobars](/developers/design-documents/info-bars),
[bookmarks](/user-experience/bookmarks), extensions) by a single shortcut and a
way to circle through them. The first approach is simpler, but the second has
more long-term merit, avoiding discoverability issues with multiple keyboard
shortcuts.**

**Once focus is on the toolstrip, arrow keys should be used to traverse between
toolstrip buttons, and Space/Enter keys to activate them.**

**### Screen reader support**

**The first piece for supporting screen readers is provided by enabling full
keyboard access (including focus tracking) to the UI. Secondly, correct
information needs to be exposed through the platform-specific accessibility
interface ([MSAA](http://msdn.microsoft.com/en-us/library/ms971310.aspx) on
Windows). Providing this information for toolstrips themselves is almost landed
(<http://codereview.chromium.org/155446>), and the next step will be to provide
coverage for the toolstrip buttons.**

**The real challenge here will be to appropriately give access to the
information otherwise connected to the toolstrip buttons (e.g. the slideout with
builder status), as well as interactive Omnibox icons.**

**### Low vision**

**The Extensions toolstrip already respects OS font resizing (on Windows:
Control Panel &gt; Display &gt; Appearance tab, and choose 'Extra Large Fonts').
Theming (as below with High Contrast) will help for users with contrast
issues.**

**### High contrast**

**All of the extensions UI should respect the theming imposed by the [High
Contrast
mode](http://www.microsoft.com/windowsxp/using/accessibility/highcontrast.mspx).
Support for this is being developed this quarter (Q3) by Stephen White, and
should cover Extensions as well.**

**### Developer documentation**

Anyone developing an extension should be aware of accessibility best practices
for the parts not covered by Chrome browser accessibility. Included in this are
things such as HTML files and JS content (including dynamically generated HTML).

Developer documentation pages should be built along accepted guidelines and best
practices for accessible static HTML, where appropriate.
