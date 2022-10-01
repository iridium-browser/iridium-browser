---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-february-23-2015
title: Monday, February 23, 2015
---

We had two new people joining the meeting this week:

- Joshua Bell (jsbell)

- Works on Storage APIs (TL) and ServiceWorker.

- Interested in some of the non-layout Warden tasks.

- Alex Russell (slightlyoff)

- TL for Fizz. Available to consult on API questions and JS interaction.

Updates since last meeting (on [Wednesday, February
18th](/teams/layout-team/meeting-notes/wednesday-february-18)):

Performance Tracking (benjhayden)

- Looking at perf regressions on a webkit performance test.

Scrolling (skobes) \[crbug.com/417782\]

- Fixed Mac smooth scrolling regression.

- Working on fixed position elements this week.

Rename Rendering -&gt; Layout \[crbug.com/450612\] (dsinclair)

- Trying to rename RendereBlock to LayoutBlock. Will have to land

outside of office hours and/or close the tree. Will coordinate with

the infra team.

Measure API (jchaffraix)

- Synced up with slightlyoff on API surface and implementation.

- Continuing work on prototype and write-up.

Line Boxes (hartmanng, szager) \[crbug.com/321237\]

- No real progress last week. Plan to resume this week.

Flexbox (cbiesinger) \[crbug.com/426898\]

- Working on updating flexbox implementation to match latest spec

revision.

Text (kojii, wjmaclean, eae)

- Vertical flow performance test is up and running. (kojii)

- Temporary reverted vertical flow to simple path for the branch cut.

(kojii)

- Studying line breaker and exploring ideas:

- Optimize width() when the length is one character.

- Line breaker to use heuristic to start from likely break

opportunities. (kojii)

- Studying text iteration code, making it easier to follow. (wjmaclean)

- Investigated Mac test regressions. (eae)

- Plan to ressume work on complex text performance work, avoiding an

extra copy of the string and unnecessary normalization. Requires an

updated version of HarfBuzz which is tricky on Linux as we use the

system version. Have some ideas. (eae)

Misc

- Finished blink starter bugs (svg image cache), image resourcem,

Landed and done. (paulmeyer)

- Renamed --dump-render-tree to --run-layout-tests. Done but documentation

needs updating. Old flag deprecated but still there for now.

Assertions/Regressions/blocking bugs

- Working through list of ClusterFuzz assertions (cbiesinger).

- Fixed a CluserFuzz selection range bug and got "rewarded" by being

assigned another one. (szager)
