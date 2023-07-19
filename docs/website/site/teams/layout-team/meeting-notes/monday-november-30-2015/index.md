---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-november-30-2015
title: Monday, November 30, 2015
---

Updates since last meeting (on Monday, November 23rd):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Flag specific expectations for root layer scrolling landed. (skobes)
- Have patch for RTL scrollbar placement, investigating custom scrollbar
test failures. (skobes)
- Smooth scrolling; working on main thread toggled, compositors driven
animations, a little complicated but have a prototype. On track to hit
OKR. (skobes)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Still problems with flexbox/scrollbars (need to add height of
scrollbar to flex basis). Beed to sync with cbiesinger, original
problem with devtools still exists. Need to figure out how to test and
how to proceed. (szager)
- Release blockers and misc flexbox bugs. (cbiesinger)
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last week.
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Found out that support for column-span:all is really broken in nested
fragmentation contexts. So that's what I'm currently working on. If I
find nothing else after this, I'll then be ready to file the patch
that adds support for printing multicol.
CSS Houdini (ikilpatrick)
- Working on design doc for worklets, will finish today and circulate.
- Plan to finish up the worklet spec this week and started feeling out
the, as of yet unnamed, new canvas spec (splitting RenderContext2D
into discrete parts).
- Start writing patches for worklets.
Standards work
- Preparing to upstream flexbox tests, we now have a version of check-
layout that runs on top of testharness. (cbiesinger)
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- No update since last week.
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- Plan to send Intent to Implement this week.
- Have pending spec patch that needs to be upstreamed.
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- The memory model for intersection observer is really complicated,
especially when multiple documents are involved. Still not sure about
how it fits in with oilpan. Making solid progress but getting the
memory model right is tricky and time consuming. (szager)
Text (eae, drott, kojii)
- Updated memory infra CL (tracking font related memory usage), had
discussion with memory team. We might want to move some allocation to
use partition alloc. (drott)
- Verified ZWJ emoji android fix. (drott)
- Started proparing cl for removing simple text, there are a few
dependencies left, addressed two but still a few left. One on svg
that might have perf impact. (drott)
- HarfBuzz roll to 1.10, might want to backport to m48. (drott)
- Improved cache key and invalidation logic for shape cache, should
lead to reduced memory usage. (eae)
- Fixed a bunch of text related ASSERT. (eae)
- Always use complex text enabled on trunk. (eae)
HTML Tables (dgrogan, jchaffraix)
- Brain dump from jchaffraix to dgrogan continues.
- Trying to figure out the madness that is border collapsing and come up
with a plan. (dgrogan, jchaffraix)
Bug Health
- On track to not only meet but exceed bug health OKRs:
- Reduced unconfirmed/untriaged bug count by 60% (690 down to 377),
OKR is 50%.
- Reduced total bug backlog by 16% (2310 down to 1927), OKR is 15%.
- A lot of hard work by the entire team, thank you everyone! Let's
keep this up!
Misc
- Worked on checked-layout.js that works with test harness, allows for
upstreamable layout tests. (jsbell)
- Added support for flag specific test expectations, will greatly aid in
doing feature work and large refactorings going forward. (skobes)
- Fixed CounterNode crasher. (eae)
