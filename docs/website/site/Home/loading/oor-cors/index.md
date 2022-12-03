---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/loading
  - Loading
page_name: oor-cors
title: 'OOR-CORS: Out of Renderer CORS'
---

**CORS: Cross-Origin Resource Sharing**

Cross-Origin Resource Sharing is a standardized mechanism to negotiate access
permissions among the web browser and servers for a visiting site. See, [MDN's
document](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS) for details.

Chrome 79 will replace the CORS implementation to be more secure. As a result,
there are behavior changes for several reasons, and it will cause compatibility
issues. New CORS implementation, aka OOR-CORS, will be rolled out incrementally,
starting on January 6th, 2020, over the following several weeks. For WebView, it
will be enabled later so that WebView based applications can migrate safely. It
will probably happen at Chrome 83-84.

For enterprise users, we are providing Enterprise Policies to mitigate the
compatibility issues, or manage to use the legacy CORS implementation until
Chrome 82. See [Chrome Enterprise release notes - Chrome
79](https://support.google.com/chrome/a/answer/7679408) for details. These are
available on the Chrome Admin Console now.

**Extra Resources for WebView developers**

We have a document for WebView specific details to clarify CORS behaviors on
Android/WebView specific schemes, APIs and so on. Please check the document,
[CORS and WebView
API](https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/cors-and-webview-api.md).

**Behavior Changes by OOR-CORS**

**Resource Timing API does not count CORS preflight request as a separate
entry**

With the legacy CORS, [Resource Timing
API](https://w3c.github.io/resource-timing/) counts CORS preflight request as a
separate entry. But this is not aligned with the spec requirement. Once the
OOR-CORS is rolled out, it does not. See [the relevant crbug
entry](https://bugs.chromium.org/p/chromium/issues/detail?id=982924) for
detailed discussion.

**Cross-origin redirects for &lt;img crossorigin=anonymous&gt; do not send
Cookies any more**

With the legacy CORS, Chrome had a bug to send cookies even after cross-origin
redirects, though this violated the spec. Once the OOR-CORS is rolled out, it
does not.

**XHR failures for intent://... will dispatch readystatechange and error events
(Android only)**

With the legacy CORS, Android Chrome had a bug to fail silently without
notifying any error on fetching intent:// over XHR. Once the OOR-CORS is rolled
out, it will dispatch **readystatechange** and **error** events correctly. For
other APIS, image loading, Fetch API, and so on, it correctly reports errors
until today, and from now on.

**Extensions' webRequest API (Desktop only)**

There are announced API changes. See [the API document,
chrome.webRequest](https://developer.chrome.com/extensions/webRequest), for
details. There are three major changes explained as "Starting from Chrome 79,"
in the document.

**Internally modified requests will also follow the CORS protocol**

With the legacy CORS, internally modified requests didn't follow the CORS
protocol correctly. For instance, Chrome sometime injects extra headers in
enterprise uses for access controls. In such case, Chrome won't send CORS
preflight even for the case that the modified requests does not meet the "simple
request" conditions. But once the OOR-CORS is fully enabled, Chrome will follow
the CORS protocol strictly even if the request is modified by intermediate code
as it can as possible. This also affects Chrome Extensions, and part of the
announced API changes are related to this enforcement. This behavior change may
affect Chrome Extensions that intercepts and modifies requests to Google
services or responses from Google services.

**Behavior Changes by Other Blink Updates**

**BUG: Redirects from allowlisted content scripts blocked by CORB+CORS in Chrome
79**

See [crbug.com/](http://crbug.com/)[1034408](http://crbug.com/1034408). There is
a temporary breakage in the original CORS implementation at Chrome 79. This will
be fixed by enabling OOR-CORS or updating to Chrome 80.

**CSS -webkit-mask starts using CORS-enabled requests from Chrome 79**

See [crbug.com/](http://crbug.com/)[786507](http://crbug.com/786507) and
[crbug.com/](http://crbug.com/)[1034942](http://crbug.com/1034942). This will
result in observing CORS related errors if the same URL is also requested by
other no-cors requests and the server does not care for HTTP caches. [CORS
protocol and HTTP
caches](https://fetch.spec.whatwg.org/#cors-protocol-and-http-caches) section in
the fetch spec will help you to understand the problem.

**Origin header from Extensions' background page is changed from Chrome 80 (may
be postponed for breaking many?)**

See [crbug.com/](http://crbug.com/)[1036458](http://crbug.com/1036458). When
Chrome sends a request from Extensions' background page and Origin header is
needed, chrome-extensions://&lt;extensions id&gt; has been set. But from Chrome
80, the origin of the target URL will be used.

**Troubleshooting**

If a site stops loading sub-resources correctly, or stops working correctly,
please [open the
DevTools](https://developers.google.com/web/tools/chrome-devtools/open) and [see
the console](https://developers.google.com/web/tools/chrome-devtools/console) to
check if there are CORS related errors. You may see the following keywords:

*   CORS
*   Cross-origin requests
*   Access-Control-Allow-...
*   preflight

You can check all keywords in [the relevant source
code](https://cs.chromium.org/chromium/src/third_party/blink/renderer/platform/loader/cors/cors_error_string.cc)
if you are interested in.

If there are, something OOR-CORS incompatible issues may happen.

The first check point for desktop users is Chrome Extensions. As explained in
the behavior changes, there are some API changes, and some Chrome Extensions may
not follow up the change. You can try a new Chrome profile to see if the same
problem happens without any Chrome Extensions. If it solves the problem, one of
your installed Chrome Extensions may cause the issue. You can disable each
Chrome Extensions at chrome://extensions/ step by step to find the problematic
one. Once you find the one, please report the issue to the Chrome Extensions
developers. For enterprise users, there is a workaround,
[CorsMitigationList](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=CorsMitigationList).
If you set this policy (empty list is also fine), all Chrome Extensions run in a
compatible mode. This will cause small negative performance impact, but it will
work.

If you are a bit familiar with CORS, you can do further debugging to have a
crafted workaround or to find a solution. If you see CORS preflight failures in
the logs, and you are sure that the target server does not handle CORS preflight
correctly, you can craft the
[CorsMitigationList](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=CorsMitigationList)
not to send a preflight request for such condition. You will put your seeing
non-standard HTTP header name into the
[CorsMitigationList](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=CorsMitigationList).
For instance, if the CORS preflight has the following line in the request
header:

> Access-Control-Request-Headers: my-auth,my-account

adding "my-auth" and "my-account" into the
[CorsMitigationList](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=CorsMitigationList)
will stop sending the CORS preflight for the case. Registered header names will
be exempted from the CORS preflight condition checks as [CORS-safelisted request
headers](https://fetch.spec.whatwg.org/#cors-safelisted-request-header). Note
that this may allow malicious attackers to exploit through potential server side
vulnerability on handling these headers.

CORS releated detailed network transaction can not be observed via DevTools'
[Network](https://developers.google.com/web/tools/chrome-devtools/network) tab.
You need to take a [NetLog dump](/for-testers/providing-network-details) for
further investigation. You can use
==[netlogchk.html](/Home/loading/oor-cors/netlogchk.html)== to analyze the
obtained NetLog dump to see if there is CORS related error. [NetLog
Viewer](https://netlog-viewer.appspot.com/#import) is general purpose online
tools to check details on the dump.

The last resort for enterprise users is
[CorsLegacyModeEnabled](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=CorsLegacyModeEnabled).
It will allow you to use the legacy CORS instead of OOR-CORS. For other users,
setting chrome://flags/#out-of-blink-cors to Disabled will have the same effect.
But this option will be removed at Chrome m83. So please be careful about that.
You should contact us through [this bug report
link](https://bugs.chromium.org/p/chromium/issues/entry?components=Blink%3ESecurityFeature%3ECORS,Blink%3ELoader&cc=toyoshim@chromium.org).
Concrete repro steps or [NetLog dump](/for-testers/providing-network-details)
will help us and make investigation smooth.

**WebView Specific Information**

OOR-CORS will be launched for WebView in M83 incrementally. We will start
enabling the feature step by step from Jun.1st, 2020.

**Mitiation:** Enterprise policies are not available on WebView, but the
OOR-CORS can be controlled via [WebView
DevTools](https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/developer-ui.md)
per device basis. The document says the tool is supported in WebView 84+. But,
actually, it experimentally supports OOR-CORS feature control in WebView 83. The
device needs to [enable USB
debugging](https://developer.android.com/studio/debug/dev-options) to inactivate
the OOR-CORS. But once the setting is changed, users can disable the debugging
mode.

**NetLog dump:** WebView does not allow end users to take NetLog dump, and
“userdebug” or “eng” builds of Android system are needed to take. See [Net
debugging in
WebView](https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/net-debugging.md)
for details.

**DevTools:** End users can not use DevTools for remote debugging as they do for
Chrome. This is because the debugging functionality is disabled by default on
recent Android systems. WebView application developers can modify their
application code to allow remote debugging to debug CORS issues. See the
article, [Remote Debugging
Webviews](https://developers.google.com/web/tools/chrome-devtools/remote-debugging/webviews).

**Bug Reports:** Please file a report from [this
link](https://bugs.chromium.org/p/chromium/issues/entry?components=Blink%3ESecurityFeature%3ECORS,Blink%3ELoader,Mobile%3EWebView&cc=toyoshim@chromium.org)
(Components: Blink&gt;SecurityFeatyre&gt;CORS,
Blink&gt;Loader,Mobile&gt;WebView; CC: toyoshim@chromium.org). Early reports
without strong confidence are welcomed as the team wants to get early feedback
so that the team can provide a fixed binary ASAP.

Information for Chrome Developers

**FYI Builders**

Now the OOR-CORS is enabled by default on the main waterfall, and OOR-CORS
enabled fyi bots were turned down. Insteads Blink-CORS bots are running to
monitor legacy Blink implementation.

-
[linux-oor-cors-rel](https://ci.chromium.org/p/chromium/builders/ci/linux-oor-cors-rel)
(turned down)

- [Android WebView P OOR-CORS FYI
(rel)](https://ci.chromium.org/p/chromium/builders/ci/Android%20WebView%20P%20OOR-CORS%20FYI%20%28rel%29)
(turned down)

-
[linux-blink-cors-rel](https://ci.chromium.org/p/chromium/builders/ci/linux-blink-cors-rel)

- [Android WebView P Blink-CORS FYI
(rel)](https://ci.chromium.org/p/chromium/builders/ci/Android%20WebView%20P%20Blink-CORS%20FYI%20%28rel%29)
- wpt may fail and should be be compared with [Android WebView P FYI
(rel)](https://ci.chromium.org/p/chromium/builders/ci/Android%20WebView%20P%20FYI%20%28rel%29)
tracked by [crbug.com/1011098](http://crbug.com/1011098)
