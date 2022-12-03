---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
page_name: socks-proxy
title: Configuring a SOCKS proxy server in Chrome
---

To configure chrome to proxy traffic through the SOCKS v5 proxy server
***myproxy:8080***, launch chrome with these two command-line flags:

--proxy-server="socks5://***myproxy:8080***"

--host-resolver-rules="MAP \* ~NOTFOUND , EXCLUDE ***myproxy***"

Explanation

The --proxy-server="socks5://myproxy:8080" flag tells Chrome to send all http://
and https:// URL requests through the SOCKS proxy server "myproxy:8080", using
version 5 of the SOCKS protocol. The hostname for these URLs will be resolved by
the *proxy server*, and not locally by Chrome.

*   NOTE: proxying of ftp:// URLs through a SOCKS proxy is not yet
            implemented.

The --proxy-server flag applies to URL loads only. There are other components of
Chrome which may issue DNS resolves *directly* and hence bypass this proxy
server. The most notable such component is the "DNS prefetcher".Hence if DNS
prefetching is not disabled in Chrome then you will still see local DNS requests
being issued by Chrome despite having specified a SOCKS v5 proxy server.

Disabling DNS prefetching would solve this problem, however it is a fragile
solution since once needs to be aware of all the areas in Chrome which issue raw
DNS requests. To address this, the next flag, --host-resolver-rules="MAP \*
~NOTFOUND , EXCLUDE myproxy", is a catch-all to prevent Chrome from sending any
DNS requests over the network. It says that all DNS resolves are to be simply
mapped to the (invalid) address 0.0.0.0. The "EXCLUDE" clause make an exception
for "myproxy", because otherwise Chrome would be unable to resolve the address
of the SOCKS proxy server itself, and all requests would necessarily fail with
PROXY_CONNECTION_FAILED.

Debugging

There are a lot of intricacies to configuring proxy settings as you intend:

*   Different profiles can use different proxy settings
*   Extensions can modify the proxy settings
*   If using the system setting, other applications can change them, and
            there can be per-connection settings.
*   The proxy settings might include fallbacks to other proxies, or
            direct connections
*   Plugins (for instance Flash and Java applets) can bypass the Chrome
            proxy settings alltogether
*   Other third-party components in Chrome might issue DNS resolves
            directly, or bypass Chrome's proxy settings.

The first thing to check when debugging is look at the Proxy tab on
about:net-internals, and verify what the effective proxy settings are:

chrome://net-internals/#proxy

Next, take a look at the DNS tab of about:net-internals to make sure Chrome
isn't issuing local DNS resolves:

chrome://net-internals/#dns

Next, to trace the proxy logic for individual requests in Chrome take a look at
the Events tab of about:net-internals:

chrome://net-internals/#events
