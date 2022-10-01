---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-october-19-2015
title: Monday, October 19, 2015
---

Updates since last meeting (on Monday, October 12th):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Fixed a regression from coordinated scrollbars. (skobes)
- Made progress on getting RTL working with root layer
scrolling (but still broken for various reasons). (skobes)
- Fixed an issue where elements become unscrollable
after an animation. (skobes)
- Discovered the cc side of smooth scrolling has been totally
broken for a while due to inner/outer viewport issues; ymalik
has a fix pending. (skobes)
- Working on getting unit tests passing with root layer scrolling
turned on, long slog. Making progress. (szager)
- Got the RTL issue (where the initial position of the scrollbar
was incorrect for RTL frames) all about figured out, on to the
next one. (szager)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Spent some time last week going through flexbox bugs, doing
bug triage and fixing issues.
- Discovered a couple of new fun bugs involving negative margins
where the resulting flexbox could end up getting a negative size.
Turns out the spec isn't clear on the correct behavior, working with
tab and fantasi on getting it clarified.
CSS Grid Layout (svillar, jfernandez, rego)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No updates since last week.
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Working on the usual suspects; nested multicol, get it good enough for
printing, and so on.
- Also working on fixing a blocker regression pertaining to printing an
overflowing flexbox inside a bottom-aligned fixed positioned box
(in the Chrome UI). The fix is straight-forward (believe it or not), but
I'm struggling with coming up with a LayoutTest. Looks like I'll have
to fix the printing test framework first (or possibly even the engine itself).
CSS Houdini (ikilpatrick)
- Last week, working on the isolated worker spec which is a spec
about a separate js exec context for all custom houdin stuff.
- Focusing on the CSS Paint API spec this week.
- Preparing for TPAC, will be there for the entire event. Will gladly
bring up issues or concerns on behalf of others.
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Still working on the line layout api, going thgough layout svg,
converting calls to go through the API. (pilgrim)
- Got a couple of smaller patches in last week, working on a
larger one this week. Probably about another two weeks of
SVG api work remaining. (pilgrim)
Text (eae, drott, kojii)
- Share more code between SVG text and CSS text. (kojii)
- Fixed fonts when lang attribute is set, investigation continues. (kojii)
- Investigate inheritance of anonymous inline and partial fix. (kojii)
- Pushing Writing Modes Level 3 to CR. (kojii)
- Proposed Update of Unicode Vertical Text Layout (UTR#50)
to publish soon. (kojii)
- Working on line break spec issues with CSS WG and Unicode. (kojii)
- Enabled complex text as experimental web platform feature, this
results in _all_ text going down the complex text path. If it sticks the
plan is to enable it for stable and then (eventually) delete the simple
text code path (yay!). Required 90% of all layout tests to be
rebaselined, due to problems with our tooling this mostly had to be
done manually. (eae)
- Fixed a bunch of ref-tests to make them more resilient against
minute text rendering differences. (eae)
- Speculative fix for a top windows crasher where fontMetrics is
null during line height computation. (eae)
- Working on getting shaper-drive-segmentation landed, very close
now but a few issues remaining. Should land this week! (drott)
Misc:
- Have a batch of bugs around lists, selection gaps, and caret. (wkorman)
- Meeting with joel about pick up running rebaseline bot. Will
run it on interim basis. (wkorman)
- Paint performance optimizations for slimming paint v2 where the
compositor side of things wants to make use of rtrees, needs
bounds of display items. Will change to accumulate during paint,
should lead to a nice performance win. (wkorman).
- We'll likely need to pull someone off their current project to help out
with intersection observer, it's a top blink-level priority for the quarter
and we need to make sure we make progress on it. (eae)
Logistics:
- jsbell gardening this week and helping out with cr-blink triage.
