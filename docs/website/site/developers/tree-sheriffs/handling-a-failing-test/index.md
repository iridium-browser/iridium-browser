---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/tree-sheriffs
  - Tree Sheriffs
page_name: handling-a-failing-test
title: Handling a failing test
---

[TOC]

**File a bug:**

At [crbug.com](http://crbug.com) and leave as much context about the problem in
the bug report. At least make sure to include the following in the bug report:

*   **Copy+paste the relevant parts of the log into the bug report** --
            it is not sufficient to give a URL to the bot log, as the logs
            eventually get deleted.
*   Comment detailing the action you took (disabled, on what platform).
*   Indicate if the test is Flaky (intermittently failing), constantly
            failing, timing out, crashing, etc.
*   Use Component Tests-Disabled.
*   Link to build logs for that test:
            <http://chromium-build-logs.appspot.com/>
*   Wherever possible, assign an owner who will actively look into it.
*   Wherever possible, add any applicable feature components, and use
            OWNERS or git log for that test file to assign an owner or cc
            someone who can actively look into it.
*   Handy shortlink for filing this type of bug:
            <http://bit.ly/disabled-test-tracker>

[An example bug](http://crbug.com/130358) (Comment #0 demonstrates how to file
the tracker, comment #1 shows the CL to disable the test)

The document outlining the disabled tests process is
[here](http://bit.ly/chrome-disabled-tests).

### Disable the test:

Prefix DISABLED_ to the name of the crashing/timing out test.

```none
TEST(ExampleTest, CrashingTest) {
```

becomes

```none
// Crashes on all platforms.  <http://crbug.com/1234>
TEST(ExampleTest, DISABLED_CrashingTest) {
```

If the test is crashing/timing out on a proper subset of the major platforms
(some, but not all), use an #ifdef to only disable the test for those platforms.

```none
// Crashes on Mac/Win only.  <http://crbug.com/2345>
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#define MAYBE_CrashingTest DISABLED_CrashingTest
#else
#define MAYBE_CrashingTest CrashingTest
#endif
TEST(ExampleTest, MAYBE_CrashingTest) {
```

Notice the use of the MAYBE_ moniker: it's possible that the name of the test is
an identifier in the testing code, e.g., the name of the test is the same name
as the method being tested in the test.

If the test is failing only on non-CQ, FYI bots (e.g. only when a specific WIP
feature is enabled), then consider disabling the test using a filter file placed
under //testing/buildbot/filters (and referring to the filter file in bot
configuration using --test-launcher-filter-file).

### FAILS_ and FLAKY_ are no longer used:

Previously FAILS_ and FLAKY_ prefixes were used to continue running tests and
collecting results. Due to build bot slowdowns and false failures for developers
we no longer do so. This was discussed in the Feb 2012 [Disabling Flaky Tests
thread](https://groups.google.com/a/chromium.org/group/chromium-dev/browse_thread/thread/fcec09fc659f39a6/ef71cbf47185c095#).

FAILS_ and FLAKY_ are ignored by the builders and not run at all. To collect
data for potential flaky tests, just enable them as normal, the builder will
automatically retry any tests 3 tests, so the flakiness shouldn't cause any tree
closures (but the flakiness dashboard will still be told of those flakes).

#### When you see "Running TestCase" in a browser_tests test:

Follow the appropriate step above, wrapping C++ lines with `GEN('');` See [WebUI
browser_tests - conditionally run a
test...](/Home/domui-testing/webui-browser_tests#TOC-Conditionally-run-a-test-using-gene)

**Disabling Java Tests:**

Add @DisabledTest as explained in [Android Tests](/developers/testing/android-tests).
