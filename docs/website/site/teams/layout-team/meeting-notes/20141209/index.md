---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: '20141209'
title: Tuesday, December 9, 2014
---

Attendees: benjhayden, cbiesinger, dsinclair, dglazkov, eae, jchaffraix, nduca,
szager, skobes
Introductions, quick updates:
eae: In SFO, currently working on text and line layout cleanup/performance
szager: In SFO, recently transferred to blink from chrome infrastructure.
dan: In WAT, working on paint invalidation and not mutating the render tree
during layout
cbiesinger: In NYC, working on all sorts of things, mostly cleanup at the
moment.
jchaffraix: In MTV, working on rendering and project warden
skobes: In MTV, scrolling cleanup, warden
benjhayden: In MTV, recently transferred to blink to work on layout performance.
Project Warden update from Dan:
Warden is a project to clean up long standing issues, mostly in the
layout/rendering space, including:
- widget tree hierarchy
- asserts, clusterfuzz
- fixing full screen to work correctly
- avoiding render tree mutations during layout
- move line layout to layout unit
- unify scrolling paths
- type system for blink (screen vs layout coordinates)

Project warden is a project under the layout team and will continue as is. With
the additional resources of the entire layout team we can now take on additional
tasks. Performance is a key goal but we are currently suffering from reliable
metrics and limitations when it comes to performance testing.

Legacy webkit performance tests are non-ideal in that they are sampling based
and do properly account for scheduled cpu time. Modern telemetry tests account
for scheduling and measures time on a low, fine grained level resulting in more
reliable metrics. We should investigate converting existing tests to
telemetry-style or creating new layout performance tests to guide our work. We
also need to profile the layout code based on these tests and real world
websites to determine performance bottlenecks and hot code paths.

Overhead:
- Weekly stand-up meetings?
- Re-purpose the existing project warden meetings (Mondays at 11am PST)
- Bi-quarterly OKR meetings (beginning of quarter to grade last quarters OKRs
and set new objectives, mid-quarter check-in).
- Set up team mailing list? Decided not to and use blink-dev for the time being.
- Set up team web site? Yes.
OKR candidates:
- position sticky?
- measurement api
- better perf tets
- report on whats slow
- performance work
- layout hooks (from glitter)
- natural animations
- subpixel when animating
Action items:
- Add all layout team members to weekly stand-up \[dsinclair\]
- Schedule OKR meeting for \[eae\]
- Come up with short list of proposed OKRs in preparation for next meeting
\[eae, jchaffraix\]
- Set up web site for team \[eae\]
- Discuss performance testing strategies \[benjhayden, nduca, eae\]
- Explain team role in overall blink \[deferred until after OKR meeting\]
