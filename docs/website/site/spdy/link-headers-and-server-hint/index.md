---
breadcrumbs:
- - /spdy
  - SPDY
page_name: link-headers-and-server-hint
title: Server Push and Server Hints
---

In order to deliver resources to browsers more quickly, SPDY employs two
mechanisms, Server Push and Server Hint.

### Server Push

Server Push is where the server pushes a resource directly to the client without
the client asking for the resource. The server is making an assumption here that
pushing the resource is desirable. Pushing a cacheable resource can be risky, as
the browser might already have the resource and the push can be redundant.

Specifications:

*   [SPDY Specification](/spdy/spdy-protocol)

Server Hint

Server Hint is a mechanism where the server can notify the client of a resource
that will be needed before the client can discover it. The server does not send
the entire contents of the resource, but rather just the URL as an early part of
a response. The client can then validate its cache (potentially even eliminating
the need for a GET-if-modified-since), and will formally request the resource
only if needed.

Server Hint is implemented using the LINK header with HTTP, and overlaps with
existing link prefetching semantics.

Specifications:

*   [LINK rel=prefetch from
            Mozilla](https://developer.mozilla.org/en/Link_prefetching_FAQ)
*   [LINK
            rel=subresource](/spdy/link-headers-and-server-hint/link-rel-subresource)

### Comparison of Server Push and Server Hint

<table>
<tr>
<td> Pro</td>
<td> Con</td>
</tr>
<tr>
<td>Server Push</td>

*   <td> Lowest Latency Delivery Mechanism</td>
    *   <td>Saves 1 round-trip</td>

*   <td>If the client already has the resource cache, the load is
            wasteful</td>
*   <td>The Application must decide if it should push the resource, the
            protocol cannot know whether the client has a resource</td>

</tr>
<tr>
<td> Server Hint</td>

*   <td>Provides early discovery of critical resources</td>
*   <td>Can include etag or last-modified information to avoid
            over-the-network cache validation checks</td>
*   <td>Allows the client to participate in cache validation before
            sending the resource</td>

*   <td>Still requires a round-trip between the client and server to
            fetch the resource.</td>

</tr>
</table>
