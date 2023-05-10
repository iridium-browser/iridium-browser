---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-march-23-2015
title: Monday, March 23, 2015
---

Performance Tracking (benjhayden)

- Added a couple new pages to the key_silk_cases benchmark suite.

- Continuing counter work.

- What further pages should be added? jchaffraix has a list.

Scrolling (skobes) \[[crbug.com/417782](http://crbug.com/417782)\]

- Fixed a flaky test but turns out it is still flaky on one of the

oilpan bots. Not sure why. Will coordinate with oilpan team to try to

track down the issue.

Rename Rendering -&gt; Layout (dsinclair)
\[[crbug.com/450612](http://crbug.com/450612)\]

- Renamed renderer in layout/svg.

- Renamed Renderer to LayoutObject in ImageResource and

LayoutTreeBuilder.

- Renamed \*styleForRenderer to \*styleForLayoutObject.

Measure API (jchaffraix)

- Collected use cases and have enough to justify continued work on

prototype.

- Out-of-tree measurements will be tricker than expected, a lot

trickier. Might be a problem that affects more than measure.

Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- Continuing on flexbox work. Progress is slower than expected as

updating tests takes a lot of time.

Line Boxes (szager) \[[crbug.com/321237](http://crbug.com/321237)\]

- Found root cause for a significant chunk of layout test failures,

including incorrect wrapping in buttons. Turns out leading whitespace

is to blame. We subtract the width of a space character using a

floating point representation of the width, introducing imprecision.

- Hopeful that this'll be the last code change before landing. (szager)

Isolate core/fetch (japhet) \[[crbug.com/458222](http://crbug.com/458222)\]

- Continued quested to isolate fetch directory from the rest of core.

Mostly there.

- Changed DEPS rule to enforce core/fetch isolation, white listed

existing bad includes (down to 7).

Page scale handling (bokan) \[[crbug.com/459591](http://crbug.com/459591)\]

- Coordinate transformation cleanup in FrameView.

- Added documentation for coordinate spaces, see

<https://www.chromium.org/developers/design-documents/blink-coordinate-spaces>

Text (kojii, wjmaclean, eae)

- ​Reviewing test results for bidi and vertical flow in W3C test suites

with W3C contributors. (kojii)

- Fixed vertical alignment in vertical flow, one of the biggest failures

in the W3C test suites. (kojii)

- Getting back to text iterator work. (wjmaclean)

- Fixed top Mac crasher, caused by incorrect assumption in HarfBuzz

CoreText shaper. (eae)

- Backported fix for reverse_range tab-crash to M41 after it getting

some media attention. (eae)

ClusterFuzz

- Various ClusterFuzz bugs (wjmaclean, jchaffraix, szager)

- Working on a table layout crash bug, have repro but not sure of cause

yet, will coordinate with wjmaclean as needed. (wkorman)

Misc

- Helped with repository merge issues (szager)

- Remove old ICU hacks/code from wtf/text now that we control the

version of ICU used. (jsbell)

Logistics:

- hartmanng switching to another chrome team, dsinclair will take over

in-progress work.
