---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/okrs
  - Objectives and Key Results
page_name: 2015q2
title: 2015 Q2
---

List of objectives and key results for the second quarter of 2015.

## Overview

*   Objective / Key Result Owner(s) Status
*   Gain insight into real world layout performance
    *   [Have a report characterizing layout performance for all layout
                scenarios.](#o0r0) \[benjhayden\] 0.0
*   Improve high-level layout design
    *   [Have design document for grand node measure
                refactoring.](#o1r0) \[jchaffraix\] 0.3
    *   [List markers not modifying layout tree during layout (in
                dev).](#o1r1) \[dsinclair\] 1.0
    *   [Initial implementation of menu item not modifying layout tree
                during layout (in canary).](#o1r2) \[dsinclair\] 1.0
*   Improve capabilities of the web platform
    *   [Have an experimental out-of-tree node measure API.](#o2r0)
                \[jchaffraix\] 0.0
    *   [Update flexbox implementation to match latest version of
                specification.](#o2r1) \[cbiesinger\] 0.8
    *   [Improve CJK vertical text support \[redacted, internal
                metric\].](#o2r2) \[kojii\] 0.9
*   Rationalize text rendering
    *   [Make complex text as fast as simple text.](#o3r0) \[eae\] 1.0
    *   [Remove the simple text code path.](#o3r1) \[eae\] 0.0
    *   [Build the plan to support updated Unicode Bidirectional
                Algorithm.](#o3r2) \[kojii\] 0.2
    *   [Raise the “Passed” rate of CSS Writing Modes Level 3 to 85%
                including new tests.](#o3r3) \[kojii\] 1.0
*   Improve code health
    *   [Move line layout to LayoutUnit.](#o4r0) \[szager\] 1.0
    *   [Finish root layer scrolling.](#o4r1) \[skobes\] 0.7
    *   [Ensure that bugs get automatically filed for clusterfuzz
                asserts.](#o4r2) \[cbiesinger\] 1.0

## Details

### Have a report characterizing layout performance for all layout scenarios.

Owner(s):benjhayden
Status:0.0

Abandoned to focus on telemetry. Ben moved to telemetry team to ensure that the
larger blink team has access to the performance data we need to ensure that RAIL
is a success.

### Have design document for grand node measure refactoring.

Owner(s):jchaffraix (dsinclair, cbiesinger, skobes, eae)
Status:0.3

Superseded by larger layout refactoring proposal (“moose”). Part of the
refactoring work could still be relevant/applicable in isolation and has been
broken out into specific design documents; unrooted layout, removing deprecated
paint layer.

### List markers not modifying layout tree during layout (in dev).

Owner(s):dsinclair
Status:1.0

Done

### Initial implementation of menu item not modifying layout tree during layout (in canary).

Owner(s):dsinclair
Status:1.0

Done.

### Have an experimental out-of-tree node measure API.

Owner(s):jchaffraix
Status:0.0

Abandoned due to the high implementation complexity, high maintenance costs and
relatively low level of enthusiasms from prospective clients. Does not
necessarily make sense in isolation and should be considered as a part of a
bigger layout refactoring or work to support custom layout. Most prospective use
cases can be satisfied with a text metrics API.

### Update flexbox implementation to match latest version of specification.

Owner(s):cbiesinger
Status:0.8

Mostly done, will drag into Q3. Initial estimate was a little bit too
optimistic.

### Improve CJK vertical text support \[redacted, internal metric\].

Owner(s):kojii
Status:0.9

We unblocked all existing blockers. The metric is stalled for other problems,
and one topic is still under discussions, but Blink is no longer the blocker at
this point and the failure to meet the goal is outside the control of the blink
team.

### Make complex text as fast as simple text.

Owner(s):eae (szager, kojii, behdad)
Status:1.0

Done. Complex path speed up by between 3.5x and 30x, as fast or faster than
simple path on benchmarks.

### Remove the simple text code path.

Owner(s):eae
Status:0.0

Was gated on making complex text as fast as simple. Was a bit too ambitious
perhaps, should be doable early Q3.

### Build the plan to support updated Unicode Bidirectional Algorithm

Owner(s):kojii
Status:0.2

Work under way, will carry over into Q3.

### Raise the “Passed” rate of CSS Writing Modes Level 3 to 85% including new tests

Owner(s):kojii
Status:1.0

86% as of early June.

### Move line layout to LayoutUnit.

Owner(s):szager (dsinclair)
Status:1.0

Done!

### Finish root layer scrolling.

Owner(s):skobes
Status:0.7

Fixed various things such as anchor scrolling, quirks mode, hit testing. Partial
coverage of unit tests with RLS enabled. Some issues remain with pinch viewport,
custom scrollbars, etc. Aiming to finish in Q3.

### Ensure that bugs get automatically filed for clusterfuzz asserts.

Owner(s):cbiesinger
Status:1.0

Done!
