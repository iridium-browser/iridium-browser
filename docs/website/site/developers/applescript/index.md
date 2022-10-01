---
breadcrumbs:
- - /developers
  - For Developers
page_name: applescript
title: Information for Third-party Applications on Mac
---

JavaScript injection via AppleScript will soon be disabled by default in Chrome
stable. We’re making this change in order to protect users from injection of
unwanted content.

Users who need this functionality can re-enable it by going to the menu bar,
View &gt; Developer &gt; Allow JavaScript from Apple Events

Alternatively, applications that legitimately need to inject JavaScript into
Chrome can do so by publishing an[
extension](https://developer.chrome.com/extensions) and using Native Messaging
to communicate with their application process. For more information, see[
https://developer.chrome.com/extensions/nativeMessaging](https://developer.chrome.com/extensions/nativeMessaging)

For automation, developers can also use Telemetry:
<https://github.com/catapult-project/catapult/blob/master/telemetry/README.md>
