---
breadcrumbs:
- - /spdy
  - SPDY
page_name: spdy-authentication
title: SPDY Server and Proxy Authentication
---

Support for Server and Proxy Authentication in SPDY is very similar to Server
and Proxy Authentication in HTTP.

## Server Authentication

When a client sends a request to an origin server that requires authentication,
the server can reply with a `401 Unauthorized`" response, and a
`WWW-Authenticate` header that defines the authentication scheme to be used. The
client then retries the request with an `Authorization` header appropriate to
the specified authentication scheme.

There are four options for sever authentication, Basic, Digest, NTLM and
Negotiate (SPNEGO). The first two options were defined in [RFC
2617](http://www.ietf.org/rfc/rfc2617.txt), and are stateless. The second two
options were developed by Microsoft and specified in [RFC
4559](http://www.ietf.org/rfc/rfc4559.txt), and are stateful; otherwise known as
multi-round authentication, or connection authentication.

### Stateless Authentication

Both Basic and Digest Server Authentication are stateless. These schemes define
ways for the browser to encode a username and password in the HTTP request which
the server can verify. In the case of Basic Auth, the client simply base64
encodes the password. In the case of Digest, the client hashes the username,
password, and a variety of other attributes together. Once a server requests
Basic or Digest Authentication, the client is able to generate a correct
`Authorization` header for any subsequent request to the same server.

Basic and Digest authentication is handled in SPDY in much the same way as with
HTTP. Here is a sample SPDY session using Basic authentication:

```none
 Client                        Server
 ------                        ------
 *** Client makes initial, unauthenticated request to the server.
   |------------------------------>|
   | 1) SYN_STREAM                 |
   |    stream_id = 1              |
   |    url = http://example.com/a |
   |                               |
   |<------------------------------|
   | 2) SYN_REPLY                  |
   |    stream_id = 1              |
   |    status = 401               |
   |    www-authenticate = Basic   |
   |                               |
 *** Client retries request with authorization header.
   |------------------------------>|
   | 3) SYN_STREAM                 |
   |    stream_id = 3              |
   |    url = http://example.com/a |
   |    authorization = Basic XXX  |
   |<------------------------------|
   | 4) SYN_REPLY                  |
   |    stream_id = 3              |
   |    status = 200               |
   |                               |
 *** Client request next resource with authorization header.
   |------------------------------>|
   | 5) SYN_STREAM                 |
   |    stream_id = 5              |
   |    url = http://example.com/b |
   |    authorization = Basic XXX  |
   |<------------------------------|
   | 4) SYN_REPLY                  |
   |    stream_id = 5              |
   |    status = 200               |
   |                               |
```

### Stateful Authentication

### The NTLM and Negotiate Authentication scheme are more complicated. Instead of enabling the browser to generate an `Authorization` header which can be used for subsequent requests to the server, these schemes attempt to authenticate the connection. After the authentication is complete, the server marks the connection as authorized, and the client sends subsequent requests without any `Authorization` header.

Multi-round authentication is handled in SPDY in much the same way as with HTTP.
A new stream is created for each round in the process, corresponding to a
request/response pair in the HTTP process.

```none
 Client                        Server
 ------                        ------
 *** Client makes initial, unauthenticated request to the server.
   |---------------------------------->|
   | 1) SYN_STREAM                     |
   |    stream_id = 1                  |
   |    url = http://example.com/a     |
   |                                   |
   |<----------------------------------|
   | 2) SYN_REPLY                      |
   |    stream_id = 1                  |
   |    status = 401                   |
   |    www-authenticate = Negotiate   |
   |                                   |
 *** Client retries request with authorization header.
   |---------------------------------->|
   | 3) SYN_STREAM                     |
   |    stream_id = 3                  |
   |    url = http://example.com/a     |
   |    authorization = Negotiate AA   |
   |<----------------------------------|
   | 4) SYN_REPLY                      |
   |    stream_id = 3                  |
   |    status = 401                   |
   |    www-authenticate = Negotiate BB|
   |                                   |
 *** Client retries request with authorization header.
   |---------------------------------->|
   | 3) SYN_STREAM                     |
   |    stream_id = 3                  |
   |    url = http://example.com/a     |
   |    authorization = Negotiate CC   |
   |<----------------------------------|
   | 4) SYN_REPLY                      |
   |    stream_id = 3                  |
   |    status = 200                   |
   |    www-authenticate = Negotiate DD|
   |                                   |
 *** Client request next resource without authorization header.
   |---------------------------------->|
   | 5) SYN_STREAM                     |
   |    stream_id = 5                  |
   |    url = http://example.com/b     |
   |                                   |
   |<----------------------------------|
   | 4) SYN_REPLY                      |
   |    stream_id = 5                  |
   |    status = 200                   |
   |                                   |
```

However, one difference in how stateful authentication is handled between HTTP
and SPDY results from the multiplexed nature of SPDY sessions. With a single
SPDY session, a client can sent multiple requests in parallel before receiving
the first response. This complicates the stateful authentication schemes which
are attempting to perform a single authentication of the **connection**. If the
server receives two requests (streams 1 and 3) on an unauthenticated session, it
will reply 401 Unauthorized to each stream. However, it is not entirely clear
how the authentication should continue.

One option would be for the client to complete the authentication on the first
unauthorized stream and then assume that the connection will then be
authenticated and continue issuing requests without authorization headers.
However, it is possible for different resources on the same server to require
different authentication. These resources would need to specify different
"realms" when requesting authentication. When the client receives a 401, it
would need to be aware of any outstanding authentication transaction for that
same realm, and wait for it to complete.

An alternative option would be for the client to complete the authentication for
both unauthorized streams. However, as currently specified, there is no obvious
way to distinguish the 2nd attempt to request the first resource from the 2nd
attempt to request the second resource. It is possible the the URL in the
headers might be identical. It would seem to require that the client pass a
`X-Associated-Stream, or X-Authorization-Context` header in each subsequent
authentication request so the server is able to correctly understand the context
of the request.

## Proxy Authentication

When a client sends a request to a proxy server that requires authentication,
the same basic protocol is used. Instead of a 401 Unauthorized response, the
proxy can reply with a `407 Proxy Authentication Required` response, and a
`Proxy-Authenticate` header instead of an Authenticate. The client then retries
the request with a `Proxy-Authorization` header in place of an Authorization
header.
