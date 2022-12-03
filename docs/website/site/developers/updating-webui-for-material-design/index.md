---
breadcrumbs:
- - /developers
  - For Developers
page_name: updating-webui-for-material-design
title: Updating WebUI for Material Design
---

We intend to migrate the WebUI in Chromium to implement the [material
design](http://www.google.com/design/spec/material-design/introduction.html)
spec. This document explains the technical aspects of our approach and serves as
a landing page for resources used by developers working on this migration.
Questions can be submitted to the
[chromium-polymer](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-polymer)
Google group, but general Polymer questions are better answered at
[polymer-dev@googlegroups.com](https://groups.google.com/forum/#!forum/polymer-dev).

[This CL](https://codereview.chromium.org/1223793018/) demonstrates the addition
of a new WebUI page that uses Polymer components.

## Working with Polymer elements

### Importing new Polymer elements

In order to make use of Polymer elements in WebUI, they must first be added to
Chromium.

To add a Polymer component not already in Chromium:

1.  Add the element to `third_party/polymer/v1_0/bower.json`:

    ```none
    "iron-icon": "PolymerElements/iron-icon#~1.0.0"
    ```

    Also add all of the component's dependencies (listed in its `bower.json`. We
    list all dependencies explicitly to prevent accidental additions of other
    third-party libraries.
2.  From `third_party/polymer/v1_0`, run `reproduce.sh`. See the README
            for more details.
3.  If the element needs to be built into Chrome (as opposed to elements
            only used for demos), use tools/polymer to add the necessary `.css`,
            `.extracted-js` and `.html` files to
            `ui/webui/resources/polymer_resources.grdp`:

    ```none
    tools/polymer/polymer_grdp_to_txt.py ui/webui/resources/polymer_resources.grdp > polymer.txt
    vim polymer.txt # add iron-icon/iron-icon.html, etc.
    tools/polymer/txt_to_polymer_grdp.py polymer.txt > ui/webui/resources/polymer_resources.grdp
    git diff ui/webui/resources/polymer_resources.grdp # verify changes
    ```

Once the resources are added to Chrome, they can be included from another HTML
page via `chrome://resources:`

> `<link rel="import"
> href="chrome://resources/polymer/v1_0/iron-icon/iron-icon.html">`

### Adding custom elements

Find the appropriate place for the new element. For example, elements within the
"settings" namespace use `chrome/browser/resources/settings`.

Separate HTML and JS into their own files instead of using inline scripts to
ensure elements will work under
[CSP](https://developer.chrome.com/extensions/contentSecurityPolicy).

Add the element's resources to the appropriate `.grd` file. Do not include demo
files.

### Sharing Polymer elements

Polymer elements intended to be shared across WebUI reside in
src/ui/webui/resources/cr_elements.

New WebUI pages implementing material design should make use of these elements
when convenient.

## Localization

## Prefer C++-side $i18n{key} templating. When necessary, use `I18nBehavior` to
dynamically translate strings in JS.

## Accessibility

Polymer elements provide their own ARIA roles and labels, and a11y support
should be improving soon in Polymer 1.0. Custom elements can also provide their
own ARIA roles and labels when necessary.

Shadow DOM is already supported in some screen readers, such as OS X's VoiceOver
and Chrome OS's [ChromeVox Next](/developers/accessibility/chromevox).

As with light DOM, the shadow DOM tree can be explored from
chrome://accessibility.

## Compilation

[Using the Closure
Compiler](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/closure_compilation.md)
to typecheck your code is strongly recommended. This requires [annotating your
JavaScript with JSDoc
tags](https://developers.google.com/closure/compiler/docs/js-for-compiler) and
paying attention to types. [Closure's Polymer
pass](https://github.com/google/closure-compiler/wiki/Polymer-Pass) allows for
annotation of Polymer-specific constructs.

## Testing

See [Testing WebUI with
Mocha](/developers/updating-webui-for-material-design/testing-webui-with-mocha).

We also want to be able to perform visual testing to check for regressions in
element layout and looks.

## Clients

WebUI pages implementing material design:

*   [Settings](/developers/updating-webui-for-material-design/settings-material-design)
*   Extensions
*   [Media Router](/developers/design-documents/media-router)
*   History
*   PDF viewer
*   Downloads (complete)
