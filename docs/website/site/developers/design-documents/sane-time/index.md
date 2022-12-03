---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: sane-time
title: Sane time
---

"...because any sane person knows what day it is!" -- Raz

## Background

There are cases in Chrome where it is important to have an accurate time source.
Although most modern operating systems now automatically synchronize computer
clocks to some external time source, some computers may still have a skewed
clock for various reasons: time syncing may not be configured correctly, the
user may have manually set the clock erroneously, malware commonly changes the
system time, etc. Thus, it is desirable for Chrome itself to keep track of time
from an external source and its offset from the time on the local computer.

The use of time in TLS (for certificate verification) has two consequences.
First, we cannot depend upon a client’s ability to make a TLS connection to
establish the time! And second, it’s important that the source of time be
secure, because TLS security depends upon it. An attacker who can control the
time on the user’s computer can, for example, cause Chrome to accept an expired
certificate as valid.

[Time Sources](/developers/design-documents/time-sources) discusses a similar
problem for Chrome OS devices.

## Design

We desire that the time be known to within a few minutes. Assuming that the time
on any Google server is accurate, and that we can communicate to Google machines
securely, we call any time that we receive from a Google server over a secure
channel "sane time" (because it's just accurate enough to be considered sane).

In the case where the client’s clock is badly wrong, we won’t be able to
establish a TLS connection. Hence, we will use a purpose-built time server.

#### What about NTP?

NTP is a protocol built expressly for time synchronization. But relying on it in
Chrome has some disadvantages:

    It requires maintaining NTP client code in Chrome, which is a higher burden
    than the solution outlined here, both in code size and developer time.

    Chrome may be unable to send UDP, as suggested by metrics from QUIC. Making
    time available to the widest range of clients suggests we should prefer
    HTTP.

    There are [attacks on NTP](http://www.cs.bu.edu/~goldbe/NTPattack.html).

Another possibility is to rely on the local time if it's already running an NTP
client. This is also problematic, as it involves writing code to detect running
NTP clients and verifying that it's sanely configured.

#### Representation and accuracy

In Chrome, there are two clocks: a wall clock (base::Time) and a non-decreasing
clock (base::TimeTicks). The wall clock is what changes when the local time is
changed, so we cannot reliably use it to compare against the server time unless
we can detect when it changes (currently not possible). Furthermore, we have no
way of detecting when it changes when Chrome isn't running!

The non-decreasing clock, which is usually a tick count since the computer has
started, is unaffected when the local time is changed. However, it becomes
meaningless across restarts of Chrome (since the computer may be rebooted
between restarts). Furthermore, it may stop when the computer is put to sleep,
so even though it is intended for measure durations, it gives inaccurate results
if the computer was asleep for the measured time interval.

Chrome can detect when the computer goes to sleep and wakes, but only on some
platforms. So instead, we define an idea of “sanity”, meaning that the wall
clock and non-decreasing clock have recorded approximately equal times since the
last network time query. The internal representation of time thus consists of a
tuple of (network time, wall clock time, tick clock time), which is serialized
to a pref and restored at startup. Note that a restart that resets the tick
clock will cause the deserialized representation to be rejected as insane.

Finally, we record an uncertainty (base::TimeDelta) that accounts for both the
coarseness of the time representation, and the latency in fetching it. But, to
reiterate, we are aiming only for an approximation: availability is much more
important than accuracy.

#### Time protocol

Since the [Omaha
protocol](https://github.com/google/omaha/blob/master/doc/ServerProtocol.md) has
the needed security properties, and since it works over HTTP, and since it
already has a server fleet deployed, we’ll use it as the basis for a temporary
time protocol. Client requests will include a random nonce as a GET parameter,
which is signed in the server’s response. (This incidentally defeats caching.)
This is fairly heavyweight, of course, but since we are still experimenting we’d
rather leverage existing code than design and support a lightweight protocol
with a long lifetime. The responses look like this:

> **)\]}'**

> **{"protocol_version": 1, "current_time_millis": 1457471762000}**

To avoid excess queries, queries will be sent only when the time is insane or
unavailable, but no more often than every (say) 60 minutes. We’ll make this
number configurable, to allow experimentation & to enable us to disable the
feature if necessary. Another possible refinement is platform-specific
adjustments, e.g. to save network traffic on Android or to allow more frequent
updates on Windows, which tends to have less accurate clocks. On startup, if
sane time is unavailable via deserialization, Chrome will wait for a random
backoff period before issuing its first time query.

This hopefully avoids most “thundering herd” scenarios, since time queries are
issued only in response to local events.

For managed devices, we may need to supply a way to disable time queries.

Use of sane time in SSL certificate validation

One day, it would be nice to use secure time for all certificate validation
decisions, but it will take a while to get there.

As a first step, we’ll use sane time to validate certificate rejections: if the
rejection was due to the wall clock being wrong, Chrome displays an interstitial
asking the user to correct it, and records SHOW_BAD_CLOCK to UMA.

Use of sane time in upgrades

Sane time is used in upgrade_detector_impl.cc to detect when Chrome needs to be
updated.

## Code changes

Addition of the NetworkTimeTracker class was done in bug
[146090](http://crbug.com/146090). The SSL “bad clock” interstitial was done in
bug [414843](http://crbug.com/414843). The bug for the changes below is
[589700](http://crbug.com/589700).

Time is kept by the
[NetworkTimeTracker](https://code.google.com/p/chromium/codesearch#chromium/src/components/network_time/network_time_tracker.h)
class. The following changes will be needed to bring its implementation into
line with this design:

    Change the source of time updates. Once we’ve stood up the new time servers,
    we’ll add code to periodically query them. The most natural way to do this
    is to change NetworkTimeTracker to manage time updates on its own, rather
    than than relying on external callers (currently just variations_service.cc)
    to provide time. UpdateNetworkTime should become a private method.

    For managed devices, implement a policy to disable time queries.

    Add the notion of “sanity” to NetworkTimeTracker. This means recording the
    value of the tick clock, as well as the value of the wall clock, so that
    they can be compared. Most likely we’ll change received_network_time to
    simply return false when the clock is insane.

    The amount of skew allowed between the wall clock and the tick clock can be
    generous (hours?), but we’ll still want to establish by some experimental
    means that most clients can run for a long time without becoming insane.

And in addition, to benefit from these changes:

1.  Change ssl_error_handler.cc to call
            NetworkTimeTracker::GetNetworkTime to decide whether to display the
            “please fix your clock” interstitial. If it is simple to extract the
            validity period of the rejected certificate, we’ll ensure that sane
            time falls within its validity period before showing the
            interstitial. If that’s not easy to do, we’ll have to define a rule
            of thumb, like “only display the interstitial if the wall clock is
            more than 24 hours different from sane time.”
