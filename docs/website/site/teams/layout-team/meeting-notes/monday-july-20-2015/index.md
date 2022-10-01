---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-july-20-2015
title: Monday, July 20, 2015
---

Updates since last meeting (on Monday, July 13th):

Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]

- Continuing root layer scrolling conversion. Judging progress is hard

and we don't necessarily know how much work remains, plan is to turn

on root layer scrolling unconditionally and then run all he tests in

order to see how many test failures we have and then use that as a

burn-down list. Obviously progress isn't linear with the number of

test failures but should give us a rough idea and a way to track

progerss. (szager)

CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- Back from vacation, resuming flexbox work.

CSS Grid Layout (svillar) \[[crbug.com/79180](http://crbug.com/79180)\]

- No update.

Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]

- No update, mstensho on vacation.

Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]

- Landed a couple of line-layout API patches. (ojan)

Isolate core/fetch (japhet) \[[crbug.com/458222](http://crbug.com/458222)\]

- No update.

CSS Test Suites (jsbell, kojii)

- No update.

Text (eae, drott, kojii)

- Dealing with fallout from complex text refactoring/optimization. (eae)

- Adding support for tab characters to complex text path, which in turn

will unblock work to enable the complex path by default. (kojii)

- Continuing work on rationalizing the font fallback implementation and

logic. Have a good understanding of the system and working on adding

tests to ensure that we do not regress and that our system has good

test coverage. (drott)

- Our font selection implementation is not necessarily spec compliant

at the moment, looking into why and working on improving our

font selection tests. (drott)

Misc:

- Triaging and updating baselines for the Mac 10.10 bots. (joelo, leviw)

- Helping windows team with win 10 changes/tests. (eae)

Logistics:

- mstensho on vacation this week.

- skobes on vacation this week.

- pilgrim on vacation today, back tomorrow.

- cbiesinger planning trip to SF on Aug 3rd \[tentative\].
