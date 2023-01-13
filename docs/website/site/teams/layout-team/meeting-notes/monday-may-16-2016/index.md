---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-may-16-2016
title: Monday, May 16, 2016
---

Updates since last meeting (on Monday, April 25, 2016):
Scrolling
- Wrapping up overflow: auto work. Split up into three separate patches,
first one landed last week and fixes scrollbars in general in the
presence of "interesting" flex direction, writing mode and text
direction combinations. \[[crrev.com/392981](http://crrev.com/392981)\] (szager)
- The second patch (wip) overhauls how the flex algorithm handles
overflow: auto scrollbars. It basically makes it much clearer and
reduces unnecessary work that flex algorithm has to do to handle the
presence of scrollbars. Shouldn't have any visible changes. (szager)
- The third patch changes how intrinsic width is calculated in the
presence of scrollbars. It's a major change that will improve how
things look and will fix some incorrect coordinates that are reported
back. Right now, for instance, a zero width box width vertical
overflow will report a clientWidth of -15px. Relatively small change
but I think it'll be nice and improve things. Some fun interactions
between layout and paint clipping. (szager)
- Added a new ref test that tests all combinations and it'll keep us
honest going forward. Looking into up-streaming it. (szager)
Scroll Anchoring \[[crbug.com/558575](http://crbug.com/558575)\]
- Fixed a scroll anchoring subpixel positioning bug, was very annoying
as text kept flickering during load when anchored. (skobes)
- Have a solution for interaction between scroll event handlers and
scroll anchoring. Detects when it's bouncing between two locations and
prevents it. (skobes)
CSS Flexbox
- Flexbox specification discussions at the CSS Working group face-to-
face last week, made progress on intrinsic width calculations among
other things. (ikilpatrick)
CSS Grid Layout \[[crbug.com/79180](http://crbug.com/79180)\]
- Floated grid containers in Blink (rego)
- Fix computed style with distribution offsets (rego)
- Place positioned items with vertical writing modes (rego)
- Grid layout reviews (svillar)
- Position resolution with auto-repeat tracks (svillar)
- Auto-repeat tracks computation (svillar)
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Multicol / fragmentation is in a known good state. (mstensho)
CSS Houdini
- Last week was the Houdini face-to-face meeting. We didn't talk much
about the CSS Paint API, it's reasonably stable. We did, however,
spent about four hours talking about the CSS Layout API.
(ikilpatrick)
- Moving in a pretty good direction. The other vendors seemed pretty
happy with the overall shape of the API. Plan to update the spec
proposal in time for TPAC. (ikilpatrick)
- Plan to work on the worklets spec this week, mainly on the security
APIs. (ikilpatrick)
- Start design doc for off-main-thread worklets. (ikilpatrick, flackr)
Add API for layout \[[crbug.com/495288](http://crbug.com/495288)\]
- Finishing up report on line layout API. (dgrogan)
- Continued LayoutView/FrameView conversion. (pilgrim)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- Started working on design doc. Will share draft with team in a few
weeks. (eae)
CSS Containment \[[crbug.com/312978](http://crbug.com/312978)\]
- One remaining issue arund interaction between contain: paint and will-
change: transform. Will try to track down the root cause this week in
preparation for shipping by default. (eae)
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Shipping in M51.
Resize Observer (atotic)
- Spent a day in SF selling Resize Observer to Mozilla, IE/Edge, and
Safari. They seemed to like it, no major problems. Mozilla already
filed a tracking bug. Plan to send out an Intent-to-Implement later
this week. (aleks)
- Have a pending implementation, working with szager to get memory
management right with regards to lifetime and the oilpan GC. (aleks)
Tables (dgrogan)
- Microsoft working on updated CSS Table compatibility specification.
- Made progress on a couple of table bugs, have a good handle on the
root cause and will resume work this week. (dgrogan)
Text (eae, drott, kojii)
- Looked at an issue where we need to upconvert to the complex path on
Android for small-caps font font-variant-numeric. (drott)
- Continued android memory regression, trying to figure out where the
extra memory is spent and why it only affects Android. Independently
found the same issue that tzik and kojii identified where glyphs are
double cached. (drott)
- Added instrumentation for the glyph bounds cache. (drott)
- small-caps and font-variant-numeric will ship in M52! (drott)
- Resumed work on change to render unicode control characters in
accordance to spec. (eae)
Misc:
- Discussed test strategy for blink wrt w3c and web-platform-tests.
Learned about gecko process for keeping tests in sync. (jsbell)
- Spent the last two weeks doing general block layout clean up and
refactorings. Moving code and logic to where it makes more sense.
(mstensho)
