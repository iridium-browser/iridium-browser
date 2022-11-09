---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-june-27-2016
title: Monday, June 27, 2016
---

Updates since last meeting (on Monday, June 6, 2016):
Scrolling
- Dealing with a scroll related performance regression: when you change
the existence off scrollbars on a box it goes and sets a flag that
invalidates the width available for children which forces a re-layout
for all children. This happens even if the content width didn't
change. Landed a patch to fix that; it may not be perfect yet but it's
close, needs a few tweaks. (szager)
- Assisted ymalik in investigating smooth scroll jank issues. (skobes)
\[[crbug.com/616995](http://crbug.com/616995)\]
- Working on a fix for incorrect repainting of negative z-index children
on scroll. (skobes) \[[crbug.com/596060](http://crbug.com/596060)\]
Scroll Anchoring \[[crbug.com/558575](http://crbug.com/558575)\]
- Thinking about the scroll anchoring cycle problem
\[[crbug.com/601906](http://crbug.com/601906)\],
will likely add a limit on number of adjustments between user scrolls
per Kenji's suggestion. (skobes)
CSS Flexbox
- Fixing flexbox regressions. Number of open flexbox regression bugs is
steadily decreasing. No new major issues detected or reported latelty.
(cbiesinger)
- Refactored flexbox layout algorithm to make it easier to understand
and maintain. (cbiesinger)
CSS Grid Layout \[[crbug.com/79180](http://crbug.com/79180)\]
- Fixed some small layout bugs related to intrinsic sizes and border/
padding. (jfernandez)
- Working on grid orthogonal flows. (jfernandez)
- Working on new CSS alignment parsing logic. (jfernandez)
- Get rid of LayoutBox::hasDefiniteLogicalWidth|Height(). (rego)
- Fixed issues with height percentages on grid items. (rego)
- Gave CSS Grid update talk at BlinkOn 6 in Munich. (rego)
- Implemented repeat (auto-fit). (svillar)
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- No updates since last week. Morten out for the next two months.
CSS Houdini
- Added support for prefixed properties to the CSS Paint API. (glebl)
- Renamed Geometry to PaintSize for the CSS Paint API to match latest
spec update. \[[crbug.com/578252](http://crbug.com/578252)\] (glebl)
- Enabled worklets and the CSS Paint API as experimental web platform
features. (ikilpatrick)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- Working on design doc, will be ready for circulation among the team
post BlinkOn. (eae, ikilpatrick)
CSS Containment \[[crbug.com/312978](http://crbug.com/312978)\]
- No updates since last week.
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Misc minor intersection observer fixes. (szager)
- OOPIF will need to support intersection observer, unclear what
priority or who should own the work. Complicated as it needs to walk
through the frame tree, if a frame is in another process you cannot do
that so some architecture changes are needed to have each process
compute intersections at frame time and have the browser process
process the intermediate results. (szager)
Resize Observer (atotic)
- No updates since last week.
Tables (dgrogan)
- No updates since last week.
Text (eae, drott, kojii)
- Made changes to the font cache to use ref counting which should allow
for better cache invalidation. (drott)
- Found a way to get zero-copy access to the font data tables from skia,
avoiding an extra copy per FontPlatformData. This is potentially a
very big win! (drott)
Misc:
- Rebaseline bot update: wangxianzhu did some excellent work, it should
be operating correctly now. Was previously potentially rebaselining
with stale result as it didn't wait for all bots to cycle before
triggering. One should still closely monitor baselines but it should
be working correctly now. (wkorman)
- Fixing some bugs around paint invalidation rects,
mapToVisualRectandAncestor. Bugs with flipped blocks and relative
position specifically. Likely to also affect intersection observer.
(wkorman)
- Rolled back dropdown change where we reserved extra space as it broke
a number of sites. Trying a new approach that better matches other
browsers. (glebl)
