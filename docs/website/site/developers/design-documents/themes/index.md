---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: themes
title: Themes
---

If you want to write a theme, please see the documentation at:
<http://code.google.com/chrome/extensions/themes.html>

## Implementation

Themes are installed and packaged as extensions - once installed, the theme
specification is copied into your browser preferences, allowing users to
override individual theme choices. This theme data is loaded and managed by
browser_theme_provider.cc (which also handles telling everyone to repaint when a
theme is applied).

#### Theme provider

The theme provider has the following interface:

GetBitmapNamed(int id)

This wraps ResourceBundle::GetBitmapNamed and provides themed images if they
exist, falling back to the default ResourceBundle images otherwise.

GetColor(int id)

Returns an SkColor with the specified ID (see the list of ids in
browser_theme_provider.h)

#### Views Implementation

All views have access to a theme provider through their GetThemeProvider()
method - this returns the root widget's theme provider. In browser-ui land, the
theme provider hangs off the profile, and the root widget is profile aware.

In some cases, the root widget is not profile aware, and so you will have to
inject the theme provider into the view. An example of this is tab_renderer.cc,
which doesn't have access to the profile while in its dragged state.
