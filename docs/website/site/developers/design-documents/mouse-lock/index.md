---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: mouse-lock
title: Mouse Lock (Pointer Lock)
---

This document describes implementation details of the [Pointer
Lock](http://dvcs.w3.org/hg/pointerlock/raw-file/default/index.html) JavaScript
specification (In Chrome code it is referred to as Mouse Lock), the Pepper/NaCl
[MouseLock](https://developers.google.com/native-client/pepper16/pepperc/struct_p_p_b___mouse_lock),
and the shared user interface Full Screen Bubble UI .

**What is Pointer Lock?** From the specification:

> The Pointer Lock API provides for input methods of applications based on the
> movement of the mouse, not just the absolute position of a cursor. A popular
> example is that of first person movement controls in three dimensional
> graphics applications such as games. Movement of the mouse is interpreted for
> rotation of the view-port, there is no limit to how far movement can go, and
> no mouse cursor is displayed.

> Pointer Lock is related to Mouse Capture \[MDN-SETCAPTURE\]. Capture provides
> continued event delivery to a target element while a mouse is being dragged,
> but ceases when the mouse button is released. Pointer Lock differs by being
> persistent, not limited by screen boundaries, sending events regardless of
> mouse button state, hiding the cursor, and not releasing until an API call or
> specific release gesture by the user.

**Demos:**

[media.tojicode.com/q3bsp/](http://media.tojicode.com/q3bsp/)

<https://developer.mozilla.org/en-US/demos/detail/bananabread>

**Chrome 16 introduced Mouse Lock only in full screen mode:**

*   Pepper and JavaScript apps can programmatically obtain mouse lock
            after full screen has been entered.
    *   For trusted sites, full screen is entered without a user prompt
                to confirm.
    *   For untrusted sites full screen is entered and the user is
                prompted (see strings below). Mouse lock will not be possible
                until the user confirms fullscreen.
    *   Attempts to lock the mouse prior to requesting full screen will
                fail.
    *   Attempts to lock the mouse after full screen is entered but
                before it is confirmed will be merged with the full screen
                request and presented to the user as a single confirmation.
*   After locking, exit instructions are shown for a brief amount of
            time (see strings below).
*   A content setting remembers the permission for any domain a user
            previously allowed. (done in
            [97768](http://code.google.com/p/chromium/issues/detail?id=97768))
*   Mouse lock can be exited in many ways:
    *   Programmatically.
    *   User pressing Esc.
        *   There is a conflict with Esc meaning "stop loading".
                    Assuming the page is loading content and mouse lock is
                    enabled, repeated presses to Esc will:
            *   ESC to exit mouse lock & full screen
            *   ESC to stop loading
            *   ESC again is handled by the page.
    *   Another window or tab getting focus.
    *   Full screen mode is exited.
    *   Navigation or a page refresh.

**Chrome 21 added support for Mouse Lock without the full screen prerequisite:**

*   A request to lock the mouse when the tab is not in full screen will
            succeed if it is made with a user gesture (e.g. a mouse click)
*   For trusted sites mouse lock will be entered with exist instructions
            presented (see strings below).
*   For untrusted sites the user is prompted to allow mouse lock (see
            strings below)
    *   If full screen is then requested, the prompt will be merged to a
                single prompt
*   After mouse lock is accepted the site will be marked trusted, and
            the mouse locked.
*   If full screen is requested while the mouse is locked and user
            confirmation is required the mouse will be unlocked to allow the
            user to allow full screen (the site will need to request mouse lock
            again).

Also, when an application exits (i.e. not due to the user pressing ESC, etc.)
and reenters mouse lock there is no user gesture required and no exit
instructions are shown. This provides for an improved user experience when an
application frequently shifts in and out of mouse lock, such as a game with
first person controls but a cursor used for inventory management.

**Chrome 22 shipped Pointer Lock (the JavaScript API) default on.**

*   Access restricted to same document only, and blocked from sandboxed
            iframes.

**Chrome 23 enables Pointer Lock via sandboxed iframes.**

*   &lt;iframe sandbox="webkit-allow-pointer-lock ..."&gt;

**Strings:**

*   "You have gone full screen. Exit full screen (F11)"
*   "MyExtension triggered full screen Exit full screen (F11)"
    *   It will be "An extension triggered full screen Exit full screen
                (F11)" if the extension is already uninstalled.
*   "**==google.com==** is now full screen. \[Allow\] \[Exit full
            screen\]"
*   "**==google.com==** is now full screen and wants to disable your
            mouse cursor. \[Allow\] \[Exit\]"
*   "**==google.com==** wants to disable your mouse cursor. \[Allow\]
            \[Deny\]"
*   "**==google.com==** is now full screen. Press ESC to exit."
*   "**==google.com==** is now full screen and disabling your mouse
            cursor. Press ESC to exit."
*   "**==google.com==** has disabled your mouse cursor. Press ESC to
            exit."
*   (URLs in all strings are bold)

**Content Settings Preferences Page Strings:**

> Mouse Cursor

> (_) Allow all sites to disable the mouse cursor

> (o) Ask me when a site tries to disable the mouse cursor (recommended)

> (_) Do not allow any site to disable the mouse cursor

> \[Manage Exceptions...\]

> Mouse Cursor Exceptions

State transitions for the initial Chrome 16 launch were enumerated in this
[spreadsheet](https://spreadsheets.google.com/spreadsheet/ccc?key=0Ah7RuMHPdFJYdEFMSndkblFyWWNKdU9vUUloUk5GVVE#gid=0),
which is now out of date with the addition of extension triggered full screen
and non fullscreen permission of mouse lock.

**Tests are in:**

*   [fullscreen_controller_browsertest.cc](http://code.google.com/codesearch#search/&exact_package=chromium&q=fullscreen_controller_browsertest.cc&type=cs)
*   [fullscreen_controller_interactive_browsertest.cc](http://code.google.com/codesearch#search/&exact_package=chromium&q=fullscreen_controller_interactive_browsertest.cc&type=cs)
*   [fullscreen_mouselock.py](http://code.google.com/codesearch#search/&exact_package=chromium&q=fullscreen_mouselock.py&type=cs)

Related open issues:
[Feature=Input-MouseLock](http://code.google.com/p/chromium/issues/list?can=2&q=Feature%3DInput-MouseLock&colspec=ID+Pri+Mstone+ReleaseBlock+Area+Feature+Status+Owner+Summary&cells=tiles)
