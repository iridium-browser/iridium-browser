---
breadcrumbs:
- - /developers
  - For Developers
page_name: os-x-keyboard-handling
title: OS X keyboard handling
---

This document describes how keyboard events are handled on OS X. Some of this is
common for all three platforms, some of it is specific to OS X.

## Requirements

The central principles behind all design decisions are

1.  Keyboard events should not use synchronous IPC calls.
2.  All key events should be forwarded to the web page first, and only
            if the web page does not handle and event it should be handled by
            Chromium itself (with the exception of a few reserved shortcuts)
    1.  The javascript events send for key presses should be similar to
                what Safari sends (this is not true yet, [bug
                25249](http://crbug.com/25249))
3.  The events that end up being handled by Chromium should behave like
            keyboard events normally behave on OS X
    1.  Key equivalents should blink the menu item they activate
    2.  It should be possible to configure custom keyboard shortcuts in
                System Preferences
    3.  Text input should support everything the Cocoa text input system
                supports (IME, configurable text actions, emacs shortcuts, etc

## Life of a keypress

TODO(thakis): Write

Copy all links from my notes, have a version of Apple's diagram with annotations
where we intercept and redispatch what.

Cocoa sends all events to -\[NSApplication sendEvent:\]. The standard
implementation of this checks if cmd or ctrl are pressed (of if the key is an
arrow key), and if so sends the event via -performKeyEquivalent: down to the
window, which does a pre-order traversal of all views in the window and calls
-performKeyEquivalent: on every view until a view handles the event. The
responder chain is not used during key equivalent processing. If no view handles
the event, it is next sent to the menu, then to the key view loop, and if it is
not handled by those, down the regular path of key events.

For normal key events, -\[NSWindow sendEvent:\] is called, which sends -keyDown:
to the window's first responder. The first responder either handles the event or
forwards it to the next responder in the responder chain.

FOO about pKE being a problem and keyDown: being good.

The first approach I tried was to have -\[RenderWidgetHostViewMac
performKeyEquivalent:\] always return YES for key events that would be handled
by the menu (by checking if cmd is down) and then ipc the event that was passed
in to the renderers. Other key events would make it through to -keyDown: and be
processed correctly through the flow there. If the key comes back unprocessed
from the renderer, I passed it to -\[NSMenu performKeyEquivalent:\] so that it
would trigger a menu item. There were several problems: Some keys didn't make it
through to -keyDown: (notably, ctrl-tab, which Cocoa swallowed for use in the
view loop), and some keys that Cocoa should handle get dropped (for example,
cmd-\`, which is not handled by the main menu).

The next approach was that -\[CrApplication sendEvent:\] asks the window if it
wants to shortcircuit the event. The window checks if its first responder is a
RenderWidgetHostViewMac and if so, sends the key over ipc and tells the
application to drop the key. If the key comes back unhandled from the renderer,
it is sent to -\[NSApp sendEvent:\] again (with a flag set that the
shortcircuiting mechanism should not be used this time). This will then let
cocoa give the key to the menu and handle keyboard shortcuts like cmd-\`.
(TODO(thakis): Mention that original key events are kept in a queue on the
browser side and the original object is used during redispatch).

The current approach is based on -performKeyEquivalent: again, as the
sendEvent-based approach disabled input menu toggling if the web page swallowed
the key press for that. To work around the problems this approach had on the
first try, the event redispatch is kept from the old approach (that maked cmd-\`
work), and we use an SPI to make sure ctrl-tab is not swallowed by the key view
loop handling. (TODO(thakis): Probably, nobody cares about the history? Remove
the old two approaches, at least from the main text)

FIXME(thakis): Explain how the shortcut blocklist (cmd-w etc) works.

FIXME(thakis): Explain what isSystemKey keys are.

FIXME(thakis): Explain why there are window-level and browser-level equivalent
shortcuts. Talk about global_keyboard_shortcuts_mac, but also about how this
being keycode-based sucks.

FIXME(thakis): Explain how keys are IPCd.

FIXME(thakis): Explain why the key needs to go to the menu before going go
g_k_s_m and _then_ to \[NSApp sendEvent:\].

FIXME(thakis): Explain how IME works.

FIXME(thakis): Explain how emacs key bindings are implemented.

TODO(thakis): Split this up into several sections. Possible sections: "Big
picture" with subsections Key event interception, IPC, Redispatch. "Gory
details" with all the FIXMEs above?

## Testing

To guard against regressions, the keyboard handling code should be thoroughly
unit-tested. As of now, it is not very tested. This section contains a list of
interesting keyboard handling bugs, as a list of things to think of when
changing the keyboard handling code. I will write tests for all of these once
in-process browser tests work.

*   Keyboard events should go to JS first and only be processed by the
            browser if javascript doesn't handle them. Bug , No Test. All other
            test cases assume that js does not handle the keypress.
*   Cmd-up/down should scroll to end of the page. Bug, Test .
*   Cmd-1-8 should switch to tab 1-8, no matter what is focussed. Bug,
            Test .
*   Cmd-left/right should go back/forward if the web is focussed, but
            not if the omnibox is focussed or a text field on the web is
            focussed. Bug, Test .
*   Cmd-opt-left/right should switch tabs.
*   Ctrl-Tab should switch tabs and not trigger the view loop.
*   Cmd-\` should cycle windows. It should be possible to change the key
            for this in System Preferences, and the other key should still work.
*   Backspace should go back if no textbox is focusses. It should not go
            back while IME is active.
*   Cmd-c, cmd-v, cmd-x should blink their menu entries.
*   Cmd-z, cmd-shift-z do not yet blink their menu entries, but they
            should work none the less.
*   Tab should make focus jump between web page elements when the web
            has focus.
*   Ctrl-left/right should go to start/end of line
*   Cmd-space (or whatever is configured to trigger this) should always
            change the current input language, and not give the web an
            opportunity to swallow that keypress.
*   Cmd-left/right should go back/forward and not scroll the page if the
            page has a horizontal scrollbar.

These are still broken at the time of this writing:

*   Cmd-shift-v should work even though it doesn't have a menu entry
            yet.
*   html accesskey attribute
*   Hitting arrow left/right while IME is active should move the cursor.
