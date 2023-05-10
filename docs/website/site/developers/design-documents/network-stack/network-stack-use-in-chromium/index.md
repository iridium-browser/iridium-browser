---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
page_name: network-stack-use-in-chromium
title: Network Stack Use in Chromium
---

[TOC]

## Overview

Chromium is the primary consumer of the Chromium network stack. While there are
other consumers, the main one is the Chromium browser process. For the most
part, the network stack runs in the browser process's IO thread, and interfaces
with the Chromium network stack via `ChromeURLRequestContext` and
`URLRequest`/`URLFetcher`.

### ChromeURLRequestContext

The Chromium browser process uses several instances of
`ChromeURLRequestContext`, a subclass of
`[URLRequestContext](/developers/design-documents/network-stack#TOC-Overview)`.
For a given profile, there are a number of `ChromeURLRequestContext` instances:

*   The "main" `ChromeURLRequestContext`. The majority of `URLRequest`s
            are associated with this `ChromeURLRequestContext` instance.
*   The "media" `ChromeURLRequestContext`. This one uses a different
            `[HttpCache](/developers/design-documents/network-stack/disk-cache)`
            instance that is optimized for large media requests (video). It
            shares many objects, such as the `HostResolver`,
            `[CookieMonster](/developers/design-documents/network-stack/cookiemonster)`,
            etc. with the main `ChromeURLRequestContext`.
*   The "extension" `ChromeURLRequestContext`. It shares many objects
            with the main `ChromeURLRequestContext`. This instance helps service
            chrome-extension:// requests.

There are also some other `ChromeURLRequestContext`s, such as sync's
`HttpBridge::RequestContext`, the `ConnectionTester`'s
`ChromeURLRequestContext`, etc. These are not tied to the profile, although with
a multiple profile Chromium, sync's `HttpBridge::RequestContext` likely would
have to be tied to a profile.

Note that the "off the record" (Incognito) profile uses a special OTR
`ChromeURLRequestContext`, in addition to the media and extension contexts, that
shares some objects, although notably not the `HttpCache` or `CookieMonster` etc
that are important to be separated for an off the record profile. The
`HostResolver` is shared.

Many of the objects in `ChromeURLRequestContext` use `scoped_refptr`s to share
ownership. This causes a lot of problems with object destruction ordering and
reference cycles. Many of these members are being moved towards being owned
externally, by the `IOThread`, although that choice may have to change to
`ProfileImpl` or something to support multiple profiles in Chromium.

Note that currently ChromeURLRequestContext contains a lot of extra members that
don't exist in `URLRequestContext`. These are being moved out, because they have
nothing to do with `URLRequestContext` and were only placed in
`ChromeURLRequestContext` because it was a convenient per-profile object, and
originally there weren't many ChromeURLRequestContext objects.

### ChromeURLRequestContextGetter:

`ChromeURLRequestContext` is constructed and destroyed on the same thread.
However, sometimes other threads need to hold references to them, even before
they get constructed. `ChromeURLRequestContextGetter` is a handle for
`ChromeURLRequestContext` that will lazily construct it on the first attempt to
use it. Accesses to the contained `ChromeURLRequestContext` are only allowed on
the IO thread.

#### URLRequest usages

*   `ResourceDispatcherHost` creates `URLRequests` when the renderer or
            plugin processes issue resource requests. It retains pointers to all
            the `ChromeURLRequestContextGetters`.
*   `URLFetcher` is another frontend for `URLRequest`. It allows any
            thread with access to the appropriate
            `ChromeURLRequestContextGetter` to proxy requests to the IO thread.
            It provides a simplified delegate interface that returns the full
            response body as a single string. There are many users of
            `URLFetcher` in the Chromium browser process, including the omnibox,
            extensions, sync (which uses its own `ChromeURLRequestContext`),
            etc. Usually these objects use the "main"
            ChromeURLRequestContextGetter object.

#### NetworkChangeNotifier

*   IntranetRedirectDetector - Lets us probe the network to see if ISPs
            are trying to redirect requests to non-existent hosts.
*   GoogleURLTracker - Lets us probe Google to see the appropriate
            domain for the search engine.

#### Network predictor

It uses `URLRequest::Interceptor` to hook into all `URLRequests` to watch them
come and go, so it can analyze the referers to learn the subdomains used in
subsequent requests. It uses this knowledge to predict network requests. For
likely requests, it keeps around the pointer to the "main"
`ChromeURLRequestContext`'s `HostResolver`, so it can perform DNS prefetching.
For requests that are almost guaranteed to happen, it goes even further and uses
the "main" `ChromeURLRequestContext`'s `HttpStreamFactory` to preconnect a
socket.
