---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-march-30-2015
title: Monday, March 30, 2015
---

We had Rune Lillesveen (rune) and David Vest (davve) from Opera join the

meeting this week in an effort to better coordinate our work. We also had

Ian Killpatrick (ikilpatrick) from the Chrome Sydney team join us.

The massive Rendering -&gt; Layout rename project is done, thanks to

everyone who helped out and to Dan in particular who did most of the

heavy lifting!

The new multi-column implementation was enabled for tests and exposed as

an experimental web platform feature on Friday! Congrats Morten!

Updates since last meeting (on Monday, March 23rd):

Performance Tracking (benjhayden)

- Continuing work on LayoutAnalyzer and counters. Counters will be

exposed via trace events allowing us to see the number of layouts on a

per element type basis.

- Working on adding a command line flag for android to allow metrics to

be collected on an attached phone.

Scrolling (skobes) \[[crbug.com/417782](http://crbug.com/417782)\]

- Fixed the scroll animation test that was failing on the leak bots.

- Fixed a bug with scrolling to fixed-position #anchor targets.

- Still tests failing that I need to dig into, and I need to add more

virtual test suites as right now it is only running fast/scrolling.

Rename Rendering -&gt; Layout (dsinclair)
\[[crbug.com/450612](http://crbug.com/450612)\]

- Done. All files have been moved and renamed. The rendering/ directory

is gone!

First letter refactoring (dsinclair)
\[[crbug.com/391288](http://crbug.com/391288)\]

- Fixed first-letter hit testing, discovered a couple of new bugs.

List marker refactoring \[[crbug.com/370461](http://crbug.com/370461)\]

- Getting back to list-marker work, will turn the marker into a ::marker

psedo element. \[crbug.com/457718\]

Measure API (jchaffraix)

- Been thinking about supporting measure out-of-tree and trying to come

up with a good strategy. So far none of the approaches tried have

panned out. Will continue to experiment and evaluate ideas to guide

future work.

Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]

- Finished implementing spec changes for min-width auto.

- Started working on positioning of position: absolute elements.

- Working on importing w3c test suite for flexbox.

Line Boxes (szager) \[[crbug.com/321237](http://crbug.com/321237)\]

- Finished going through the layout test failures for Glenn's patch, and

landed a few patches to fix tests. All of the remaining test failures

are OK to rebaseline.

- Ran the patch through the perf bots, and the results were not as I had

hoped. There appeared to be significant performance regressions in the

blink_perf.layout test suite.

- Working on eliminating wrapper classes (FloatWillBeLayoutUnit) to test

theory that they explain at least part of the regression.

- Found a number of places where we use the wrong unit, we need to

decide exactly where to draw the line between LayoutUnit and float.

Isolate core/fetch (japhet) \[[crbug.com/458222](http://crbug.com/458222)\]

- Continuing work to isolate core/fetch from the rest of core.

Page scale handling (bokan) \[[crbug.com/459591](http://crbug.com/459591)\]

- Remove old pinch-zoom paths from Blink.

Blink componentization (pilgrim) \[[crbug.com/428284](http://crbug.com/428284)\]

- Moving things from core to modules; core/storage and core/timing.

Text (kojii, wjmaclean, eae, rune)

- Continued work on text iterator cleanup, fixed naming to be more

descriptive. (wjmaclean)

- Working on a change that pulls out the part of TextIterator that

handles text from the part that handles traversing. (wjmaclean)

- Profiling line breaking. (eae)

- Started working on design doc for removing simple text. (eae)

- Fixed text overflow box bug that blocked the Slimming Paint work.

(eae)

- Fixed an assertion problem with baseLayoutStyle which lead to

finding/reporting a problem causing unnecessary layouts.

\[[crbug.com/471079](http://crbug.com/471079)\] (rune)

- Fix vertical flow regression when a major web font site provides

broken tables. (kojii)

- Default to complex path more cases in vertical flow. (kojii)

- Working with W3C and internal experts for bi-di and vertical flow

issues. (kojii)

Style resolution (rune)

- Cache element indices for :nth-child and :nth-last-child selectors.

Content sizing (davve)

- Working on content sizing/intrinsic size for HTML and SVG.

Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]

- New multi-column implementation enabled as experimental web platform

feature!

Standards (ikilpatrick, slightlyoff)

- Pushing the layout portion of w3c houdini forward in preparation for

the next meeting by prototyping. (ikilpatrick)

ClusterFuzz

- Trying to reduce an obscure clusterfuzz crasher in RenderBlockFlow.

(wkorman)

Discussions:

\[ cbiesinger's work to import the w3c test suite prompted a discussion

about w3c tests. \]

&lt;slightlyoff&gt; Are we mostly writing blink specific layout tests or w3c

web platform tests today?

&lt;eae&gt; For regression and performance work we use blink specific layout

tests pretty much exclusively. For feature work we're trying to move to

a world where we use web platform tests tests but we're not there yet.

&lt;jsbell&gt; Dirk (dpranke) started work in tooling to support w3c web

platform tests, I have headcount for an engineer to take this over and

work on improving the tooling.

&lt;jsbell&gt; Once better tooling and procedures are in place we should start

"strongly encouraging" people to write web platform tests for new

features.

&lt;slightlyoff&gt; Can we do that today?

&lt;jsbell&gt; It is too too painful to do today, tooling needs to be improved

first.
