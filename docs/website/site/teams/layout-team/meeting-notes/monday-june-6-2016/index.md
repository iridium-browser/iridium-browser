---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-june-6-2016
title: Monday, June 6, 2016
---

Updates since last meeting (on Monday, May 23, 2016):
Scrolling
- Landed all of the scrollbar fixes! Finally bringing sanity and good
test coverage to the scrollbar space. (szager)
- Scrollbar regression in web inspector, looks like an incorrect
workaround in client code. Needs further investigation. (szager)
- Found and fixed regression in google spreadsheets, due to js trying
to compensate for scrollbars. (szager)
- All regressions identified thus far have been due to client side js
working around or trying to compensate for scrollbars. Long tail but
new behavior is "better" and we're now more consistent with IE/Edge.
(szager)
Scroll Anchoring \[[crbug.com/558575](http://crbug.com/558575)\]
- No updates since last week.
CSS Flexbox
- Back from vacation, dealing with a couple of regressions including a
release blocker. (cbiesinger)
CSS Grid Layout \[[crbug.com/79180](http://crbug.com/79180)\]
- No updates since last week.
- Grid talk scheduled for BlinkOn 6 in Munich.
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- No updates since last week. Morten out for the next two months.
CSS Houdini
- Helping audio folks with worklets, they're in a good space design
wise. Got something that'll work and that we're happy with.
(ikilpatrick)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- Working on design doc, will be ready for circulation among the team
post BlinkOn. (eae, ikilpatrick)
CSS Containment \[[crbug.com/312978](http://crbug.com/312978)\]
- Identified a couple of security bugs involving containment and
tables. (eae, dgrogan)
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Changed dispatch to make the order of notifications deterministic.
(szager)
Resize Observer (atotic)
- Initial implementation up for review. It's looking pretty good. We
have a spec we're happy with and that we're implementing. (atotic)
- Two remaining issues to match spec, don't quite know how to expose
that as a part of the API. Plan is to check in what we have behind
a flag and to iterate. (atotic)
Tables (dgrogan)
- Change invalidation logic for border width changes for rows, fixing
a bug with floats. (dgrogan)
Text (eae, drott, kojii)
- More FontCache analysis and looking at lifecycle of FontPlatformData
objects. Goal is to reduce complex path font memory consumption and
achieve enabling complex by default on Android. (drott)
- We're basically leaking FontPlatformData objects and hb_blob_t with
font table information over the lifetime of a renderer. This is
especially bad for single process. (drott)
- Identified a RefPtr cycle issue. (drott)
- Identified an issue of storing full FontPlatformData objects as key
in FontDataCache. (drott)
- Plan to experiment with an LFU eviction plus threshold approach.
(drott)
- Fixed a couple of line brekaing bugs involving element boundaries due
to diffrences in rounding between prefered width calculations and
line breaking. (eae)
Misc:
- Fixed an interop issue wrt list-style inside. We now match the spec
and the Mozilla/Edge behavior. (glebl)
- We now support color fonts, including color emoji, on Windows 8.1
and later. (kulshin) \[[crbug.com/333011](http://crbug.com/333011)\]
- Using page zoom for device scaling enabled by default on ChromeOS.
(oshima) \[[crbug.com/485650](http://crbug.com/485650)\]
