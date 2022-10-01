---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-april-27-2015
title: Monday, April 27, 2015
---

Performance Tracking (benjhayden)

- Preliminary LayoutAnalyzer-powered UMA findings. A few surprises

already, lots of opportunity for further analysis and more data.

- Working on LayoutTree trace object snapshots and various

LayoutAnalyzer improvements.

Scrolling (skobes) \[[crbug.com/417782](http://crbug.com/417782)\]

- Reviewed bokan's RootFrameViewport change, and investigated a

background painting issue with root layer scrolling + slimming paint

(trchen is now working on a fix).

- Plan to work on testing root layer scrolling on Android this week.

Looking at failing layout tests related to fractional scroll offsets.

Line Boxes (szager) \[[crbug.com/321237](http://crbug.com/321237)\]

- Continued work on line layout LayoutUnit conversions. Have a patch

that sets the barrier between float and LayoutUnit at the InlineBox

level, which seems like the sensible place.

- There is still a performance regression that I haven't been able to

eliminate. Was able to get some small performance wins by looking at

profiling data, but I've hit a wall with that approach. Will discuss

next steps and come up with a plan this week.

Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- More flexbox updates.

- Trying to get answers to flexbox spec questions, was promised answers

this week.

List marker refactoring (dsinclair)
\[[crbug.com/370461](http://crbug.com/370461)\]

- Working on performance of list marker code. New version is slower

then old version. html5-full-render on ToT was ~16.5 seconds for me,

with ListMarkerPseudoElement is ~17.5 seconds.

- Discussions this week on how to proceed with ListMarkerPseudoElement.

Misc Warden (dsinclair, pilgrim)

- Finishing work on decoratedName split, all annotations are moved

over, just finishing off rebaselines. (dsinclair)

- More renderer to layoutObject renames. (dsinclair)

- Wrapped up selection gap clipping rework. (wkorman)

- Working on slimming LayoutObject and pushing bits down to

descendants. (pilgrim)

Text (eae)

- Added unit tests for complex text shaping (eae)

Style resolution (rune)

- Working on type selectors for SVG in HTML documents and picking up

componentized style resolver work again. There is some question

about whether the SVG bug is worth fixing.

Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]

- Found a new crash. Maybe something changed on trunk. Will fix then

reenable multi-col.

Layout refactoring

- esprehn, ikilpatrick, cbiesinger, ojan, leviw working on coming up

with concrete proposals.
