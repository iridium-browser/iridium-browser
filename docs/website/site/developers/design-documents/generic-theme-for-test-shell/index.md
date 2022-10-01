---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: generic-theme-for-test-shell
title: A Generic theme for the Test Shell
---

When running layout tests, we'd like to minimize the differences we get between
platforms, so that we don't have to maintain a large number of different
baselines. The best example of this is on Windows, where different versions of
Windows can render form controls and other widgets differently depending on your
desktop settings (classic mode vs. XP theme vs. Vista theme vs. Aero, etc.).

In order to solve this problem, we've [implemented a generic set of
widgets](http://src.chromium.org/viewvc/chrome?view=rev&revision=26161) for the
test shell (in
[test_shell_webthemecontrols.{cc,h}](http://src.chromium.org/viewvc/chrome/trunk/src/webkit/tools/test_shell/test_shell_webthemecontrol.cc)
and
[test_shell_webthemeengine.{cc,h}](http://src.chromium.org/viewvc/chrome/trunk/src/webkit/tools/test_shell/test_shell_webthemeengine.cc)
), and the test shell uses those instead of the native controls. The downside to
this, of course, is that we won't catch any regressions in the native widget
mapping code.

The new generic widgets are rendered using
[Skia](http://code.google.com/p/skia/) and are designed explicitly for
testability (rather than aesthetic concerns). Each widget is drawn differently
depending on which state the control is in (disabled, readonly, active, hovered,
etc.), and each widget is drawn differently from each other (e.g., the upper
part of the scroll bar and the lower part of the scroll bar are distinctly
different). This is different from what you might do when design real widgets
for users, because you might actually want different widgets to look the same.
In this case, we'd rather catch programmatic errors like passing the wrong
widget ID or the wrong state in.

This image gives you an idea of what all of the "generic" controls look like:

[<img alt="image"
src="/developers/design-documents/generic-theme-for-test-shell/test_shell_generic_theme.png">](/developers/design-documents/generic-theme-for-test-shell/test_shell_generic_theme.png)

As you can see, it's none too pretty, but it's quite clear what's what. Some of
the things to look for:

*   Each control state (normal, disabled, read only, hovered, etc.) is
            rendered in a different background color. Controls that are active
            in one form or another (have the keyboard focus, are hovered over,
            or are part of a larger control that has a different part active)
            also get triangles marked in one of the four corners of the widget.
*   Widgets that have "directionality" (e.g., scroll bar top vs. scroll
            bar bottom) have a square marked in the corresponding edge of the
            control (you can see this on the scroll bars in the image above).

This code is currently windows-specific, only because it implements the
[WebThemeEngine](http://src.chromium.org/viewvc/chrome/trunk/src/webkit/api/public/win/WebThemeEngine.h)
interface (as found in [src/webkit/api/win
](http://src.chromium.org/viewvc/chrome/trunk/src/webkit/api/public/win/)). We
could certainly genericize this further and use the same theme on other
platforms (Mac, Win), with the same tradeoff (we would test less of the actual
production code).
