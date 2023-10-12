---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-privacy
  - Chromium Privacy
page_name: privacy-sandbox
title: The Privacy Sandbox
---

[TOC]

## Overview

The Privacy Sandbox project’s mission is to “Create a thriving web ecosystem
that is respectful of users and private by default.”
The main challenge to overcome in that mission is the pervasive cross-site
tracking that has become the norm on the web and on top of which much of the
web’s ability to deliver and monetize content has been built. Our first
principles for how we’re approaching this are laid out in the [Privacy Model for
the Web explainer](https://github.com/michaelkleber/privacy-model). We believe
that part of the magic of the web is that content creators can publish without
any gatekeepers and that the web’s users can access that information freely
because the content creators can fund themselves through online advertising.
That advertising is vastly [more
valuable](https://services.google.com/fh/files/misc/disabling_third-party_cookies_publisher_revenue.pdf)
to publishers and advertisers and [more engaging and less annoying to
users](https://services.google.com/fh/files/misc/disabling_third-party_cookies_publisher_revenue.pdf)
when it is relevant to the user.
We plan to introduce new functionality to serve the use cases that are part of a
healthy web that are currently accomplished through cross-site tracking (or
methods that are indistinguishable from cross-site tracking). As that
functionality becomes available we will place more and more restrictions on the
use of third party cookies, which are the most common mechanism for cross-site
tracking today and eventually deprecate them entirely. In parallel to that we
will aggressively combat the current techniques for non-cookie based cross-site
tracking, such as fingerprinting, cache inspection, link decoration, network
tracking and Personally Identifying Information (PII) joins.
More about our intentions in [“Building a more private web: A path towards
making third party cookies
obsolete.](https://blog.chromium.org/2020/01/building-more-private-web-path-towards.html)”

## Building Privacy Sandbox

We see three distinct tracks:

### Replacing Functionality Served by Cross-site Tracking

Since third party cookies have been a part of the web since before its
commercial coming of age in the 90s, critical functionality that the web has
come to rely on (e.g., single sign-on, and personalized ads) has been developed
assuming that functionality exists. In order to transition the web to a more
privacy respecting default, it is incumbent upon us to replace that
functionality as best we can with privacy-conscious methods.
In the ideal end state, from a user’s perspective, there won’t be any difference
between how the web of today and the web in a post-Privacy Sandbox world work,
except that they will be able to feel confident that the browser is working on
their behalf to protect their privacy and when they ask questions about how
things work they will like the answers they find. In addition, if a given user
is either uncomfortable with or just doesn’t like personalized advertising, they
will have the ability to turn it off without any degradation of their experience
on the web.
Relevant use-cases:

*   Combating Spam, Fraud and DoS:[ Trust Tokens
            API](https://github.com/WICG/trust-token-api)
*   Ad conversion measurement:
    *   [Click Through Conversion Measurement Event-Level
                API](https://github.com/csharrison/conversion-measurement-api)
    *   [Aggregated Reporting
                API](https://github.com/csharrison/aggregate-reporting-api)
*   Ads targeting:
    *   Contextual and first-party-data targeting fits into proposal of
                Privacy Model in that it only requires first party information
                about the page that the user is viewing or about that user’s
                activity on their site.
    *   Interest-based targeting:
                [FLoC](/Home/chromium-privacy/privacy-sandbox/floc)
    *   Remarketing: [Private Interest Groups, Including Noise
                (PIGIN)](https://github.com/michaelkleber/pigin) now replaced by
                [Two Uncorrelated Requests, Then Locally-Executed Decision On
                Victory
                (TURTLE-DOV)](https://github.com/michaelkleber/turtledove)
*   Federated login:
    *   [WebID](https://github.com/samuelgoto/WebID)
*   SaaS embeds, third-party CDNs:
    *   [Partitioned cookies
                (CHIPS)](https://github.com/DCtheTall/CHIPS)

### Turning Down Third-Party Cookies

As noted above, the third party cookies are the main mechanism by which users
are tracked across the web. We eventually need to remove that functionality, but
we need to do it in a responsible manner.
Relevant projects:

*   Separating First and Third Party Cookies: [Requirement to label
            third party cookies as “SameSite=None, as well as require them to be
            marked Secure](https://web.dev/samesite-cookies-explained/)
*   Creating [First-Party Sets
            ](https://github.com/krgovind/first-party-sets/)
*   [Removing third party cookies
            ](https://blog.chromium.org/2020/01/building-more-private-web-path-towards.html)

### Mitigating workarounds

As we’re removing the ability to do cross-site tracking with cookies, we need to
ensure that developers take the well-lit path of the new functionality rather
than attempt to track users through some other means.
Our focus (more details to be added)

*   Fingerprinting:
    *   [Privacy Budget](https://github.com/bslassey/privacy-budget)
    *   Removing Passive Fingerprinting Surfaces
        *   [Client-side language
                    selection](https://github.com/davidben/client-language-selection)
        *   [User-Agent String](https://github.com/WICG/ua-client-hints)
    *   Reducing Entropy from Surfaces
        *   [Device
                    Sensors](https://bugs.chromium.org/p/chromium/issues/detail?id=1018180)
        *   [Battery
                    Level](https://bugs.chromium.org/p/chromium/issues/detail?id=661792)
    *   [IP Address](https://github.com/bslassey/ip-blindness)
*   Cache inspection
    *   [Partitioning HTTP
                Cache](https://docs.google.com/document/d/1XJMm89oyd4lJ-LDQuy0CudzBn1TaK0pE-acnfJ-A4vk/edit)
    *   [Connection Pools](https://fetch.spec.whatwg.org/#connections)
    *   [DNS Cache, Sessions Tickets, HTTP Server
                properties](https://github.com/MattMenke2/Explainer---Partition-Network-State)
    *   [Other
                caches](https://docs.google.com/document/d/1V8sFDCEYTXZmwKa_qWUfTVNAuBcPsu6FC0PhqMD6KKQ/edit#heading=h.ve7o178iijzr)
*   Navigation tracking
    *   [Referer
                clamping](https://groups.google.com/a/chromium.org/d/msg/blink-dev/aBtuQUga1Tk/n4BLwof4DgAJ)
*   Network Level tracking
    *   [DNS](/developers/dns-over-https)
    *   [SNI](https://github.com/tlswg/draft-ietf-tls-esni)
    *   [Moar
                TLS](https://www.usenix.org/sites/default/files/conference/protected-files/enigma_slides_schechter.pdf)

## How to participate

In general, we welcome the community to give feedback by filing issues on
explainers hosted on Github, via the blink-dev intent posts or in any relevant
standards body. For ads focused API proposals in particular. we encourage you to
give [feedback](https://github.com/w3c/web-advertising/blob/master/README.md) on
the [web standards community](https://www.w3.org/community/web-adv/) proposals
via GitHub and make sure they address your needs. And if they don’t, file issues
through GitHub or [email](https://lists.w3.org/Archives/Public/public-web-adv/)
the W3C group.
If you rely on the web for your business, please ensure your technology vendors
engage in this process and share your feedback with the trade groups that
represent your interests.
