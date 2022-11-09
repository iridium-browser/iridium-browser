---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: network-stack
title: Network Stack
---

**Warning:** This document is somewhat outdated. See
<https://chromium.googlesource.com/chromium/src/+/HEAD/net/docs/life-of-a-url-request.md>
for more modern information.

[TOC]

## Overview

The network stack is a mostly single-threaded cross-platform library primarily
for resource fetching. Its main interfaces are `URLRequest` and
`URLRequestContext`. `URLRequest`, as indicated by its name, represents the
request for a [URL](http://en.wikipedia.org/wiki/URL). `URLRequestContext`
contains all the associated context necessary to fulfill the URL request, such
as [cookies](http://en.wikipedia.org/wiki/HTTP_cookie), host resolver, proxy
resolver, [cache](/developers/design-documents/network-stack/http-cache), etc.
Many URLRequest objects may share the same URLRequestContext. Most `net` objects
are not threadsafe, although the disk cache can use a dedicated thread, and
several components (host resolution, certificate verification, etc.) may use
unjoined worker threads. Since it primarily runs on a single network thread, no
operation on the network thread is allowed to block. Therefore we use
non-blocking operations with asynchronous callbacks (typically
`CompletionCallback`). The network stack code also logs most operations to
`NetLog`, which allows the consumer to record said operations in memory and
render it in a user-friendly format for debugging purposes.

Chromium developers wrote the network stack in order to:

*   Allow coding to cross-platform abstractions
*   Provide greater control than would be available with higher-level
            system networking libraries (e.g. WinHTTP or WinINET)
    *   Avoid bugs that may exist in system libraries
    *   Enable greater opportunity for performance optimizations

## Code Layout

*   net/base - Grab bag of `net` utilities, such as host resolution,
            cookies, network change detection,
            [SSL](http://en.wikipedia.org/wiki/Transport_Layer_Security).
*   net/disk_cache - [Cache for web
            resources](/developers/design-documents/network-stack/disk-cache).
*   net/ftp - [FTP](http://en.wikipedia.org/wiki/File_Transfer_Protocol)
            implementation. Code is primarily based on the old HTTP
            implementation.
*   net/http -
            [HTTP](http://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol)
            implementation.
*   net/ocsp -
            [OCSP](http://en.wikipedia.org/wiki/Online_Certificate_Status_Protocol)
            implementation when not using the system libraries or if the system
            does not provide an OCSP implementation. Currently only contains an
            NSS based implementation.
*   net/proxy - Proxy ([SOCKS](http://en.wikipedia.org/wiki/SOCKS) and
            HTTP) configuration, resolution, script fetching, etc.
*   net/quic - [QUIC](/quic) implementation.
*   net/socket - Cross-platform implementations of
            [TCP](http://en.wikipedia.org/wiki/Transmission_Control_Protocol)
            sockets, "SSL sockets", and socket pools.
*   net/socket_stream - socket streams for WebSockets.
*   net/spdy - HTTP2 (and its predecessor) [SPDY](/spdy) implementation.
*   net/url_request - `URLRequest`, `URLRequestContext`, and
            `URLRequestJob` implementations.
*   net/websockets -
            [WebSockets](http://en.wikipedia.org/wiki/WebSockets)
            implementation.

## Anatomy of a Network Request (focused on HTTP)

[<img alt="image"
src="/developers/design-documents/network-stack/Chromium%20HTTP%20Network%20Request%20Diagram.svg">](/developers/design-documents/network-stack/Chromium%20HTTP%20Network%20Request%20Diagram.svg)

### URLRequest

```none
class URLRequest {
 public:
  // Construct a URLRequest for |url|, notifying events to |delegate|.
  URLRequest(const GURL& url, Delegate* delegate);
  
  // Specify the shared state
  void set_context(URLRequestContext* context);
  // Start the request. Notifications will be sent to |delegate|.
  void Start();
  // Read data from the request.
  bool Read(IOBuffer* buf, int max_bytes, int* bytes_read);
};
class URLRequest::Delegate {
 public:
  // Called after the response has started coming in or an error occurred.
  virtual void OnResponseStarted(...) = 0;
  // Called when Read() calls complete.
  virtual void OnReadCompleted(...) = 0;
};
```

When a `URLRequest` is started, the first thing it does is decide what type of
`URLRequestJob` to create. The main job type is the `URLRequestHttpJob` which is
used to fulfill http:// requests. There are a variety of other jobs, such as
`URLRequestFileJob` (file://), `URLRequestFtpJob` (ftp://), `URLRequestDataJob`
(data://), and so on. The network stack will determine the appropriate job to
fulfill the request, but it provides two ways for clients to customize the job
creation: `URLRequest::Interceptor` and `URLRequest::ProtocolFactory`. These are
fairly redundant, except that `URLRequest::Interceptor`'s interface is more
extensive. As the job progresses, it will notify the `URLRequest` which will
notify the `URLRequest::Delegate` as needed.

### URLRequestHttpJob

URLRequestHttpJob will first identify the cookies to set for the HTTP request,
which requires querying the `CookieMonster` in the request context. This can be
asynchronous since the CookieMonster may be backed by an
[sqlite](http://en.wikipedia.org/wiki/SQLite) database. After doing so, it will
ask the request context's `HttpTransactionFactory` to create a
`HttpTransaction`. Typically, the
`[HttpCache](/developers/design-documents/network-stack/http-cache)` will be
specified as the `HttpTransactionFactory`. The `HttpCache` will create a
`HttpCache::Transaction` to handle the HTTP request. The
`HttpCache::Transaction` will first check the `HttpCache` (which checks the
[disk cache](/developers/design-documents/network-stack/disk-cache)) to see if
the cache entry already exists. If so, that means that the response was already
cached, or a network transaction already exists for this cache entry, so just
read from that entry. If the cache entry does not exist, then we create it and
ask the `HttpCache`'s `HttpNetworkLayer` to create a `HttpNetworkTransaction` to
service the request. The `HttpNetworkTransaction` is given a
`HttpNetworkSession` which contains the contextual state for performing HTTP
requests. Some of this state comes from the `URLRequestContext`.

### HttpNetworkTransaction

```none
class HttpNetworkSession {
 ...
 private:
  // Shim so we can mock out ClientSockets.
  ClientSocketFactory* const socket_factory_;
  // Pointer to URLRequestContext's HostResolver.
  HostResolver* const host_resolver_;
  // Reference to URLRequestContext's ProxyService
  scoped_refptr<ProxyService> proxy_service_;
  // Contains all the socket pools.
  ClientSocketPoolManager socket_pool_manager_;
  // Contains the active SpdySessions.
  scoped_ptr<SpdySessionPool> spdy_session_pool_;
  // Handles HttpStream creation.
  HttpStreamFactory http_stream_factory_;
};
```

`HttpNetworkTransaction` asks the `HttpStreamFactory` to create a `HttpStream`.
The `HttpStreamFactory` returns a `HttpStreamRequest` that is supposed to handle
all the logic of figuring out how to establish the connection, and once the
connection is established, wraps it with a HttpStream subclass that mediates
talking directly to the network.

```none
class HttpStream {
 public:
  virtual int SendRequest(...) = 0;
  virtual int ReadResponseHeaders(...) = 0;
  virtual int ReadResponseBody(...) = 0;
  ...
};
```

Currently, there are only two main `HttpStream` subclasses: `HttpBasicStream`
and `SpdyHttpStream`, although we're planning on creating subclasses for [HTTP
pipelining](http://en.wikipedia.org/wiki/HTTP_pipelining). HttpBasicStream
assumes it is reading/writing directly to a socket. SpdyHttpStream reads and
writes to a `SpdyStream`. The network transaction will call methods on the
stream, and on completion, will invoke callbacks back to the
`HttpCache::Transaction` which will notify the `URLRequestHttpJob` and
`URLRequest` as necessary. For the HTTP pathway, the generation and parsing of
http requests and responses will be handled by the `HttpStreamParser`. For the
SPDY pathway, request and response parsing are handled by `SpdyStream` and
`SpdySession`. Based on the HTTP response, the `HttpNetworkTransaction` may need
to perform [HTTP
authentication](/developers/design-documents/http-authentication). This may
involve restarting the network transaction.

### HttpStreamFactory

`HttpStreamFactory` first does proxy resolution to determine whether or not a
proxy is needed. The endpoint is set to the URL host or the proxy server.
`HttpStreamFactory` then checks the `SpdySessionPool` to see if we have an
available `SpdySession` for this endpoint. If not, then the stream factory
requests a "socket" (TCP/proxy/SSL/etc) from the appropriate pool. If the socket
is an SSL socket, then it checks to see if
[NPN](https://tools.ietf.org/id/draft-agl-tls-nextprotoneg-01.txt) indicated a
protocol (which may be SPDY), and if so, uses the specified protocol. For SPDY,
we'll check to see if a `SpdySession` already exists and use that if so,
otherwise we'll create a new `SpdySession` from this SSL socket, and create a
`SpdyStream` from the `SpdySession`, which we wrap a `SpdyHttpStream` around.
For HTTP, we'll simply take the socket and wrap it in a `HttpBasicStream`.

#### Proxy Resolution

`HttpStreamFactory` queries the `ProxyService` to return the `ProxyInfo` for the
GURL. The proxy service first needs to check if it has an up-to-date proxy
configuration. If not, it uses the `ProxyConfigService` to query the system for
the current proxy settings. If the proxy settings are set to no proxy or a
specific proxy, then proxy resolution is simple (we return no proxy or the
specific proxy). Otherwise, we need to run a [PAC
script](http://en.wikipedia.org/wiki/Proxy_auto-config) to determine the
appropriate proxy (or lack thereof). If we don't already have the PAC script,
then the proxy settings will indicate we're supposed to use [WPAD
auto-detection](http://en.wikipedia.org/wiki/Web_Proxy_Autodiscovery_Protocol),
or a custom PAC url will be specified, and we'll fetch the PAC script with the
`ProxyScriptFetcher`. Once we have the PAC script, we'll execute it via the
`ProxyResolver`. Note that we use a shim `MultiThreadedProxyResolver` object to
dispatch the PAC script execution to threads, which run a `ProxyResolverV8`
instance. This is because PAC script execution may block on host resolution.
Therefore, in order to prevent one stalled PAC script execution from blocking
other proxy resolutions, we allow for executing multiple PAC scripts
concurrently (caveat: [V8](http://en.wikipedia.org/wiki/V8_(JavaScript_engine))
is not threadsafe, so we acquire locks for the javascript bindings, so while one
V8 instance is blocked on host resolution, it releases the lock so another V8
instance can execute the PAC script to resolve the proxy for a different URL).

#### Connection Management

After the `HttpStreamRequest` has determined the appropriate endpoint (URL
endpoint or proxy endpoint), it needs to establish a connection. It does so by
identifying the appropriate "socket" pool and requesting a socket from it. Note
that "socket" here basically means something that we can read and write to, to
send data over the network. An SSL socket is built on top of a transport
([TCP](http://en.wikipedia.org/wiki/Transmission_Control_Protocol)) socket, and
encrypts/decrypts the raw TCP data for the user. Different socket types also
handle different connection setups, for HTTP/SOCKS proxies, SSL handshakes, etc.
Socket pools are designed to be layered, so the various connection setups can be
layered on top of other sockets. `HttpStream` can be agnostic of the actual
underlying socket type, since it just needs to read and write to the socket. The
socket pools perform a variety of functions-- They implement our connections per
proxy, per host, and per process limits. Currently these are set to 32 sockets
per proxy, 6 sockets per destination host, and 256 sockets per process (not
implemented exactly correctly, but good enough). Socket pools also abstract the
socket request from the fulfillment, thereby giving us "late binding" of
sockets. A socket request can be fulfilled by a newly connected socket or an
idle socket ([reused from a previous http
transaction](http://en.wikipedia.org/wiki/HTTP_persistent_connection)).

#### Host Resolution

Note that the connection setup for transport sockets not only requires the
transport (TCP) handshake, but probably already requires host resolution.
`HostResolverImpl` uses assorted mechanisms including getaddrinfo() to perform
host resolutions, which is a blocking call, so the resolver invokes these calls
on unjoined worker threads. Typically host resolution usually involves
[DNS](http://en.wikipedia.org/wiki/Domain_Name_System) resolution, but may
involve non-DNS namespaces such as
[NetBIOS](http://en.wikipedia.org/wiki/NetBIOS)/[WINS](http://en.wikipedia.org/wiki/Windows_Internet_Name_Service).
Note that, as of time of writing, we cap the number of concurrent host
resolutions to 8, but are looking to optimize this value. `HostResolverImpl`
also contains a `HostCache` which caches up to 1000 hostnames.

#### SSL/TLS

The network stack uses [BoringSSL](https://boringssl.googlesource.com/boringssl/)
to handle the TLS connection logic. The bridge between the `StreamSocket` class
and BoringSSL can be found in `SSLClientSocketImpl`.

TODO: talk about network change notifications
