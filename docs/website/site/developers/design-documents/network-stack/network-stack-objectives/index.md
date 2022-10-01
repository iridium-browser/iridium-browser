---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
page_name: network-stack-objectives
title: Network Stack Objectives
---

## Q2 2014 Objectives

### Blink

*   Make WebSocket scalable
    *   Switch WebSocket to new stack in Chromium
    *   Ensure that WS/HTTP2 mapping work with HTTP2 spec
    *   Revive upgrade success rate experiment
    *   Make permessage-compress spec ready for IESG review
*   Extend XMLHttpRequest for streaming use cases
    *   Streams API standardization
*   Optimize networking APIs
*   Promises Blink bindings

## Q4 2011 Objectives

#### Performance

*   SPDY
*   mobile tuning
*   DNS resolver
*   HTTP pipelining prototype

#### **SSL**

*   captive portals support
*   origin-bound certificates
*   DOMCrypt API

#### **Developer productivity**

*   analysis view of net-internals logs
*   API cleanup

## Q2 2011 Objectives

#### Improve test coverage

*   Add tests of SSL client authentication (wtc)
*   Set up automated test environment for HTTP Negotatie and NTLM
            authentication (asanka, cbentzel)
*   Add drag-n-drop, fine-grained cancels tests to Downloads
            (rdsmith,ahendrickson,asanka)

#### Fix bugs and clean up / refactor code

*   Clean up network stack API, threading model, etc. (willchan, wtc)
*   Use base, net, and crypto as DLLs on Windows (rvargas)
*   Refactor Socket classes to support server, UDP, and other transport
            sockets (mbelshe, willchan)
*   Finish Downloads System major refactors (dataflow, file
            determination, state granularity) (ahendrickson, rdsmith)
*   Fix Download incorrect name problems -- see http://crbug.com/78200
            (asanka)
*   Fix Downloads error detection and cache interface (ahendrickson)
*   Substantially reduce downloads crashers. Tentative Goal: halve
            "crashes touching downloads directory / total downloads initiated"
            metric (rdsmith, others)

#### Improve network performance / features

*   SPDY (willchan)
*   NSS certificate verification and revocation checking (wtc)
*   SSL client authentication to destination server through HTTPS proxy
            (mattm, wtc)
*   WPAD over DHCP (joi)
*   Roll out Anti-DDoS functionality (joi)
*   \[Stretch\] Add Download resumption after error (ahendrickson)

**Documentation**

*   Write design document for HTTP authentication (cbentzel)

## Q1 2011 Objectives

Improve test coverage

*   Set up test environment for HTTP Negotiate and NTLM authentication
            (asanka, cbentzel, wtc) - 0.1 Have a manual test environment.
            Started work on automated test environment at the very end of the
            quarter
*   Write new tests, enable and deflake existing ones for the download
            subsystem (rdsmith, ahendrickson) -- 0.8 Existing tests deflaked
            (major accomplishment), some new tests but not many.
*   Add tests of SSL client authentication (wtc) -- 0.0 Did not work on
            it.

Fix bugs and clean up / refactor code

*   Fix download subsystem bugs - crashes, corruption, etc. (rdsmith,
            ahendrickson) -- 0.6 Fixed several bugs, but didn't get anywhere
            near as far with this as intended.
*   Clean up download subsystem code (rdsmith, ahendrickson) -- 0.7
            Control flow much cleaner, main path deraced. Two important
            refactors not done last quarter (dataflow, file determination); will
            be highpri this quarter.
*   Refactor safebrowsing code (lzheng)
*   Fix HTTP authentication bugs - background tabs, authentication
            freezes/crashes, Negotiate authentication failures on Unix. (asanka,
            cbentzel) - 0.7 Addressed a lot of key remaining issues, such as
            background tab.
*   Clean up network stack API - URLRequestContext, etc. (willchan)
*   Use base as a DLL, a prerequisite for using net as a DLL (rvargas) -
            0.7 working on getting projects to compile cleanly

Improve network performance / features

*   TLS enhancements - OCSP stapling in NSS and integration with Windows
            CryptoAPI, Snap Start (wtc, agl, rsleevi) -- 0.7 OCSP stapling
            turned on for Linux and Windows, but not Mac OS X. Finished
            implementation of Snap Start.
*   Add extension API for HTTP authentication prompt (stretch) (asanka,
            cbentzel) - 0.0 did not start
