---
breadcrumbs: []
page_name: spdy
title: SPDY
---

**SPDY is deprecated. This page is out of date.**

SPDY was an experimental protocol for the web with the primary goal to reduce
latency of web pages. Its successor is
[HTTP/2](http://httpwg.org/specs/rfc7540.html).

*   Documentation
    *   [SPDY: An experimental protocol for a faster
                web](/spdy/spdy-whitepaper)
    *   [SPDY protocol specification](/spdy/spdy-protocol)
        *   [Server Push and Server
                    Hint](/spdy/link-headers-and-server-hint)
    *   Research
        *   [An Argument For Changing TCP Slow
                    Start](/spdy/An_Argument_For_Changing_TCP_Slow_Start.pdf) -
                    Mike Belshe 01/11/10
        *   [More Bandwidth Doesn't Matter
                    (much)](http://docs.google.com/a/chromium.org/viewer?a=v&pid=sites&srcid=Y2hyb21pdW0ub3JnfGRldnxneDoxMzcyOWI1N2I4YzI3NzE2)
                    - Mike Belshe 04/08/10
    *   Standards Work
        *   [SSL Next-Protocol-Negotiation Extension
                    ](http://tools.ietf.org/html/draft-agl-tls-nextprotoneg-00.html)-
                    Adam Langley 01/20/10
        *   [TLS False
                    Start](https://tools.ietf.org/html/draft-bmoeller-tls-falsestart-00)
                    - Langley, Modadugu, Mueller 06/2010
        *   [TLS Snap
                    Start](http://tools.ietf.org/html/draft-agl-tls-snapstart-00)
                    - Adam Langley 06/18/2010
    *   [SPDY Server and Proxy
                Authentication](/spdy/spdy-authentication)
*   Code
    *   Chromium client implementation:
                <https://cs.chromium.org/chromium/src/net/spdy/>
*   Discuss
    *   <http://groups.google.com/group/spdy-dev>
*   External work
    *   Servers
        *   Jetty Web Server:
                    <http://wiki.eclipse.org/Jetty/Feature/SPDY>
        *   Apache module for SPDY: <http://code.google.com/p/mod-spdy/>
        *   Python implementation of a SPDY server:
                    <http://github.com/mnot/nbhttp/tree/spdy>
        *   Ruby SPDY: <https://github.com/igrigorik/spdy>
        *   node.js SPDY: <https://github.com/indutny/node-spdy>
    *   Libraries
        *   Netty SPDY (Java library):
                    <http://netty.io/blog/2012/02/04/>
        *   Ruby wrapper around Chromium SPDY framer:
                    <https://github.com/romanbsd/spdy>
        *   Go SPDY: <http://godoc.org/code.google.com/p/go.net/spdy>
        *   Erlang SPDY: <https://github.com/RJ/erlang-spdy>
        *   Java SPDY:
                    <http://svn.apache.org/repos/asf/tomcat/trunk/modules/tomcat-lite>
        *   C SPDY (libspdy): <http://libspdy.org/index.html> (to be
                    used by libcurl:
                    <http://daniel.haxx.se/blog/2011/10/18/libspdy/>)
        *   C SPDY (spindly): <https://github.com/bagder/spindly>
        *   C SPDY (spdylay): <https://github.com/tatsuhiro-t/spdylay>
        *   iPhone SPDY: <https://github.com/sorced-jim/SPDY-for-iPhone>
    *   Browsers
        *   Google Chrome
        *   Mozilla Firefox
*   Tools
    *   [Summary of SPDY tools](/spdy/spdy-tools-and-debugging)
    *   [Chrome page benchmarking
                tool](/developers/design-documents/extensions/how-the-extension-system-works/chrome-benchmarking-extension)
