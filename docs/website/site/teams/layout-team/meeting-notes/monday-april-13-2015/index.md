---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-april-13-2015
title: Monday, April 13, 2015
---

Performance Tracking (benjhayden)

- LayoutAnalyzer-based UMA histograms.

- Benchmark metric work.

- Made progress understanding bug 369123 (Absolute position and display:

inline-block are order-dependant)

- Experimenting with data-mining traces

First letter refactoring (dsinclair)
\[[crbug.com/391288](http://crbug.com/391288)\]

- Fixed bug where a block and an inline both used first-letter.

- Fixed bug where changing the text content didn't updated the layout

resulting in the first-letter disappearing.

- Broke first-letter hit testing while trying to fix the same. Added a

bunch of new tests for hit testing pseudo classes.

Measure API (jchaffraix)

- Working on proposal/brainstorming document for layout refactoring.

Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- Added check to ensure that min-size cannot be negative.

- Changes to bring min-width auto up to spec in review.

- Working on positioning of position: absolute elements.

Line Boxes (szager) \[[crbug.com/321237](http://crbug.com/321237)\]

- Eliminating wrapper classes helped improve performance but there is

still a performance regression.

- Trying to figure out the right place to draw the line between floats

and layout units. Text metrics still uses (and will continue to use)

floats, box layout uses (and will continue to use) layout units.

Doing most text metric in floats and converting at the boundary

between the block and inline trees might make sense.

Isolate core/fetch (japhet) \[[crbug.com/458222](http://crbug.com/458222)\]

- Continuing work to isolate core/fetch from the rest of core.

RenderObject cleanup (pilgrim) \[[crbug.com/436473](http://crbug.com/436473)\]

- Continuing to pull things out of RenderObject.

Text (kojii, eae)

- Started work to improve caching for complex text shaping results.

(eae)

- Running/writing tests to check where we are; in some cases complex

text is already faster than simple text. In others it is up to 5x

slower. (eae)

- Worked on CSS Writing Mode test suites with contributors. (kojii)

Misc Warden

- Cleaning up bindings, replacing custom bindings with auto-generated.

(jsbell)

- Started to plan storage type cleanup. (jsbell, pilgrim)
