---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-august-10-2015
title: Monday, August 10, 2015
---

Updates since last meeting (on Monday, August 3rd):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Fixed setPageSclaeFactor and anchor root layer scrolling. (skobes)
- Turned on scroll restoration tests for root layer scrolling. These
tests ensure that scroll positions are correctly restored on history
navigation. (skobes)
- A Samsung contributor has agreed to help with custom scrollbar work.
- Containing work cleaning up DepricatedPaintLayerScrollableArea in an
effort to have sane scroll bounds for the root layer. (szager)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Helped szager with a flexbox scrollbar bug that triggered unspeced and
untested behavior.
- Sent intent to ship and intent two unprefix and will follow up on
those this week. The intent to unprefix looks ready to go ahead with.
- Had meeting with tabatkins about absolute position for flex items.
- Flexbox bug triage.
CSS Grid Layout (svillar) \[[crbug.com/79180](http://crbug.com/79180)\]
- Fixed bug where grid containers reported the wrong preferred width.
Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]
- First patch for nested multicol is just around the corner.
- Blocked on reviews for multicol improvements.
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Successfully posted several API patches after help from Levi. Converted
several more bits of line box and descendants to use the new line
layout API. Nothing broken. yay! (pilgrim)
- Some weird edge cases and debugging code that relies on printing out
layout objects. Ongoing discussion. (pilgrim)
Text (eae, drott, kojii)
- Follow up from text workshop in Helsinki, Behdad and Drott working on
optimizing the Blink/HarfBuzz integration.
- Fixed space normalization bug where cr/lf characters where incorrectly
collapsed. (eae)
- Debugging several crashers in line layout. (eae)
Logistics:
- drott on vacation until Friday.
