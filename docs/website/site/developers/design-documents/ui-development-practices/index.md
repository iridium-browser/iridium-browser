---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: ui-development-practices
title: UI Development Practices
---

Guidelines

Principles for developing for Chrome. These best practices center around
ensuring features are implemented as efficiently as possible.

## Content Area (UI in a Tab)

The UI that lives in the content area should be implemented with HTML
technologies on desktop platforms. (Either web or native can be used for the
content area UI on Android and iOS.) This HTML can be either WebUI or plain HTML
and JS bundled with Chrome. It should work with no connection and lo-fi
connections. The expectation is that the implementation does not contain
polyfills, code that only executes for other browsers, or arcane Web development
practices.

## Non-Content Area (UI in Popups or in Chrome’s Chrome)

UI outside of the content area should be implemented using
[Views](https://code.google.com/p/chromium/codesearch#chromium/src/ui/views/)
(in C++) for Windows/Linux/ChromeOS. Mac UI is done with Cocoa although work is
progressing on porting Views to it as well. The expectation is that UI of this
style is limited to small, transient windows with low-complexity UI. On Clank,
this UI should be implemented using the Android View system.

## Manipulating DOM on Arbitrary Pages

Some features need to inspect or modify the DOM of the current tab, which can be
any arbitrary website. Blink's Web\* C++ API is intentionally simple and limited
to guide you to use it only for what is practical. When Blink’s C++ API is
impractical, use [isolated
world](https://cs.chromium.org/chromium/src/content/public/browser/render_frame_host.h?q=ExecuteJavaScriptInIsolatedWorld&sq=package:chromium&dr=CSs&rcl=1467300303&l=130)
script injection. The script injected should be as lean as possible and should
not contain any code that deals with other browsers (for example Closure).

Because script injection carries a runtime and memory cost, it is expected that
it is driven by a user action that has an expectation of work. A good example is
‘translate’. If inspecting every page that the user might open is required,
script injection is not recommended. For this case, consult with
chrome-eng-review@ for an appropriate solution. For example, reader mode
[converted](https://crbug.com/509869) the triggering logic from script injection
to native implementation.

## Prototyping and Exploring the Solution Space

Experimenting can be accomplished with an extension. Non-confidential features
can be published on the Chrome Web Store and installed manually for early
testing. Confidential extensions can be enabled only for trusted testers so they
are invisible to the public. Extensions can use content scripts to do script
injection and can use page actions and browser actions to provide UI outside the
content area. Extensions can also use NaCL, which has full access to the Pepper
API.

Component extensions (i.e. extensions automatically installed into Chrome) are
also a prototyping option, but should only be used if there is something needed
that a regular extension cannot do. Extensions should strive to only execute on
explicit user action and never on startup. Persistent background pages should be
avoided. Event pages should only be used for infrequent items like push
messaging, hardware events, alarms etc…, but not common events like navigation.
If you need to do something that imposes a constant load or requires updating
state on each navigation, consult the extensions team; they might be able to
provide a declarative API so the job can be done with low impact. Failing that,
consult with chrome-eng-review@ if you need to deviate from these guidelines.
