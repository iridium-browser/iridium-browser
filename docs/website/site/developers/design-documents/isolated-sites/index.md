---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: isolated-sites
title: Isolated Sites
---

### NOTE: This project is no longer being pursued, and <http://crbug.com/69335> has been marked WontFix. We are investigating other ways to get similar protections once the [Site Isolation](/developers/design-documents/site-isolation) effort is deployed.

### Motivation

For browsing high value web sites (e.g., your bank account), Isolated Sites help
provide stronger protection against cross-origin attacks, such as reflected XSS
and CSRF. See this paper for more details: [App Isolation: Get the Security of
Multiple Browsers with Just
One](http://www.charlesreis.com/research/publications/ccs-2011.pdf).

### How it works

A user designates a set of URLs to be be an "isolated site." This can be done
either via an extension API or via chrome://settings/content (still to be
flushed out).

When you perform a top-level navigation to an isolated site, it will behave as
if you are visiting it from a separate browser. Any state that would be used for
browsing (cookie store, cache, localstorage, etc.) will not be shared with your
normal browsing session. This includes any subresource loads, subframes, or
javascript XHR calls. A dedicated renderer processes are used as well, so that
any in-memory state is also isolated from your normal browsing session. This
helps shield the isolated app from most known cross-origin attacks, from
reflected XSS to full WebKit exploits.

As an example, suppose you have designated http://isolatedsite.com to be
isolated and that http://isolatedsite.com serves the following HTML:

&lt;html&gt;&lt;body&gt;&lt;iframe
src="http://thirdparty.com"&gt;&lt;/iframe&gt;&lt;/body&gt;&lt;/head&gt;

During your browsing session, you directly visit http://thirdparty.com in a
different tab and authenticate. Now Chrome has cookies for thirdparty.com in
your general browsing session. However, within the isolated site, the
http://thirdparty.com iframe on http://isolatedsite.com does not have access to
those cookies, so you do not appear logged in. The iframe is loaded in the
context of the isolated site, so it behaves as if it is on a different browser
than your first tab. Conversely, any cookies set in the iframe inside the
isolated site will remain attached only to the isolated site and not affect the
state of the rest of the browser.

### Requirements

The browser process must create a partitioned view of the data based on the site
that is requesting the data. This includes modifying at least the following
objects to support partitioning the data:

*   Cookies and Cache (via URLRequestContext)
*   HTML5 Storage (via DomStorage)
*   IndexDB (via IndexDBContext)
*   Filesystem access (via FileSystemContext)
*   AppCache access (via AppCacheService)
*   QuotaManager access (via QuotaManager)
*   DatabaseTracker access (via DataBaseTracker)

For now, we are aiming to partition this data on a per-process basis. This may
require Chrome to create a larger number of processes (rather than sharing
processes when a limit is reached), but it is simpler than modifying WebKit to
handle multiple storage contexts within a single renderer process.

### Current State

Tracking bug here: <http://crbug.com/69335> (marked WontFix)

Much of the state isolation infrastructure needed for this effort now exists in
the StoragePartition class, which is used by Chrome's Platform Apps. Chrome no
longer supports navigating from one StoragePartition to another, though, which
would be necessary for this Isolated Sites proposal.
