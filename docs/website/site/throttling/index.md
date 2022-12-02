---
breadcrumbs: []
page_name: throttling
title: Anti-DDoS HTTP Throttling of Extension-Originated Requests
---

Chrome 20 and later implements a mechanism that is intended to prevent
distributed denial of service (DDoS) attacks from being perpetrated, maliciously
or accidentally, by extensions running within Chrome. Chrome 12 through 19 had a
different flavor of this feature, see
[here](/throttling/anti-ddos-http-throttling-in-older-versions-of-chrome).

To disable the feature, which may be useful for some extension developers: Pass
the **--disable-extensions-http-throttling** command-line flag when starting
Chrome.

The way the mechanism works is, once a few server errors (HTTP error codes 500
and greater) in a row have been detected for a given URL (minus the query
parameters), Chrome assumes the server is either unavailable or overloaded due
to a DDoS, and denies requests to the same URL for a short period of time. If,
after this period of time, requests keep failing, this "back-off interval"
period is increased using an exponential factor, and so on and so forth until
the maximum back-off interval is reached. It's important to note that failures
due to the throttling itself are not counted as failures that cause the back-off
interval to be increased.

The back-off delay is calculated as follows: delay = initial_backoff \*
multiply_factor^(effective_failure_count - 1) \* Uniform(1 - jitter_factor, 1\]

For the canonical details on the back-off parameters used, see
<http://src.chromium.org/viewvc/chrome/trunk/src/net/url_request/url_request_throttler_entry.cc?view=markup>.
What follows is based on the most recent values for these parameters as of this
writing:

The back-off parameters used in the formula above will (at maximum values, i.e.
without the reduction caused by jitter) add 0-41% (distributed uniformly in that
range) to the "perceived downtime" of the remote server, once exponential
back-off kicks in and is throttling requests for more than about a second at a
time. Once the maximum back-off is reached, the added perceived downtime
decreases rapidly, percentage-wise.

Another way to put it is that the maximum additional perceived downtime with
these numbers is a couple of seconds shy of 15 minutes, and such a delay would
not occur until the remote server has been actually unavailable at the end of
each back-off period for a total of about 48 minutes.

Back-off does not kick in until after the first 4 errors, which helps avoid
back-off from kicking in on// flaky connections. To simplify life for web
developers, throttling is never used for URLs that resolve to localhost.

If you believe your extension may be having problems due to HTTP throttling, you
can try the following:

1.  Pass the **--disable-extensions-http-throttling** command-line flag
            when starting Chrome, to see if the problem reproduces with the
            feature turned off.
2.  With exponential back-off throttling of extension requests turned on
            (i.e. **without** the command-line flag above), visit
            [chrome://net-internals/#events](javascript:void(0);) and keep it
            open while you reproduce the problem you were seeing. Diagnostic
            information will be added to the log, which may help you track down
            what is happening.
