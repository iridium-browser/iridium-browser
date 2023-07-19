---
breadcrumbs:
- - /spdy
  - SPDY
page_name: spdy-tools-and-debugging
title: SPDY Tools and Debugging
---

With a new protocol, tools and debugging aids are always in short supply. If you
have time to help build additional tools, please do so and feel free to post
them here!

Debugging tools:

*   Chrome
    *   about:net-internals
        This URL is a magic URL in Chrome which provides data about the chrome
        network stack. It has been fully updated to enumerate SPDY frames and
        status.
        *   Events Tab: This tab shows network events. Look for
                    SPDY_SESSSION events and you can see every SPDY frame sent
                    or received by Chrome.
        *   SPDY Tab (Chrome 9+): This tab shows the current state of
                    existing, active SPDY sessions
    *   command line flags
        Chrome has lots of command line flags to assist with altering SPDY
        behavior:
        *   --use-spdy=ssl
            Forces Chrome to always use SPDY, even if not negotiated over SSL
        *   --use-spdy=no-ssl
            Forces Chrome to always use SPDY, but without SSL
        *   --host-resolver-rules="MAP \* myspdyserver:8000"
            If you've got a server running SPDY, and you want content to be
            fetched from it, even if it doesn't match the URL you are requesting
            (e.g. you've cached a site and want to play it back to the browser
            without hitting the real-world site), this rule with alter host
            resolution for a set of URLs to be your simulated server.
    *   Chrome Inspector
        *   The existing chrome tools for testing HTTP also work with
                    SPDY.
*   Speed Tracer
    Speed Tracer is an extension for profiling browser internal events, network
    events and CPU usage.
    <https://chrome.google.com/extensions/detail/ognampngfcbddbfemdapefohjiobgbdl>
*   Chromium page benchmarker
    The page benchmarker can test HTTP and SPDY urls through chrome.
    <https://www.chromium.org/chrome-benchmarking-extension>
*   Flip-in-mem-edsm-server
    The flip server is a fully implemented SPDY server or SPDY-to-HTTP gateway.
    Full source is available in the chromium tree at net/tools/flip_server. You
    can read more about it here:
    <http://www.chromium.org/spdy/running_flipinmemserver>
*   Wireshark SPDY extension
    The wireshark extension needs some love. You'll find it checked into the
    chromium source tree under net/tools.
