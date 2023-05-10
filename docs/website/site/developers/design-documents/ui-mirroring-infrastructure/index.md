---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: ui-mirroring-infrastructure
title: UI Mirroring Infrastructure
---

[TOC]

## Objective

One of the goals for the launch of Chromium is to support 40 UI languages; that
is, the UI needs to be localized for each language such that any UI text element
(menus, titles, labels, etc.) is displayed in the target language. In order to
make sure that Chromium can be easily localized to different languages (an
effort known as *localizability*), the Chromium source code is structured such
that localizable strings are stored in locale-specific DLLs. This way, the
actual UI strings are loaded from the right DLL, depending on the target locale.
There is one more localizability aspect which applies only to right-to-left
(RTL) languages, such as Hebrew and Arabic. In such languages, translating the
strings to the target language is not enough. In order to provide a perfect
right-to-left look and feel to the browser, the entire UI needs to be mirrored.

This document starts by providing a brief overview of the UI mirroring
technology in general and in particular the Windows support for UI mirroring.
Then, the newly added ChromeViews mirroring infrastructure is described. Using
this new infrastructure, the Chromium UI elements can be easily mirrored for RTL
locales. Finally, and most importantly, the document provides a set of
guidelines for writing Chromium UI elements that don't break when mirrored in
RTL UIs.

## Windows UI mirroring architecture

### An example

The Windows operating system itself has been localized for a large set of
languages, including right-to-left languages like Hebrew. Thus, users of the
Hebrew version of Windows are presented with a mirrored version of the English
OS UI. Before we examine the mirroring Windows API available to application
developers, it will be helpful to compare the mirrored and the non-mirrored
versions of a fundamental Windows UI component - Windows Explorer. Looking at
the manner in which the Windows OS mirrors its core UI elements will shed some
light on the ChromeViews Mirroring Infrastructure design. The following diagram
shows the English version of Windows Explorer we are all familiar with:

[<img alt="image"
src="/developers/design-documents/ui-mirroring-infrastructure/explorer_window_en_us.jpg">](/developers/design-documents/ui-mirroring-infrastructure/explorer_window_en_us.jpg)

The first thing to note about the English version is that left alignment is used
for all the text elements. Also, there are two groups of highlighted UI elements
(the red group and the black group) that behave differently when displayed in a
mirrored UI. The elements in both groups contain an image (a bitmap or an icon).
All the elements in the red group (elements 1, 2 and 3) contain direction
sensitive images; that is, the images in these elements must be flipped
horizontally when displayed in mirrored UIs. Not flipping direction sensitive
graphics can cause various UI problems. For example, not flipping the resize
bitmap (item 3) will cause the UI to appear incorrectly - a Southeast-oriented
resize bar will appear in the Southwest corner of the window. Not flipping the
back button (item 1) will result in a confusing user experience - the button
will say 'Back' but the arrow will point to a direction indicating going
'Forward'. All the elements in the black group (elements A, B and C) do not
contain direction sensitive images. Thus, the images in these elements must look
the same in mirrored and non-mirrored UIs. Generally, legal trademarks, logos,
favicons, etc. are not direction sensitive and flipping them horizontally in
mirrored UIs does not make sense. Following is the Hebrew (and therefore
mirrored) version of Windows Explorer:

[<img alt="image"
src="/developers/design-documents/ui-mirroring-infrastructure/explorer_window_he.jpg">](/developers/design-documents/ui-mirroring-infrastructure/explorer_window_he.jpg)

The first thing to note about the Hebrew version is that right alignment is used
for all the text elements (English and Hebrew). Also, all the UI text (menus,
buttons, status bar info, etc.) is translated to Hebrew. All the UI elements and
sub-elements are mirrored recursively. For example, the position of the folder
tree view is mirrored (it is on the right-hand side of the window and not the
left-hand side). The elements in the tree view (the plus signs, the directory
icons, etc.) also appear in their mirrored position within the tree view. Note
how even though the position of all the highlighted elements is mirrored, the
elements in the black group look exactly the same as they did in the English
version while the elements in the red group are flipped horizontally.

### How does Windows do it?

