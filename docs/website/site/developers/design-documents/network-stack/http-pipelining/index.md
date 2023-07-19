---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
page_name: http-pipelining
title: HTTP Pipelining
---

## Objective

Speed up Chrome's network stack by enabling HTTP Pipelining. Pipelining issues
multiple requests over a single connection without waiting for a response.

## Risks

*   Broken servers. Servers may ignore pipelined requests or corrupt the
            responses.
*   Broken proxies. May cause the same problems. Some users are behind
            "transparent proxies," where the requests are proxied even though
            the user has not explicitly specified a proxy in their system
            configuration.
*   Front of queue blocking. The first request in a pipeline may block
            other requests in the pipeline. The net result of pipelining may be
            slower page loads.

## Mitigation

Response headers must have the following properties:

*   HTTP/1.1
*   Determinable content length, either through explicit Content-Length
            or chunked encoding
*   A keep-alive connection (implicit with HTTP/1.1)
*   No authentication

Pipelining does not begin until these criteria have been met for an origin (host
and port pair). If at any point one of these fail, the origin is black-listed in
the client. If an origin has successfully pipelined before, it is remembered and
pipelining begins immediately on next use.

## Status

The option to enable pipelining has been removed from Chrome, as there are known
crashing bugs and known front-of-queue blocking issues. There are also a large
number of servers and middleboxes that behave badly and inconsistently when
pipelining is enabled. Until these are resolved, it's recommended nobody uses
pipelining. Doing so currently requires a custom build of Chromium.
