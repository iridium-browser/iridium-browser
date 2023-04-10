---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-august-31-2015
title: Monday, August 31, 2015
---

Updates since last meeting (on Monday, August 24th):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Tracked down issue with scroll bounds change and determined that the
failing tests where due to a bug in the old implementation. New test
results are correct. Will require a rebaseline. (szager)
- Still having some problems with the flexbox scollbar change, need to
figure out why the fix is causing a perf regression. Happy with code
change and convinced the fix is correct and that the new behavior is
better. (szager)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Unprefixed sizing keywords.
- Continuing work to bring implementation up to spec.
- Various bug fixes.
CSS Grid Layout (svillar) \[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last week.
Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]
- Continuing work on multicol balancing and nesting.
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Landed several API patches. Estimate a couple of days worth of work
remains. (pilgrim)
Text (eae, drott, kojii)
- Working on CJK fonts pri 1 issues. (kojii)
- Working on complex path blockers and recent regressions. (kojii)
- Resolved all open spec issues in Writing Modes in CSS F2F. (kojii)
- Investigating unprefix planning for Writing Modes. (kojii)
- Continued on shaper driven segmentation work. Have working prototype.
Quite a bit of work remains to handle all edge cases, vertical text
and emphasis marks. (drott)
- Various Windows font fallback fixes. (eae)
Misc:
- Ripping out linebox contain implementation following last weeks intent
to deprecate and remove. (wkorman)
- Misc bug fixes, mostly related to rounding and line boxes. (szager)
- Plugin persistance was re-landed last week, this allows plugin state
to persist across reattach and is a big win. (dsinclair)
Logistics:
- leviw on vacation (that thing in the desert) this week and next.
- skobes on vacation this week.
- szager back from Seattle.
- esprehn, ojan, ikilpatrick in Paris for CSS F2F meeting until mid week.
- cbiesinger out until Wednesday.
