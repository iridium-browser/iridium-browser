---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: fullscreen-mac
title: fullscreen-mac
---

​**Fullscreen in Mac Chrome (draft)**

The decision is to support a version of fullscreen that serves as both
"presentation mode" and "maximized mode". We will support tab switching, window
switching, and have a floating slide-down tab strip / toolbar / omnibox /
bookmark bar to aid in navigation. Glen and Cole have tentatively approved this
for the Mac. There are no current plans to build a floating bar for
Windows/Linux. The rationale is that no space-efficient "maximized mode" is
available on the Mac, since the menu bar is always visible outside of fullscreen
mode; such a mode is desirable for use on computers with small screens, in
particular, on notebooks.

Fullscreen mode features:

*   Normally, as on other platforms, only the web contents will be
            displayed.
*   When the mouse is moved to the top of the screen and left there for
            longer than an instant, the menu bar and also a floating bar (with
            the normal set of "bar" controls, including a tab strip) will slide
            down. See DVD Player for a comparable UI but note that its
            implementation is slightly flawed since there's no delay in its
            slide down. This floating bar will remain visible while the mouse
            remains over it (or, alternatively, will only be hidden when the
            mouse leaves it for long enough).
*   Some other operations, such as Cmd-L, should also cause the floating
            bar to appear.
*   Switching and re-ordering tabs will be fully supported. Tearing off
            tabs will be inhibited, as will dropping tabs in from other windows.
            (This is for simplicity, and may be reconsidered later.)
*   Most operations will be supported, including:
    *   creating new tabs (including download and history tabs)
    *   creating new windows (which will pop up in front; see below)
    *   pinning tabs
    *   accessing the tab context menu
    *   etc.
*   We should allow windows to pop in front, since web pages frequently
            use popup windows. Thus one can switch to other windows, including
            normal (non-fullscreen) windows. More specifically:
    *   z-order will be handled "as usual", so that when active a
                fullscreen window will usually be on top
    *   the user has the opportunity to activate other windows, which
                will then appear over the fullscreen window
    *   popup windows should also pop up over the fullscreen window
    *   if there are other windows visible over a fullscreen window and
                then the user re-activates that fullscreen window, that window
                will move to the top of the z-order (i.e., show in front, hiding
                all other windows)
*   Minimize should be supported, though it will take the window out of
            fullscreen mode. Zoom should be disabled.
*   The download shelf will appear as normal. This is the same as on
            other platforms.
*   Application and popup windows may also be made fullscreen.
*   Dialog-opening operations should also be supported, though there
            remains the question of from where sheets should hang.
*   UI elements which appear as part of the web content area, such as
            infobars and the NTP bookmark bar, will continue to appear as such.
            That is, they will be visible even when the floating bar is not
            visible, and the floating bar will slide down over them.
*   In fullscreen mode, Chrome should interact with the system in the
            way other applications do when they are in fullscreen mode:
    *   Spaces will be allowed and a fullscreen window will appear to
                take one space.
    *   Exposé will be allowed and fullscreen windows will be part of
                the exposed windows.

Quirks and limitations:

*   Cmd-space for Spotlight doesn't work when the menu bar is hidden
            (general problem in OS X).

Open questions and issues:

*   This space is intentionally left nonblank.

Implementation notes:

*   it'd be a good idea to make some sort of "FullscreenController"
            which can be tied to (and owned by) the BWC, just to make things
            cleaner
*   for the stuff requiring delays, we can probably use
            -performSelector:afterDelay:
*   we need to check see if anyone registers for notifications from the
            window
*   the controller will also have to handle keeping the floating bar
            showing when things in the bar have focus
*   be careful when changing the view hierarchy, since views often
            assume that they're the subview of something fixed
*   having to change the view hierarchy significantly (at least the
            bookmark bar and the tabstrip have to move, even if the other things
            are permanently in a "BarView") reduces the benefit of a cleaner
            model.
