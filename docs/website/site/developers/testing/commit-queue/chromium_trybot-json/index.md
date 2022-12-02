---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
- - /developers/testing/commit-queue
  - Chromium Commit Queue
page_name: chromium_trybot-json
title: Analyze step(s)
---

The analyze step is used to determine if a compile is necessary for the current
platform and if a compile is necessary the set of targets (tests) to execute. By
default all try bots run the analyze step. To make a bot not run the analyze
step add the name of the bot to the non_filter_builders key (non_filter_builders
is expected to be an array of strings) in the corresponding trybot file. For
example, to opt out the trybot win_chromium_rel_swarming add
win_chromium_rel_swarming to the key non_filter_builders in the file
[chromium_trybot.json](http://src.chromium.org/viewvc/chrome/trunk/src/testing/buildbot/chromium_trybot.json).

The analyze step does it work by running a custom gyp generator. In order for
the analyze step to produce reliable results all dependencies need to be
correctly listed. This currently isn't the case for a set of files. For example,
data files for tests are not listed as dependencies. Such dependencies are
handled by adding them to the exclusions list of the file
[trybot_analyze_config.json](http://src.chromium.org/viewvc/chrome/trunk/src/testing/buildbot/trybot_analyze_config.json)
.

Bots that further filter out the set of tests to run based on the changed files
is currently opt in. Opt in is done by adding the name of the bot to
filter_tests_builders in the appropriate config file for the bot.
