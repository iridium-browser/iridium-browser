---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-march-9-2015
title: Monday, March 9, 2015
---

Performance Tracking (benjhayden)

- Continuing work on exporting counters.

- Adding layout metrics to the blink performance tests.

- Working on simplifying the layout metrics implementation.

Scrolling (skobes) \[crbug.com/417782\]

- Ran into an issue with paint invalidation rects that block the later

scroll implementation.

Rename Rendering -&gt; Layout (dsinclair) \[crbug.com/450612\]

- Renamed RenderTreeBuilder to LayoutTreeBuilder.

- Renamed various methods; renderer() to layoutObject(), rendererIs\* to

layoutObjectIs\*, and createRenderer\* to createLayoutObject\*.

Measure API (jchaffraix)

- Spent much of last week discussing the measurement API with

stakeholders.

- Trying to determine whether there is a use case for the measure API in

isolation or if it only makes sense as part of as part of custom

layout. If you have a use case reach out to jchaffraix.

- No doubt that there is need for a text metrics API.

Line Boxes (hartmanng, szager) \[crbug.com/321237\]

- Dealing with layout test failures, five-ish tests needs debugging on

linux, looking into windows at the moment. Mostly false positives,

some actual failures. (hartmanng)

- Dealing with layout test failures on Mac. (szager)

- Spent a lot of time fixing broken tests, eae argued this might not be

the best use of my time. Will mark broken tests as broken going

forward to unblock the conversion work. (szager)

Flexbox (cbiesinger) \[crbug.com/426898\]

- Flexbox work is taking a bit longer than expected, the win-width auto

change took all of last week. Still dealing with test failures.

- Two week estimate was a bit too optimistic.

Isolate core/fetch (japhet) \[crbug.com/458222\]

- Continuing on isolating the fetch directory from the rest of core.

Blink componentization (pilgrim) \[crbug.com/428284\]

- Working on modularizing various things in core, storage is now

modules/storage.

- Next up is (blocking on fetch) is core/xml.

Text (kojii, wjmaclean, eae)

- Continuing work on text iterator. Trying to modularize it to make it

easier to understand. Many misnamed and confusing methods, tyring to

rationalize that. (wjmaclean)

- Line breaker optimizations based on profiling data resulted in ~10%

speedup for line-layout and text perf tests. (eae)

- FontMetrics::ascent(), descent(), and height() optimizations resulted

in up to 15% speedup for complex text perf tests, ~3-5% for simple

text. (kojii)

Misc

- Helping jochen on git repository merge. Their strategy is sound and

they are doing the right thing. Will help with infra work as needed.

(szager)

- Created minimized tests case and tracked down root cause for starter

bug, was an invalidation bug. (wkroman)

Logistics

- cbiesinger in SF next week.

- eae out Tue-Thu this week and Mon-Tue next week.

- wjmaclean out Thu-Fri this week and all next week.

- paulmeyer won't be working on layout for the next couple of weeks.
