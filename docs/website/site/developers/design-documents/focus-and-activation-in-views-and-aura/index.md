---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: focus-and-activation-in-views-and-aura
title: Focus and Activation in Views and Aura
---

Focus and Activation are closely related.

### Definitions

Focused window - this is the window that receives keyboard input. The focused
window is always or is always contained by the Active window.

Active window - this is the window that is rendered as key - i.e. the one that
contains the focused element. Different platforms will render this differently
and may show additional affordances e.g. a different treatment in the window
switcher.

### Notes

A "top level" window is a window that has no parent other than window manager
constructs. Think of a Chrome window.

An "activatable" window is a top level window that can be activated. Not all top
level windows can be activated at any given time. For instance in Ash, if the
screen is locked calling GetActivatableWindow() for a window behind the lock
screen may return a window in front of the lock screen, whereas
GetTopLevelWindow() will return the non-activatable toplevel behind the lock
screen. For this reason GetActivatableWindow is a method most useful to the
focus/activation implementation, and GetTopLevelWindow is the method most useful
to general application code.

In Aura, the CoreWM library provided by Views handles focus and activation
changes in the class FocusController. The FocusController implements two
interfaces - aura::client::FocusClient and aura::client::ActivationClient. Every
RootWindow must have an associated Focus/ActivationClient implementation
associated with it, though several RootWindows can share the same
implementation. For Desktop-Aura, each top level DesktopNativeWidgetAura creates
a separate FocusController for the associated RootWindow. For Ash, there is a
single FocusController shared by all RootWindows managed by the
DisplayController.

The FocusController tracks the current active and focused windows, and sends
notifications when either of these change to registered implementations of
aura::client::FocusChangeObserver and aura::client::ActivationChangeObserver.

Views also tracks focus, for views::View subclasses within a views::Widget.
Think of it this way. In the Aura world, an aura::Window can have focus, and can
have an embedded View hierarchy via NativeWidgetAura/Widget. Within that focused
aura::Window there can be an individual View that has focus, e.g. the omnibox
textfield vs. a button etc. Each toplevel views::Widget has a
views::FocusManager which handles View focus. It also handles things like focus
traversal. Focus traversal is what happens when you press Tab. The traversal
code figures out what View to focus next when you press Tab or Shift+Tab.

When a views::Widget is deactivated, its FocusManager stores the focused view in
ViewStorage for restoration later. The history behind this is somewhat vestigial
but the storage/restoration step is still necessary. When the Widget is later
reactivated, the last focused view is refocused. This is true in both non-Aura
Win32 Views and in Aura Views.

As far as what can be focused or activated in Aura, FocusController delegates to
an implementation of the FocusRules interface to determine what can be focused
or activated. There are different implementations for Ash (AshFocusRules) and
Desktop-Aura (DesktopFocusRules). These rules enforce specifics like not
activating windows behind the lock screen, etc.
