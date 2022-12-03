---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: tabs
title: Tabs
---

#### [<img alt="image"
src="/user-experience/tabs/tab.png">](/user-experience/tabs/tab.png)

Tabs are the title bar-like representation of a webpage - like title bars, they
can be moved independently, but can also be grouped together to form a single
window of tabs. Chromium's UI is based around the treatment of tabs and their
content as individual top-level elements rather than as children of a parent
window.

## Keyboard Switching

Ctrl+Tab, Ctrl+Shift+Tab, Ctrl+PgUp and Ctrl+PgDn can all be used to switch back
and forth between tabs. While there is great demand for an MRU-ordered switcher,
we've so far been unable to find an MRU switcher that makes sense beyond the
first three most recent tabs, or one that works well with background-created
tabs.

## Overflow

Rather than enforcing a minimum size, tabs continue to shrink as tabs are added
- this was found to be more satisfying than forcing users to swap to a different
tab management model at some arbitrary cutoff point. It is also assumed that
users with a large number of tabs will be more able to manage those tabs, and
future work (such as a tab switcher overlay) may help address those cases.

## Misc

The tabs are depth ordered from left to right, with the active tab always in
front - tabs do not maintain any other depth order (changing the depth sort
based on focus order was more confusing than useful).
