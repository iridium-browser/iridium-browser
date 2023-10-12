---
breadcrumbs: []
page_name: x-subresources
title: X-Subresources
---

## Andrew Oates &lt;[aoates@google.com](mailto:aoates@google.com)&gt;, Bryan McQuade &lt;[bmcquade@google.com](mailto:bmcquade@google.com)&gt;, Mike Belshe &lt;[mbelshe@google.com](mailto:mbelshe@google.com)&gt;

## The Problem

A typical modern webpage fetches many external resources. www.cnn.com, for
example, loads 150+ external resources on a fresh page load. The browser has no
way of knowing which resources will be required for a page until it reaches the
tags that reference them, or until a script sends off the request dynamically.
The more time it takes for the browser to discover that it will need a resource,
the more time it will take to download the entire page.

In older browsers, this problem was exacerbated by the fact that the browsers
would block when downloading external JavaScript resources. Blocking on
JavaScript downloads delayed the time to discover additional subresources needed
in the page. Recent browsers (IE8+, Safari 4+, Chrome2+, FF3.5+) can download
scripts in parallel, which mitigates this particular part of the problem.

We've also seen traces from real world web pages, where the top of the page is
loaded with several kilobytes (even after compression) of inline CSS and
JavaScript. If you had 20KB of such information at the top of the page, and
downloaded the page over a slow network (such as a 256Kbps cell phone), it could
request those resources as much as 600ms earlier than it otherwise could! Now,
the observant reader may notice that if the network is fully utilized there is
no point in adding additional resources to the download, as they will compete
for bandwidth and slow each resource down. However, such networks usually have
large RTT in addition to low bandwidth. The time to get requests to the server
can be 200+ms. Although the bandwidth may not be immediately available, getting
the RTTs started in parallel provides an advantage. Further, servers that use
this experiment can intelligently schedule the responses (potentially even
across connections) to avoid such contention issues.

Evidence suggests that we can improve page load times for some sites if the
server can inform the client of subresources earlier than it does today.

## Challenges

When downloading additional resources sooner, we need to ensure that the
downloading of those resources does not defer the downloading of primary
content. For example, the browser may be critically blocked on the downloading
of the main HTML or the main CSS file. If we inform the client of low-priority
images earlier, we should make sure that downloading them will not delay the
higher priority resources.

## The Experiment

If browser's knew what subresources they would need for a given page, they could
kick off the requests at the start of the page load, and wouldn't have to block
on fetching javascript files (only on execution). This experiment proposes a new
HTTP header, "X-Subresources" which can inform the client of subresources that
may be needed when downloading a resource much earlier than when the client
would otherwise discover the resource. The browser can optionally use this
header to download content sooner than it otherwise would have done so.

## Other Solutions

Many alternative approaches to this problem exist. One simple one would be to
consider putting this information into the HTML content (perhaps as LINK
headers) as opposed to putting it into the HTTP headers. There are advantages to
this approach, because having the server know about subresources for a given
content page can be difficult to discover or manage. This approach would inform
the browser of the subresources marginally later, but probably not significantly
later. We haven't rejected this approach, and it may be a good idea to support
as well. The reason we chose the HTTP header is because we'd like the origin
server to be able to discover these optimizations on-the-fly without
intervention from the content author. Content authors often do not know about
the "tricks" to optimally load a web page, and giving servers the tools so that
they can perform these changes automatically, we hope this feature is more
useful. We could, of course, have servers modify the content being issued to the
client on-the-fly, but this can be more difficult to do efficiently. All in all,
we hope the solution we've chosen is a good starting point for this experiment.

## Informal Specification

Client-side

If a client wishes to receive subresource notification headers, it should
include a X-subresource header on the appropriate requests:

X-Subresource: enabled

When a client receives a response with the X-subresource header, it should
decode the value, and generate requests for the URIs listed in the header. If
multiple subresources are listed, it should fetch them in the order that is
listed.

### Server-side

When a server receives a request with an appropriate X-subresource header, it
may generate a list of subresources to be used by the requested page. It could
do this by scraping the HTML, consulting a database, etc. It should then return
this list as a X-subresource header to the client as part of the HTTP response
header block.
The server must not use X-Subresource headers for resources which are not
cacheable, or whose lifetime might expire before the end of the current page
load.

#### Header Format

The X-subresource header has the following format:

header-value ::= type ; resource \[; type; resource\]

type ::= text

resource ::= URI

Each resource consists of a type and a URI. The type is the mime-type of the
URI, such as "text/html", "image/gif", etc. The URI is a standard HTTP URI.

For example:

X-subresource: text/css; http://www.foo.com/styles/foo.css;
image/gif;/images/img1.gif
