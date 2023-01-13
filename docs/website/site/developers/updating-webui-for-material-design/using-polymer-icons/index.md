---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/updating-webui-for-material-design
  - Updating WebUI for Material Design
page_name: using-polymer-icons
title: Adding SVG icons
---

SVG icons should be added to a common iconset, or an iconset for your WebUI.
These can be custom SVG icons or copies of Polymer's
`[iron-icons](https://github.com/PolymerElements/iron-icons)`.

## Creating an iconset

To create the iconset your WebUI will use, copy the format of an existing icons
file (e.g.,
[cr_elements/icons.html](https://source.chromium.org/chromium/chromium/src/+/HEAD:ui/webui/resources/cr_elements/icons.html)).
Change the name of the `iron-iconset-svg` for your WebUI, and add the SVGs you
need (see below). Example:

```none
<iron-iconset-svg name="downloads" size="24">
  <svg>
    <defs>
      <g id="foo">...</g>
      ...
    </defs>
  </svg>
</iron-iconset-svg>
```

## Adding icon definitions

### For Polymer icons from iron-icons:

*   Find the SVG `<g>` icon definitions you need in
            [PolymerElements/iron-icons](https://github.com/PolymerElements/iron-icons).
*   Copy the SVG definitions.
*   Insert each line (alphabetically) into the appropriate `icons.html`
            file for your WebUI. (For commonly used icons, use
            `cr_elements/icons.html`.)

### For custom icons:

You can include custom icons or [Google Material
icons](https://design.google.com/icons/) in the same iconset, but mention their
source in a comment. Icons exported from GUI tools often are messy; please
**minify custom icons** by flattening transforms, removing meta tags like
`<title>`, and rounding weird numbers like 30.0002118. The [SVGOMG
optimizer](https://jakearchibald.github.io/svgomg/) does most of this for you;
use it.

If the icon is not the same size as the iconset, either scale it to size, or
ensure the `<g id="foo">` tag includes the `viewBox` attribute with the original
dimensions. Some code (settings) has multiple iconsets (e.g. "cr" vs. "cr20")
depending on the export size. You can skip the viewBox if you put the icon in
the right set.

Unless color is important (e.g. a Chromium logo), **remove hard-coded colors**
(e.g. `fill="#000"`) so our WebUI can specify colors in CSS.

## Using icons

Import the relevant `icons.html` from your UI **instead of** importing Polymer's
`iron-icons.html` (which would load almost a thousand SVGs).

Use the icon in your UI with your custom iconset name prefix wherever you need
an icon name (`iron-icon`, `paper-icon-button`, etc.):

```none
<link rel="import" href="chrome://downloads/icons.html">
<iron-icon icon="downloads:warning"></iron-icon>
```

## Common icons

A few icons are used in many pages. These can be found in
`[cr_elements/icons.html](https://code.google.com/p/chromium/codesearch#chromium/src/ui/webui/resources/cr_elements/icons.html&q=file:cr_elements/icons%5C.html&sq=package:chromium&type=cs&l=1)`
under the iconset name "`cr`". Having frequently-used icons here prevents
excessive duplication of SVG definitions while keeping imports small across all
pages.

To use these icons, simply import `cr_elements/icons.html` and use the `cr`
prefix:

```none
<link rel="import" href="chrome://resources/cr_elements/icons.html">
<iron-icon icon="cr:cancel"></iron-icon>
```
