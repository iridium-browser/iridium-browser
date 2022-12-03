---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: perf-regression-detection
title: Regression Detection for Performance Tests
---

This page describes the automated regression detection tool for ChromeOS
performance tests. The tool is located at
src/third_party/autotest/files/client/common_lib/perf_expectations.

## 1. Introduction

**perf_expectations.json** is the place where perf expectations are specified.
Each expectation is key-value pair. The key is formated as
"BOARD_NAME/TEST_NAME/TRACE_NAME", for example,
"lumpy/desktopui_PyAutoPerfTests/FPS_ScrollTextPage". The value is a dictionary
like: {"improve":"50.32", "regress": "43.23", "better": "lower"}

"better": the value for this field is "lower" or "higher", indicating whether

this is a "higher-is-better" or "lower-is-better" case.

"improve": for "higher-is-better" case, if the actual perf value is higher than
it, improvement is identified.

for "lower-is-better" case, if the actual perf value is lower than it,
improvement is identified.

"regress": for "higher-is-better" case, if the actual perf value is lower than
it, regression is identified.

for "lower-is-better" case, if the actual perf value is higher than it,
regression is identified.

**expectation_checker.py** provides APIs that can be called at the end of a
performance test. The checker compares actual performance values against
expectations that are specified in perf_expectations.json.

## 2. How to decide the baseline

Currently, we need to manually figure out the values of "improve" and "regress"
for each expectation. One strategy could be reasoning the baseline from a
recent-look-good performance result. For example, for
"lumpy/desktopui_PyAutoPerfTests/FPS_ScrollTextPage",

the most recent result is 353.17. We decide to allow 15% tolerance. Then we can
calculate the "improve" value as 353.17 \* (1 + 0.15) = 406.1455. Similarly, the
"regress" value can be calculated as 353.17 \* (1 - 0.15) = 300.1945.

## 3. How to modify a test to detect regression.

At the end of a performance test, initialize a perf_expectation_checker and then
call perf_expectation_checker.compare_multiple_traces(...) to compare actual
performance values with pre-defined expectations. By defaults,
perf_expectations.json under the same folder of this file will be used.

Example:

At the end of
'src/third_party/autotest/files/client/site_test/desktopui_PyAutoPerfTests/desktopui_PyAutoPerfTests.py:run_once()',
you can add the following code to detect regression/improvement:

` checker = expectation_checker.perf_expectation_checker(`

` "desktopui_PyAutoPerfTests")`

` result = checker.compare_multiple_traces(perf_dict)`

` if result['regress']:`

` raise error.TestFail('Pyauto perf tests regression detected: %s' %`

` result['regress'])`

` if result['improve']:`

` raise error.TestWarn('Pyauto perf tests improvement detected: %s' %`

` result['improve'])`

## 4. Notes

The idea of this tool is adopted from the automated regression detection tool
for Chrome. Related information can be found at:

http://www.chromium.org/developers/tree-sheriffs/perf-sheriffs

http://src.chromium.org/viewvc/chrome/trunk/src/tools/perf_expectations

Ideally, the expectation checker should work the same way as the tool for
Chrome. It currently has fewer features. In the future, like what Chrome team
does, we can add a script, e.g. "make_expectations.py" which automatically
reasons the thresholds values ("improve" and "regress") and update
perf_expectations.json. We will need to add some additional fields in a
expectation so that it will look like the following:

`"lumpy/desktopui_PyAutoPerfTests/FPS_ScrollTextPage": `

` {"builda": "R23-2804.0.0", "buildb": "R23-2811.00", "improve": "50.32",
"regress": "43.23",`

` "better": "lower", "tolerance":"0.05", "sha1": "xd3234sdf32s"}`

When we want to adjust the baseline, we will first figure out a build number
range (builda, buildb) for which the performance results look good. Then we will
run make_expectaions.py to automatically update the values of "improve" and
"regress". Please refer to

http://src.chromium.org/viewvc/chrome/trunk/src/tools/perf_expectations/make_expectations.py
to see how the idea is currently working for Chrome.
