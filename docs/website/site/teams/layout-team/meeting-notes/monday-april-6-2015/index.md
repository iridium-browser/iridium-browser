---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-april-6-2015
title: Monday, April 6, 2015
---

Most offices had the day off due to Easter so this update is limited

to the SFO, MTV and NYC offices.

Updates since last meeting (on Monday, March 30th):

Performance Tracking (benjhayden)

- About to land a LayoutMetric rewrite that should give more details.

- Started working on LayoutAnalyzer UMA histograms after discussion

with chrishtr.

Scrolling (skobes) \[crbug.com/417782\]

- Fixed a positioning bug affecting scrollToAnchor.

Measure API (jchaffraix)

- Started thinking about the grand block layout refactoring and smaller

cleanup tasks that can be done in parallel.

Flexbox (cbiesinger) \[crbug.com/426898\]

- Working on positioning of position: absolute elements.

- Working on importing w3c test suite for flexbox.

Blink componentization (pilgrim) \[crbug.com/428284\]

- Started working on moving clipboard from core to modules.

Text (kojii, wjmaclean, eae, rune)

- Removed incorrect optimization for fixed-pitch fonts. (eae)

- Renamed SurrogatePairAwareTextIterator to UTF16TextIterator and added

support for multi character glyphs which was previously handled separately.
(eae)

Cleanup/code health

- Removed canHaveGeneratedChildren from RenderObject and turned it into

a static function. (pilgrim)
