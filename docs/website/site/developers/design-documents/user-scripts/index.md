---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: user-scripts
title: User Scripts
---

Chromium and Google Chrome (version 4 and higher) have built-in support for
Greasemonkey-style user scripts.

To use, click on any .user.js file. You should see an install dialog. Press OK
to install.

Known issues:

*   Chromium does not support @require, @resource, unsafeWindow,
            GM_registerMenuCommand, GM_setValue, or GM_getValue.
*   GM_xmlhttpRequest is same-origin only.

### Match Patterns

The preferred way to specify the pages that a user script should run against in
Chromium is the **@match** attribute. Here are some examples of its use:

// ==UserScript==

// @match http://\*/\*

// @match http://\*.google.com/\*

// @match http://www.google.com/\*

// ==/UserScript==

See [these
comments](https://code.google.com/p/chromium/codesearch#chromium/src/extensions/common/url_pattern.h&q=file:url_pattern.h&sq=package:chromium&l=1)
for details on the @match syntax.

Support for Greasemonkey-style @include patterns is also implemented for
compatibility, but @match is preferred.

With Greasemonkey-style @include rules, it is not possible for Chrome to know
for certain the domains a script will run on (because google.\* can also run on
google.evil.com). Because of this, Chrome just tells users that these scripts
will run on all domains, which is sometimes scarier than necessary. With @match,
Chrome will tell users the correct set of domains a user script will run on.

### Idle Injection

In Chromium/Google Chrome, Greasemonkey scripts are injected by default at a new
point called
"[document-idle](http://code.google.com/chrome/extensions/content_scripts.html)".
This is different than Greasemonkey, which always injects at document-end.

The document-idle injection point is selected automatically by the browser for
the best user-perceived performance. If the document has many external resources
like images that slow down page load, the browser will run the script at
document-end, like Greasemonkey, while waiting for resources. However, if the
page loads quickly, scripts may not be run until after window.onload has
occurred -- much later than with Greasemonkey.

The main impact this has on script developers is that you should \*not\* wait
for window.onload in Greasemonkey scripts intended for use with Chromium/Google
Chrome, because it may have already occurred when your script has run.

Note that there is normally no reason to wait for window.onload in any
Greasemonkey script, even in Firefox. Document-end and document-idle are both
guaranteed to run after the entire DOM is parsed, which is the usual thing
script developers are interested in having occurred. If for some reason you
really need your script to run after window.onload, you can check the
document.readystate property. If it is "complete", then you can assume onload
has occurred. If it isn't, then you can listen for onload.
