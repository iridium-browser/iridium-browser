---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-april-11-2016
title: Monday, April 11, 2016
---

Updates since last meeting (on Monday, April 4, 2016):
Scrolling
- Focus on root layer scrolling for now. The top most element, the
layout view, (frame view contains layout view, both are scrollable
areas. Frame view has special magic, plan is to have LayoutView handle
scrolls using the standard ScrollableContainer logic. Been ongoing for
awhile, skobes has done most of the work but has been pulled into
other things. (szager)
CSS Flexbox (cbiesinger)
- Spent way too much time on scrolling and will have to spent more time
on scrolling. Still haven't found a good solution, trying to move
scroll clamping to after the layout phase.
- Landed a few cleanup patches.
- Focus on other flexbox issues this week.
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No updates since last week -
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Fixed multicol assertion failures. (mstensho)
- Improved the forced fragmentainer break implementation: Only allow
those at class A break points. (mstensho)
<https://drafts.csswg.org/css-break/#possible-breaks>
CSS Houdini
- Last week got custom paint end to end working, out for review.
(ikilpatrick)
- Working on style invalidation this week. (ikilpatrick)
- Next up, spec stuff for houdini. (ikilpatrick)
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- As promised, spent last week working on layout view. Migrated callers
to use the new block layout API (LayoutViewItem). (pilgrim)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- mstensho getting up to speed.
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- No updates since last week -
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Intersection observer on by default in M51. Yay! (szager)
Text (eae, drott, kojii)
- Attended Edge Summit, interesting discussion on potential underlying
issue with system font cache. (drott)
- Submitted Conference Abstract to 40th Unicode Conference. (drott)
- Worked with dpranke@ to get hb-ot-font linkage issue out of the way,
and landed hb-ot-font change. (drott)
- Continued work on Small Caps Implementation. (drott)
- Started looking into hyphenation support and sent out a outline for
a implementation proposal. (kojii)
- Added comma and dot segmentation for shaping cache in order to fix
a perf regression for minimized js in devtools. (eae)
Logistics:
- skobes out until end of the month.
- leviw leaving the team and Blink in a month. Will be trying to offload
knowledge and transfer ownership. Get your questions and code reviews
in while you can!
