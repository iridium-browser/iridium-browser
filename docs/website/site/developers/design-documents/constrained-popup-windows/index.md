---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: constrained-popup-windows
title: Constrained Popup Windows
---

## Overview

The window.open() API has been abused to open intrusive advertising, which is
regretful since there are legitimate reasons to open a new window that enhance
user experience. Traditionally, popup windows are blocked when they aren't
explicitly requested by a user keypress or mouse click. Currently, other
browsers either don't alert the user when a popup is blocked, or they show a
user alert bar offering to either allow the site or disable the popup blocking
notification.
Instead, we display the title bar of the new popup in the lower right corner of
the web page when said page tries to open a window not caused by a user gesture.
Only the title bar is displayed and if the user navigates away from the site
without interacting with the popup, it is automatically dismissed. It's not even
a real window that shows up in the task bar. This is a tradeoff. Popup windows
are slightly more intrusive than when they are simply blocked without any user
notification, but in return, there is no list of allowed sites to manually
manage, and false positives become significantly less confusing.

## Implementation

When the Browser process receives a message requesting a new popup
(TabContents::AddNewContents()), the TabContents has a chance to build a
ConstrainedWindow instead of passing the message off to
Browser::AddNewContents() to do the normal creation of a new window.
ConstrainedWindows are owned by the TabContents and are "attached" to the lower
right hand corner of the TabContent's visible area.
Clicking or dragging a constrained popup window's titlebar will convert the
popup into a full fledged window (ConstrainedWindowImpl::Detach()).
