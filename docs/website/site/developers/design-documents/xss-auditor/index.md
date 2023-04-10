---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: xss-auditor
title: XSS Auditor
---

## Note: [An Intent to Deprecate and Remove the XSS Auditor](https://groups.google.com/a/chromium.org/forum/#!msg/blink-dev/TuYw-EZhO9g/blGViehIAwAJ) was published on 15-July-2019. The feature was [permanently disabled](https://chromium.googlesource.com/chromium/src.git/+/73d3b625e731badaf9ad3b8f3e6cdf951387a589) on 5-August-2019 and shortly after fully [removed](https://bugs.chromium.org/p/chromium/issues/detail?id=968591#c10) for Chrome 78.

## Design

The XSS Auditor runs during the HTML parsing phase and attempts to find
[reflections](https://en.wikipedia.org/wiki/Cross-site_scripting#Reflected_(non-persistent))
from the request to the response body. It does not attempt to mitigate Stored or
DOM-based XSS attacks.

If a possible reflection has been found, Chrome may ignore (neuter) the specific
script, or it may block the page from loading with an ERR_BLOCKED_BY_XSS_AUDITOR
error page.

The original design <http://www.collinjackson.com/research/xssauditor.pdf> is
the best place to start. The current rules are an evolved response to things
observed in the wild.

## Performance

Processing costs are essentially zero unless the URL or POST body includes any
of the four characters **" &gt; &lt; '**. When those characters are found, we
only invoke heavy processing on those attributes that might be dangerous. This
stands in contrast to the XSS Filter in Internet Explorer, which runs costly
regular expressions.

## False Negatives (Bypasses and bugs)

Bypasses are [not considered security
bugs](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/faq.md#Are-XSS-filter-bypasses-considered-security-bugs),
but should be logged against the [XSS Auditor component](https://goo.gl/4xSVhV)
as functional issues. Stored (attacks where the script is stored in a site's
database, for instance) and DOM-based XSS attacks (where JavaScript on the page
unsafely manipulates content retrieved from document.location.href, for
instance) are out-of-scope and do not need to be logged.

## False Positives

## Unlike implementations in some other browsers, Chrome's XSS Auditor runs on
same-origin navigations; this increases the risk of a false-positive but
provides better protection against multi-stage attacks.

## The XSS Auditor looks for reflected content within the context of executable
(script) nodes. By checking for reflections only in contexts where script may be
executed, the Auditor avoids many types of false positive. Because text that is
reflected into a non-executable context (e.g. into a &lt;textarea&gt; element)
does not trigger the Auditor.

## However, the Auditor has no way of knowing whether a given script block which
appears in both the request and the response was truly reflected from the
request to the response. For instance, [consider this
page](http://webdbg.com/test/xss/alert12345.aspx) that *always* contains the
markup &lt;script&gt;alert("12345");&lt;/script&gt;. If the user navigates to
this page normally, the Auditor does not trigger. However, if the user navigates
to this page [using a URL](http://webdbg.com/test/xss/auditor.aspx) whose query
string contains ?query=&lt;script&gt;alert("12345");&lt;/script&gt;, the Auditor
concludes that the script appearing within the response page may be a reflection
from the request (an XSS attack), and the XSS Auditor blocks the response.

## In the past, the XSS Auditor defaulted to neutering only the
potentially-reflected block, leaving the rest of the page intact. However, this
creates a vulnerability whereby an attacker may
"[snipe](https://bugs.chromium.org/p/chromium/issues/detail?id=825675)" an
unwanted block of script from a victim page by sending the script-to-kill in the
request body. To mitigate such attacks, as of Chrome 57, the XSS Auditor [now
blocks](https://www.chromestatus.com/features/5748927282282496) the response
entirely.

## In some scenarios, a site legitimately wishes to send SCRIPT markup from one
page to another where that markup may already appear (for instance, in the HTML
Editing UI of a CMS or bulletin board site). In such cases, the site developers
should either encode the content before transferring it (e.g. encode the content
as base64, such that it is not identified as a reflection), or by opting out of
the XSS Auditor by setting the HTTP response header.

## Control via Response Header

Sites may control the XSS Auditor's behavior using the X-XSS-Protection response
header, either disabling the feature or changing its mode.

This Response header disables the Auditor:

X-XSS-Protection: 0

This Response header enables the Auditor and sets the mode to neuter:

X-XSS-Protection: 1

This Response header enables the Auditor and sets the mode to block (the
default):

X-XSS-Protection: 1; mode=block

When the feature is enabled (state "1"), Chrome's XSS Auditor allows the header
to specify a report URI to which violation reports should be sent. In Chrome 64
and 65, that URI must be same-origin to the page, but that limit was [removed in
Chrome 66](https://crbug.com/811440). If your report URL includes a semicolon,
be sure to [wrap the URL in quotation marks](https://crbug.com/825557).

X-XSS-Protection: 1; mode=block; report=https://example.com/log.cgi

X-XSS-Protection: 1; report="https://example.com/log.cgi?jsessionid=132;abc"

## Debugging via View Source

## If the XSS Auditor blocks or modifies a response because it detected
reflected script, the View Source (CTRL+U) view for that page will point to the
script that was reflected:

## [<img alt="View-Source view highlights the reflected text"
src="/developers/design-documents/xss-auditor/XSSAuditorViewSource.png">](/developers/design-documents/xss-auditor/XSSAuditorViewSource.png)

## Code

The code for the XSS Auditor feature can be found in Blink at
[XSSAuditor.h](https://cs.chromium.org/chromium/src/third_party/blink/renderer/core/html/parser/xss_auditor.h)
and
[XSSAuditorDelegate.h](https://cs.chromium.org/chromium/src/third_party/blink/renderer/core/html/parser/xss_auditor_delegate.h?).
Performance tests can be found
[here](https://cs.chromium.org/chromium/src/third_party/blink/perf_tests/xss_auditor/).

## History

The XSS Auditor was introduced [in Chrome
4](https://blog.chromium.org/2010/01/security-in-depth-new-security-features.html)
in 2010.

## Abuse

The XSS Auditor works by matching request data to data in a response page, which
may be cross-origin. This creates the possibility that the XSS Auditor may be
abused by an attacker to determine the content of a cross-origin page, a
violation of Same Origin Policy.

In a canonical attack, the attacker frames a victim page with a string of
interest in it, then attempts to determine that string by making a series of
successive guesses until it **detects blocking** by the XSS Auditor. If the XSS
Auditor blocks the page, the attacker concludes that their guess was correct.
This form of attack is constrained to pages matching certain
[criteria](https://bugs.chromium.org/p/chromium/issues/detail?id=176137#c17).

To combat this threat, the attacker should not be able to detect that the
Auditor has blocked loading of the page. Most known
[detection](https://crbug.com/176137) [mechanisms](https://crbug.com/396544)
have been fixed, although a website with a content injection vulnerability may
remain [vulnerable](https://crbug.com/396544#c14) and [quirks of
same-origin-policy](http://blog.portswigger.net/2015/08/abusing-chromes-xss-auditor-to-steal.html)
may permit detection of blocking.
