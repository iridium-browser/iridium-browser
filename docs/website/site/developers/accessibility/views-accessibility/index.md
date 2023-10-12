---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/accessibility
  - Accessibility for Chromium Developers
page_name: views-accessibility
title: Views accessibility
---

The Views module provides a UI abstraction layer used on both Windows and Chrome
OS. Views also provides accessibility support automatically on both platforms,
so if you're building a UI using Views or even writing a new View, making it
accessible is pretty easy.

### Keyboard Access

In Chromium, there are some "panes" that are not keyboard-accessible by default,
by we provide a special mode to get access to them. For example, the main
Toolbar and the Bookmarks Bar are not normally part of the tab order (with the
exception of the location bar / omnibox). However, if your press F6 (on Windows;
Ctrl+F1 on Chrome OS), it will focus the first item in the toolbar and you can
use Tab to interact with the rest of the items in the toolbar. See [Windows
accessibility](/developers/accessibility/windows-accessibility) for a little
more information on the underlying model and user expectations for keyboard
accessibility.

If you're adding a new pane, or some other UI that appears in the main browser
window but not inside of tab contents, you probably want it to have similar
keyboard accessibility.

In Views, you can make a view accessible in this way with the AccessiblePaneView
class, which is used as a base class for the toolbar, bookmarks bar, infobars,
and more in Chromium. When you subclass AccessiblePaneView, you can designate
controls that are not normally part of the tab order, but are keyboard-focusable
if the user first focuses your pane (typically by pressing F6) and then tabs to
the controls. Use View::set_accessibility_focusable(true) to identify views that
should be focusable in this special mode but not under normal circumstances.

### Screen reader / accessibility API access

Chromium provides access to screen readers and magnifiers that interact with
Windows using accessibility APIs. When you create a new View, or build a new bit
of UI using Views, it's important that you support these APIs so that all users
can access the controls even if they have trouble seeing or using a mouse.
Luckily most of the work has been done for you!

If you're just building UI (like a toolbar or dialog) using existing views,
usually all that's necessary is to provide an accessible name / title for any
control that doesn't already have accessible text.

If you're creating new widgets, every accessible View subclass should override
this method:

virtual void GetAccessibleState(ui::AccessibleViewState\* state);

AccessibleViewState looks like this:

struct UI_EXPORT AccessibleViewState { public: AccessibleViewState();
~AccessibleViewState(); // The view's role, like button or list box.
AccessibilityTypes::Role role; // The view's state, a bitmask containing fields
such as checked // (for a checkbox) and protected (for a password text box).
AccessibilityTypes::State state; // The view's name / label. string16 name; //
The view's value, for example the text content. string16 value; // The name of
the default action if the user clicks on this view. string16 default_action; //
The keyboard shortcut to activate this view, if any. string16 keyboard_shortcut;
// The selection start and end. Only applies to views with text content, // such
as a text box or combo box; start and end should be -1 otherwise. int
selection_start; int selection_end; // The selected item's index and the count
of the number of items. // Only applies to views with multiple choices like a
listbox; both // index and count should be -1 otherwise. int index; int count;
};

When that method is called, call the inherited implementation and then modify
|state| to reflect the current state of your view. Most views should just update
the role and name, or perhaps the value and state. For example, a control that
implements a multi-select list should return the current selected value in
|value| and update |index| and |count| as appropriate.

Finally, if your control dynamically changes value, you may need to call
NotifyAccessibilityEvent, like this:

GetWidget()-&gt;NotifyAccessibilityEvent( this,
ui::AccessibilityTypes::EVENT_VALUE_CHANGED, true);

The only way to test your application is by actually using a screen reader.
Chromium's accessibility developers will be glad to assist you with this! To
make our jobs easier, please first figure out a plan for keyboard accessibility
and implement that completely - you shouldn't need our help with that. Once
everything works great with the keyboard, we can help with screen reader
accessibility testing to see what might need to be labeled.
