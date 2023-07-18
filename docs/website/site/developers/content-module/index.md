---
breadcrumbs:
- - /developers
  - For Developers
page_name: content-module
title: Content module
---

This documentation moved to the repository at
<https://chromium.googlesource.com/chromium/src/+/HEAD/content/README.md>
(except for the diagram below).

[<img alt="image"
src="/developers/content-module/Content.png">](https://docs.google.com/a/chromium.org/drawings/d/13yo_bSgwVdOUJFCIeVLL_rmtQ2SqElmxouC81q46GAk/edit?hl=en_US)

The diagram above illustrates the layering of the different modules. A module
can include code directly from lower modules. However, a module can not include
code from a module that is higher than it. This is enforced through DEPS rules.
Modules can implement embedder APIs so that modules lower than them can call
them. Examples of these APIs are the WebKit API and the Content API.
