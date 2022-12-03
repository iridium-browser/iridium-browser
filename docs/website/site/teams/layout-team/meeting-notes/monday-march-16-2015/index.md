---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-march-16-2015
title: Monday, March 16, 2015
---

eae OOO, jsbell taking notes (poorly - apologies for mangling your words and
leaving out any important bits)

Measure API (jchaffraix)

- still working on proposal for measure

- finished layer refactor/renaming

Flexbox (cbiesinger)

- almost done w/ first part of flexbox spec change

- tests missing, importing w3c tests

Isolate core/fetch (japhet)

- big class responsible for most violations is mostly done

- lots of smaller classes to tackle next

Scrolling (skobes)

- position fixed works w/ root layer scrolling

- looking at layout test failures

Performance Tracking (benjhayden)

- adding pages to key silk pages benchmarks

Rename Rendering -&gt; Layout (dsinclair)

- splitting layout name out into decorated name \[jsbell: didn't catch all of
this\]

- updating lots of layout tests to have more correct results

- renderer -&gt; layout object renaming

- fun content shell resize issue causing subsequent tests to fail, trying to
track it down

Line Boxes(hartmanng, szager)

- working through float to layout unit test failures

- hartmanng: windows - tons of failures, but everything so far looks like it can
rebased, but only a few dozen out of a few hundred reviewed so far

- szager: made it through nearly all tests on mac, two real regressions - line
wrapping, svg and platform specific layout; digging into those

Misc:

- jchaffraix: now working on a release blocker

- szager: helping out jochen with repo merge

- wkorman: still ramping up; tell me about merging to release branches...?

- japhet: other half of time spent on site isolation stuff

- jsbell: nothing warden-related this week (other than these notes)
