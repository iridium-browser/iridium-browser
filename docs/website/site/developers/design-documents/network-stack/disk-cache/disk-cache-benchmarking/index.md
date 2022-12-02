---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
- - /developers/design-documents/network-stack/disk-cache
  - Disk Cache
page_name: disk-cache-benchmarking
title: Disk Cache Benchmarking & Performance Tracking
---

## Summary

More backend work in the disk cache is demanding good tools for ongoing disk
cache performance tracking. The [Very Simple
Backend](/developers/design-documents/network-stack/disk-cache/very-simple-backend)
implementation needs continuous A/B testing to show its implementation is
advancing in speed. To track progress on this backend, and also to permit
comparisons between alternative backends provided in net, two ongoing
methodologies are proposed.

## Proxy Backend with Replay

The Proxy Backend is a simple [Disk Cache
Backend](/developers/design-documents/network-stack/disk-cache) and Entry
implementation that pass through to an underlying Entry and Backend, but
recording a short log with parameter information & timing information to allow
replay.

This log can then be replayed in a standalone replay application which takes the
log, constructs a backend (perhaps a standard blockfile backend, a [very simple
backend](/developers/design-documents/network-stack/disk-cache/very-simple-backend)
or a log structured backend), and performs the same operations at the same
delays, all calls as if from the IO thread. The average latency of calls, as
well as system load during the test can then permit A/B comparison between
backends.

Pros:

*   Well suited to use on developer workstations.
*   Very simple to collect logs and run.
*   Very fast to run tests.
*   Low level, includes very little noise from outside of the disk
            cache.
*   Using logs with multiple versions of the same backend allows
            tracking progress over time of a particular backend.

Cons:

*   Sensitive to evictions: if two different backends have different
            eviction algorithms, the same operations on two backends can result
            in different logs. For instance an OpenEntry() on the foo backend
            could find an entry that then is Read/Written to when the same log
            played on a bar backend.
*   Compares all low level operations equally: backend operations in the
            critical path of requests block launching requests. Other backend
            operations (like WriteData) almost never occur in the critical path,
            and so performance may impact rendering less. Without a full
            renderer, this impact is hard to measure.
*   Does not track system resource consumption. Besides answering
            requests, the backend is consuming finite system resources (RAM,
            buffer cache, file handles, etc...), competing with the renderer for
            resources. This impact isn't very well measured in the replay
            benchmark.

## browser_test with corpus

Starting up with a backend, the browser_test loads a large corpus of pages
(either from a local server, or a web server simulating web latency, or possibly
even the actual web), with an initially empty cache, and then runs either that
same corpus or a second corpus immediately afterwards with the warm cache. This
can be repeated using different backends, to permit A/B comparisons.

The browser test should introspect on UMA; outputs should include HttpCache.\*
benchmarks, as well as PLT.BeginToFirstPaint/PLT.BeginToFinish.

Pros:

*   Well suited to ongoing performance dashboards.
*   Tracks a metric more closely connected to user experience.
*   Allows comparison of backends with wildly different eviction
            behaviour.

Cons:

*   Noisier, since it includes much, much more than just the Chrome
            renderer.
*   Slower to run.
*   Because of ongoing renderer changes, comparisons of the same backend
            over time are problematic without a lot of patch cherry picking.
