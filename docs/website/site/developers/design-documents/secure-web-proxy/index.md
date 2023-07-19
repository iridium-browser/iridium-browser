---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: secure-web-proxy
title: Secure Web Proxy
---

## Introduction

A secure web proxy is a web proxy that the browser communicates with via SSL, as
opposed to clear text. In insecure public networks, such as airports or cafes,
browsing over HTTP may leave the user vulnerable to cookie stealing, session
hijacking or worse. A secure web proxy can add a significant layer of defense in
these cases.

## **Using a Secure Web Proxy with Chrome**

To make use of a secure web proxy, Chrome needs to be configured to use a [proxy
auto-config](http://en.wikipedia.org/wiki/Proxy_auto-config) file which specify
the `HTTPS` proxy type. For example:

```none
 function FindProxyForURL(url, host) { return "HTTPS secure-proxy.example.com:443"; }
```

This pac file can be specified by starting Chrome with the `--proxy-pac-url=...`
command line argument, or through the settings dialog. Please be aware that
other browser do not support the `HTTPS` proxy type in a .pac file, so modifying
the system-wide proxy configuration to use such a .pac file might be
inadvisable.

Alternatively, a secure web proxy can be specified by using the
`--proxy-server=https://<proxy>:<port>` command line argument. For example:

```none
 chrome --proxy-server=https://secure-proxy.example.com:443
```

Since the communication between Chrome and the proxy uses SSL, next protocol
negotiation will be used. If the servers supports HTTP/2, then the proxy will
act as an HTTP/2 Proxy.

## **Running a Secure Web Proxy**

**While all the details of running a secure web proxy are out of scope for this
document, here are two suggestions. If you are already running a web proxy, you
use [stunnel](http://www.stunnel.org/) to convert it into a secure web proxy.
For example:**

```none
stunnel -f -d 443 -r localhost:8080 -p cert.pem 
```

**This would cause stunnel to listen for SSL connections on port 443 and send
any HTTP requests to the web proxy running on port 8080.**

**Alternatively, the popular proxy program Squid appears to offer support for
running as a secure web proxy via the [https_port
directive](http://www.squid-cache.org/Doc/config/https_port/).**

## **Debugging Certificate Errors**

Debugging certificate errors for a secure web proxy [may be
difficult](https://bugs.chromium.org/p/chromium/issues/detail?id=1130233)
because the certificate information is not readily visible. Certificate
information is captured in NetLogs (capture with chrome://net-export, view with
<https://netlog-viewer.appspot.com/>). Alternatively, without the proxy
configured, navigate directly to the proxy's endpoint (e.g.
<https://123.123.123.123:1234/>) in a new tab and you'll get the typical
certificate error experience shown for server certificate errors. After that
page works without error, reconfigure the secure web proxy.
