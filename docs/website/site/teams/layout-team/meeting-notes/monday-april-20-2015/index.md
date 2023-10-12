---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-april-20-2015
title: Monday, April 20, 2015
---

Performance Tracking (benjhayden)
- Audit trace view for layout analyzer.
- Split LayoutAnalyzer::LayoutBlockRectangleChanged into X/Y/Width/
HeightChanged.
- LayoutTreeAsText as JSON.
Scrolling (skobes) \[[crbug.com/417782](http://crbug.com/417782)\]
- Crashing bugs, fixed use after free in FrameView.
- Issue with the interaction between painting the viewport and view
scrolling.
- Don't force layout for scrollTo(0, 0). (rune)
List marker refactoring (dsinclair)
\[[crbug.com/370461](http://crbug.com/370461)\]
- List element pseudo elements, marker inside wasn't getting the
correct side for the parent container, need to notify parent.
- Found issue where if you modify the subtree you may need to reinsert
the elements as it needs to be in a specific place of the tree, i.e.
inside a form element.
Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Landed min-width auto, handful of regressions as expected, burning
down one by one.
- Identified a performance regression, computeLogicalWidthUsing is
slow, caused a 25% slowdown for flexbox as it calls the compute
method more frequently. Have a fix that should be landed by now.
Line Boxes (szager) \[[crbug.com/321237](http://crbug.com/321237)\]
- Going after perf regressions. Got a CPU profile and did some work to
allow perf tools to run on render process only in multi process
chrome.
- Have made progress on performance, still some regressions for floats.
Blink componentization (pilgrim) \[[crbug.com/428284](http://crbug.com/428284)\]
- Slimming down render object (aka layout object), expect to continue
this week.
Text (eae, rune)
- Started work on implementing a word-cache for complex text shaper,
first step is to add unit tests for existing and desired behavior.
(eae)
- Don't clear FontFaceCache if no @font-face were removed. (rune)
Style resolution (rune)
- Various selector matching issues.
Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]
- New multi-col implementation was reverted due to clusterfuzz issues,
working on identifying and fixing the potential problems. Aim to
re-land before the end of the week.
Layout refactoring
- jchaffraix, esprehn, ojan working on coming up with concrete proposals
for changes to how we do layout based on ideas and concepts from the
last few weeks of discussions on the topic. Will be shared with wider
blink team.
