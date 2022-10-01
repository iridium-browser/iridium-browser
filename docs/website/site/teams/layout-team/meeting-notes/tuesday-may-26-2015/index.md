---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: tuesday-may-26-2015
title: Tuesday, May 26, 2015
---

Highlights

- New multicol implementation shipped!

- Line layout in layout units landing this week!

Updates since last meeting (on Monday, May 18th):

Performance Tracking (benjhayden)

- Continuing work on trace viewer.

Scrolling (skobes) \[crbug.com/417782\]

- Root layer scrolling broke &lt;body&gt; stretching to viewport in quirks

mode; fix is waiting on code review.

- Page scale factor is a bigger concern: page scaling changes

content size relative to scroll bounds, which doesn't work with root

layer scrolling as currently implemented. Need to teach LayoutView /

DPLScrollableArea how to apply the page scale factor, which could get

messy.

Line Boxes (szager) \[crbug.com/321237\]

- Ready to land, will require a ton of rebaselines.

Flexbox (cbiesinger) \[crbug.com/426898\]

- Discussions around absolut positioning within flexbox.

- Updated handling of min-width/min-height.

List marker refactoring (dsinclair) \[crbug.com/370461\]

- List marker patches in M44 branch. Yay!

Menu list refactoring (dsinclair) \[crbug.com/370462\]

- Menu list changes landed, with hack to use empty text node instead

of BR for line breaks to ensure proper alignment.

Fullscreen (dsinclair) \[crbug.com/370459\]

- Taking over work to update fullscreen implementation to not modify

layout tree during layout. Building on previous work by Julien and

James among others.

Misc Warden (dsinclair, pilgrim)

- Contining work to slim-dopwn LayoutObject. (pilgrim)

- Plan to start helping with WTF to Base conversion this week. (pilgrim)

Region-based multi-column support (mstensho) \[crbug.com/334335\]

- New multicol implementation shipped!

Text

- Working on justification crash issue. (kojii)

- Investigating orthogonal writing modes issues. (kojii)

- Unicode variation selectors support. (kojii)

- Shaping AAT fonts broken on Mac for certain scripts. (eae)

- Continuing work on complex text performance. (eae)

Importing CSS Test Suites (kojii)

- Investigating importing CSS Writing Modes test suites

Removing DeprecatedPaintLayer (chadarmstrong)

- Intern Chad Armstrong started, is going to be working on killing

DeprecatedPaintLayer.

- Design doc:
https://docs.google.com/document/d/1YApHVA0hJDWeFGxHPpJ6J5JM5Wcz2h3s24KPSFp8VMg/edit?usp=sharing
