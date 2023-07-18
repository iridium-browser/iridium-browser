---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: sheriffing
title: Handling Blink failures
---

AKA gardening or sheriffing. See also [the documentation of the Test
Expectations
files](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_test_expectations.md),
the [generic sheriffing docs](/developers/tree-sheriffs), and the
[Chromium-specific sheriffing
docs](/developers/tree-sheriffs/sheriff-details-chromium).

## Bots

Chromium has many kinds of bots which run different kinds of builds and tests.
The [chromium.webkit](https://build.chromium.org/p/chromium.webkit/waterfall)
builders run the Blink-specific tests.

You can monitor them using
[Sheriff-o-matic](https://sheriff-o-matic.appspot.com/) , just like the
non-Blink bots.

Even among the WebKit bots, there are several different kinds of bots:

*   ["Layout"
            bots](https://build.chromium.org/p/chromium.webkit/waterfall?category=layout&failures_only=true)
            and [non-Oilpan
            bots](https://build.chromium.org/p/chromium.webkit/waterfall?builder=WebKit+Win+non-Oilpan&builder=WebKit+Win+non-Oilpan+(dbg)&builder=WebKit+Mac+non-Oilpan&builder=WebKit+Mac+non-Oilpan+(dbg)&builder=WebKit+Linux+non-Oilpan&builder=WebKit+Linux+non-Oilpan+(dbg))
    *   This is where most of the action is, because these bots run
                Blink's many test suites. The bots are called "layout" bots
                because one of the biggest test suites "Web Tests" was called
                LayoutTests, which is found in third_party/blink/web_tests and
                run as part of the webkit_tests step on these bots. Web tests
                can have different expected results on different platforms. To
                avoiding having to store a complete set of results for each
                platform, most platforms "fall back" to the results used by
                other platforms if they don't have specific results. Here's a
                [diagram of the expected results fallback
                graph](https://docs.google.com/a/chromium.org/drawings/d/1KBTihR80H42GB0be0qK2CyM-pPUoMdnHqYaOsNK85vI/edit).
*   Leak bots
    *   These are just like the layout bots, except that if you need to
                suppress a failure, use the
                [web_tests/LeakExpectations](https://cs.chromium.org/chromium/src/third_party/blink/web_tests/LeakExpectations)
                file instead. You can find some additional information in
                [Gardening Leak
                Bots](https://docs.google.com/a/google.com/document/d/11C7zFNKydrorESnE6Nbq98QNmKRMrhSwVMGxkx4fiZM/edit#heading=h.26irfde6145p)
*   [ASAN
            bot](http://build.chromium.org/p/chromium.webkit/waterfall?show=WebKit%20Linux%20ASAN)
    *   This also runs tests, but generally speaking we only care about
                memory failures on that bot. You can suppress ASAN-specific
                failures using the
                [web_tests/ASANExpectations](https://cs.chromium.org/chromium/src/third_party/blink/web_tests/ASANExpectations)
                file.
*   [MSAN
            bot](https://build.chromium.org/p/chromium.webkit/builders/WebKit%20Linux%20MSAN)
    *   Same deal as the ASAN bot, but catches a different class of
                failures. You can suppress MSAN-specific failures using the
                [web_tests/MSANExpectations](https://cs.chromium.org/chromium/src/third_party/blink/web_tests/MSANExpectations)
                file.

## Tools

Generally speaking, developers are not supposed to land changes that knowingly
break the bots (and the try jobs and the commit queue are supposed to catch
failures ahead of time). However, sometimes things slip through ...

### Sheriff-O-Matic

[Sheriff-O-Matic](http://sheriff-o-matic.appspot.com/) is a tool that watches
the builders and clusters test failures with the changes that might have caused
the failures. The tool also lets you examine the failures. [There is more
documentation here](/developers/tree-sheriffs/sheriff-o-matic).

### Rolling back patches

To roll back patches, you can use either git revert or
[drover](/developers/how-tos/drover). You can also use "Revert" button on
Gerrit.

### Flakiness dashboard

The [flakiness
dashboard](http://test-results.appspot.com/dashboards/flakiness_dashboard.html#useWebKitCanary=true)
is a tool for understanding a test’s behavior over time. Originally designed for
managing flaky tests, the dashboard shows a timeline view of the test’s behavior
over time. The tool may be overwhelming at first, but [the
documentation](/developers/testing/flakiness-dashboard) should help.

### Contacting patch authors

Comment on the CL or send an email to contact the author. It is patch author's
responsibility to reply promptly to your query.

**Test Directories**

The web platform team has a large number of tests that are flaky, ignored, or
unmaintained. We are in the process of finding [teams to monitor test
directories](https://docs.google.com/spreadsheets/d/1c7O3fJ7aTY92vB5Dfyi_x2VYu4eFdeEeNTys6ECOviQ/edit?ts=57112a09#gid=0),
so that we can track these test issues better. Please note that this should not
be an individual, but a team. If you have ideas/guesses about some of these
directories, please reach out to the team and update the sheet. This is the
first step, and the long term plan is to have this information on a
dashboard/tool somewhere. Watch this space for updates!
