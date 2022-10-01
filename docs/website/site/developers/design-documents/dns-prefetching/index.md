---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: dns-prefetching
title: DNS Prefetching
---

[TOC]

## Problem

DNS resolution time can lead to a significant amount of user perceived latency.
The time that DNS resolution takes is highly variable. Latency delays range from
around 1ms (locally cached results) to commonly reported times of several
seconds.

## Solution

DNS prefetching is an attempt to resolve domain names before a user tries to
follow a link. This is done using the computer's normal DNS resolution
mechanism; no connection to Google is used. Once a domain name has been
resolved, if the user does navigate to that domain, there will be no effective
delay due to DNS resolution time. The most obvious example where DNS prefetching
can help is when a user is looking at a page with many links to various domains,
such as a search results page. When we encounter hyperlinks in pages, we extract
the domain name from each one and resolving each domain to an IP address. All
this work is done in parallel with the user's reading of the page, using minimal
CPU and network resources. When a user clicks on any of these pre-resolved
names, they will on average save about 200 milliseconds in their navigation
(assuming the user hadn't already visited the domain recently). More importantly
than the average savings, users won't tend to experience the "worst case" delays
for DNS resolution, which are regularly over 1 second.

## **Architecture**

Chromium's implementation of DNS prefetching does not use the browser's network
stack at all. Instead, it relies on external threads to resolve the names,
thereby warming the DNS cache of the operating system (completely ignoring any
cache in the application network stack). The advantage of this approach was that
it was completely compatible with all network stacks (it is external), and
prevented accidental regressions on the main network stack.

Since some DNS resolutions can take a long time, it is paramount that such
delays in one resolution should not cause delays in other resolutions. Toward
this end (on Windows, where there is no native support for asynchronous DNS
resolution), Chromium currently employs 8 completely asynchronous worker threads
to do nothing but perform DNS prefetch resolution. Each worker thread simply
waits on a queue, gets the next requested domain name, and then blocks on a
synchronous Windows resolution function. Eventually the operating system
responds with a DNS resolution, the thread then discards it (leaving the OS
cache warmed!), and waits for the next prefetch request. With 8 threads, it is
rare than more than one or two threads will block extensively, and most
resolution proceed rather quickly (or as quickly as DNS can service them!). On
Debug builds, the "about:histograms/DNS.PrefetchQueue" has current stats on the
queueing delay.

## **Manual Prefetch**

Chromium uses the "href" attribute of hyperlinks to find host names to prefetch.
However, some of those hyperlinks may be redirects, for example if the site is
trying to count how many times the link is clicked. In those situations, the
"true" targeted domain is not necessarily discernible by examining the content
of a web page, and so Chromium not able to prefetch the final targeted domain.

To improve the speed of redirects, content authors can add the following tag to
their page:

&lt;link rel="dns-prefetch"
href="[//host_name_to_prefetch.com](http://the_worlds_best_vendor.com/)"&gt;

The above "link rel" tag has no impact on the visual rendering of the page, but
causes Chromium to prefetch the DNS resolution of "host_name_to_prefetch.com" as
though there was an actual href targeted at a path in that domain. The double
slashes indicate that the URL starts with a host name (as specified in [RFC
1808](http://www.ietf.org/rfc/rfc1808.txt)). It is equivalent (but unnecessary)
to use a full URL such as "http://host_name_to_prefetch.com/".

## **DNS Prefetch Control**

By default, Chromium does not prefetch host names in hyperlinks that appear in
HTTPS pages. This restriction helps prevent an eavesdropper from inferring the
host names of hyperlinks that appear in HTTPS pages based on DNS prefetch
traffic. The one exception is that Chromium may periodically re-resolve the
domain of the HTTPS page itself.
An inquisitive content author (for example, a commenter on a blog) may abuse DNS
prefetching to attempt to monitor viewing of content containing links. For
example, links with novel subdomains, when resolved during a prefetch, may
notify a domain's resolver that a link was viewed, even if it was not clicked.
In some such cases, the authority serving the content (such as a blog owner, or
webmail server) may wish to preclude such abusive monitoring.
To allow webmasters to control whether DNS prefetch is enabled or disabled,
Chromium includes a DNS Prefetch Control mechanism. It can be used to turn DNS
prefetch on for HTTPS pages, or turn it off for HTTP pages.

Chromium watches for an HTTP header of the form "X-DNS-Prefetch-Control" (case
insensitive) with a value of either "on" or "off." This setting changes the
default behavior for the rendered content. A "meta http-equiv" tag of the same
name can be used to make policy changes within a page. If a page explicitly opts
out of DNS prefetch, further attempts to opt in are ignored.

For example, the following page from
[==https://content_author.com/==](https://content_author.com/) would cause
Chromium to prefetch "b.com" but not "a.com", "c.com", or "d.com".

&lt;a
href="[==http://a.com==](http://www.google.com/url?q=http%3A%2F%2Fa.com&sa=D&sntz=1&usg=AFQjCNEYnDzIlRjJbXDcx3Cax_hkvntAFQ)"&gt;
A) Default HTTPS: No prefetching &lt;/a&gt;
&lt;meta http-equiv="x-dns-prefetch-control" content="on"&gt;
&lt;a
href="[==http://b.com==](http://www.google.com/url?q=http%3A%2F%2Fb.com&sa=D&sntz=1&usg=AFQjCNFq7tUq50NgFVyTsk2WMeiRf6l6WQ)"&gt;
B) Manual opt-in: Prefetch domain resolution. &lt;/a&gt;
&lt;meta http-equiv="x-dns-prefetch-control" content="off"&gt;
&lt;a
href="[==http://c.com==](http://www.google.com/url?q=http%3A%2F%2Fc.com&sa=D&sntz=1&usg=AFQjCNFTPfaTERg_4COAyTmt8OqdKsFsCA)"&gt;
C) Manual opt-out: Don't prefetch domain resolution &lt;/a&gt;
&lt;meta http-equiv="x-dns-prefetch-control" content="on"&gt;
&lt;a
href="[==http://d.com==](http://www.google.com/url?q=http%3A%2F%2Fd.com&sa=D&sntz=1&usg=AFQjCNEJTYzjWlVmnzIooEfteTYJZzXmzA)"&gt;
D) Already opted out: Don't prefetch domain resolution. &lt;/a&gt;
Child frames also inherit the DNS prefetch control opt-out setting from their
parent. The DNS prefetch control setting applies only to hyperlinks and not to
the manual prefetch mechanism.

