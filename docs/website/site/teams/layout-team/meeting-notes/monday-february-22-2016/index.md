---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-february-22-2016
title: Monday, February 22, 2016
---

Updates since last meeting (on Tuesday, February 16th):
Scrolling (skobes)
- Fixed crashes caused by stale LayoutObject pointers in scroll anchor.
- Fixed an old bug in coordinated scrollbar where we hold stale
scrollbar pointers.
- Simplifications in paint layer scrollable area.
- Playing around with scroll anchoring on real world web sites.
- Discovered a new smooth scroll regression.
CSS Flexbox (cbiesinger)
- Finished up the scrolling fixes last week, at least I thought I did.
szager found another issue relating to nested flexboxes. Not quite
sure what the correct fix is, will require further work.
- More flexbox release blockers relating to overflow: auto.
- Imported Mozilla flexbox tests, pass the vast majority. One failure is
paint related and will probably fixed by slimming paint v2 (the
fundamental composting bug), a few fail due to lack of visibility
collapse support.
- Struggle with upstreaming tests to w3c continues, synced up with
fantasi and have a plan to move forward.
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last week -
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Multi column properties unprefixed! Congrats!
\[[crrev.com/376249](http://crrev.com/376249)\]
- Added partial support for modern breaking properties. Work on these
continues.
CSS Houdini (ikilpatrick)
- Successes in splitting up RenderingContext2d, patch out that has an
LGTM already. Should be ready to land today.
- Started adding dev-tool support to worklets, plan to have it hooked up
and working at the end of this week.
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Working on removing the default operator for the Line Layout API.
Covering gaps in the API where we are relying on implicit conversion.
(pilgrim)
- Started working on the Block Layout API. Trying to figure out the
hierarchy, still up in the air. (pilgrim)
- Hope to finish up Line Layout API and add include guard by the end of
the week. A bit hairier than expected but cautiously optimistic.
(dgrogan)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- No update since last week.
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Spent most of last week doing spec work for IntersectionObserver.
\[szager\]
- This week, I have a few code changes to make so our Intersection
Observer implementation matches the latest version of the spec.
After that plan to ramp down work on IO for the time being. \[szager\]
Text (eae, drott, kojii)
- kojii/drott/behdad doing a text workshop in Tokyo, dealing with text
shaping and font fallback issues.
HTML Tables (dgrogan)
- No update since last week.
Misc
- Fixed getBoundingClientRect for collapsed ranges. (eae)
- Removed mac specific backspace handling in editing. (eae)
- Fixed a number of release blockers. (leviw)
Logistics
- ikilpatrick in Waterloo, Canada next Week.
- leviw out Monday (today).
- dgrogan out Friday and next Monday.
- behdad/drott in Tokyo all week.
