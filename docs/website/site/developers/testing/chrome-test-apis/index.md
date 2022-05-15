---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: chrome-test-apis
title: chrome.test APIs
---

The chrome.test extension API is defined in
[src/extensions/common/api/test.json](https://code.google.com/p/chromium/codesearch#chromium/src/extensions/common/api/test.json&q=test.json&sq=package:chromium&type=cs),
that file also has some documentation for some of the methods it defines. The
same code that injects all other extension APIs in the right context
([extensions::Dispatcher::UpdateBindingsForContext](https://code.google.com/p/chromium/codesearch#chromium/src/extensions/renderer/dispatcher.cc&l=1089))
also is responsible for injecting the chrome.test bindings (there is some
special code there to not inject the chrome.test API unless the process has a
--test-type command line flag). The
[src/extensions/common/api/_api_features.json](https://code.google.com/p/chromium/codesearch#chromium/src/extensions/common/api/_api_features.json&l=270)
file determines in what contexts the chrome.test API (and other extension APIs
as well) will be injected.