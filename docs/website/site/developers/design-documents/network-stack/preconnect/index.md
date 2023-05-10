---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
page_name: preconnect
title: Preconnect
---

[TOC]

## **Overview**

The networking stack implements the blink interface for the [W3C Resource Hints
preconnect specification](http://www.w3.org/TR/resource-hints/#preconnect) to
allow site developers to request connections to servers in anticipation of
future requests. The predominant use cases are:

1.  sub-resources for the current page that the preload scanner is not
            capable of identifying. This can include resources discovered by
            styles being applied (background images, fonts) as well as resources
            initiated by scripts (analytics beacons, ads domains, etc).
2.  Top-level domains used in future navigations. For example, clinking
            on a link to an url shortener or click tracker that then redirects
            to the actual page. If the page knows the resulting final
            destination it can start initiating the connection in parallel with
            the navigation to the redirector.

[Issue 450682](https://code.google.com/p/chromium/issues/detail?id=450682)
tracks the implementation as well as issues that came up during implementation
and experimentation.

## Implementation Notes

### Connection Pools (not all connections are the same)

Chrome maintains separate connection pools for the various different protocols
(http, https, ftp, etc). Additionally, separate connection pools are managed for
connections where cookies can be transmitted and connections where they can not
in order to prevent tracking of users when a request is determined to be private
(when blocking cookies to third-party domains for example). The determination of
which connection pool a request will use is made at request time and depends on
the context in which it is being made. Top-level navigations go in the
cookie-allowed pool while sub-resource requests get assigned a pool based on the
domain of the request, the domain of the page and the user's privacy settings.

Connections can not move from one pool to another so the determination of which
pool a connection will be assigned to needs to be made at the time when the
connection is being established. In the case of TLS connections, the cost of not
using a preconnected connection can be quite high, particularly on mobile
devices with limited resources.

The initial implementation for preconnect did not take private mode into account
and all connections were pre-connected in the cookies-allowed connection pool
leading to
[situations](https://code.google.com/p/chromium/issues/detail?id=468005) where
the preconnected connection was not leveraged for the future request.

### Preconnect Connection Pool Selection Heuristic (proposed)

The two different use cases for preconnect require different treatment for
determining which connection pool a connection should belong to. In the case of
a sub-resource request the document URL should be considered and the relevant
privacy settings should be evaluated. In the case of the top-level navigation
the document URL should NOT be considered and the connection. There are no hints
that the site owner can provide to indicate what kind of request will be needed
so it is up to the browser to guess correctly.

For preconnect requests that are initiated while the document is being loaded
(before the onload event while the HTML is being parsed and scripts are being
executed), Chrome will assume that preconnect requests are going to be for
sub-resources that are going to be requested in the context of the current
document and the connection pool will be selected by taking the owning
document's URL into account.

For preconnect requests that are initiated after the document has finished
loading, Chrome will assume that preconnect requests will be used for future
page navigations and the connections will not take the current document's URL
into account when selecting a connection pool (which effectively means the
connection will be established in the allows-cookies pool).

This does leave a few possible cases for mis-guessing but should handle the vast
majority of cases. Specifically:

1.  Top-level navigations that are preconnected before the page has
            finished loading may end up in the wrong connection pool as the
            document's URL will be considered.
2.  Sub-resource requests that are preconnected after the page has
            finished loading may end up in the wrong pool if the request
            requires a private connection.
