---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/updating-webui-for-material-design
  - Updating WebUI for Material Design
page_name: settings-material-design
title: 'Settings: Material Design'
---

**Eng**: dbeam@, dpapad@, dschuyler@, hcarmona@, michaelpg@, stevenjb@,
tommycli@

**PM**: tbuckley@

**UX**: bettes@

**Code Location**: chrome/browser/resources/settings,
chrome/browser/ui/webui/settings

## Objective

To create a new Settings experience for Chromium/Chromium OS which will
implement the material design spec, using web components as a maintainable and
modular implementation.

## Using

Visit chrome://md-settings, or use #enable-md-settings in chrome://flags. This
is still a work in progress; do not expect all settings to work until launch.

## Model

The Settings UI is composed of many Polymer elements, which use two-way data
binding to update a local prefs model. A Polymer element observing this prefs
model persists changes back to the pref store via a set of granular JS APIs.

**Design Documents**

Design documents can be found
[here](https://drive.google.com/open?id=0B6VE9j_QZJCrfnZhLUNWeTlIX1JKdEJEWkoweC1sNzlKbG9MTmUyX2t1akVPenQ4UFVRWGc&authuser=2).
The following are a few major docs:

*   [Chrome Settings API
            Wrapper](https://docs.google.com/a/chromium.org/document/d/1PnGKrP6_dXS3L1h36buxHPW3zSWup5FaTaGthed3CUc/edit#heading=h.xgjl2srtytjt)
            - describes interaction between Chrome Settings page UI elements and
            underlying Chrome APIs.
*   [chrome.settingsPrivate
            API](https://docs.google.com/a/chromium.org/document/d/1PCQltNDdyZyuPHUdqYIjeObRrVOhf8WTr6BLfuTQ4ew/edit?pli=1#heading=h.adz03dmup02t)
            - describes the new extension API used to interact with the pref
            store from JS.
*   [i18n Design
            Document](https://docs.google.com/document/d/12hK4HEKOo4w9L5DT34hlTKX32SpeNepD7FoZTXRCJQY/edit#)
            - describes C++ i18n templates.
*   [Settings Routing and
            Navigation](https://docs.google.com/document/d/1VUbDrsJ4eAOAEODeegjwGNeT3iHCKWu46kJAW4gX1WM/edit)
            - describes the new routing approach.

## Individual settings pages

Each category of settings will now reside in its own page. Each page will be
represented by its own custom element, to be encapsulated within a variant of
&lt;neon-animated-pages&gt;. We split the HTML and JS such that the HTML for a
given page only contains the layout, and the JS defines the
&lt;settings-*foo*-page&gt; element.

Also, each individual page element will have a reference to a shared prefs
property. This prefs property will contain the shared model for all settings,
which in turn will be kept in sync with the Chrome pref store.

Example page code:

> **a11y_page.html:**

> `<dom-module id="settings-a11y-page">`
> ` <template>`
> ` <div class="layout vertical">`
> `` ` <settings-checkbox checked="{{prefs.settings.a11y.highContrastEnabled}}">` ``
> `` `` ` $i18n{a11yHighContrastMode}` `` ``
> `` ` </settings-checkbox>` ``
> `` ` <settings-checkbox checked="{{prefs.settings.a11y.screenMagnifierEnabled}}">` ``
> ` $i18n{a11yScreenMagnifier}`
> `` ` </settings-checkbox>` ``
> ` </div>`
> ` </template>`
> ` <script src="a11y_page.js"></script>`
> `</dom-module>`

> **a11y_page.js:**

> `Polymer({`

> ` is: `'settings-a11y-page',

> ` properties: {`

> /\*\*

> ` * Preferences state.`

> \*/

> ` prefs: {`

> ` type: Object,`

> ` notify: true,`

> ` }`

> ` }`

> `});`
