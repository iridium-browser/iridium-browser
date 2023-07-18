---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-february-1-2016
title: Monday, February 1, 2016
---

Updates since last meeting (on Monday, January 25th):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Prototyping scroll anchoring and added a flag for enabling it,
--enable-scroll-anchoring. Working on a basic implementation. Current
design looks solid and well encapsulated. Still needs testing in the
real world but looks promising. (skobes)
- Changed LayoutRoots to avoid making scrollbars trigger relayout.
(leviw)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Fixed a number of flexbox regressions last week. As of now all known
regressions in 49 and 50 have been resolved. Most 48 regression have
been addressed, merged a last fix this morning.
- Flexbox interop work continues, including a follow up to a temporary
fix for a regression.
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last meeting.
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Intent to unprefix multicol as is turned out not to be too popular.
Gated on generic breaking properties. Will send out a separate intent
for that. Breaking properties are generic and apply to all paginated
(and in theory region based) content. Not simply an alias for -webkit
prefixed properties as the values differ.
- Fixed a number of multi-col bugs.
CSS Houdini (ikilpatrick)
- Presented custom paint, custom properties and custom layout at CSS
working group meeitng in Sydney. (ikilpatrick)
- Updated custom paint, custom properties and custom layout specifications.
(ikilpatrick)
- Barista for the houdini working group, exceeding expectations.
(ikilpatrick)
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Moved resolvedColor to use line layout api, had a lot of cascading
effects that allows for simplifications in TextPainter and further
allows the API to be used in unit tests. Nice properties and
simplification ensued. (pilgrim)
- Multiple line layout patches includes a potential large change to the
API that was discussed last week. Fully up to speed on api work and
have a few layout patches pending. (dgrogan)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- Discussed CSS containment at CSS working group meeting, positive signals
from working group and other implementors. (leviw)
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Added optimization to skip render-throttled iframes during hit
testing. (szager)
Text (eae, drott, kojii)
- Presented CSS Writing Modes, CSS Text Decorations, and Snap at CSS
Working Group meeting in Sydney. (kojii)
- Emoji segmentation, tests on all platforms. Some blockers on Android
due to lack of capabilities with regard to font family based font
selection for fallback. (drott)
- Enabling open type features through CSS keywords. (kojii)
- Dealing with regressions and fallout following font fallback overhaul.
(drott)
HTML Tables (dgrogan)
- Misc table interop fixes relating to percentage height resolution.
(dgrogan)
- Working on a table regression dating back to the WebKit days, will
send out to Morten for review later today. (dgrogan)
Misc
- Made int to layout unit conversion explicit to avoid unnecessary
round-trips and v8 problems during style conversion. (leviw)
Logistics:
- ikilpatrick, leviw, kojii, and eae in Sydney for Houdini and CSS WG
meetings. <https://wiki.csswg.org/planning/sydney-2016>
- drott, behdad, kojii in Tokyo for Text workshop February 19-28.
