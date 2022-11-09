---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-april-4-2016
title: Monday, April 4, 2016
---

Updates since last meeting (on Monday, March 28, 2016):
Short update this week as a lot of people are due to Edge conf.
Scrolling (skobes)
- Restored ScopedLogger.
- Continued to work on on getting ScrollAnchor to work during smooth
scroll animation. (skobes)
CSS Flexbox (cbiesinger)
- Fixed a scrolling bug, caused a small regression, looking into it now.
- Landed a work-around to allow DI into branch, have a better patch
that I'll keep working on this week. Currently breaks one of our
tests.
- Might be helpful to have scroll behavior better defined in spec.
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No updates since last week -
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- No updates since last week -
CSS Houdini
- No updates since last week -
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Bunch of progress on layout view item, working on getting rid of
anything that touches LocalFrame and LayoutObject. Moving to new
shiny LayoutItem. (pilgrim)
- Have about ten files left, some pending review. (pilgrim)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- Sent out hackathon report for initial review. (leviw)
- Started working on design doc. (eae)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- No updates since last week -
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Getting back into Intersection observer, dotting the i's and crossing
the t's before branch point on Friday, Have one significant patch to
land, today, hopefully. (szager)
- Figure out behavioral difference between actual implementation and
polyfill. (szager)
- Prep work for shipping. (szager)
Text (eae, drott, kojii)
- Looking into android memory regression with mixed success, getting the
tests working locally have proven to be quite challenging. Working
with the Chrome Android team to resolve. (eae, drott)
Misc
- Fixed crash in localCaretRectForEmptyElement. (eae)
Logisitcs:
- skobes out until end of the month.
- eae out Wed-Fri.
- drott, leviw, ikilpatrick at edge conf Mod-Tue.
