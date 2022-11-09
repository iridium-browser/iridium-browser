---
breadcrumbs:
- - /developers
  - For Developers
page_name: testing
title: Testing and infrastructure
---

[TOC]

## Highlighted Child pages

*   [Running tests at home](/developers/testing/running-tests)
*   [Chromium build
            infrastructure](/developers/testing/chromium-build-infrastructure)
*   [Fixing Flaky Tests](/developers/testing/fixing-flaky-tests)
*   [Blink Web Tests](/developers/testing/webkit-layout-tests)
*   [Try Server usage](/developers/testing/try-server-usage)
*   [Commit Queue](/developers/testing/commit-queue)
*   [Isolated testing](/developers/testing/isolated-testing) and
            Swarming
*   ==[GPU Testing](/developers/testing/gpu-testing)==
*   [WebGL Conformance
            Tests](/developers/testing/webgl-conformance-tests)
*   [Builder Annotations](/system/errors/NodeNotFound)
*   [WebUI browser tests](/Home/domui-testing/webui-browser_tests)
*   [chrome.test.\* APIs](/developers/testing/chrome-test-apis)

## Overview

Chromium development is heavily test driven. In order to maintain a rapid rate
of development across multiple platforms and an ever increasing set of features,
it is imperative that test suites be updated, maintained, executed, and evolved.
Any new features should have test coverage and in addition most changes should
have test coverage. As a contributor to Chrome you are expected to write quality
tests that provide ample code coverage. As a reviewer you are expected to ask
for tests. **The** [**Chromium Continuous Integration
system**](/developers/testing/chromium-build-infrastructure) **is employed to
run these tests 24x7**.

## Expectations

Developers contributing code are expected to run all tests. This is not
typically feasible on a single workstation, so [try
servers](/developers/testing/try-server-usage) are used. When contributing code,
consider whether your change has enough testing. If it is a new feature or
module, it should almost certainly be accompanied by tests.

## Test Development Infrastructure

To assist with building tests, several pieces of infrastructure exist. Here are
some tools you might find useful:

*   chrome/test/automation - Chromium includes a mechanism for driving
            the browser through automation. This is primarily used with the UI
            tests.
*   [gtest](http://code.google.com/p/googletest/) - Google Test is
            Chromium's C++ test harness.
*   **image_diff** - A mechanism for comparing bitmaps.

## Inducing a crash

This can be useful to test breakpad:

*   about:crash - will cause a renderer crash.
*   about:inducebrowsercrashforrealz - will cause a browser crash.

## Subpages

{% subpages collections.all %}
