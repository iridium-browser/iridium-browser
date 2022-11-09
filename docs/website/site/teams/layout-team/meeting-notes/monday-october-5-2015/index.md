---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-october-5-2015
title: Monday, October 5, 2015
---

Updates since last meeting (on Monday, October 5th):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Landed change that enables coordinated scrollbars everywhere. (skobes)
- Landed unit test fixes for root layer scrolling. (szager)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Worked on the flexbox layout tests, in the W3C repository. The W3C
repository is the source of truth that we pull in periodically.
- Misc flexbox bug fixes.
CSS Grid Layout (svillar, jfernandez, rego)
\[[crbug.com/79180](http://crbug.com/79180)\]
- Reviewed several grid layout patches. (svillar)
- Working on grid container height sizing when there are min|max
content constraints and min|max height restrictions. The CL is
ready for review after revamping the original design. (svillar)
- min-width|height: auto fixes for vertical writing modes. (svillar)
- Working on definite size detection for abspos items. (rego)
- Fixed abspos items behavior with implicit lines and unknown named
lines. (rego)
- Content and self alignment fixes for spanning items. (jfernandez)
- Implemented auto margin alignment for grid items. (jfernandez)
- Added 0fr support for track sizes. (jfernandez)
Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]
- Fixed pagination strut propagation for blocks.
- Removed unused clearPaginationInformation from LayoutState.
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Landed two layout API changes that added a couple of methods missing
form the API around scaling. (pilgrim)
- Converted inline text box painters to not use the line layout api but
also not call any methods on inline text box. (pilgrim)
- Houdini spec preparation work, supporting style team. (leviw)
Text (eae, drott, kojii)
- Fixed selection painting on high-DPI where before we would clip the
selection rect incorrectly causing thin white lines between certain
words due to rounding. Removed clipping entirely as it is no longer
needed in a post selection-gap-painting world. (eae)
- Continued on layout tests failures for shaper driven segmentation,
found issue in HarfBuzz relating to end of run handling for webfonts.
Working with behdad to resolve it. (drott)
- Discussion on blink-dev around custom text rendering by exposing
access to fonts. (drott)
- Will continue working on remaining issues for shaper driven
segmentation this week. Outstanding questions around small caps
handling. (drott)
- Selection highlighting in vertical writing mode (ltr) relating to
selection gap painting removal. (wkorman)
Misc:
- Fixed float stable blocker. (szager)
- drougan looking for good starter bugs.
- Working on test infrastructure for runtime enabled features to ensure
that features are reset between test runs by auto generating the js
hooks rather than having everyone roll their own. (wkorman)
- Misc stable & beta blocker bugs. (leviw)
Logistics:
- eae & leviw in Oslo this week, visiting Opera and then head to
Helsinki to work with drott early next week.
- pilgrim on vacation Wed-Fri.
- cbisinger on vacation next Monday.
- drougan on recruiting trip rest of week.
