---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-june-1-2015
title: Monday, June 1, 2015
---

Updates since last meeting (on Tuesday, May 25th):

Performance Tracking (benjhayden)

- Continuing to focus on trace viewer overall rather than limited to

layout performance.

- Visualizing layout in trace viewer.

- Interaction records.

Scrolling (skobes) \[[crbug.com/417782](http://crbug.com/417782)\]

- Figured out a way to run the WebFrameTest unit tests with booth root

level scrolling turned on and off. Going through tests to catch

regressions.

- Learned more about page scale factor, turns out that the initial scale

factor cause the FrameView to be a different size.

Line Boxes (szager) \[[crbug.com/321237](http://crbug.com/321237)\]

- Landed over the weekend, re-baselines almost done. Yay!

Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- Updated min-size handling to match latest version of spec.

- Sycned up with Microsoft and Mozilla about their flexbox plans to

ensure compatibility across browsers.

Menu list refactoring (dsinclair)
\[[crbug.com/370462](http://crbug.com/370462)\]

- Done!

Fullscreen (dsinclair) \[[crbug.com/370459](http://crbug.com/370459)\]

- Landed fix to persist plugins over re-attach. Subsequently reverted

due to browser_test failures. Looking into failures and plan to

re-land this week.

Misc Warden (dsinclair, pilgrim)

- Containing work to slim-dopwn LayoutObject. (pilgrim)

Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]

- Will start to delete the old multicol code this week. It's no longer

possible to enable it and no major regressions have been found.

Text

- Continuing work on complex text performance. (eae)

- Experimenting with only inheriting inheritable properties into text

nodes, not yet convinced that it affects performance noticeably, but

it WILL increase memory usage, since we create a new ComputedStyle

object for each text node (instead of sharing with the parent).

Each text node that has the same parent could easily share the same

ComputedStyle object, though, but we'd still be going to increase

memory usage. (mstensho)

- Fixed justification crash issue. (kojii)

- Investigating orthogonal writing modes issues. (kojii)

- Unicode variation selectors support in progress. (kojii)

Importing Test Suites

- Working on importing CSS Writing Modes test suites. (kojii)

- Imported HTML tests. (tkent)

Removing DeprecatedPaintLayer \[[crbug.com/260898](http://crbug.com/260898)\]
(chadarmstrong)

- Started looking at moving hit testing from DeprecatedPaintLayer to

Box model object.

- jchaffraix helping chadarmstrong get up to speed.

Add API for layout \[[crbug.com/495288](http://crbug.com/495288)\] (leviw)

- Meeting last where where the plans for the grand layout refactoring

was discussed. We agreed that the first step is to define (and over

time refine) an API for interacting with layout. Meeting notes at

https://www.chromium.org/blink/layout-team/meeting-notes/may-28-2015

- Wrote up a document outlining the plan, sent out to blink-dev.

https://docs.google.com/document/d/1qc5Ni-TfCyvTi6DWBQQ_S_MWJlViJ-ikMEr1FSL0hRc

Multipart images \[[crbug.com/308999](http://crbug.com/308999)\] (japhet)

- Implemented support for multipart image documents.

Misc Performance

- Looked into SVG spinner performance and found layout bug where SVG

paths where regenerated unnecessarily. Fixing the bug should give us

at least a 50% perf improvement. (esprehn)

Logistics

- cbiesinger gardener Friday/Monday.

- Mid quarter OKR check-in on Thursday at 10am PST.
