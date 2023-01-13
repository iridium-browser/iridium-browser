---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: incognito
title: Incognito
---

[<img alt="image"
src="/user-experience/incognito/incognito2.png">](/user-experience/incognito/incognito2.png)

Incognito mode is a window-level mode - all pages viewed within this window are
not persisted to the user's history, and incognito pages use a temporary cookie
store that is blank at the start of the incognito session.
Chromium's backend could be made capable of running incognito on a per-tab
basis, but we keep the mode at the window level to avoid the confusion of having
a tabstrip of mixed-mode tabs.
To avoid having to back-delete a page's history, we do not allow dragging
between incognito and regular-mode windows.
