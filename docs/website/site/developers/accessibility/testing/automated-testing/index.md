---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/accessibility
  - Accessibility for Chromium Developers
- - /developers/accessibility/testing
  - Accessibility Testing
page_name: automated-testing
title: Accessibility Automated Testing
---

Accessibility is tested in several different places. This page documents some of
the most widely-used systems.

## Unit testing

This is the most basic accessibility testing. It is designed to test methods of
a particular accessibility class. To build the unit testsuite do:

```none
autoninja -C out/Default accessibility_unittests
```

To run:

```none
out/Default/accessibility_unittests
```

You can use `--gtest_filter` option to specify which test to run. For example:

```none
out/Default/accessibility_unittests --gtest_filter="AXInspectScenario*" 
```

### Add new tests

The unit test file should be postfixed by `_unittest` and placed next to a
testing file. For example, for `ax_inspect_scenario.h` file it will be
[`ax_inspect_scenario_unittest.cc`](https://source.chromium.org/chromium/chromium/src/+/main:ui/accessibility/platform/inspect/ax_inspect_scenario_unittest.cc).
You should also add the test file to under
[`accessibility_unittests`](https://source.chromium.org/chromium/chromium/src/+/main:ui/accessibility/BUILD.gn;l=222)
block.

## Browser testing

If you need a test to run it in a browser, then you should use browser
testsuite. It consist of `browser_tests` to test browser UI and
`content_browsertests` to test a web document accessibility. To build the
browser tests you do:

```none
autoninja -C out/Default browser_tests
```

or

```none
autoninja -C out/Default content_browsertests
```

To run:

```none
out/Default/browser_tests
```

or

```none
out/Default/content_browsertests
```

Use `--gtest_filter `option to specify which test to run.

Test files shall be postfixed by `_browsertest` and placed next to a tested
file. You can refer to [this
test](https://source.chromium.org/chromium/chromium/src/+/main:content/browser/accessibility/accessibility_tree_formatter_mac_browsertest.mm)
as an example. Also the test file should be added into corresponding block in
`Build.gn`, for example, under
[content_browsertests](https://source.chromium.org/chromium/chromium/src/+/main:content/test/BUILD.gn?q=%22test(%22content_browsertests%22)%20%7B%22&ss=chromium%2Fchromium%2Fsrc).

### Dump Accessibility Testing

This testsuite is designed to test platform accessibility APIs. It is part of
`browser_tests` and `content_browsertests` testsuites. Please refer to [the
docs](https://source.chromium.org/chromium/chromium/src/+/main:content/test/data/accessibility/readme.md)
for details.

## Blink Layout Tests

Blink layout tests are another important way accessibility is tested. There are
a number of accessibility tests in this directory
`third_party/blink/web_tests/accessibility/`. These tests are critically
important for testing Blink implementation of accessibility at a low-level,
including parsing html and sending notifications.

Here's a sample command line for running the tests:

```none
ninja -C out/Debug blink_tests
python third_party/blink/tools/run_web_tests.py --no-show-results --no-retry-failures --results-directory=results accessibility/
```

To run just one test:

```none
out/Release/content_shell --run-web-tests third_party/blink/web_tests/accessibility/test_name.html
```

To run just one test, on Mac OS X:

```none
out/Release/Content\ Shell.app/Contents/MacOS/Content\ Shell --run-web-tests third_party/blink/web_tests/accessibility/test_name.html
```

## Audit testing

Another testing type is accessibility audit testing aslo known as
[axe-core](/developers/accessibility/testing/axe-core).
