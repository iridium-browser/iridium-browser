---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-privacy
  - Chromium Privacy
- - /Home/chromium-privacy/privacy-sandbox
  - The Privacy Sandbox
page_name: floc
title: FLoC Origin Trial & Clustering
---

**This page refers to the origin trial for the initial version of FLoC, which
ran from Chrome 89 to 91.**

---

See [web.dev/floc](https://web.dev/floc) for an explanation of the idea behind
this experimental new advertising-related browser API, a component of Chrome's
Privacy Sandbox effort to support web advertising without user tracking. To
participate in the development process, see the [FLoC GitHub
repository](https://github.com/WICG/floc).

Even for developers experienced with [origin
trials](https://web.dev/origin-trials/) and [third-party origin
trials](https://web.dev/third-party-origin-trials/), the FLoC origin trial is a
bit different. That's because FLoC is two different things: a JavaScript API
that offers a signal which we hope will prove useful for interest based ads
targeting, and also an on-device clustering algorithm that generates the signal.

Figuring out the right way to perform that clustering is still very much an open
question. During the course of the Origin Trial we expect to introduce multiple
possible clustering algorithms, and we solicit feedback concerning both the
privacy and the utility of the clusters produced. We hope that during the Origin
Trial, the ad tech community will collectively figure out which tasks are well
served by the FLoC approach. As we inevitably find areas where FLoC could do
better, we look forward to public discussion about what modifications to
clustering might help serve those uses.

You might wonder: once there are multiple clustering algorithms performing FLoC
assignment, how do you know which one you're getting? Per the [draft spec for
the API](https://wicg.github.io/floc/), the object returned by cohort = await
document.interestCohort(); has two keys: an id indicating which cluster the
browser is in, and a version, a label that identifies the algorithm used to
compute that id. (The API is not permitted in an insecure context, or where
blocked by a Permissions-Policy, or on a site where you've used Chrome settings
to block cookies.)

We realize this strange situation, of a single API that might be wrapped around
multiple different possible algorithms, means the Origin Trial of FLoC is not
for the faint of heart. If you're still interested in joining us during this
early experimental stage of our development, check out [this
page](https://developer.chrome.com/blog/floc/) for the details of how to take
part.

# FLoC Algorithm Versions

## Version "chrome.2.1"

This algorithm was introduced in Chrome 89. It is similar to the approach called
SortingLSH that was
[described](https://github.com/google/ads-privacy/blob/master/proposals/FLoC/FLOC-Whitepaper-Google.pdf)
by our colleagues in Google Research and Ads in October 2020, which their
experiments indicated performs rather well for [some types of ad
targeting](https://blog.google/products/ads-commerce/2021-01-privacy-sandbox/#jump-content:~:text=in%2Dmarket%20and%20affinity%20Google%20Audiences):
"Affinity Audiences" (like "Cooking Enthusiasts") and "In-Market Audiences"
(like "people actively researching Consumer Electronics").

In this clustering technique, people are more likely to end up in the same
cohort if they browse the same web sites. Only the domain of the site is used —
not the URL or the contents of the pages, for example.

The browser instance's cohort calculation is based on the following inputs:

    A subset of the registrable domain names (eTLD+1's) in the browser's Chrome
    history for the seven-day period leading up to the cohort calculation.

    A domain name is included if some page on that domain either:

        uses the document.interestCohort() API, or

        is detected as loading ads-related resources (see [Ad Tagging in
        Chromium](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/ad_tagging.md)).

    The API is disabled, and the domain name is ignored, on any page which is
    served with the HTTP response header Permissions-Policy: interest-cohort=().

    Domain names of non-publicly routable IP addresses are never included.

The inputs are turned into a cohort ID using a technique we're calling
PrefixLSH. It is similar to a SimHash variant called SortingLSH that was
[described](https://github.com/google/ads-privacy/blob/master/proposals/FLoC/FLOC-Whitepaper-Google.pdf)
by our colleagues in Google Research and Google Ads last October.

    The browser uses each domain name included in the inputs to
    deterministically produce one 50-dimensional floating-point vector whose
    coordinates are pseudorandom draws from a Gaussian distribution, with the
    pseudorandom number generator seeded from a hash of the domain name. (Note:
    ultimately in all the 50-dimensional vectors described here, only the first
    20 coordinates are ever used; the length of 50 is vestigial.)

    The browser then uses the full set of domain name inputs to
    deterministically produce a 50-bit Locality-Sensitive Hash bitvector, where
    the i'th bit indicates the sign (positive or negative) of the sum of the
    i'th coordinates of all the floating-point vectors derived from the domain
    names.

    A Chrome-operated server-side pipeline counts how many times each 50-bit
    hash occurs among [qualifying
    users](https://github.com/WICG/floc#qualifying-users-for-whom-a-cohort-will-be-logged-with-their-sync-data)
    — those for whom we log cohort calculations along with their sync data.

    The 50-bit hashes start in two big cohorts: all hashes whose first bit is 0,
    versus all hashes whose first bit is 1. Then each cohort is repeatedly
    divided into two smaller cohorts by looking at successive bits of the hash
    value, as long as such a division yields two cohorts each with at least 2000
    qualifying users. (Each cohort will comprise thousands of people total, when
    including those Chrome users for whom we don't sync cohort data.)

    The result is a list of cohorts represented as Locality-Sensitive Hash
    bitvector prefixes, which we number in lexicographic order and distribute to
    all Chrome browsers. Any browser can calculate its own 50-bit hash, find the
    unique prefix of that vector which appears in the list of cohorts, and read
    off the corresponding cohort ID.

    Note that this is an unsupervised clustering technique; no Federated
    Learning is used (despite the "FL" in the name). The only parameters of the
    clustering model are the details of pseudorandom number generation and the
    minimum cluster size threshold.

After creation of the list of cohorts based on Locality-Sensitive Hash bitvector
prefixes, we impose additional filtering criteria. Any time a browser instance's
cohort is filtered, the promise returned by document.interestCohort() rejects,
without further indication of the reason for rejection.

    Some filtering is calculated by the server-side pipeline, and the result is
    included with the list of cohort prefixes distributed to all Chrome
    instances:

        A cohort is filtered if it has too few qualifying users. (This is not
        possible at the outset, since the server-side clustering pipeline would
        not produce an under-sized cohort, but it could happen over time as
        people's browsing behavior changes. We do not handle changing cohort
        sizes by re-calculating the list of LSH prefixes, since that would
        change the meaning of existing cohorts ids.)

        A cohort is filtered if the browsing behavior of its qualifying users
        has a higher-than-typical rate of visits to web pages on sensitive
        topics. See [this
        paper](https://docs.google.com/a/chromium.org/viewer?a=v&pid=sites&srcid=Y2hyb21pdW0ub3JnfGRldnxneDo1Mzg4MjYzOWI2MzU2NDgw)
        for an explanation of the t-closeness calculation.

    Other filtering happens in an individual browser instance:

        An individual browser instance's cohort is filtered if the inputs to the
        cohort id calculation has fewer than seven domain names.

        An individual browser instance's cohort is filtered any time its user
        clears any browsing history data or other site data; a new cohort id is
        eventually re-computed without the cleared history.

        An individual browser instance's cohort is filtered in incognito
        (private browsing) mode

All details are specific to this particular version of FLoC clustering, and
subject to change in future clustering algorithms.

Observed statistics of the cohorts created by this clustering algorithm, based
on data from qualifying Chrome users:

    Number of cohorts, before any filtering: 33,872

    Number of LSH bits used to define a cohort: between 13 and 20

    Minimum number of qualifying Chrome users in a cohort: 2000

    Minimum number of different qualifying Chrome user browsing histories (sets
    of visited domains) in a cohort: 735

    Number of cohorts filtered due to sensitive browsing t-closeness test
    (t=0.1): 792 (approx. 2.3%)
