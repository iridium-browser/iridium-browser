---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: ssca
title: Mitigating Side-Channel Attacks
---

At the beginning of 2018, researchers from Google's [Project
Zero](https://googleprojectzero.blogspot.com/2014/07/announcing-project-zero.html)
[disclosed](https://googleprojectzero.blogspot.com/2018/01/reading-privileged-memory-with-side.html)
a series of new attack techniques against speculative execution optimizations
used by modern CPUs. Security researchers will continue to find new variations
of these and other side-channel attacks. Such techniques have implications for
products and services that execute third-party code, including Chrome and other
browsers with support for features like JavaScript and WebAssembly.

The Chrome Security Team has written [a document covering the variety of defense
techniques
available](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/side-channel-threat-model.md).

**Protecting users with Site Isolation**

Chrome has been working on a feature called [Site
Isolation](/Home/chromium-security/site-isolation) which provides extensive
mitigation against exploitation of these types of vulnerabilities. With Site
Isolation enabled, the amount of data exposed to side-channel attacks is reduced
as Chrome renders content for each website in a separate process. This allows
websites to be protected from each other by the security guarantees provided by
the operating system on which Chrome is running.

Site Isolation is enabled by default on Windows, Mac, Linux, and Chrome OS since
Chrome 67, and can also can be controlled [via enterprise
policies](https://support.google.com/chrome/a/answer/7581529) or [with
chrome://flags](https://support.google.com/chrome/answer/7623121). More details
can be found in our blog post [Mitigating Spectre with Site Isolation in
Chrome](https://security.googleblog.com/2018/07/mitigating-spectre-with-site-isolation.html).

Site Isolation is most effective when website developers follow modern security
best practices:

    Where possible cookies should use
    [SameSite](https://tools.ietf.org/html/draft-ietf-httpbis-rfc6265bis-02#section-5.3.7)
    and[ HTTPOnly](https://www.owasp.org/index.php/HttpOnly) attributes and
    pages should avoid reading from document.cookie.

    Make sure [MIME types are
    correct](/Home/chromium-security/site-isolation#TOC-Recommendations-for-Web-Developers)
    and specify an X-Content-Type-Options: nosniff response header for any URLs
    with user-specific or sensitive content, to take full advantage of
    [Cross-Origin Read
    Blocking](https://chromium.googlesource.com/chromium/src/+/HEAD/services/network/cross_origin_read_blocking_explainer.md)
    (CORB).

Web developers should also see the [Meltdown/Spectre
WebFundamentals](https://developers.google.com/web/updates/2018/02/meltdown-spectre)
post.

Spectre and Meltdown

The attacks known as [Spectre and Meltdown](https://meltdownattack.com/),
originally
[disclosed](https://googleprojectzero.blogspot.com/2018/01/reading-privileged-memory-with-side.html)
by Project Zero, have implications for Chrome. For information about other
Google products and services, including Chrome OS please see the [Google Online
Security
Blog](https://security.googleblog.com/2018/01/todays-cpu-vulnerability-what-you-need.html).

These attacks are mitigated by Site Isolation. Additionally, staring in Chrome
64, Chrome's JavaScript engine [V8](https://www.v8project.org/) has included
[further mitigations](https://github.com/v8/v8/wiki/Untrusted-code-mitigations)
which provide protection on platforms where Site Isolation is not enabled.

In line with
[other](https://blog.mozilla.org/security/2018/01/03/mitigations-landing-new-class-timing-attack/)
[browsers'](https://blogs.windows.com/msedgedev/2018/01/03/speculative-execution-mitigations-microsoft-edge-internet-explorer/)
response to Spectre and Meltdown, Chrome disabled[
SharedArrayBuffer](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/SharedArrayBuffer)
in Chrome 63 starting on Jan 5th 2018, and modified the behavior of other APIs
such as performance.now to help reduce the efficacy of side-channel attacks.

SharedArrayBuffer is now re-enabled in Chrome versions where Site Isolation is
on by default.

GLitch

Researchers from Vrije Universiteit Amsterdam disclosed
[details](https://www.vusec.net/wp-content/uploads/2018/05/glitch.pdf) of the
GLitch attack. Part of the attack uses high-precision GPU timers available in
WebGL to obtain information that is then used to perform a
[Rowhammer-style](https://en.wikipedia.org/wiki/Row_hammer) bit-flip attack.

Starting in Chrome 65, the
[EXT_disjoint_timer_query](https://developer.mozilla.org/en-US/docs/Web/API/EXT_disjoint_timer_query)
and EXT_disjoint_timer_query_webgl2 WebGL extensions have been disabled, and the
behaviour of clientWaitSync and other \*Sync functions has been changed to
reduce their effective precision as clocks.

Although the GLitch attack is unrelated to Spectre and Meltdown,
EXT_disjoint_timer_query and EXT_disjoint_timer_query_webgl2 could also be used
to mount Spectre and Meltdown attacks. Accordingly, they will remain disabled in
Chrome until Site Isolation is on by default, at which point they will be
re-enabled with sufficiently reduced precision to mitigate GLitch attacks.

Also see [more
details](http://www.chromium.org/chromium-os/glitch-vulnerability-status) about
GLitch and Chrome OS.
