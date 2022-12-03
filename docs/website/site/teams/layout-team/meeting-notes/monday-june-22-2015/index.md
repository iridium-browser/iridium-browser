---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-june-22-2015
title: Monday, June 22, 2015
---

Updates since last meeting (on Tuesday, June 16th):

Scrolling (skobes, szager) \[crbug.com/417782\]

- Fixed handling of overflow clip for root layer.

CSS Flexbox (cbiesinger) \[crbug.com/426898\]

- Working on intrinsic sizes, found a problem with the spec that needs

investigation. Handling of form controls (like calendar picker) is

problematic.

- Hope to finish up absolut positioning handling this week.

CSS Grid Layout (svillar) \[crbug.com/79180\]

- Blocked on code reviews. (svillar)

- Performance optimizations for alignment. (jfernandez, svillar)

- cbiesinger to help with reviews. (cbiesinger)

Region-based multi-column support (mstensho) \[crbug.com/334335\]

- Still stabilizing multicol.

- Intend to start working on nested multicol soon, so that we can

print.

Add API for layout (leviw, pilgrim) \[crbug.com/495288\]

- Continuing work on adding API around block layout. Managed to track

down and solve the performance problems assoscaited with it (turns

out it was caused by the overhead of extra non-inlined constructors/

deconstructors). (pilgrim)

Isolate core/fetch (japhet) \[crbug.com/458222\]

- Continued work to separate core/fetch from core and considering finer

grained separation per resource type. I.e css resource handling

specific logic could be moved to the css directory.

Text (eae, drott, kojii)

- Removing dependency on glyph page tree node from complex path to allow

it to be removed entierly once the simple path is removed. Trying to

figure out exactly how it is used and where in order to break the

dependency. (drott)

- Unicode variation selectors support fixed for Win/Mac. Linux needs our

harfbuzz in unofficial builds. (kojii)

- Investigating bundling our harfbuzz for unofficial Linux. (kojii)

- Investigating abspos issues in vertical flow. (kojii)

- CL adding support for word-by-word shaping up for review, performance

looks very good, will try to land this week. (eae)

Layout Bugs:

We currently have 1581 open bugs for the Cr-Layout label, down from 1592

last week.

- Untriaged: 468 (-7)

- Priority 0: 0

- Priority 1: 134 (-1)

- Priority 2: 957 (-1)

- Priority 3: 19

We need to get the number of bugs down so from now on each weekly update

will include the number of bugs for each category and the delta from the

week before.

Logistics:

- szager OOO all week.

- eae gardener Wed-Thu.

- OKR meeting on Thursday.
