---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-february-29-2016
title: Monday, February 29, 2016
---

The layout team is a long-term team that owns the layout code in blink.
See <https://www.chromium.org/blink/layout-team> for more information.
Updates since last meeting (on Tuesday, February 22nd):
Scrolling (skobes)
- Last week, figured out that scroll anchoring is super broken. Working
on fixing that. We're not intercepting frameview scrolls correctly.
(skobes)
- Three new smooth scrolling regressions reported last week, working on
fixing those. (skobes)
- Aim to get back to helping out with scrolling work next week.
(szager)
CSS Flexbox (cbiesinger)
- Fixed all the scrolling bugs! Much rejoicing! Sadly the fix has been
reverted to due causing a crash in FrameView.
- Upstreaming flexbox tests to w3c.
- Started working on percentage sizing of children of flex items. A lot
of user requests and required by the updated spec.
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last week -
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- No update since last week -
CSS Houdini (ikilpatrick)
- Last week, landed PaintRenderingContext2d changes. Fun times with a ms
compiler bug.
- Got debugging for worklets working locally, coordinating with devtools
team about ways to integrate it into devtools.
- This week, talking to compositor worker team about API things.
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Have been filling out the new block layout API class hierarchy, should
be done creating all the necessary classes this week at which point I
can report back on how much work each one of the will be. (pilgrim)
- Chased down some remaining gaps in the line layout API but didn't
finish. (dgrogan)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- Started working on the scafolding for LayoutNG. (leviw)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- No update since last week -
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Working on a few coding issue around intersection observers, keeping
up with spec changes. (szager)
- More or less wrapped up on heavy duty part of that, converting some
tests to w3c style. (szager)
- We should do some perf testing, we don't really know how fast or slow
our implementation is. (szager)
- Also signed up to run an experiment where we use intersection observer
to see how frequently we encounter iframes that load but are never
viewed. Gather date to inform possible interventions down the line.
(szager)
Text (eae, drott, kojii)
- Cleaned up platform specific FontCache code. (eae)
- Updated windows font fallback logic for win 7, 8, 8.1 and 10. (eae)
- Added support for Glagolitic script. (eae)
- Started looking into antialiasing toggling on windows and a better way
to query the system for font preferences that works without punching
holes through the sandbox. (eae)
HTML Tables (dgrogan)
- No update since last week.
Misc
- Have no more release blockers, yay! After battling trying to figure
out root cause for no 1 crasher for days, finally found repro. Tracked
down to "remove plugin size hack" change, reverting that fixed it.
Still some issues with regard to life cycle state but it's within
tolerance. Frames and widgets are handled differently. Specifically
from the parent frame (main frame) we have multiple loops where we go
into our child frames and tell them to move their life cycle along,
but we don't do that for widgets. When a frame within a widget (web
view plugin) we special case that causing all sorts of unpredictable
behavior. Should be fixed at some point but not a priority at the
moment. (leviw)
- Have begun the code research phase for the exploratory flipped blocks
work. at a minimum we'll learn things. (wkorman)
- Pretended it was 1997 and fixed full-page-zoom for frames (as in
frameset). (eae)
