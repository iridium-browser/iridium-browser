---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-january-4-2016
title: Monday, January 4, 2016
---

Welcome back every, I hope you all had a good holiday break.
With a new year comes new goals. Before we can set the high-level 2016
goals and Q1 OKRs though we should reflect on where we are and evaluate
our Q4 progress.
I'll reach out to everyone individually over the next week or so to
start the OKR process and will schedule a team-wide OKR meeting in a
week or two. In the mean time please try to summarize and score your Q4
OKRs and start thinking about what you want to work on in Q1 and beyond.
Updates since last meeting (on Monday, November 30th):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Landed fix for smooth scrolling target updates. (skobes)
- Sizing main graphics layer for root layer scrolling. (skobes)
- Updated scroll anchoring design doc and plan start to implementing
it this week. (skobes)
- Ready to turn onm smooth scrolling! Patch is ready and plan is to flip
the switch this week (skobes)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- No updates since last meeting, cbiesinger still out -
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last meeting.
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Managed to add support for printing. Involved solving the nested mult
column use case which was tricky. Once that was in place printing
support wasn't too much work.
- Considering unprefixing, seems reasonable.
- Fixing random multicol bugs.
CSS Houdini (ikilpatrick)
- Started to implement worklets, have patch out. Implementing all the
basic stuff for that.
- Also this week updating the CSS custom paint API.
- Intent to implement for custom paint this week or next.
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Finishing up line layout text this week, part of the line layout api.
- Final patches to implement box which will be ready for review shortly.
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- Plan to send Intent to Implement this week.
- Have pending spec patch that needs to be upstreamed.
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Trying to land Intersection Observer patch, not having much success.
(szager)
Text (eae, drott, kojii)
- Fixed a number of issues around complex text. (eae)
- Code cleanup following the move to always use complex text. (eae)
- Got simple text removal patch ready to go. Primed and ready to land as
soon as 49 branches. (drott)
- Zurich meeting with behdad and i18n, discuss improved fullback for
punctuation and symbols. Needs some manual tweaking. Need better
handling for color emoji and emoji combinations. Might need a symbol
iterator and then explicit font selection. (drott)
HTML Tables (dgrogan, jchaffraix)
- No update since last meeting.
Bug Health
- We managed to exceed our ambitious bug health goals by quite a bit
for Q4:
- Reduced untriaged bugs by over 70%, far exceeding goal of 50%.
Down to 180 unconfirmed/untriaged from 690 at the start of Q4.
- Reduced total open bug count by over 25%, exceeding goal of 15%.
Down to 1688 open bugs from 2310 at the start of Q4.
- A lot of hard work by the entire team, thank you everyone!
- Plan to institute a triage rotation in Q1, will be ~30 min a week for
one person (rotating). Details to follow.
Misc
- Stabalizing the rebaseline bot, is stable now, some open issues.
Looking at moving it to the build bot, blocked on infra issue that has
since been resolved. (wkorman)
- Fixed one layout blocker bug, felt good and nice to get a break from
trying to land intersection observer. More of the same this week.
(szager)
- Gave talk on layout at web engine hack fest in Spain. (drott)
Logistics:
- drott out Wednesday,m public holiday.
- eae, ikilpatrick, ojan in Seattle Thu-Fri for CSS Houdini meeting
with Microsoft.
