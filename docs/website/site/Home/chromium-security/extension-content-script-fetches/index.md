---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: extension-content-script-fetches
title: Changes to Cross-Origin Requests in Chrome Extension Content Scripts
---

*tl;dr: To improve security, cross-origin fetches will soon be disallowed from
content scripts in Chrome Extensions. Such requests can be made from extension
background pages instead, and relayed to content scripts when needed. **\[The
document has been edited on 2020-09-17 to reflect that CORS-for-content-scripts
has successfully launched in Chrome 85****.\]***

## Overview

When web pages request cross-origin data with
[fetch](https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API) or
[XHR](https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest) APIs, the
response is denied unless
[CORS](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS) headers allow it.
In contrast, extension content scripts have traditionally been able to [fetch
cross-origin data](https://developer.chrome.com/apps/xhr) from any origins
listed in their extension's
[permissions](https://developer.chrome.com/extensions/declare_permissions),
regardless of the origin that the content script is running within. As part of a
broader [Extension Manifest
V3](https://docs.google.com/document/d/1nPu6Wy4LWR66EFLeYInl3NzzhHzc-qnk4w4PX-0XMw8/edit?usp=sharing)
effort to improve extension security, privacy, and performance, these
cross-origin requests in content scripts will soon be disallowed. Instead,
content scripts will be subject to the same request rules as the page they are
running within. Extension pages, such as background pages, popups, or options
pages, are unaffected by this change and will continue to be allowed to bypass
CORS for cross-origin requests as they do today.

Our data shows that most extensions will not be affected by this change.
However, any content scripts that do need to make cross-origin requests can do
so via an extension background page, which can relay the data to the content
script. We have a migration plan below to help affected extension developers
make the transition to the new model.

## Problems with Cross-Origin Requests

To prevent leaks of sensitive information, web pages are generally not allowed
to fetch cross-origin data. Unless a valid CORS header is present on the
response, the page's request will fail with an error like:

> Access to fetch at 'https://another-site.com/' from origin
> 'https://example.com' has been blocked by CORS policy: No
> 'Access-Control-Allow-Origin' header is present on the requested resource. If
> an opaque response serves your needs, set the request's mode to 'no-cors' to
> fetch the resource with CORS disabled.

Chrome has recently launched a new security feature called [Site
Isolation](https://developers.google.com/web/updates/2018/07/site-isolation)
which enforces this type of restriction in a more secure way. Specifically, Site
Isolation not only blocks the response, but prevents the data from ever being
delivered to the Chrome renderer process containing the web page, using a
feature called [Cross-Origin Read
Blocking](https://developers.google.com/web/updates/2018/07/site-isolation#corb)
(CORB). This helps prevent the data from leaking even if a malicious web page
were to attack a security bug in Chrome's renderer process, or if it tried to
access the data in its process with a [Spectre
attack](https://security.googleblog.com/2018/07/mitigating-spectre-with-site-isolation.html).

Content scripts pose a challenge for Site Isolation, because they run in the
same Chrome renderer process as the web page they operate on. This means that
the renderer process must be allowed to fetch data from any origin for which the
extension has permissions, which in many cases is *all* origins. In such cases,
Site Isolation would have less effectiveness when content scripts are present,
because a compromised renderer process could hijack the content scripts and
request (and thus leak) any data from the origins listed in the extension.
(Thankfully, this is not a problem for Spectre attacks, which cannot take
control of content scripts. It is a problem if an attacker can exploit a
security bug in Chrome's renderer process, though, allowing the attacker to
issue arbitrary requests as if they came from the content script.)

To mitigate these concerns, future versions of Chrome will limit content scripts
to the same fetches that the page itself can perform. Content scripts can
instead ask their background pages to fetch data from other origins on their
behalf, where the request can be made from an extension process rather than a
more easily exploitable renderer process.

## Planned Restrictions

As described above, content scripts will lose the ability to fetch cross-origin
data from origins in their extension's permissions, and they will only be able
to fetch data that the underlying page itself has access to. To fetch additional
data, content scripts can send messages to their extension's background pages,
which can relay data from sources that the extension author expects.

This transition will occur in stages, to try to minimize disruption to extension
developers.

Stage #1: Remove ability to bypass CORB from content scripts

**In Q1 2019**, Chrome removed the ability to make cross-origin requests in
content scripts for new and previously unaffected extensions, while maintaining
an "allowlist" of affected extensions that may continue to make such requests
for the time being. This change started in Chrome 73.

We continue to work with developers of extensions on the allowlist to migrate to
the new method of requesting cross-origin data, to help them prepare for
Extension Manifest V3. We remove such extensions from the allowlist as they
migrate, helping to improve the security of Chrome and the effectiveness of Site
Isolation against advanced attackers.

Stage #2: Remove ability to bypass CORS from content scripts **\[edited on
2020-09-17\]**

In **Q2 2020**, Chrome removed the ability to bypass CORS in cross-origin
requests from content scripts, subject to the same “allowlist” as above. This
change started in Chrome 85.

The changes means that cross-origin fetches initiated from content scripts will
have an Origin request header with the page's origin, and the server has a
chance to approve the request with a matching Access-Control-Allow-Origin
response header.

Extensions that were previously added to the “allowlist” will be unaffected by
the changes in Chrome 85. However, the new CORS behavior for content scripts may
actually make it easier for some extensions to move off of the allowlist, if
their fetches would now be approved by the server with an
Access-Control-Allow-Origin response header. We know this is the case for
several extensions that were included on the allowlist.

Stage #3: Deprecating and removing the “allowlist” **\[edited on 2020-09-17\]**

\[edited on 2020-05-28\] **In October 2020**, we will publish the allowlist of
extensions, to inform users of the security risks of using any extensions still
on the list. We hope to shrink the list as quickly as possible, because using
these extensions will weaken Chrome's defenses against cross-site attacks.
Before publishing or deprecating the allowlist, we'll send an advance notice to
the
[chromium-extensions@](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-extensions)
discussion list.

\[added on 2020-09-17\] In Chrome 87 we have deprecated and removed the
CORB/CORS allowlist. According to [the Chrome
dashboard](https://chromiumdash.appspot.com/schedule), this Chrome release is
tentatively planned to ship to the Beta channel around October 15th 2020 and to
the Stable channel around November 17th 2020. Extensions that haven’t migrated
to the new security model may be broken in Chrome 87 and above.

## Recommended Developer Actions

To prepare for Extension Manifest V3 and avoid being on the allowlist of
extensions that pose a cross-site security risk, we recommend that affected
extension developers take the following actions:

### 1. Determine if Your Extension is Affected \[edited on 2020-03-09\]

You can test whether your extension is affected by the planned CORB and CORS
changes by running Chrome 81 or later (starting with version 81.0.4035.0) with
the following [command line flags](/developers/how-tos/run-chromium-with-flags)
to enable the planned behavior:

> --force-empty-corb-allowlist
> --enable-features=OutOfBlinkCors,CorbAllowlistAlsoAppliesToOorCors

Alternatively (starting with version 85.0.4175.0) opt into the changes by
setting chrome://flags/#cors-for-content-scripts to “Enabled” and
chrome://flags/#force-empty-CORB-and-CORS-allowlist to “Enabled”:

> [<img alt="image"
> src="https://drive.google.com/uc?id=1xPpHGMmjgMOYDEZH1U02XvAxDHDPMwoc&export=download">](https://drive.google.com/file/d/1xPpHGMmjgMOYDEZH1U02XvAxDHDPMwoc/view?usp=drive_web)

If your extension makes cross-origin fetches from content scripts, then your
extension may be broken and you may observe one of the following errors in the
DevTools console:

> Cross-Origin Read Blocking (CORB) blocked cross-origin response &lt;URL&gt;
> with MIME type &lt;type&gt;. See
> https://www.chromestatus.com/feature/5629709824032768 for more details.

**\[added on 2020-03-09\]**:

> Access to fetch at 'https://another-site.com/' from origin
> 'https://example.com' has been blocked by CORS policy: No
> 'Access-Control-Allow-Origin' header is present on the requested resource. If
> an opaque response serves your needs, set the request's mode to 'no-cors' to
> fetch the resource with CORS disabled.

If you see the errors above, you can verify whether the changes described on
this page are the cause by temporarily disabling the planned behavior. (It is
possible that the errors might appear for other reasons.) To test with the
planned behavior disabled, run Chrome 81 or later (starting with version
81.0.4035.0) with the following [command line
flags](/developers/how-tos/run-chromium-with-flags):

> --disable-features=CorbAllowlistAlsoAppliesToOorCors
> --enable-features=OutOfBlinkCors

### 2. Avoid Cross-Origin Fetches in Content Scripts

When cross-origin fetches are needed and the server does not provide an
Access-Control-Allow-Origin response header for the page's origin, perform them
from the [extension background
page](https://developer.chrome.com/extensions/background_pages) rather than in
the content script. Relay the response to the content scripts as needed (e.g.,
using [extension messaging
APIs](https://developer.chrome.com/extensions/messaging)). For example:

> **Old content script, making a cross-origin fetch:**

> > var itemId = 12345;

> > var url = "https://another-site.com/price-query?itemId=" +

> > encodeURIComponent(request.itemId);

> > fetch(url)

> > .then(response =&gt; response.text())

> > .then(text =&gt; parsePrice(text))

> > .then(price =&gt; ...)

> > .catch(error =&gt; ...)

> **New content script, asking its background page to fetch the data instead:**

> > chrome.runtime.sendMessage(

> > {contentScriptQuery: "queryPrice", itemId: 12345},

> > price =&gt; ...);

> **New extension background page, fetching from a known URL and relaying
> data:**

> > chrome.runtime.onMessage.addListener(

> > function(request, sender, sendResponse) {

> > if (request.contentScriptQuery == "queryPrice") {

> > var url = "https://another-site.com/price-query?itemId=" +

> > encodeURIComponent(request.itemId);

> > fetch(url)

> > .then(response =&gt; response.text())

> > .then(text =&gt; parsePrice(text))

> > .then(price =&gt; sendResponse(price))

> > .catch(error =&gt; ...)

> > return true; // Will respond asynchronously.

> > }

> > });

### 3. Limit Cross-Origin Requests in Background Pages

If an extension's background page simply fetches and relays *any* URL of a
content script's choice (effectively acting as an open proxy), then similar
security problems occur. That is, a compromised renderer process can hijack the
content script and ask the background page to fetch and relay sensitive URLs of
the attacker's choosing. Instead, background pages should only fetch data from
URLs the extension author intends, which is ideally a small set of URLs which
does not put the user's sensitive data at risk.

> **Good message example:**

> {

> contentScriptQuery: "queryPrice",

> itemId: 12345

> }

> This approach limits which URLs can fetched in response to the message. Here,
> only the itemId is provided by the content script that is sending the message,
> and not the full URL.

> **Bad message example:**

> {

> contentScriptQuery: "fetchUrl",

> url: "https://example.com/any/path/or/site/allowed/here"

> }

> In this approach the content script may cause the background page to fetch any
> URL. A malicious website may be able to forge such messages and trick the
> extension to get access to any cross-origin resources.

### 4. Keep in Touch if Needed \[edited on 2020-05-28\]

We have reached out to developers whose extensions are on the allowlist.

If your extension is on the allowlist and no longer needs to be, please [**file
a bug
here**](https://bugs.chromium.org/p/chromium/issues/entry?template=Defect+report+from+user&components=Internals%3ESandbox%3ESiteIsolation&blocking=846346&cc=lukasza@chromium.org&summary=Remove+extension+%3Cextension+id%3E+from+the+CORB+allowlist)
to have it removed. You may verify that your extension no longer needs to be on
the allowlist by testing that it continues to work after launching Chrome
81.0.4035.0 or higher with the following [command line
flags](/developers/how-tos/run-chromium-with-flags):

> --force-empty-corb-allowlist
> --enable-features=OutOfBlinkCors,CorbAllowlistAlsoAppliesToOorCors

Alternatively (starting with version 85.0.4175.0) opt into the changes by
setting chrome://flags/#cors-for-content-scripts to “Enabled” and
chrome://flags/#force-empty-CORB-and-CORS-allowlist to “Enabled”:

> [<img alt="image"
> src="https://drive.google.com/uc?id=1xPpHGMmjgMOYDEZH1U02XvAxDHDPMwoc&export=download">](https://drive.google.com/file/d/1xPpHGMmjgMOYDEZH1U02XvAxDHDPMwoc/view?usp=drive_web)

If your extension is not yet on the allowlist and still depends on cross-origin
requests, these requests may stop working and you may observe the following
errors in the DevTools console:

> Cross-Origin Read Blocking (CORB) blocked cross-origin response &lt;URL&gt;
> with MIME type &lt;type&gt;. See
> https://www.chromestatus.com/feature/5629709824032768 for more details.

or **\[added on 2020-03-09\]**:

> Access to fetch at 'https://another-site.com/' from origin
> 'https://example.com' has been blocked by CORS policy: No
> 'Access-Control-Allow-Origin' header is present on the requested resource. If
> an opaque response serves your needs, set the request's mode to 'no-cors' to
> fetch the resource with CORS disabled.

If this happens, please update your extension as described above.

\[edited on 2019-09-21\] Under exceptional circumstances, we may still consider
adding an extension to the allowlist, but we're now generally avoiding this when
possible, because users of allowlisted extensions are vulnerable to additional
security attacks. To report issues encountered during the Chrome 87 deprecation
of the allowlist, please [open a Chromium
bug](https://bugs.chromium.org/p/chromium/issues/entry?template=Defect+report+from+user&components=Internals%3ESandbox%3ESiteIsolation&blocking=846346&cc=lukasza@chromium.org&summary=CORB/CORS+allowlist+deprecation+in+Chrome+87).

## Summary

Removing cross-origin fetches from content scripts is an important step in
improving the security of Chrome, since it helps prevent leaks of sensitive data
even when Chrome's renderer process might be compromised. We apologize for the
inconvenience of the migration, but we appreciate your help in keeping Chrome's
users as secure as possible.
