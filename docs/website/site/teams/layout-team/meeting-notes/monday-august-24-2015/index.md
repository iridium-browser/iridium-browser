---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-august-24-2015
title: Monday, August 24, 2015
---

Updates since last meeting (on Monday, August 17th):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Working on an issue where Element.clientWidth and clientHeight are not
excluding the width/height of scrollbars as expected. (skobes)
- There is logic for coordinates scrollbars in the compositor that has a
check in scrolling coordinator that only applies to the main frame on
some platforms. Needs investigation as we'd like this to apply for
root layer scrolling on all platforms. (skobes)
- Trying to land a fix for scroll bounds but encountered a couple of
test failures that only occur on Mac and Windows. Do not have access
to either platform at the moment (working remotely). Looks like off-by
-one errors. Will work with skobes to track it down. (szager)
- Have been continuing to struggle with flexbox scrollbar change. Had
perf regression that prevented landing it. Fixed regression mostly,
is close but is a great deal of variance in inspector tests between
runs. Working on getting tracing information from running layout
tests. Will reach out to tracing-team. (szager)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Added use counters for flexbox intrinsic size change and for the
prefixed intrinsic size keywords.
- Unprefixed the intrinsic size keywords, with the exception of fill.
Will be in M 46.
- Looked into deprecating the old prefixed intrinsic size keywords,
only supported by us and webkit, fairly low usage.
- fill-avalible will be kept as prefixed, pending spec changes to
unprefix. Oddly seems to be the most used one.
CSS Grid Layout (svillar) \[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last week.
Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]
- Refactoring the multicol balancing implementation. It currently
assumes that the layout engine feeds the balancer with accurate data
(space shortage and explicit breaks) for each layout object once and
only once for each layout pass. This is an incorrect assumption,
because sometimes we relayout a subtree (typically because the initial
logical top estimate was wrong) (in which case what the balancer got
during the first pass is most likely wrong). Sometimes we also just
skip an entire subtree as an optimization (which may mean that the
balancer doesn't get all the data it's supposed to get). Trying to
change it to performing everything related to column balancing after
layout of a flow thread, instead of partially during layout and
partially after layout.
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Landed several API patches. Estimate a couple of days worth of work
remains. (pilgrim)
Text (eae, drott, kojii)
- Landed font-stretch and better matching for font-style, had some
issues that delayed landing it but it's in now and we have great test
coverage for it. (drott)
- Discussed font mathcing tests with adobe. (drott)
- Will continue work on font fallback this week. (drott)
- Encountered a skia bug where the full font name wasn't returned as
expected on windows, was quickly fixed by bungeman, thank you!
- Working on blink/harfbuzz integration, plan is to have harfbuzz do the
font fallback instead of doing it prior to shaping. That way combining
marks, among other things, can be better supported and we avoid having
to reinvent the wheel given that harfbuzz already has support for font
fallback. Long process and complicated dependencies. Will require
changes to how we handle variance (like small caps and emphasis marks)
as our current implementation cannot be supported and will need to be
updated. Should also result in cleaner code. (drott)
- Fixed handling of CSS word-spacing for non-breaking spaces. (eae)
- Investigating test changes required for enabling complex text
everywhere. Might be able to disable kerning and ligatures for some
scripts temporarily to avoid having to rebaseline all tests and to
make the change in two stages; (1) enable complex text everywhere,
(2) enable kerning & ligatures. Not sure it's worth the effort. (eae)
- Working on blockers for unifying simple path and complex path. (kojii)
- Enabled Unicode Variation Selector in ChromeOS/Linux. (kojii)
- Fix line breaking issue around ruby. (kojii)
- Working on CSS Writing Modes issues. (kojii)
Misc:
- Subpixel bugs. (leviw)
- Burning down Mac OS 10.10 test expectations, down 900 lines last week
alone! (leviw)
Logistics:
- leviw goes on vacation (that thing in the desert) on Wed + two weeks.
- skobes goes on vacation on Thu + one week.
- szager on vacation Wed and Fri. Back in SF on Mon.
- esprehn, ojan, ikilpatrick in Paris for CSS F2F meeting this week.
