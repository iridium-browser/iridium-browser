---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: panels
title: Panels
---

**Overview**

Panels are windows that are visible to the user even while the user is
interacting with other applications. The small windows are positioned at the
bottom of the screen, with minimal manual window management by the user. This
API will allow extension developers to create and use panels.

[<img alt="image"
src="/developers/design-documents/extensions/proposed-changes/apis-under-development/panels/Screen%20shot%202011-10-06%20at%203.47.53%20PM.png">](/developers/design-documents/extensions/proposed-changes/apis-under-development/panels/Screen%20shot%202011-10-06%20at%203.47.53%20PM.png)

**Use cases**

An extension opens small "pop up" windows, for example, separate chat sessions,
calculator, media player, stock/sport/news ticker, task list, scratchpad, that
the user wants to keep visible while using a different application or browsing a
different web site. Scattered "pop up" windows are difficult for the user to
keep track of, therefore panels are placed along the bottom of the screen and
are "always on top".

The user would like easy control of chat windows: finding them, moving them out
of the way, etc. Window management of separate chat "pop ups" is time consuming.
All panels can be minimized/maximized together.

**Could this API be part of the web platform?**

Eventually, yes. The current needs are from extension developers. If we figure
out how to make this work in the wider context of web platform, we will propose
it for the web platform at that point.

**Do you expect this API to be fairly stable?**

Yes.

**What UI does this API expose?**

Panels will be a new type of window with some unique behaviors and UI.

**How could this API be abused?**

An extension could open an excessive number of panels, repeatedly open and close
panels, or unnecessarily draw the user's attention to a panel.

**How would you implement your desired features if this API didn't exist?**

Extensions could continue to use "pop up" windows and let users manually arrange
them on their screen such that they will not be obscured by other applications.
Users would manage each "pop up" individually.

**Are you willing and able to develop and maintain this API?**

Yes.

**API spec**

Extend the chrome.windows.\* API to add a new "panel" type, as Panels are simply
another type of window.

chrome.windows.create with type="panel" will create a new Panel window. Unlike
other window types, Panels do not take focus by default. The focused attribute
can be used to change the default behavior.

chrome.windows.update has a new drawAttention option to attract the user's
attention to a Panel. This API can be used for all window types. The behavior to
attract the user's attention will vary depending on window type and OS.

All other actions on the panel can be achieved via manipulations on the
DOMWindow or the window contents, e.g. change the size of the div containing all
window content to resize the panel.

Panels are regular windows as far as other extensions \[that did not create the
panel\] are concerned and are included in all other windows-related API, e.g.
chrome.windows.getAll, chrome.extensions.getViews.
