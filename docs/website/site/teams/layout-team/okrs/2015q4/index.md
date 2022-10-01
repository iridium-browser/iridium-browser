---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/okrs
  - Objectives and Key Results
page_name: 2015q4
title: 2015 Q4
---

List of objectives and key results for 2015 Q4.

## Overview

*   Objective / Key Result Owner(s) Score
*   RAIL (Measuring User Experience)
    *   [Ship CSS containment to beta](#o0r0) leviw 0.60
    *   [Smooth scrolling for wheel and keyboard scrolls in Win/Linux
                enabled on trunk](#o0r1) skobes, ymalik 1.00
    *   [Intersection observer on trunk](#o0r2) szager 0.95
    *   [Integrate font related memory consumption to
                memory-infra](#o0r3) drott 1.00
*   Interop (Interoperability, Compatibility, Conformance)
    *   [Have fullscreen implementation not modify layout tree](#o1r0)
                bokan 0.00
    *   [Have flexbox match latest version of spec](#o1r1) cbiesinger
                0.80
    *   [Work with other vendors to nail down Custom Layout API](#o1r2)
                ikilpatrick 0.90
    *   [Support Blink/Android hyphen design work](#o1r3) szager 1.00
    *   [Replace flexbox tests with W3C ones](#o1r4) cbiesinger 1.00
    *   [Unprefix CSS Writing modes](#o1r5) kojii 0.70
    *   [CSS font-feature-settings](#o1r6) kojii 0.00
*   Code health
    *   [Have root layer scrolling pass all unit tests](#o2r0) skobes,
                szager 0.50
    *   [Have prototype of ruby on top of custom layout \[kojii,
                houdini-dev\]](#o2r1) blink-houdini, kojii 1.00
    *   [Remove simple text](#o2r2) eae, drott 0.90
    *   [Ship improved font fallback to stable](#o2r3) drott 0.75
    *   [Finish wrapping block layout in an API](#o2r4) leviw, pilgrim
                0.70
    *   [Add class level documentation to all key layout classes](#o2r5)
                jchaffraix 1.00
    *   [Feasibility study for replacing flipped flocks writing mode
                implementation](#o2r6) wkorman 0.00
*   Bug Health
    *   [Continue to triage all bugs within one week](#o3r0) eae 1.00
    *   [Fix all (new) P0/P1 within one release](#o3r1) eae 0.80
    *   [Reduce untriaged bug count by 50%](#o3r2) eae, jchaffraix 1.00
    *   [Reduce total bug backlog by 15%](#o3r3) eae 1.00

## Details

### Ship CSS containment to beta

Owner(s):leviw
Score:0.60

Editor’s Draft https://drafts.csswg.org/css-containment/ \[mid quarter\] On
track, intent to implement has been sent. Patch up for review. \[end of
quarter\] CSS and Paint containment implemented and available as experimental
web platform features. We're further along implementation wise than expected
however behind on shipping as CSS contain has yet to ship to beta.

### Smooth scrolling for wheel and keyboard scrolls in Win/Linux enabled on trunk

Owner(s):skobes, ymalik
Score:1.00

http://crbug.com/575 \[mid quarter\] On track, looking good. \[end of quarter\]
Enabled on trunk in time for the M49 branch.

### Intersection observer on trunk

Owner(s):szager
Score:0.95

\[mid quarter\] On track, harder and more time consuming than estimated. \[end
of quarter\] More work and much harder than originally estimated. Implementation
has been ready for over a month but delayed due to difficulties in getting it
reviewed and difficulties around agreeing on a memory model that'll make sense
in a post oil-pan world. Work completed about ten days after the end of quarter.

### Integrate font related memory consumption to memory-infra

Owner(s):drott
Score:1.00

\[mid quarter\] On track. \[end of quarter\] Memory instrumentation added for
key font metrics.

### Have fullscreen implementation not modify layout tree

Owner(s):bokan
Score:0.00

\[mid quarter\] De-staffed. At risk. \[end of quarter\] De-staffed as bokan
wasn't available to layout-dev to work on fullscreen or layout in general. Needs
new owner.

### Have flexbox match latest version of spec

Owner(s):cbiesinger
Score:0.80

\[mid quarter\] On track. \[end of quarter\] ?

### Work with other vendors to nail down Custom Layout API

Owner(s):ikilpatrick
Score:0.90

\[mid quarter\] On track. \[end of quarter\] Several meetings and discussions
with other vendors., Have broad consensus and expect us to be able to spec it in
Sydney.

### Support Blink/Android hyphen design work

Owner(s):szager
Score:1.00

\[mid quarter\] On track. \[end of quarter\] Sufficient support for design work
provided. drott/szager up to speed.

### Replace flexbox tests with W3C ones

Owner(s):cbiesinger
Score:1.00

\[mid quarter\] On track. \[end of quarter\] Done?

### Unprefix CSS Writing modes

Owner(s):kojii
Score:0.70

\[mid quarter\] On track. \[end of quarter\]

### CSS font-feature-settings

Owner(s):kojii
Score:0.00

\[mid quarter\] At risk. \[end of quarter\] ?

### Have root layer scrolling pass all unit tests

Owner(s):skobes, szager
Score:0.50

\[mid quarter\] De-emphasized to focus on intersection observer. Still making
solid progress though. \[end of quarter\] Some progress, but less than expected
due to Stefan being pulled of to do Intersection Observer. Steve fixed at least
one of the unit tests and a few of the layout tests, and added infrastructure
for tracking future progress on the layout tests via flag-specific expectations.

### Have prototype of ruby on top of custom layout \[kojii, houdini-dev\]

Owner(s):blink-houdini, kojii
Score:1.00

\[mid quarter\] Rough prototype exists, needs further work. \[end of quarter\]
Two independent prototypes created using slightly different APIs. Successfully
demonstrated that it is viable to re-implement ruby on top of a custom layout
API. Neither are production ready and both depend on a polyfil, as planned.

### Remove simple text

Owner(s):eae, drott
Score:0.90

\[mid quarter\] Always use complex text enabled on trunk. Plan is to remove code
for M50. \[end of quarter\] Harder than anticipated due to a number of
compatibility and perf issues, most severe being a perf regression in Mac AAT
backend due to n squared font fallback which took awhile to resolve. Patch to
remove simple text uploaded and approved. Will commit as soon as M49 has been
branched. Missing end of Q4 goal by two weeks.

### Ship improved font fallback to stable

Owner(s):drott
Score:0.75

\[mid quarter\] Shipped to beta. On track. \[end of quarter\] In 48 which goes
to stable in Jan 26. Missing end of Q4 goal by one month.

### Finish wrapping block layout in an API

Owner(s):leviw, pilgrim
Score:0.70

\[mid quarter\] At risk. \[end of quarter\] Still not quite done but getting
closer.

### Add class level documentation to all key layout classes

Owner(s):jchaffraix
Score:1.00

\[mid quarter update\] On track. \[end of quarter\] Comprehensive class level
documentation added as header comments to all key layout classes.

### Feasibility study for replacing flipped flocks writing mode implementation

Owner(s):wkorman
Score:0.00

http://crbug.com/535637 \[mid quarter\] Delayed to focus on paint performance.
At risk. \[end of quarter\] De-emphasized to focus on slimming paint which is
higher performance. Will carry over to Q1.

### Continue to triage all bugs within one week

Owner(s):eae
Score:1.00

\[mid quarter\] On track \[end of quarter\] All bugs triaged within one week of
filing (for new bugs) or one week of having the CR-Blink-Layout label added (for
old bugs, working down the backlog).

### Fix all (new) P0/P1 within one release

Owner(s):eae
Score:0.80

\[mid quarter\] On track \[end of quarter\] Still some ambiguity aroud the
definition of a P1 bugs. All P0 and release-blockers fixed within one release.
75% of all P1s, majority of remaining ones should like have been classified as
P2.

### Reduce untriaged bug count by 50%

Owner(s):eae, jchaffraix
Score:1.00

\[mid quarter\] On track to exceed goal. Reduced by 60% thus far. \[end of
quarter\] Reduced by over 70%, far exceeding goal of 50%. Down to 180
unconfirmed/untriaged from 690 at the start of the quarter.

### Reduce total bug backlog by 15%

Owner(s):eae
Score:1.00

\[mid quarter\] On track to exceed goal. Reduced by 16% thus far. \[end of
quarter\] Reduced total open bug count by over 25%, exceeding goal of 15%. Down
to 1688 open bugs from 2310 at the start of the quarter.
