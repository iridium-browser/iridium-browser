---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: adding-performance-tests
title: Adding Performance Tests
---

Chrome runs a battery of [performance tests](https://chromeperf.appspot.com/)
against every build. These tests are monitored by the [perf
sheriffs](http://www.chromium.org/developers/tree-sheriffs/perf-sheriffs) for
regressions. The best ways to ensure your feature stays fast and gets faster is
to add a performance test.

1.  **Create the test.** Most new perf tests will want to use the
            [Telemetry](/developers/telemetry) framework. All enabled Telemetry
            tests are automatically detected and run by the perf bots. If you
            know what you are doing and have a good reason not to use Telemetry,
            a perf test may be any program or script which outputs results in
            the format the builder understands. You'll need to edit the bot
            configurations to run the test.
2.  **Announce it.** When you add, remove, or change a test, you can
            email chrome-speed-infra@google.com.
3.  **Monitor the results.** Finally, monitor the results for
            regressions. Once the test is monitored, it will show up in the
            dashboard by default. To do so, use the "Report Issue &gt; Request
            Monitoring for Tests" menu item on the [perf
            dashboard](http://chromeperf.appspot.com/), or email
            chrome-speed-infra@google.com
