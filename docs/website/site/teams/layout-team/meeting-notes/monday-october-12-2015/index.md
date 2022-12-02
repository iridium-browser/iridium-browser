---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-october-12-2015
title: Monday, October 12, 2015
---

Updates since last meeting (on Monday, October 5th):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Misc root layer scrolling fixes. (skobes)
- Plan to start working on smooth scrolling this week. (skobes)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Fixed height calculation to consider tallest column.
- FIxed handling of minimum for fill available.
CSS Grid Layout (svillar, jfernandez, rego)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No updates since last week.
Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]
- Reworked column rebalancing.
- Fix handling for block pagination struts.
- Changes to break handling.
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- API fixes fot SVG text chunk handling. (pilgrim)
Text (eae, drott, kojii)
- Fixed further selection painting bugs. (eae)
- Fixed measurement for RTL list-markers where we reversed the
text twice causing incorrect ordering and metrics. (eae)
- Continued work on shaper driven segmentation, will start to
land patches upstream this week. (drott)
- Unprefixed CSS Writing Modes landed, working on some follow up
fixes. (kojii)
- Trying to better integrate CSS text code path with SVG text code
path for vertical flow. (kojii)
- Cleaning up tests in CSS WG and specs for coming CR. (kojii)
Misc:
- drougan started on his first bug, getting familiar with the
Blink code base.
- drott hit size and encoding limits in Skia test infra, working
with them to resolve it.
- Change in review for list marker spacing. (wkorman)
- Lots of tracing, test case fiddling, and reading of code for
WebViewImpl::layout xploring whether there are further layout/paint
optimizations we can make. (wkorman)
- Continuing work on documentation (deep dived into reflection to
understand the design - <https://codereview.chromium.org/1391943005> +
more PaintLayer documentation). (jchaffraix)
- Mostly done documenting important objects (hopefully I could finish
these this week), after that is deep diving into features (transform,
paint invalidation, baselines...). (jchaffraix)
Logistics:
- eae & leviw in Helsinki to work with drott until Thursday.
- cbisinger on vacation this Monday.
- drougan back from recruiting trip.
