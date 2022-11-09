---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: march-2-2015
title: Monday, March 2, 2015
---

We had three new people joining the meeting this week:
- Mark Pilgrim (pilgrim)
- Works on Blink in Chapel Hill. Much of his recent work falls under
code-health and Project Warden.
- Nate Chapin (japhet)
- Works on Blink in Mountain View. Working on low-level blink platform
code cleanup and resource handling.
- Walter Korman (wkorman@google.com)
- New to the Blink team in SF, transfer from Glass. Getting up to
speed on Blink. Not sure which sub-team he'll end up on yet.
Updates since last meeting (on Monday, February 23rd):
Performance Tracking (benjhayden)
- Working ob exporting diagnostic information for performance tracking.
Scrolling (skobes) \[crbug.com/417782\]
- Mostly understand the issues around fixed position.

- Dealing with layout tests failures and fallout.
Rename Rendering -&gt; Layout (dsinclair) \[crbug.com/450612\]
- Massive RendereBlock to LayoutBlock change landed, touched 50k+ files!
- Plan to move RenderLayer and Renderer this week.
Measure API (jchaffraix)
- Addressed comments and concerns in Measure API proposal, started
circulating and soliciting feedback.
- Working on typing information and fragments this week.
Line Boxes (hartmanng, szager) \[crbug.com/321237\]
- Back to work on layout unit conversion, working on text rendering and
going through test failures fixing poorly written tests and getting
rid of float imprecision type failures.
Flexbox (cbiesinger) \[crbug.com/426898\]
- Working on updating flexbox implementation to match latest spec
revision. One of the changes involves rolling out a two year old CL
that removed a feature that has since been re-added to the spec.
- Estimate another two weeks of work.
Isolate core/fetch (japhet) \[crbug.com/458222\]
- Working on isolating core/fetch from the rest of core. Currently
touches frame and document which it probably shouldn't.
Blink componentization (pilgrim) \[crbug.com/428284\]
- Moving things from core to modules; core/storage and core/timing.
Text (kojii, wjmaclean, eae)
- Started looking into text iteration code in detail, discovered that
BitStack has been incorrect since day one. (wjmaclean)
- Cleaning up text iteration code. (wjmaclean)
- Continued work on HarfBuzz normalization performance improvements,
CL ready but blocked on perf numbers.
- Performance work for complex path with eae and Dominik. (kojii)
- 5-9% gain for CJK text landed.
- 3-4x gain for all languages close to land. With this we should be
pretty close to match the performance of the simple text path for
vertical text.
- Exploring a few more ideas.
- Added shared shaper base class to ease the transition to the complex
path. (kojii)
Page scale handling (bokan) \[crbug.com/459591\]
- Auditing window vs frame coordinates.
Assertions/Regressions/blocking bugs
- Working through list of ClusterFuzz assertions (cbiesinger).
- Dealing with Mac core text crashers/blockers. (eae)
- Help with git repository merge work. (szager)
- Looking at regression relating to document life-cycle. (walterkroman)
Misc
- Dealing with fallout from --dump-render-tree rename, heated discussion
following deprecation CL, will send out mail to blink-dev and
coordinate with eae. (paulmeyer)
- Helping philipj at Opera with syncing IDLs. (jsbell)
