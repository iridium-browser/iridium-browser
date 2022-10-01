---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/platform-predictability
  - Web Platform Predictability
page_name: compat-tools
title: Web compat analysis tools
---

When deprecating or changing web-exposed behavior, it's often important to get a
clear understanding of the compatibility impact. We have a variety of tools
available to you depending on the scenario. This page is designed to help you
choose the appropriate tool (in decreasing order of usage). For questions and
discussion, e-mail
[feature-control@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/feature-control).

## UseCounter

[UseCounter](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/use_counter_wiki.md)
is a framework in Blink which is used to record per-page anonymous aggregated
metrics on feature usage, often via the [\[Measure\] idl
attribute](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLExtendedAttributes.md#Measure_i_m_a_c).
Results are shown publicly on
[chromestatus.com](https://www.chromestatus.com/metrics/feature/popularity) (for
the [dominant milestone per
day](https://github.com/GoogleChrome/chromium-dashboard/issues/279)) . More
detailed break-downs are available to Google employees via the
[Blink.UseCounter.Features histogram using a formula with the PageVisits bucket
in the denominator](https://goto.google.com/uma-usecounter). Internally it's
also possible to look at UseCounter by the [fraction of users that hit it at
least once in a day](https://goto.google.com/uma-usecounter-peruser), and
UseCounters [hit within Android
WebView](https://goto.google.com/uma-usecounter-webview). In the vast majority
of cases, compat tradeoffs are made entirely based on public UseCounter data.

**Pros:**

*   Reflects real Chrome usage - should be the primary source of all
            compat discussions in blink
*   Has huge coverage - reflects a wide fraction of all usage of Chrome

**Cons:**

*   Requires at least a couple weeks to get any useful data (several
            months to roll out to stable)
*   Biased against scenarios where UMA tends to be disabled more often
            (eg. enterprises)
*   Can't be publicly used to get specific URLs. However, Googler's are
            starting to be able to do this internally with
            [CrUX](https://developers.google.com/web/tools/chrome-user-experience-report/)
            data ("UKM"). While limited for privacy reasons, it's already
            proving quite useful.

## Simple web search

Often it's useful to find examples of specific coding patterns in order to
understand the likely failure modes and formulate migration guidance. Use
technical web search engines like [nerdydata.com](https://nerdydata.com), or for
problems in specific libraries, ranking sites like
[libscore.com](https://libscore.com).

**Pros:**

*   Simple, easy to use
*   Results in specific URLs for further analysis

**Cons:**

*   Strictly a dumb static search, can't reliably find all uses of an
            API (especially due to Javascript minifiers which generate code like
            "a\[b\]()").

## The HTTP Archive

A slightly more advanced form of static web search is to use the [HTTP
Archive](http://httparchive.org/), a database of the top 500k websites, updated
by a crawl twice a month. See [HTTP Archive for web compat decision
making](https://docs.google.com/document/d/1cpjWFoXBiuFYI4zb9I7wHs7uYZ0ntbOgLwH-mgqXdEM/edit#heading=h.1m1gg72jnnrt)
for details on using it for compat analysis.

**Pros:**

*   Can provide an absolute measure of risk ("only 10 sites in the top
            500k appear to use this API").
*   Now [includes UseCounter
            data](https://groups.google.com/a/chromium.org/forum/#!topic/blink-api-owners-discuss/uxwEuxCRfGA),
            so can go beyond simple static search to some dynamic behavior

**Cons:**

*   Only captures behavior triggered during page load
*   Only reflects the home page of the top 500k sites
*   Analysis is more involved

## Microsoft's CSS Usage Data

[CSS usage on the web
platform](https://developer.microsoft.com/en-us/microsoft-edge/platform/data/)
is "from a Bing-powered scan" of lots of pages, and measures both CSS properties
and values. (Chrome use counters generally don't exist for values.)

## GitHub and stackoverflow deprecation warning search

Once a deprecation warning has been landed (or made it to stable), it can be
[extremely
informative](https://groups.google.com/a/chromium.org/forum/#!topic/intervention-dev/_0eSO-NjULo)
to search [GitHub](https://github.com/) issues (or other developer help sites
like [stackoverflow.com](https://stackoverflow.com/)) for discussion of the
warning generated on the console (eg. by searching for the chromestatus ID
present in the warning).

**Pros:**

*   Helps identify the real-world pain experienced by developers
*   Provides an avenue for outreach, and can [build
            goodwill](https://twitter.com/jgwhite/status/832517528899448832)

**Cons:**

*   Long turn-around time
*   Lossy - absence of signal doesn't really indicate an absence of risk

## GitHub code search

Most of the popular libraries and frameworks are present on GitHub.
[Searching](https://github.com/search) for potentially impacted code can be
useful in better understanding the risk.

**Pros:**

*   Provides unobfuscated access to code
*   Provides an avenue to engage with developers to better understand
            their use cases and their ability to apply mitigations.

**Cons:**

*   Doesn't provide much signal on magnitude of impact
*   Supports only either simple static searches.

## On-demand crawl

Occasionally it's useful to search top sites for a specific behavior (without
landing a UseCounter and waiting for the data to show up in HTTP Archive). For
advanced cases like this we can run a custom chromium build on the [telemetry
cluster](/developers/cluster-telemetry) to crawl the top 10k (or more) sites and
record whatever we like (with a temporary UseCounter). See [Using Cluster
Telemetry for UseCounter
analysis](https://docs.google.com/document/d/1FSzJm2L2ow6pZTM_CuyHNJecXuX7Mx3XmBzL4SFHyLA/edit#)
for details.

**Pros:**

*   Can have fast turn-around time (hours)
*   Usually used just for page load, but can be extended to trigger
            other interactions (scroll, clicking on links, etc.).

**Cons:**

*   Limited scope
*   Complicated and brittle. Relies on some changes to telemetry that
            cannot currently be landed. Generally found not to be worth the
            effort compared to the alternatives above.
