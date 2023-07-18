---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-february-21-2016
title: Monday, March 21, 2016
---

Updates since last meeting (on Tuesday, March 15, 2016):
Scrolling (skobes)
- Working on patch to fix absolute positoined scroll anchors within
static positioned scrollers which is an interesting edge case.
- Smooth scrolling and scroll anchoring interaction is a bit problematic
and needs some work given the absolute scroll offsets that update
immediately.
- Work on speeding up smooth scrolling in response to user feedback.
CSS Flexbox (cbiesinger)
- Fixed percentage sizing, the last remaining major task.
- Almost all of the scrolling regressions (again!) still one case
remaining that needs more investigation.
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- eae/svillar trying to figure out what would be required to ship CSS Grid.
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- The past weeks I've been working on the break-{after,before,inside}
properties. Mostly some preparatory patches so far, while I've kept
the beefy parts to myself for now.
- Also: continued cleaning up and untangled multicol stuff in
mapLocalToAncestor() and friends, which made it super-easy to fix some
a hot PaintLayer clipping bug afterwards.
CSS Houdini
- Hooking up script support for the CSS Paint API this week.
(ikilpatrick)
- Next up is generating new CSS images for the paint API. (ikilpatrick)
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Working on getting all of the LayoutView references over to the new
block layout API, will be one of the largest leaf nodes ion the new
layout API. (pilgrim)
- Will try to add precoomit hook to enforce the line layout API.
(dgrogan)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- LayoutNG hackathon last week. It happened and was glorious. We got of
to a better start than expected, got very simple layout working day
one. I think we're convinced that it's a tenable design, deciding on
next steps. (leviw, ikilpatrick, dgrogan)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- No update since last week -
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Assortment of patches for intersection observer, including all release
blockers that I'm trying tom land this week before vacation. Gated on
reviewers. (szager)
Text (eae, drott, kojii)
- Landing fix for shaping fonts across unicode ranges, now all ranges
are all shaped together in one go. Also fixes a bug reported by
typekit, made adobe happy. (drott)
- Resume work on small-caps font variant. (drott)
- Investigating line breaking performance. (eae)
HTML Tables (dgrogan)
- Dealing with a table bug fix from a few weeks ago, breaking some
chrome-only sites relying on the old incorrect behavior.
Misc
- Rebaseline bot is now actually running on the builder, no longer on
my workstation. Exciting times. (wkorman)
- More detailed research on writing modes, continues this week and
next. (wkorman)
Logistics
- cbiesinger in SF week of March 28.
- drott in SF week of March 28.
- szager on vacation starting week of March 28.
