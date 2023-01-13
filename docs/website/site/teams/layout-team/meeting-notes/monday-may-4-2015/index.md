---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-may-4-2015
title: Monday, May 4, 2015
---

Performance Tracking (benjhayden)

- Trace viewer/rail perf work.

- Layout Analyzer powered by UMA.

Scrolling (skobes) \[[crbug.com/417782](http://crbug.com/417782)\]

- Improved unite tests for scrolling.

- Removed m_inProgrammaticScroll scroll flag from FrameView.

- Removed m_viewportLayoutObject from FrameView.

Line Boxes (szager) \[[crbug.com/321237](http://crbug.com/321237)\]

- Performance data looks a lot better, some micro benchmarks still show

a slight regression but within error margin. Plan is to try to land

this week.

Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- Discussions around absolut positioning within flexbox.

- Updated handling of min-width/min-height.

List marker refactoring (dsinclair)
\[[crbug.com/370461](http://crbug.com/370461)\]

- Found solution for subtree modification problem, CL up for review.

(dsinclair)

- Change to move layout tree modification from layout walk to

NotifyChange. (dsinclair)

Misc Warden (dsinclair, pilgrim)

- Moved mustInvalidateBackgroundOrBorderPaintOnHeightChange from

LayoutObject to LayoutBox and made it private. (pilgrim)

- Made updateShapeImage, updateFillImages private. (pilgrim)

- Renderer to LayoutObject rename almost done, only editing and paint

remain. Yay! (dsinclair)

Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]

- All known fuzzers fixed.

- New multicol implementation landed today (r194883).

Layout refactoring (esprehn, ikilpatrick, cbiesinger, ojan, leviw)

- Meetings and discussion contined last week and this week. Plan is to

have one or more concrete proposals by the end of BlinkOn.

Logistics

- Most of the team in Sydney next week for BlinkOn 4, many travel down

this week.

- No meeting next week (due to BlinkOn), send updates (if any) to eae.
