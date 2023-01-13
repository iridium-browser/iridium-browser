---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: corb-for-developers
title: Cross-Origin Read Blocking for Web Developers
---

**Cross-Origin Read Blocking (CORB)** is a new web platform security feature
that helps mitigate the threat of side-channel attacks (including Spectre). It
is designed to prevent the browser from delivering certain cross-origin network
responses to a web page, when they might contain sensitive information and are
not needed for existing web features. For example, it will block a cross-origin
text/html response requested from a &lt;script&gt; or &lt;img&gt; tag, replacing
it with an empty response instead. This is an important part of the protections
included with [Site Isolation](/Home/chromium-security/site-isolation).

This document aims to help web developers know what actions they should take in
response to CORB. For more information about CORB in general, please see the
[CORB
explainer](https://chromium.googlesource.com/chromium/src/+/HEAD/services/network/cross_origin_read_blocking_explainer.md),
the [specification](https://fetch.spec.whatwg.org/#corb), the ["Site Isolation
for web developers"
article](https://developers.google.com/web/updates/2018/07/site-isolation), or
the following [talk](https://youtu.be/dBuykrdhK-A) from Google I/O 2018. (The
CORB discussion starts at [around 23:20
mark](https://youtu.be/dBuykrdhK-A?t=1401).)

#### Lessons from Spectre and Meltdown, and how the whole web is getting safer

## How can I ensure CORB protects resources on my website?

To make sure that sensitive resources on your website (e.g. pages or JSON files
with user-specific information, or pages with CSRF tokens) will not leak to
other web origins, please take the following steps to distinguish them from
resources that are allowed to be embedded by any site (e.g., images, JavaScript
libraries).

### For HTML, JSON, and XML resources:

Make sure these resources are served with a correct "Content-Type" response
header from the list below, as well as a "X-Content-Type-Options: nosniff"
response header. These headers ensure Chrome can identify the resources as
needing protection, without depending on the contents of the resources.

*   [HTML MIME type](https://mimesniff.spec.whatwg.org/#html-mime-type)
            - "text/html"
*   [XML MIME type](https://mimesniff.spec.whatwg.org/#xml-mime-type) -
            "text/xml", "application/xml", or any MIME type whose subtype ends
            in "+xml"
*   [JSON MIME type](https://mimesniff.spec.whatwg.org/#json-mime-type)
            - "text/json", "application/json", or any MIME type whose subtype
            ends in "+json"

Note that we recommend not supporting **multipart** [range
requests](https://developer.mozilla.org/en-US/docs/Web/HTTP/Range_requests) for
sensitive resources, because this changes the MIME type to multipart/byteranges
and makes it harder for Chrome to protect. Typical range requests are not a
problem and are treated similarly to the nosniff case.

In addition to the recommended cases above, Chrome will also do its best to
protect responses labeled with any of the MIME types above and without a
"nosniff" header, but this has limitations. Many JavaScript files on the web are
unfortunately labeled using some of these MIME types, and if Chrome blocked
access to them, existing websites would break. Thus, when the "nosniff" header
is not present, Chrome first looks at the start of the file to try to confirm
whether it is HTML, XML, or JSON, before deciding whether to protect it. If it
cannot confirm this, it allows the response to be received by the cross-site
page's process. This is a best-effort approach which adds some limited
protection while preserving compatibility with existing sites. We recommend that
web developers include the "nosniff" header to protect their resources, to avoid
relying on this "confirmation sniffing" approach.

### For other resource types (e.g., PDF, ZIP, PNG):

Make sure these resources are served only in response to requests that include
an unguessable CSRF token (which should be distributed via resources protected
via the HTML, JSON, or XML steps above).

## What should I do about CORB warnings reported by Chrome?

When CORB blocks a HTTP response, it emits the following warning message to the
DevTools console in Chrome:

> Cross-Origin Read Blocking (CORB) blocked cross-origin response
> https://www.example.com/example.html with MIME type text/html. See
> <https://www.chromestatus.com/feature/5629709824032768> for more details.

> (In Chrome 66 and earlier, this message was slightly different: Blocked
> current origin from receiving cross-site document at
> https://www.example.com/example.html with MIME type text/html.)

In most cases, the blocked response should not affect the web page's behavior
and the CORB error message can be safely ignored. For example, the warning may
occur in cases when the body of the blocked response was empty already, or when
the response was going to be delivered to a context that can't handle it (e.g.,
a HTML document such as a 404 error page being delivered to an &lt;img&gt; tag).
***Note:** Chrome will stop showing warning messages for empty or error
responses in Chrome 69, since these are false positives that do not affect site
behavior. If you see CORB warnings in Chrome 67 or 68, please test the site in
Chrome 69 to see if any warnings remain.*

In rare cases, the CORB warning message may indicate a problem on a website,
which may disrupt its behavior when certain responses are blocked. For example,
a response served with a "X-Content-Type-Options: nosniff" response header and
an incorrect "Content-Type" response header may be blocked. This could, for
example, block an actual image which is mislabeled as "Content-Type: text/html"
and "nosniff." If this occurs and interferes with a page's behavior, we
recommend informing the website and requesting that they correct the
"Content-Type" header for the response.

If you suspect Chrome is incorrectly blocking a response and that this is
disrupting the behavior of a website, [please file a Chromium
bug](https://goo.gl/XBoKtY) describing the incorrectly blocked response (both
the headers and body) and/or the URL serving it. You can confirm if a problem is
due to CORB by temporarily disabling it, by starting Chrome with the following
command line flag:

--disable-features=CrossSiteDocumentBlockingAlways,CrossSiteDocumentBlockingIfIsolating
