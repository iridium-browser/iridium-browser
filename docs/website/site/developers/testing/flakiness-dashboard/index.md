---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: flakiness-dashboard
title: Flakiness Dashboard HOWTO
---

## Use the Flake Portal Instead

The Flakiness dashboard is unmaintained. Please try the [Flake
Portal](https://chromium.googlesource.com/infra/infra/+/HEAD/appengine/findit/docs/flake_portal.md)
instead.

## Where do I find it?

<http://test-results.appspot.com/dashboards/flakiness_dashboard.html> (**NOTE**:
unmaintained! Try the [Flake
Portal](https://chromium.googlesource.com/infra/infra/+/HEAD/appengine/findit/docs/flake_portal.md)
instead.)

## What does it help do?

*   Identify tests that are flaky on the builders
    *   behavior on trybots is sometimes a little different, use
                <http://chromium-try-flakes.appspot.com> for insight into trybot
                flakiness
*   Identify incorrect (missing or extra) entries in test_expectations
            (tests no longer failing, or failing differently, or newly failing)
*   Compare test expectations and results across platforms, including
            upstream webkit.org to help you identify if you should rebaseline a
            test
*   Identify when a test started failing

## How do I use it?

1.  Pick the test type you are interested in from the "test results"
            dropdown
            ([layout_test_results](http://test-results.appspot.com/dashboards/flakiness_dashboard.html),
            [ui_tests](http://test-results.appspot.com/dashboards/flakiness_dashboard.html#testType=ui_tests),
            etc).
    *   Advanced usage: append #testType=ui_tests to the url
2.  Pick the builder type. This depends on the test type. For example,
            for layout_tests this is the platform: Webkit, Webkit Linux, Webkit
            Mac10.5, Webkit (dbg)(1), etc.
    *   Advanced: append #builder=Webkit%20(dbg)(1) to the url
    *   For layout_test_results, you can also pick which set of bots you
                care about from the top-right.
        *   [Main
                    builders](http://test-results.appspot.com/dashboards/flakiness_dashboard.html)
                    are the WebKit bots on the main waterfall at
                    build.chromium.org. These test Chromium at head and WebKit
                    at the revision in DEPS with test_shell.
        *   [WebKit.org
                    Canary](http://test-results.appspot.com/dashboards/flakiness_dashboard.html#useWebKitCanary=true)
                    are the WebKit bots on the fyi waterfall at
                    build.chromium.org. These test Chromium at head and WebKit
                    at head with test_shell.
        *   [Upstream](http://test-results.appspot.com/dashboards/flakiness_dashboard.html#upstreamWebKit=true)
                    are the Chromium WebKit bots at build.webkit.org. These run
                    Chrome's layout tests with DRT --chrome instead of
                    test_shell.
3.  Now, it depends what you want to do. Here are some examples:
    1.  **Identify flaky tests:**
        This is the default behavior of the dashboard. Just load the page, and
        all tests listed are flaky. For each test, you can see what % of the
        time it fails, and a visual depiction of its pass/fail history. The runs
        are listed newest--&gt;oldest from left to right.

        [<img alt="image"
        src="/developers/testing/flakiness-dashboard/flaky3.jpg">](/developers/testing/flakiness-dashboard/flaky3.jpg)

    2.  **Identify incorrect entries in test_expectations:**

        **[<img alt="image"
        src="/developers/testing/flakiness-dashboard/missingorextra.jpg">](/developers/testing/flakiness-dashboard/missingorextra.jpg)**

        1.  **For any given test type, click on "Show tests with wrong
                    expectations" (or append #showWrongExpectations=true to
                    url).**
        2.  **Examine the "missing" or "extra" columns. The missing
                    column contains entries that should be added to
                    test_expectations and the extra column contains entries that
                    should be removed from test_expectations.**
        3.  **Make sure to look at the actual test results and history
                    to verify that adding/removing expectations is correct.
                    There could always be a bug in the dashboard, or something
                    simple like a revert that the dashboard misses.**
    3.  **Identify when a test started failing**

        **[<img alt="image"
        src="/developers/testing/flakiness-dashboard/newFailingTest.jpg">](/developers/testing/flakiness-dashboard/newFailingTest.jpg)**

        1.  Enter the test name in the input field (or append
                    #tests=testname.html to url).
        2.  You can hit ? to see the legend to the test results color
                    coding.
        3.  Look at the visual results. Find the leftmost green entry.
                    The entry to the left of it will be the first time it
                    failed. Click on that box. Here you can see the webkit
                    revision and the chromium blamelist.
    4.  **Compare test expectations and results**

        **[<img alt="image"
        src="/developers/testing/flakiness-dashboard/seeResults2.jpg">](/developers/testing/flakiness-dashboard/seeResults2.jpg)**

        1.  This is where the dashboard gets really powerful! Enter a
                    test name in the input field (or append #tests=testname.html
                    to url).
        2.  Click "Show results" (or append &showExpectations=true to
                    url). To see bigger results click "Show large thumbnails or
                    append &showLargeExpectations=true).
            *   For the chromium bots, it shows the most recent failure
                        for each bot for each failure type. For example, it may
                        show a crash stack and a text diff for the same bot.
                        That's the crash stack the last time the test crashed
                        and the text diff the last time it failed text diff.
            *   For the upstream bots, it only shows results for a
                        single revision. It defaults to the most recent revision
                        on each bot, but you can click on any of the colored
                        entries and click "Show results for WebKit rXXXXX" to
                        show results for that failure (or append #revision=XXXXX
                        for a given WebKit revision).
        3.  Here you will see
            *   The test itself
            *   The expected results (you can hit ? to see the fallback
                        order for test_expectations). For each expected result,
                        the upper right hand corner lists the platforms it is
                        used for (MAC, LINUX, etc).
            *   The last failing results for each platform, stacktraces
                        (if they exist), diffs of expected vs actual results.

**## Updating test_expectations.txt**

**There is a mode that the flakiness dashboard can operate in that suggests
changes to test_expectations.txt automatically. Users indicate which changes to
include in a JSON report which can be input into a script to update
test_expectations.txt.**

**## **Where is it****

**<http://test-results.appspot.com/dashboards/flakiness_dashboard.html#expectationsUpdate=true>**

**## **How do I use it****

**This page displays the results for a single test at a time. For each test the
user has four options, presented as buttons:**

****include selected** - Include the changes indicated by the checkboxes below
in the report to be generated.**

****previous / next** - Navigate through the list of tests determined to have
incorrect expectations.**

****done** - Generate a JSON report that specifies the changes to
test_expectations.txt.**

**Once the JSON report is generated, copy it to a file on your local disk and
run the
src/webkit/tools/layout_tests/webkitpy/layout_tests/update_expectations_from_dashboard.py
script on it.**

**Note: the script doesn't currently support adding new lines to
test_expectations.txt. There is a patch out for review for this and it should be
submitted soon.**