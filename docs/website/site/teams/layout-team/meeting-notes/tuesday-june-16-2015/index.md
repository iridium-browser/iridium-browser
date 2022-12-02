---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: tuesday-june-16-2015
title: Tuesday, June 16, 2015
---

We had Sergio Villar (svillar@igalia.com) from Igalia join us this week.

Sergio is a long term contributor working primarily on CSS Grid Layout.

Updates since last meeting (on Monday, June 8th):

Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]

- Fixed hit testing by making the LayoutView absolute-positioned.

(skobes) \[crbug.com/495856\]

- working on HitTestRequest::IgnoreClipping
\[[crbug.com/499053](http://crbug.com/499053)\]

and the zoomed-out viewport issue.

CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- Continuing to update to match spec, progress according to plan.

CSS Grid Layout (svillar) \[[crbug.com/79180](http://crbug.com/79180)\]

- Fixed a regression caused by the recent migration from parenthesis

to brackets.

- Working on adding a new step to the track sizing algorithm.

- We need to find someone to help with grid layout reviews to unblock

further work.

Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]

- Fixing multicol fuzzers. Looks like the fuzzer storm has finally

calmed down this week.

Isolate core/fetch (japhet) \[[crbug.com/458222](http://crbug.com/458222)\]

- Improved API between core/fetch and the rest of core.

- A bunch of small crash/use-after-free fixes.

Text (eae, kojii)

- Fixed a long-standing issue in positioned objects layout ordering.

(kojii)

- Unicode variation selectors support in progress, waiting for reviews.

(kojii)

- Investigating orthogonal writing modes issues. (kojii)

- Investigating abspos issues in vertical flow. (kojii)

- Investigating/triaging failures of imported CSS Writing Modes tests,

also working with external contributors to fix in upstream. (kojii)

- Working on CSS Writing Modes spec issues, primarily to simplify rarely

used features. (kojii)

- Working on shaping word-by-word, initial prototype shows very

promising results. (eae)

Misc

- Added option to dump line box tree when dumping layout tree. (szager)

Layout Bugs:

We currently have 1592 open bugs for the Cr-Layout label.

- Untriaged: 475

- Priority 0: 0

- Priority 1: 135

- Priority 2: 958

- Priority 3: 19

We need to get the number of bugs down so from now on each weekly update

will include the number of bugs for each category and the delta from the

week before.

Logistics:

- skobes OOO 6/17-6/22

- drott getting up to speed.

- szager OOO next week.

- drott OOO Thu-Fri this week.

- eae gardener Wed-Thu next week.