*   Make SPDY faster (mbelshe, willchan)
*   Relax single-writer, multi-reader locking of the http cache,
            allowing readers to start reading the parts of a resource that the
            writer has written (rvargas, gavinp) - 0.0, No progress.
*   Add server hint & prefetching support - Link: header and link
            rel=prefetch. (gavinp) - 0.5, link rel=prefetch is supported, link
            header is not.
*   Release binary exploration protection for safebrowsing (lzheng)
*   Continue disk cache performance and reliability experiments
            (rvargas) - 0.8, One is done, the other one is blocked on
            infrastructure.
*   Implement offline (network disconnected) detection for Mac and Linux
            (eroman)

## Q4 2010 Objectives

**Improve test coverage**

*   Implement <http://code.google.com/p/web-page-replay/> to provide
            more complete network stack coverage and catch performance
            regressions (tonyg,mbelshe) -- 0.5 lots of good progress; up and
            running, not yet done!
*   [Improve tests for HTTP
            authentication](http://www.chromium.org/developers/design-documents/http-authentication).
            (cbentzel, wtc) - 0.2 Added unit tests and manual system-level
            tests, but still need automated system level tests.
*   [Add tests for SSL client
            authentication](http://www.chromium.org/developers/design-documents/ssl-client-authentication).
            (wtc) -- 0.2. (by rsleevi) Implemented a better way to trust a test
            root CA that doesn't require changing the system certificate store.
            Regenerated test certificates to have long validity periods.

## Fix bugs and clean up / refactor code

*   Fix bugs (everyone)
*   Improve network diagnostics (about:net-internals) to help fix bugs
            (mmenke, eroman)
*   Clean up / support previously neglected code (Downloads (rdsmith:
            0.6), SafeBrowsing(lzheng: 0.6), HTTP Auth, etc) (rdsmith, lzheng,
            ahendrickson, cbentzel)
*   Clean up valgrind reported issues in network tests (everyone) --
            0.3. Fixed some, but still have plenty more to fix.
*   Better modularize the network stack (willchan,eroman) -- 0.2. Lots
            of discussion, not many changes happened yet. A little work towards
            new URLRequestContexts

## Improve network performance / features

*   Continue running cache experiments (request throttling, performance,
            reliability) (rvargas) -- 0.9 Constant monitoring of the experiments
            and changes made as appriopriate.
*   Relax SWMR locking of the http cache (rvargas, gavinp) -- 0.5 Work
            is under way, but nothing checked in yet.
*   Continue supporting SPDY development (mbelshe, etc) -- 0.6 SPDY up
            and running on all google.com. External partners starting to
            experiment.
*   TLS latency enhancements (False Start, Snap Start, etc) (agl, wtc)
            -- 0.6. Added a certificate verification result cache. False Start
            is enabled in M8, thanks to agl's hard work. OCSP stapling works on
            Linux.
*   Better support prefetching mechanisms (Link: and X-Purpose headers,
            link rel=prefetch, resource prediction, preconnection) (gavinp, jar)
*   Continue work towards HTTP pipelining (vandebo) -- 0.0. No progress.
*   Finish user certificate import and native SSL client authentication
            (wtc) -- 0.6. No progress on user certificate import. Finished
            native SSL client authentication (rsleevi wrote the original patch),
            which completed the switchover to NSS for SSL.
*   Detect network disconnectivity and handle it better (eroman)

## Q3 2010 Objectives

Annotations on the status of each objective (at the close of the quarter) shown
in red.

### High level

*   Measure performance.
*   Improve performance.
*   Investigate and fix bugs.
*   Enterprise features.

### Specific items

**Feature work and bug fixes for SSL library / crypto. (wtc, agl, rsleevi,
davidben)**

*   Bring the NSS SSL library to feature parity with Windows Vista's
            SChannel. -- 0. Did not have time to work on this. Postponed to Q1
            2011. Will work on native SSL client auth for NSS in Q4 2010.
*   Tackle long-standing bugs in Chrome's crypto and certificate code.
            -- 0.3. Fixed some certificate verification bugs in NSS and Chrome.
            Didn't have time to tackle the major items such as thread-safe
            certificate cache and certificate verification result cache.
*   [Certificate enrollment with the HTML &lt;keygen&gt;
            tag](http://code.google.com/p/chromium/issues/detail?id=148). --
            0.7. davidben added UI and fixed many bugs in certificate
            enrollment. Remaining work is to [support all formats of
            application/x-x509-user-cert
            responses](http://code.google.com/p/chromium/issues/detail?id=37142),
            and then to test with various CAs.

**Feature work on download handling (ahendrickson)**

*   Resume partially completed downloads, including across Chrome
            restarts. -- 0.5?; preliminary CL sent out
            (<http://codereview.chromium.org/3127008/show>)
*   Measure Chrome versus IE download performance to see whether it is
            in fact slower in chrome (user reports suggest this is the case). --
            0

**Improvements to cookie handling (rdsmith)**

*   Implement alternate eviction algorithm and measure impact (to reduce
            the cookies evicted while browsing). -- 1
*   (Stretch) [Restrict access of CookieMonster to IO
            Thread](http://code.google.com/p/chromium/issues/detail?id=44083).
            -- 0

**URL Prefetching (gavinp)**

*   [Implement link
            rel=prefetch](http://code.google.com/p/chromium/issues/detail?id=13505)
            and measure impact. -- 1.0; implemented, measurement shows 10%
            improvement of PLT
*   Implement link HTTP headers and measure impact. -- 0.5; preliminary
            code reviews sent out.

**HTTP cache (rvargas, gavinp)**

*   Simultaneous streaming readers on ranges in a cache entry (to
            support video prefetch for YouTube). -- 0
*   Experiment with [request throttling at the cache
            layer](http://code.google.com/p/chromium/issues/detail?id=10727) --
            1.0

**HTTP authentication (cbentzel)**

*   Integrated Authentication on all platforms. -- 0.9; NTLM on
            Linux/OSX not supported without auth prompt.
*   Add full proxy authentication support to
            [SocketStream](http://code.google.com/p/chromium/issues/detail?id=47069)
            and
            [SPDY](http://code.google.com/p/chromium/issues/detail?id=46620). --
            0
*   [System level tests for
            NTLM/Negotiate](http://code.google.com/p/chromium/issues/detail?id=35021).
            -- 0

**Simulated Network Tester (cbentzel, klm, tonyg)**

*   Implement basic pagecycler test over a DummyNet connection -- 0.7;
            work in progress for webpage replay
            (<http://code.google.com/p/web-page-replay/wiki/GettingStarted>)
*   Record and playback of Alexa 500 rather than static pages from 10
            years ago. -- 0
*   (stretch): Minimize false positives enough to make this a standard
            builder. -- 0

**Network Diagnostics (rdsmith, mmenke, eroman)**

*   Improve error pages to better communicate network error -- 0.7; new
            error codes for proxy and offline, and reworked some other confusing
            ones. Updated text in the works.
*   Improve error page to link to system network configurator -- 0; need
            to figure out sandboxable solution.
*   Improve network diagnostics tool for configuration problems -- 0; no
            changes

**Proxy handling**

*   [Extension API for changing proxy
            settings](http://code.google.com/p/chromium/issues/detail?id=48930)
            (pamg) -- 0.5
*   [Execute PAC scripts out of
            process](http://code.google.com/p/chromium/issues/detail?id=11746)
            (eroman) -- 0; punted

**Implement HTTP pipelining (vandebo)**

*   [crbug.com/8991](http://crbug.com/8991)

**WebKit/Chrome network integration (tonyg)**

*   Support the WebTiming spec. -- 1.0; landed in Chrome 6.
*   [Enable persisting disk cache of pre-parsed
            javascript](http://code.google.com/p/chromium/issues/detail?id=32407).
            -- 0
*   Pass all of the BrowserScope tests -- 0.9; ToT chromium scores
            91/100 on the tests

**SafeBrowsing (lzheng)**

*   [Add end to end tests for
            safe-browsing](http://code.google.com/p/chromium/issues/detail?id=47318)
            -- 1.0
*   Extract the safe browsing code to its own library that can be
            re-used by other projects -- 0

---

## Past objectives

Annotations on the status of each objective (at the close of the quarter) shown
in red.

### Milestone 6 (branch cut July 19 2010).

#### #### Run PAC scripts out of process

#### [Move the evaluation of proxy auto-config scripts out of the browser
process](http://code.google.com/p/chromium/issues/detail?id=11746) to a
sandboxed process for better security. (eroman)

#### Ended up doing multi-threaded PAC execution instead, to address performance
problems associated with speculative requests + slow DNS (crbug.com/11079)

#### Cache pre-parsed JavaScript

The work on the HTTP cache side is done. Need to write the code for [WebKit and
V8 use the interface](http://code.google.com/p/chromium/issues/detail?id=32407)
and measure the performance impact. (tonyg, rvargas)

Done. M6 has pre-parsed JS in the memory cache ON by default. It has pre-parsed
JS in the disk cache is OFF by default (--enable-preparsed-js-caching).

#### Switch to NSS for SSL on Windows

Use NSS for SSL on Windows by default. We need to modify NSS to [use Windows
CryptoAPI for SSL client
authentication](http://code.google.com/p/chromium/issues/detail?id=37560). (wtc)

Done. NSS is being used for SSL on all platforms.

#### Improve the network error page

The network error page should [help the user diagnose and fix the
problem](http://code.google.com/p/chromium/issues/detail?id=40431) (see also
[issue 18673](http://code.google.com/p/chromium/issues/detail?id=18673)), rather
than merely displaying a network error code. (eroman, jar, jcivelli)

The UI of the error page has not been improved, however some user-level
connectivity tests have been added to help diagnose when a chronic network error
is happening (chrome://net-internals/#tests).

#### #### Implement SSLClientSocketPool

#### This allows us to implement [late binding of SSL
sockets](http://code.google.com/p/chromium/issues/detail?id=30357) and is a
prerequisite refactor for speculative SSL pre-connection and pipelining.
(vandebo)

#### Done.

#### #### HTTP authentication

*   #### Implement the [Negotiate (SPNEGO) authentication scheme on
            Linux and
            Mac](http://code.google.com/p/chromium/issues/detail?id=33033) using
            GSS-API. (ahendrickson)
    #### Almost completed.
*   #### Create [system-level tests for NTLM and Negotiate
            authentication](http://code.google.com/p/chromium/issues/detail?id=35021).
            (cbentzel)
    #### Hasn't been started yet.

#### #### HTTP cache improvements

*   #### Improve the coordination between the memory cache (in WebCore)
            and disk cache (in the network stack). For example, memory cache
            accesses should count as HTTP cache accesses so that the HTTP cache
            knows how to better maintain its LRU ordering. (rvargas)
    #### Still needs investigation.
*   #### Define good cache performance metrics. Measure HTTP cache's
            hit/miss rates, including "near misses". (rvargas)
    #### Still needs investigation.
*   #### Make the [HTTP
            cache](http://code.google.com/p/chromium/issues/detail?id=26729) and
            [disk
            cache](http://code.google.com/p/chromium/issues/detail?id=26730)
            fully asynchronous. Right now the HTTP cache is serving the metadata
            synchronously, which may block the IO thread.
    #### Done.
*   #### Throttle the requests.
    #### This was dependent on making the disk cache fully asynchronous, which
    only just got finished.

#### Network internals instrumentation, logging, and diagnostics

*   [Create a chrome://net page for debugging the network
            stack](http://code.google.com/p/chromium/issues/detail?id=37421).
            (eroman)
    *   This will replace about:net-internals and about:net.
    *   Allow tracing of network requests and their internal states.
    *   Diagnosing performance problems.
    *   Getting more information from users in bug reports.
    *   Exploring and resetting internal caches.

Done. Replaced the defunct about:net with the new about:net-internals.
Instruments a lot more tracing information, support for active and passive
logging, and log generation for bug reports.

#### Define Chromium extensions API for networking

Define an API for Chromium extensions to access the network stack. We already
defined an API that exposes proxy settings to extensions. (willchan)

Some drafts were circulated for network interception APIs, but work hasn't been
started yet.

The proxy settings API has been revived, and Pam is starting on it.

#### SafeBrowsing

This is a stretch goal because we may not have time to work on this in Q2.

*   Refactor SafeBrowsing code into an independent library that can be
            shared with other SafeBrowsing clients.
    Not started, however an owner was found.
*   Integrate with SafeBrowsing test suite.
    Work in progress.

#### IPv6

*   The AI_ADDRCONFIG flag for getaddrinfo is ignored on some platforms,
            causing us to issue DNS queries for IPv6 addresses (the AAAA DNS
            records) unnecessarily. AI_ADDRCONFIG also does not work for
            loopback addresses. We should find out when to pass AF_UNSPEC with
            AI_ADDRCONFIG and when to pass AF_INET to getaddrinfo, so we get the
            best host name resolution performance. (jar)
*   Implement IPv6 extensions to
            [FTP](http://code.google.com/p/chromium/issues/detail?id=35050).
            (gavinp)
    Done. Support for EPSV.

#### Speculative TCP pre-connection

Jim Roskind has an incomplete [changelist](http://codereview.chromium.org/38007)
that shows where the necessary hooks are for TCP pre-connection. (jar)

*   First do this for search (pre-connect while user types a query)
*   Eventually pre-connect based on DNS sub-resource history so that we
            pre-connect for sub-resource acquisition before containing page even
            arrives.
*   Preliminary implementation behind flag will facilitate SDPY
            benchmarking of feature.

Initial implementation has landed; it is off by default, but can be enabled with
these flags:

--enable-preconnect

--preconnect-despite-proxy

#### Improve WebKit resource loading

Improve resource loading so we can pass all of the [network tests on
Browserscope](http://www.browserscope.org/?category=network&v=top) (Chromium
issues [13505](http://code.google.com/p/chromium/issues/detail?id=13505),
[40014](http://code.google.com/p/chromium/issues/detail?id=40014),
[40019](http://code.google.com/p/chromium/issues/detail?id=40019) and WebKit
[bug 20710](https://bugs.webkit.org/show_bug.cgi?id=20710)). Most of the work
will be in WebKit. (gavinp, tonyg).

Work in progress.

#### #### Certificate UI

*   #### [Linux certificate management
            UI](http://code.google.com/p/chromium/issues/detail?id=19991).
            (summer intern?)
    #### Work in progress.
*   #### UI for [&lt;keygen&gt; certificate
            enrollment](http://code.google.com/p/chromium/issues/detail?id=148)
            on Linux and Windows: right now &lt;keygen&gt; finishes silently.
            (summer intern?)
    #### Work in progress by summer intern.

---

## Future

#### Prioritizing HTTP transactions

*   #### Support loading resources in the background (for example, for
            updating the thumbnails in the New Tab Page) without impacting
            real-time performance if the user is doing something else.
*   #### Support dynamically adjusting priorities. If the user switches
            tabs, the newly focused tab should get a priority boost for its
            network requests.

#### #### Other HTTP performance optimizations

*   #### Reuse HTTP keep-alive connections under more conditions
*   #### Resume SSL sessions under more conditions

#### #### New unit tests and performance tests

#### Some parts of the network stack, such as SSL, need more unit tests. Good
test coverage helps bring up new ports. In addition, any bugs that get fixed
should get unit tests to prevent regression.
#### We should [add performance
tests](http://code.google.com/p/chromium/issues/detail?id=6754) to measure the
performance of the network stack and track it over time.

#### ********Fix SSLUITests********

All the [SSLUITests are marked as
flaky](http://code.google.com/p/chromium/issues/detail?id=40932) now.

#### ********Better histograms********

**We need better histograms for networking.**

**#### ****Integrate loader-specific parts of WebKit into the network stack******

Parts of WebKit that throttle and prioritize resource load requests could be
moved into the network stack. We can disable WebCore's queuing, and get more
context about requests (flesh out the ResourceType enum).

#### #### Captive portals

#### [Avoid certificate name mismatch
errors](http://code.google.com/p/chromium/issues/detail?id=71736) when visiting
an HTTPS page through a captive portal.

#### #### HTTP pipelining

#### We should implement an [optional pipelining
mode](http://code.google.com/p/chromium/issues/detail?id=8991).

#### #### HTTP authentication

*   #### [support NTLMv2 on Linux and
            Mac](http://code.google.com/p/chromium/issues/detail?id=22532)

#### We also need to review the interaction between HTTP authentication and disk
cache. For example, [cached pages that were downloaded with authentication
should not be retrieved without
authentication](http://code.google.com/p/chromium/issues/detail?id=454).

#### FTP

*   reusing control connections
*   caching directory listings.

We need to be able to [request FTP URLs through a
proxy](http://code.google.com/p/chromium/issues/detail?id=11227).

#### Preference service for network settings

We strive to use the system network settings so that users can control the
network settings of all applications easily. However, there will be some
configuration settings specific to our network stack, so we need to have our own
preference service for those settings. See also [issue
266](http://code.google.com/p/chromium/issues/detail?id=266), in which some
Firefox users demand that we not use the WinInet proxy settings (the de facto
system proxy settings) on Windows.

#### Share code between HTTP, SPDY, and WebSocket

A lot of code was copied from net/http to net/socket_stream for WebSocket
support. We should find out if some code can be shared.

#### WPAD over DHCP

Support [WPAD over
DHCP](http://code.google.com/p/chromium/issues/detail?id=18575).