The Windows API provides a set of useful techniques for dealing with RTL UI
localization. The goal of the Windows UI mirroring API is to allow GUI
developers to write UI code without having to worry about mirroring the UI for
right-to-left locales. The UI is created using well known Windows GUI constructs
(HWNDs, Device Contexts, Dialog Boxes, etc.) and then an RTL layout property can
be applied to individual elements in order to mirror their UI. For example, the
property WS_EX_LAYOUTRTL can be applied to an HWND in order to mirror the UI of
the underlying window. When an RTL layout is applied to a GUI element, the
element's coordinate system is transformed in the following manner:

[<img alt="image"
src="/developers/design-documents/ui-mirroring-infrastructure/ws_layout_rtl.jpg">](/developers/design-documents/ui-mirroring-infrastructure/ws_layout_rtl.jpg)

When using an RTL layout, the right-most horizontal coordinate value is 0
(instead of the leftmost) and the coordinate values increase (instead of
decrease) when moving from right to left on the X axis. Using this
transformation, a UI element the application positions in the top left corner
will be positioned on the top right corner when the UI is mirrored. The concept
of coordinate transformation is extremely powerful since it doesn't require the
GUI application's logic to change in order to support mirroring. In reality,
however, more often than not mirroring a GUI application involves much more than
just setting a few flags. There are a few bad programming practices that cause
the GUI application to appear incorrectly when mirrored. For the most part these
have to do with incorrect conversions between screen and window coordinates. To
learn more about the Windows mirroring API and about how to avoid programming
techniques that don't work well in mirrored UIs, see the following [MSDN
article](http://msdn2.microsoft.com/en-us/goglobal/bb688119.aspx).

## ChromeViews mirroring infrastructure

Chromium has a portable, object oriented UI framework (aka *ChromeViews*) that
allows developers to write Chromium UI elements. In order to allow the Chromium
UI to be mirrored easily, a mirroring infrastructure needed to be added to the
*ChromeViews* framework. The inherent Windows support could not be used by
itself to mirror the Chromium UI for a few different reasons:

*   In order to render the Chromium UI elements inside a Windows HWND
            (such as the window created by XPFrame), the entire Chromium UI is
            rendered into a DIB and then the DIB is being *BitBlt*ed into the
            underlying HWND's device context (or HDC). Setting the
            WS_EX_LAYOUTRTL property of the top level Chromium HWND means that
            any text drawn into the DIB (using Windows' DrawText API) will need
            to be drawn such that each and every glyph is mirrored (so that
            flipping the DIB into the mirrored HDC will display the correct
            text). The problem with this method is that it will break ClearType
            because ClearType text rendering is direction dependant (which
            subpixels the ClearType algorithm sets depends on where the
            subpixels are relative to the drawn glyphs); in other words,
            flipping a bitmap containing ClearType is guaranteed to break the
            cripsness of the text.
*   *ChromeViews* is a portable framework and therefore it does not make
            much sense to use a platform-specific solution for mirroring in code
            that needs to remain portable.
*   The Windows mirroring applies to the HWND hierarchy of windows and
            controls. *ChromeViews* has its own concept of UI elements hierarchy
            and it is not possible to use the Windows API for mirroring and
            still have the flexibility of controlling mirroring in a *View*
            subclass granularity; in other words, not every *View* subclass is
            associated with a Windows HWND handle and it is therefore not
            possible to control the setting of each and every Chromium UI
            element by only using HWND mirroring. As we'll see later on, the
            Chromium mirroring infrastructure does use Windows native mirroring
            for the Chromium native UI controls.

### ChromeViews::View mirroring support

The Chromium mirroring infrastructure and API resides mostly in the
*ChromeViews::View* class since it is the base class for the majority of the
Chromium browser UI elements. Before describing the interface, it is important
to note that whenever we refer to the concept of 'locale' or 'current locale' we
refer to the Chromium UI language. The locale concept entails more than just the
UI language, but for the purpose of explaining the mirroring infrastructure, it
is OK to ignore the other locale aspects or the logic Chromium uses in order to
set the default locale.

Just like the Windows mirroring API, the corresponding Chromium API is designed
in a way that frees individual GUI developers from the burden of
programmatically mirroring UI elements and sub elements. However, the API has
certain limitations and in some cases more mirroring-related code needs to be
written for specific Chromium UI elements. Chromium *View* subclasses that
follow a certain set of conventions (described in **How to write RTL-ready
Chromium UI**, below), are going to work well in mirrored UIs and won't need to
have additional code for handling right-to-left layout issues. For a complete
documentation of the mirroring API, refer to the comments in view.h. Here is a
brief description of the new mirroring functions:

*   **GetBounds(), GetX()** overloaded versions were added that take a
            parameter indicating whether to apply the mirroring coordinate
            transformation to the X coordinate of the *View*. Most *View*
            subclasses don't need to use these new methods and should continue
            using the version of these two functions that don't take the new
            transformation parameter.
*   **UILayoutIsRightToLeft()** determines whether the layout for the
            *View* is right-to-left. The *View*'s layout is RTL if the locale's
            language is a right-to-left language AND the RTL layout is not
            disabled for the *View*.
*   **FlipCanvasOnPaintForRTLUI()** determines whether the
            *ChromeCanvas* used by the *View* to draw graphics (when the
            *View::Paint()* is invoked) should be transformed such that bitmaps
            are automatically flipped horizontally. This function returns true
            if the UI layout for the *View* is RTL AND if canvas flipping has
            been enabled for the *View* using*EnableCanvasFlippingForRTLUI* (see
            later).
*   **EnableUIMirroringForRTLLanguages()** allows RTL UI layout to be
            enabled/disabled for the *View*. This is useful if a specific *View*
            subclass instance needs to use an LTR layout regardless of the
            locale.
*   **EnableCanvasFlippingForRTLUI()** allows RTL canvas flipping to be
            enabled/disabled for the *View*. Using this method, it is easy to
            control whether or not the graphics drawn by the *View* is going to
            be flipped horizontally when the locale is RTL. Generally, canvas
            flipping is enabled for *Views* representing direction sensitive UI
            elements and it is disabled for direction insensitive*Views*.
*   **MirroredX()** returns the mirrored X coordinate for the *View*.
            There is generally no need for subclasses to use this method.
*   **MirroredLeftPointForRect()** returns the mirrored X coordinate of
            a given region relative to the *View*'s bounds. This method is used
            only when the *View* is drawing different UI elements on the canvas
            and thus needs to 'manually' mirror the position of each such
            element if the UI layout is RTL.
*   **MirroredXCoordinateInsideView()** allows mirroring a given X
            coordinate inside the *View* bounds. Generally, this functionality
            is needed only for handling drag/drop operations, to make sure that
            the drop position is aligned with the display position of the
            *Views* in question.
*   **GetViewForPoint(), ConvertPointForView(), etc.** the various
            point-to-view methods have been modified to use the mirrored
            position of the *View* so that things like mouse events use the
            mirrored point of the *View* inside the parent.

**NOTE: unlike Windows, the mirroring infrastructure in Chromium does not rely
on coordinate transformations. Regardless of the locale, mouse events are always
relative to the top left corner of the View.**

The best way to demonstrate how the API works is to look at a hypothetical
*View* subclass named Foo that contains three child *View*s. Here is the code
snippet from the header file:

class BitmapView : public ChromeViews::View {
...
public:
BitmapView() {
// By default, we want to flip the canvas so that the bitmap is flipped
// when the UI layout is right-to-left.
EnableCanvasFlippingForRTLUI(true);
}
virtual void Paint(ChromeCanvas\* canvas) {
canvas-&gt;DrawBitmapInt(bitmap_, 0, 0);
}
private:
// The bitmap we draw on the canvas.
SkBitmap bitmap_;
...
}
class Foo : public ChromeViews::View {
public:
// Layout the three child bitmap Views diagonally from top left to bottom
// right.
virtual void Layout() {
...
}
private:
BitmapView\* bitmap_one_;
BitmapView\* bitmap_two_;
BitmapView\* bitmap_three_;
...
}

The *Foo* class contains three child *View*s that are laid out diagonally from
top left to bottom right. Each child is an instance of the *BitmapView* subclass
which represents a single bitmap. The following diagram visualizes the impact of
different API calls on the manner in which *Foo* is rendered. Note that in the
example, children one, two, and three contain a bitmap with the corresponding
number.

[<img alt="image"
src="/developers/design-documents/ui-mirroring-infrastructure/mirroring_api.jpg">](/developers/design-documents/ui-mirroring-infrastructure/mirroring_api.jpg)

It is important to emphasize that even though the mirroring API contains quite a
few elements, *View* subclasses don't need to explicitly use the API in the
majority of the cases. As an example, consider *BrowserToolbarView*, which is a
*View* subclass with several child *Views*. Here is a snippet from
toolbar_view.h with the definition of the different Toolbar controls:

class BrowserToolbarView : public ChromeViews::View,
public Menu::BaseControllerDelegate,
public ChromeViews::ViewMenuDelegate,
public ChromeViews::DragController,
public LocationBarView::Delegate {
...
// Controls
ChromeViews::Button\* back_;
ChromeViews::Button\* forward_;
ChromeViews::Button\* reload_;
ChromeViews::ToggleButton\* star_;
LocationBarView\* location_bar_;
GoButton\* go_;
ChromeViews::MenuButton\* page_menu_;
ChromeViews::MenuButton\* app_menu_;
...
}

All the toolbar's UI sub elements are child *View*s and the toolbar class does
not render any UI elements itself. Instead, it delegates painting to its sub
elements (we'll explain later why this is the preferred way to structure a
*View* subclass). Here is a snippet from the code in
*BrowserToolbarView::Layout()*:

void BrowserToolbarView::Layout() {
...
back_-&gt;GetPreferredSize(&sz);
back_-&gt;SetBounds(kControlIndent,
kControlVertOffset,
sz.cx,
sz.cy);
forward_-&gt;GetPreferredSize(&sz);
forward_-&gt;SetBounds(back_-&gt;GetX() + back_-&gt;GetWidth(),
kControlVertOffset,
sz.cx,
sz.cy);
...
}

The toolbar lays out its child elements using the normal *View* positioning API
(GetX(), GetBounds(), etc.) and does not have to special case the layout for RTL
locales. Finally, here is what the toolbar looks like in RTL and LTR locales:

[<img alt="image"
src="/developers/design-documents/ui-mirroring-infrastructure/chrome_toolbar.jpg">](/developers/design-documents/ui-mirroring-infrastructure/chrome_toolbar.jpg)

The toolbar's code didn't need to change at all in order to appear correctly in
an RTL UI. The toolbar is an example of a *View* subclass for which UI mirroring
is taken care off automatically by the new framework. This is unfortunately not
the case for all the Chromium UI elements.

### Chromium support for native mirroring

Most of the Chromium mirroring infrastructure resides in *ChromeViews*. However,
since Chromium also uses Windows native UI controls, the mirroring
infrastructure includes interfaces that allow mirroring these controls easily.
Here is a brief description of the interface:

*   **NativeControl::GetAdditionalExStyle()** when *NativeControl*
            subclasses invoke *::CreateWindowEx* in order to create the
            underlying Windows control, they must bitwise-or the value returned
            from this function with the control's extended style. This way, if
            mirroring is turned on for the control, then WS_EX_LAYOUTRTL will be
            passed when creating the control.
*   **MenuHostWindow::Delegate::IsRightToLeftUILayout()** this virtual
            method lets the menu delegate control the directionality of the
            menu. By default, the direction is derived from the locale's
            direction so generally subclasses don't need to override this
            method.

## How to write RTL-ready Chromium UI

The mirroring infrastructure described in this document is designed to be
transparent to GUI developers. However, this is true only if the UI elements
adhere to a certain set of conventions. Not following these rules when writing a
UI element generally means that mirroring-specific code will need to be added to
the element so that it is displayed correctly in right-to-left locales. This
section describes the rules together with examples that demonstrate why not
following these rules is problematic. Even though it is discouraged to put
mirroring-specific code in a UI element (it is better to just follow the rules),
this section shows how to handle mirroring manually in UI components. This is
useful for UI component that, for whatever reason, can not be written in a
manner that allows the mirroring infrastructure to display the component
correctly in RTL locales.

**Rule #1: represent UI sub elements as child *View*s**

The Chromium toolbar example presented earlier is an example of a Chromium
*View* subclass that contains several child *View*s as its sub elements. The
toolbar lays out the sub elements, but it does not do any rendering. Instead,
rendering is done by the individual sub elements. In other words, for the
toolbar, the 'leaf' elements in the UI tree are the only elements that draw
graphics on the canvas during *View::Paint()*. This method plays well with the
mirroring infrastructure since the coordinate transformation can be easily
applied to the bounds of the child elements. One of the Chromium UI components
that does not adhere to this rule is the *Tab* (see chrome/browser/tabs/tab.h).
The *Tab* contains three UI sub elements. A 'close' button, a title and a
favicon. The button is represented as a *ChromeView::Button* child *View*. The
title and the favicon are both drawn directly on the canvas by the *Tab* class
during *View::Paint()*. While the tabs look correctly on left-to-right locales,
they don't on RTL locales:

[<img alt="image"
src="/developers/design-documents/ui-mirroring-infrastructure/tabs_layout_bug.jpg">](/developers/design-documents/ui-mirroring-infrastructure/tabs_layout_bug.jpg)

As the image shows, the button is positioned correctly on the left-hand side of
the tab because during *Tab::Layout()*, it is positioned on the right-hand side
(which becomes left when applying the coordinate transformation). The title and
the favicon, on the other hand, are not positioned in the same manner since they
are not child views. During *Tab::Paint()*, they are still drawn on the
left-hand side, instead of the right-hand side. Readers of this (already long
and boring) document are probably thinking the following right now: "This can
probably be solved if *View::EnableCanvasFlippingForRTLUI(true);* is invoked in
*Tab*'s constructor so that the canvas is transformed.". Well, think again:

[<img alt="image"
src="/developers/design-documents/ui-mirroring-infrastructure/tabs_flipping_bug.jpg">](/developers/design-documents/ui-mirroring-infrastructure/tabs_flipping_bug.jpg)

Flipping the canvas makes sense only for 'leaf' *View*s that draw graphics on
their entire area. When drawing different elements on the canvas, it is not
possible to use the mirroring infrastructure in order to control the appearance
of each element. The best way to fix the problem is to represent the title and
the favicon as child views. If that's not possible, the position of these
elements needs to be mirrored manually:

favicon_bounds_.set_x(MirroredLeftPointForRect(favicon_bounds_));
title_bounds_.set_x(MirroredLeftPointForRect(title_bounds_));

With this change, the tabs are displayed properly:

[<img alt="image"
src="/developers/design-documents/ui-mirroring-infrastructure/tabs_fixed.jpg">](/developers/design-documents/ui-mirroring-infrastructure/tabs_fixed.jpg)

**Rule #2: don't make assumptions such as top-left is always northwest**.

When your *View* subclass is being mirrored, its coordinate system is
transformed and therefore if you are processing a mouse event for the point
(0,0), the actual mouse position is the Northeast corner of your control. Thus,
you should not write your GUI code assuming a left-to-right coordinate system.
If the logic of your code depends on the actual position of the mouse event,
then you should use the mirroring API in order to find out whether or not the
coordinate system is transformed. See chrome\\views\\custom_frame_window.cc for
an example.

**More rules will be added later on as I continue analyzing the cause of the
most common Chromium mirroring bugs. For now, try to adhere to the initial
rules.**.

## How to test a mirrored Chromium UI element

If you write a new Chromium UI element or modify an existing element, it is
highly important that you make sure the element is mirrored correctly in
right-to-left locales. In order to do that, you can either change the UI
language of Chromium via the **Options** dialog, or you can run Chromium from
the command line such that the Hebrew UI is used:

> Windows: chrome.exe --lang=he

> Linux: Confirm the **he_IL** locale is installed on the system by running
> \`locale -a\` to list. Then: LANG=he_IL.UTF-8 LANGUAGE=he_IL chrome

> Mac: Use the Language and Region settings to change OS X to Hebrew. Then
> restart Chrome.

Once you do that, you can look at your control and make sure it doesn't have
layout bugs. Note that your machine probably doesn't include the files required
for displaying right-to-left languages and therefore all the Hebrew strings will
be rendered from left to right which will make them unreadable. It will also
make certain Unicode control characters to appear as little squares surrounding
the text. This does not impact the layout of the Chromium UI elements so it
shouldn't prevent you from making sure your UI element is properly laid out. If
you want the Hebrew strings to appear correctly and you want the annoying
squares to go away, you can easily install the required Windows right-to-left
language files by going to **Control Panel &gt; Regional and Language Options**,
selecting the **Languages** tab, and selecting the option to **Install files for
complex script and right-to-left languages (including Thai)**. Note that
installing the right-to-left files requires a restart.

I am still trying to figure out if there is a way to write a UI test that makes
sure UI mirroring is not broken so that people can make sure they don't break
anything before a check-in. Suggestions are welcome!
