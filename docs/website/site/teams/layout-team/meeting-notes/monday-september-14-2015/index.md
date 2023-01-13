---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-september-14-2015
title: Monday, September 14, 2015
---

Updates since last meeting (on Monday, August 31st):

Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]

- Landed flexbox change for overflow auto, it's causing a regression

on Mac only in how scrollbars are pained. Hope to sync up with skobes

about that today and resolve it. Other than that no problems and looks

like it'll stick. (szager)

- Landed change running a comb through the scorll bounds code. Should

be pretty reliable for getting minimum and maximum scoll positions

now, while it wasn't before. Likely to have fixed a bunch of off-by-

one scroll bound bugs. (szager).

- Intent to fix regreision and talk about next step for root layer

scrolling with skobes this week. (szager)

- sataya.m from Samsung has fixed almost all of the scrollbar tests,

these are mostly related to custom scrollbars. (skobes)

- Trying to figure out coordinated scrollbars this weeks, turn on for

just the main frame and only for certain platforms. Would be nice to

use this more broadly. (skobes)

CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- Need to re-think approach to deprecating sizing keywords due to leaks

in CSS parser which resulted in the patch to be reverted.

- Will sycn up with ojan re f2f. (cbiesinger)

- At CSS face-to-face it was decided to standardize on chrome-impl of

table fixup (i.e. not do it) which will save us some work. (ojan)

CSS Grid Layout (svillar) \[[crbug.com/79180](http://crbug.com/79180)\]

- No update since last week.

Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]

- Fixed handling for unbreakable blocks at top of a column/page.

- Updated handling of style changes to ensure they happen in tree order

for multicol.

- Fixed min-height to not have an effect if content is taller than

multicol and height is set to auto.

Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]

- Still working on line layout api, down to some asserts and debug only

code before we can get rid of the LayoutObject in InlineBox. (pilgrim)

- API is as broad as it needs to be, has all the relevant methods,

thinking about next steps. (pilgrim)

Text (eae, drott, kojii)

- Continued work on shaper driven segmentation: (drott)

- Implemented an iterative approach so that for each font we can

collect all segment supported by the font.

- Started to work on system font fallback, with the font fallback

iterator now also able to collect and fallback on system fonts.

- Sent out mail about approach to modernazing system font fallback,

on Windows 8.1+ and Mac there are sane fallback APS we could use.

Android has a fallback XML file that is resonable and on linux

fontconfig provides the necesarry information but returns way

too many fonts. (drott)

- Thinking about vertical text, needs reimplementaiton. One idea is to

merge script iteartion with orientation. Trying to implement run

segmenator which uses the script iteator and an orientation iteratior.

Will continue workin on it this week. (drott)

- Working on complex path blockers and recent regressions. (kojii)

- Working on CJK font fallback issues. (kojii)

- Updating CSS Writing Modes spec for resolutions from F2F. (kojii)

- Investigating unprefix planning for Writing Modes. (kojii)

- Writing Modes spec issues/edits, unprefix planning, and tests. (kojii)

- Discussing with drott@ on run segmentation for vertical flow. (kojii)

- Looking into high-priority fonts issues. (kojii)

Misc:

- Removed webkit linebox contained. (wkorman)

- Happy about juliens documentation improvements. (wkorman)

- Reverted patch to remove old containg block logic as it turns out it

was still needed. Will revisit and try again with tests. (ojan)

- Fixed handling of floats for blocks that establishes a new formatting

context. (mstensho)

- Fixed float margin logic to not collapse margins on page or column

boundaries. (mstensho)

Logistics:

- leviw in Amsterdam for a Polymer summit, back on Thursday.
