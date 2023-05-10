---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/moblab
  - MobLab
page_name: view-test-result-vis-wmatrix
title: View Test Result via WMatrix
---

\*\*\*DEPRECATED, PLEASE VISIT
https://www.chromium.org/chromium-os/testing/moblab \*\*\*
(20170714)

Starting with M52, Moblab AFE UI will have a link to WMatrix, which will present
a better test result viewing UI for Moblab users.

![](/chromium-os/testing/moblab/view-test-result-vis-wmatrix/Selection_980.png)

The Test Results UI might look a little overwhelming. It includes all tests that
currently existing in Chrome OS autotest.git. But might not be relevant to
Partners initially. We recommend to start with 'Suite List" in the "Quick links"
section.

![](/chromium-os/testing/moblab/view-test-result-vis-wmatrix/Selection_982.png)

Depends on the type of test suites you have run on your moblab unit, the type of
suite in the subsequent screens may differ. You may have faft, storage qual,
bvt, cts tests show up. Here is one example from our faft tests. From here you
could drill down further to see aggregated test results.

![](/chromium-os/testing/moblab/view-test-result-vis-wmatrix/Selection_978.png)

Aggregated test results of one particular suites across build and devices. Here
is a simple example of one device and one build test results. If you have your
mouse hover over the "red" section on the right hand side, you will see all the
failed tests associated with runs on that build (R52-8350.69.0). Clicking on the
red "1" in the screen below will lead you to the detailed failure screen so you
can investigate further.

NOTE: If you are running a build that is old, you might see an empty UI when you
click on the Suite Name from screen shown earlier. One work around is to add
"&days_back=15" (or whatever days you know that are required to get to your
build comparing to today's build) to see the result. We are aware of this flaw
and working on a fix for next release.

[](/chromium-os/testing/moblab/view-test-result-vis-wmatrix/Selection_977.png)
