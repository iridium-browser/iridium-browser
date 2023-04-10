---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: adding-tests-to-the-main-waterfall
title: Adding new tests to the Main Chromium Waterfall
---

The main Chromium waterfall
(<https://ci.chromium.org/p/chromium/g/main/console>) is policed by sheriffs to
keep it green as possible. When adding a new test to these bots, it's important
that the tests be free from flakiness and that the [Try
Server](/system/errors/NodeNotFound) and CQ provide developers coverage for the
test. See the design of the [Chromium Commit
Queue](/developers/testing/commit-queue) for details on how this works. Please
coordinate with the Infra team and troopers while adding these tests.

### Phase One - FYI

Make sure your test target is in one of the builder targets in
[all.gyp](https://chromium.googlesource.com/chromium/chromium/+/trunk/build/all.gyp).
Then add the test entries (GTestTestStep and BuildrunnerGTest) to
[chromium_factory](https://chromium.googlesource.com/chromium/tools/build/+/HEAD/scripts/master/factory/chromium_factory.py).
Finally put the test on the
[chromium.fyi](https://chromium.googlesource.com/chromium/tools/build/+/HEAD/masters/master.chromium.fyi)
[master.cfg](https://chromium.googlesource.com/chromium/tools/build/+/HEAD/masters/master.chromium.fyi/master.cfg)
for the platforms that make sense for the test. Watch these tests and eliminate
any flakiness that arises.

### Phase Two - Main Chromium Waterfall / TryServer

When the test has been declared stable, it's time to land it on the main
Chromium waterfall. As these parts depend on each other, try to land all the
following simultaneously (with a trooper's help):

1.  Add the test to
            [chromium_trybot.json](https://chromium.googlesource.com/chromium/src/+/HEAD/testing/buildbot/chromium_trybot.json).
2.  Add the test to the appropriate bots in
            [src/testing/buildbot/](https://chromium.googlesource.com/chromium/src/+/HEAD/testing/buildbot/).
3.  Add your test to
            [lkgr_finder.py](https://chromium.googlesource.com/chromium/tools/build/+/HEAD/scripts/tools/lkgr_finder.py)
            and CQ's
            [projects.py](https://chromium.googlesource.com/chromium/tools/commit-queue/+/HEAD/projects.py).

Most bots on the main Chromium waterfall have been converted to recipes, which
have the simplified flow listed above. If you are adding tests to a non-recipe
bot, contact infra-dev@.
