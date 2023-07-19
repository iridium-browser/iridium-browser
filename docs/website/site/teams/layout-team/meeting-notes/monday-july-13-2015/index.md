---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-july-13-2015
title: Monday, July 13, 2015
---

Updates since last meeting (on Monday, June 22nd):

Scrolling (skobes, szager) \[crbug.com/417782\]

- Working on pinch viewport. (skobes)

- Has fix for double scrollbar issue and potential fix for under-

pinch. (skobes)

- Helping out with scrolling, trying to figure out how to set, identify,

and honor scroll bounds for the root layer. (szager)

CSS Flexbox (cbiesinger) \[crbug.com/426898\]

- No update, cbiesinger on vacation.

CSS Grid Layout (svillar) \[crbug.com/79180\]

- Updated auto tracks to use automatic minimums. (svillar)

- Added support for infinitely growable tracks. (svillar)

Region-based multi-column support (mstensho) \[crbug.com/334335\]

- Looking into nested multicol, which is required for printing. Hope to

move on to writing code for it soon. (mstensho)

- Made a couple of changes to how we handle overflow at column

boundaries that fixed Wikipedia but broke Google Plus. Should be

possible to have both working at the same time. (mstensho)

Add API for layout (leviw, pilgrim) \[crbug.com/495288\]

- First part of the layout API landed and did not result in any

performance regression. (pilgrim)

Isolate core/fetch (japhet) \[crbug.com/458222\]

- Working on refactoring how Blink revalidates cached results. (japhet)

- Looking into a top user complaint; sometimes we show a blank page

without any error message or indication that it went wrong. (japhet)

CSS Test Suites (jsbell, kojii)

- Continued work to import more w3c tests, now works with the w3c test

server so that they won't require modifications any more. Looking for

volunteers to help run and validate them. (jsbell)

- Will unblocks tkent and kojii importing tests for their respective

feature work. (jsbell)

Text (eae, drott, kojii)

- Looking into font fallback and trying to understand GlyphPageTreeNode;

it serves a lot of different uses cases and we need to understand

exactly what it does and why. (drott)

- WebKit has made large changes to fallback and GlyphPageTreeNode since

the fork.

- Will write up a quick doc on fallback and then decide if we want to do

a set of quick fixes or do a larger overhaul of how we do font

fallback. (drott)

- Finally got around to finish my revamp of the complex text code and

the results are in, a 3.5x - 28x speedup across all platforms. With

the slower platforms improving the most. The results are from a micro

benchmark but given the amount of time spent in text layout on real

world websites it should have a very noticeable impact. (eae)

- Looking into what features we need to add to the complex text path in

order to remove the simple path entierly. So far it looks like adding

support for tab characters is the only major things missing. (eae)

Misc

- Profiling with perf on Android appears to be broken, working on fixing

the run-benchmark-scripts. (wkorman)

- Bringing up the Mac 10.10 bots. (joelo).

Logistics

- pilgrim on vacation this week.

- cbiesinger on vacation this week.
