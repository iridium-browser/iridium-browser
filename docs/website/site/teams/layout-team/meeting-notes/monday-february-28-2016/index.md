---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-february-28-2016
title: Monday, March 28, 2016
---

Updates since last meeting (on Monday, March 21, 2016):
Scrolling (skobes)
- Fixed ScrollAnchor handling of position:absolute. (skobes)
- 35 tests currently fail with scroll anchoring, digging into those.
(skobes)
- Working on getting ScrollAnchor to work during smooth scroll
animation. (skobes)
- ScopedLogger got removed, I am trying to get it back in. (skobes)
CSS Flexbox (cbiesinger)
- No updates since last week -
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No updates since last week -
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- No updates since last week -
CSS Houdini
- Wrote up a giant patch for CSS paint. Plan to split it up into to, fix
bugs and add tests this week before landing. (ikilpatrick)
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Converting layout view to api. (pilgrim)
- Added an include guard for the line layout api and fixed the few small
API gaps it exposed. (dgrogan)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- Finishing up document reporing the outcome from the LayoutNG
hackathon. (leviw, eae)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- Some CSS Containment discussions. Current options are layout, paint,
style, and strict. Adding one more, bike-shedding on name.
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- szager on vacation this week, eae will try to land a couple of
outstanding changes in his absence. (eae)
Text (eae, drott, kojii)
- Fixed a text wrapping bug where a fast-path incorrectly applied to LTR
content in a RTL container. (eae)
- Finished line breaking investigation and wrote up a design document
outlining my proposed new approach, using shaper information. (eae)
<https://docs.google.com/document/d/1eMTBKTnWEMDu00uS2p8Xj-l9Pk7Kf0q5y3FbcCrWYjU/>
- Started looking into a number of white-space: pre bugs. (eae)
- Fixed the invalid unicode codepoint issue that pdr@ had pointed us
to; Ugly integer truncation issue. (drott)
- Continued the work on small caps, implementation is coming along quite
well, the opentype feature itself works, but the synthesis needs more
tweaking. (drott)
- Started looking into the confused glyphs issue on Mac. (drott)
HTML Tables (dgrogan)
- Looked into a few table bugs but haven't yet made demonstrable
progress. (dgrogan)
Misc
- Have a full page of notes that are useful to me on writing mode, was
derailed due to paint perf work, expect to get to spend a few days
this week on it. (wkorman)
Logistics
- cbiesinger in SF week of March 28.
- drott in SF week of March 28.
- szager on vacation starting week of March 28.