## **Browser Startup**

Chromium automatically remembers the first 10 domains that were resolved the
last time the Chromium was started, and automatically starts to resolve these
names **very** early in the startup process. As a result, the domains for a
user's home page(s), along with any embedded domains (or anything the user
"always" visits just after startup), are generally resolved before much of
Chromium has ever loaded. When Chromium finally starts to try to load and render
those pages, there is typically no DNS induced latency, and the application
effectively "starts up" (becoming usable) faster. Average startup savings are
200ms or more, with common acceleration over 1 second.

## Omnibox

Prefetching is also used in Chromium's omnibox, where URL and/or search queries
are entered. The omnibox automatically proposes an action, either of the form of
a search query, or a URL navigation, as the user types in text. This proposed
action is considered the autocompletion of what the user is typing. Each time
the omnibox makes a proposal (suggests an autocompletion), the domain for the
underlying URL is automatically pre-resolved. This means that when a user is
entering a search query, while they type the query (typically when they enter a
space between words), Chromium will automatically prefetch the resolution of the
domain in their search provider's URL. Assuming they haven't done a search in a
while, this can save a user an average of over 100ms in getting a search result,
if not more. Similarly, if the URL is a "common" URL that they've typed in the
past (e.g., their favorite web site; bank; email provider; their company; etc.),
the savings in resolution time can be quite significant.

## Effectiveness

### **Typical Usage**

If a user resolved a domain name to an IP address recently, their operating
system will remember (cache) the answer, and then resolution time can be as low
as 0-1ms (a thousandth of a second). If the resolution is not locally cached and
needs to "go out over the network," then a bare minimum for resolution time is
about 15 ms, assuming a nearby firewall (home router?) has a cached answer to
the question. Most common names like google.com and yahoo.com are resolved so
often that most local ISP's name resolvers can answer in closer to 80-120ms. If
the domain name in question is an uncommon name, then a query may have to go
through numerous resolvers up and down the hierarchy, and the delay can average
closer to 200-300ms. More interestingly, for any of these queries that access
the internet, dropped packets, and overworked (under provisioned) name
resolvers, regularly increases the total resolution time to between 1 and 10
seconds.

### **Network Usage**

DNS resolutions use a hierarchical system, where each level in the hierarchy has
a cache, to remember previous resolutions. As a result, extra resolutions don't
generally speaking cause end-to-end internet usage. The resolutions only go as
far as needed toward the "authoritative" resolver, stopping when they reach a
resolver that already has the resolution in a cache. In addition, DNS resolution
requests are very light weight. Each request typically involves sending a single
UDP packet that is under 100 bytes out, and getting back a response that is
around 100 bytes. This minimal impact on network usage is compensated by a
significant improvement in user experience.

### Cache Eviction

The local machine's DNS cache is pretty limited. Current estimates for the
number of resolutions remembered on a Windows XP box are in the range of 50-200
domain names. As a result, if "too many" resolutions are made, then some
"necessary" resolutions might be "evicted" from the cache to make room for the
new prefetches. Chromium tries to model the underlying cache, and guess when
there is a chance that a "soon to be needed" domain resolution has been evicted.
When Chromium decides an eviction may have taken place, it can automatically
resolve the domain name again, ensuring it is either reloaded into the cache, or
marked as "recently used" so that it won't be evicted for a "while." Cache
evictions caused by Chromium can have a negative impact on other applications
that don't (yet) use prefetching techniques to keep the underlying cache "warm."

Many large internet sites (google.com, yahoo.com, etc.) commonly mark the domain
names resolutions with an expiration time in the neighborhood of 5 minutes. They
probably set the expiration time to be short so that they can better respond to
changes in supply and demand for their services. This in turn tends cause cache
evictions. As a result, most applications tend to face the problem of cache
eviction, and already employ various methods to reduce the impact.

The "about:histograms/DNS" and "about:dns" pages contain more information about
DNS prefetch activity.
