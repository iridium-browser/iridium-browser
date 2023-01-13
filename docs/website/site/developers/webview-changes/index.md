---
breadcrumbs:
- - /developers
  - For Developers
page_name: webview-changes
title: Shipping changes that are webview-friendly
---

Shipping changes to Android WebView requires some extra due-diligence.
Changing features on the web platform should always be done with care, but
here are a few ways WebView is special:

* Native apps are often built on this technology, and are less likely to use the
  WebView beta program. In general, they may pay less attention to the web
  platform than they do to the Android platform, which has a less frequent and
  different API update & deprecation mechanism.

* Analytics are more difficult to get - UKM is not present, and UMA may be less
  complete as some apps opt-out.

* A/B experiments are less effective, for similar reasons.

* There is less engineering effort on this platform, and it is often not top of
  mind for Chromium developers.

The kinds of changes that substantially affect WebView are usually the
same kinds that substantially affect other platforms, but in some cases there
may be more risk to WebView, especially if it involves places where the WebView
architecture [differs significantly](https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/web-platform-compatibility.md)
from other platform. In addition, just as with
[enterprise changes](https://www.chromium.org/developers/enterprise-changes),
some APIs may be more prevalant on that platform than others.

A change to Android WebView is *potentially high risk* if one of the following hold:
* Theh change removes an API or  changes its behavior such that app code that calls the API may start throwing an exception or otherwise hard-crash
* The change has significant impact on the architecture of the Android WebView platform as it differs from other platforms

A change to Android WebView is likely not *potentially high risk* if one of the following hold:
* The change is to ship a new API
* The possible app impact of the change is cosmetic or non-fatal (e.g. a CSS property changing behavior)
* Data analysis shows that affected apps are very rare
