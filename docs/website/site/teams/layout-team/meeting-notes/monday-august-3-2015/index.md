---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-august-3-2015
title: Monday, August 3, 2015
---

Thanks for running the show last week jsbell. Now back to our regularly

scheduled programming.

Updates since last meeting (on Monday, July 27th):

Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]

- Fixed under-painting issue on android. (skobes)

- Fixed a regression causing double scrollbars caused a previous bug

fix. (skobes)

- Identified that scrollable elements with animated gifs will do a

full re-layout even if it's composited. (skobes)

- Fixed a hairy flexbox scrollbar bug where the size of the scrollbar

wasn't taken into account when computing the flexed size. (szager)

- Containing work cleaning up DepricatedPaintLayerScrollableArea in an

effort to have sane scroll bounds for the root layer. (szager)

CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- Had a meeting with CSS flexbox spec authors last week and leared that

some areas of the spec that I've been working on implementing might

change further. Will follow up this week.

- Triaged a bunch of flexbox bugs.

- Up-streaming flexbox tests have proven harder than expected, might

take a week or two to resolve.

CSS Grid Layout (svillar) \[[crbug.com/79180](http://crbug.com/79180)\]

- cbiesinger have been helping with reviews.

Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]

- Doing a lot of cleanup and bug fixing work.

- Containing working on support for nested columns, progressing nicely

but nothing landed yet.

Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]

- Working on moving layout box and descendants over to use the API.

(pilgrim)

Isolate core/fetch (japhet) \[crbug.com/458222\]

- No update.

Text (eae, drott, kojii)

- Text workshop in Helsinki, Behdad and Drott working on optimizing the

Blink/HarfBuzz integration.

- Completed tests for styling and selection, found a few issues as a

result and have been working on fixing those. (drott)

- Unicode range matching is next. (drott)

Misc:

- Continuing to talk to people about position observer, got lots of

feedback, mostly positive. (slighgtlyoff)

Logistics:

- kojii on vacation this week.

- drott on vacation this Wednesday until next Friday.

- cbiesinger in SF this week.
