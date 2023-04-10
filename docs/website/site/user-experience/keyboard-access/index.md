---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: keyboard-access
title: 'Accessibility: Keyboard Access'
---

An important design goal is for Chrome to be fully accessible via the keyboard.
Many users with disabilities may be unable to use a mouse or other pointing
device, and there are many scenarios where power users prefer keyboard
shortcuts.

The majority of users use a combination of the keyboard and mouse, and we don't
think that full keyboard access should make things more cumbersome for users who
don't need or want every control to be focusable. So, for example, we don't
think that most toolbar controls should be part of the Tab traversal.

On the other hand, we don't believe that there should be a separate
"accessibility mode" that enables more keyboard access. We don't believe that
users should be segregated; rather, we should strive to find a balance where
most users will only use the keyboard shortcuts they choose to learn, while
users who rely completely on their keyboard will find the interface easy to
navigate without memorizing a separate shortcut for every single command.

### Keyboard Navigation and Shortcuts

Here are the keyboard shortcuts that help make Chrome accessible to users who
need full keyboard access.

First, there are keys to focus each of the toolbars:

*   **Shift+Alt+T**: Main Toolbar (contains Back, Forward, Reload, etc)
*   **Shift+Alt+B**: Bookmarks Toolbar

In addition, pressing **F6** or **Shift+F6** now switches to the next pane, with
the available panes in Chrome being:

*   The web content area (which displays the web page itself)

    Main Toolbar

    Bookmarks Toolbar

Also, pressing **Alt** or **F10** focuses the Chromium menu button in the
toolbar, since these keys are normally used to focus the menu bar in a typical
Windows application.

#### Toolbar Navigation

While in a toolbar, you can press **Tab**, **Shift+Tab**, **Home** (move to
first enabled control) and **End** (move to last enabled control) to navigate to
different controls in the toolbar. You can also use the **Left Arrow** and
**Right Arrow** keys, except notably when the Location Bar / Omnibox has focus,
because then those keys are used for text editing. (This is the same behavior as
in other Windows applications, like Microsoft Excel.)

Controls can be activated using either **Space** or **Enter** (menu buttons also
support **Down Arrow** to open menu, **Esc** to close menu). Many controls also
have a context menu (a right-click menu), which can be activated using the
**Context Menu** key on your keyboard, or by pressing **Shift+F10**.

There is one aspect of toolbar keyboard navigation that is potentially
confusing: the Location Bar is normally part of the Tab order, but having focus
in the Location Bar doesn't necessarily mean that the entire toolbar is the
active pane. In a sense, the Location Bar is a special control that is part of
the tab order of several panes. To clarify:

These keystrokes focus the Location Bar (but do not set focus to the Toolbar
pane):

*   Tabbing to the Location Bar from the web content.
*   **Ctrl+L**
*   **Alt+D**

These keystrokes set focus to the Toolbar pane:

*   **F6** (focuses the Location Bar)
*   **Alt+Shift+T** (focuses the leftmost enabled control of the Main
            Toolbar)
*   **Alt** (focuses the page menu)
*   **F10** (focuses the page menu)

The reason for this is to create minimal confusion for users who do not need
keyboard access. Users who primarily use the mouse are very unlikely to use
**F6**, so it's unlikely they will ever end up focusing various controls in the
toolbar by accident. On the other hand, users who rely on full keyboard access
are used to using **F6** to switch between window panes (e.g. in Windows
Explorer), so this should be a very easy shortcut to remember.

[Chrome extensions](https://chrome.google.com/extensions) can install [Page
Actions and Browser
Actions](http://www.google.com/support/chrome/bin/answer.py?answer=154007) in
the main toolbar. These are all fully accessible using these keystrokes. Don't
forget to try the **Context Menu** key for Page Actions and Browser Actions.

The following keys can be used to access the menus:

*   **Alt** or **F10 or Alt+F** puts focus on the Chromium menu button -
            this corresponds to the key most commonly used to focus the first
            menu in the menu bar in Windows applications.

In addition, the following keys can be used to switch tabs, in addition to the
shortcuts in the menus:

*   **Ctrl+1** through **Ctrl+8** switches to the tab at the specified
            position number on the tab strip.
*   **Ctrl+9** switches to the last tab.
*   **Ctrl+Shift+Tab** or **Ctrl+PgUp** switches to the previous tab.
*   **Ctrl+Tab** or **Ctrl+PgDown** switches to the next tab.
*   **Ctrl+W** or **Ctrl+F4** closes the current tab.
*   **ALT-F4** quits the application.

Want more information? See the [full list of keyboard
shortcuts](https://support.google.com/chrome/answer/157179?hl=en).

### Other pages on accessibility

*   [Accessiblity: Touch Access](/user-experience/touch-access)
*   [Accessibility: Low-Vision
            Support](/user-experience/low-vision-support)
*   [Accessibility: Screen reader
            support](/user-experience/assistive-technology-support)
*   [Accessibility Design
            Document](/developers/design-documents/accessibility) (for
            developers)
