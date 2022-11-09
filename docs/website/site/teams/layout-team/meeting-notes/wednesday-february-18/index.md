---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: wednesday-february-18
title: Wednesday, February 18, 2015
---

Attendees: benjhayden, cbiesinger, dsinclair, eae, jchaffraix, nduca, szager,
skobes, slightlyoff, hartmanng

Status updates:

Performance Tracking (benjhayden)

- Finished profiling most of the key silk test cases on android, added

more graphs and information to the profiling doc.

- Next up, classifying types of layout.

Scrolling (skobes) \[crbug.com/417782\]

- Smooth scrolling enabledd for RenderLayerScrollableArea.

- Fixed position elements.

win_blink flakiness (dsinclair)

- win_blink_rel now among the better bots in terms of flakiness.

- Slowly re-enabling tests, monitoring flakiness.

Rename Rendering -&gt; Layout \[crbug.com/450612\] (dsinclair)

- Trying to rename RendereBlock to LayoutBlock. Involves 30k files which

rietveld does not like. Uploading for reviews cases the upload command

to crash.

List markers (dsinclair)

- Trying to figure out how to deal with an ancestor being moved.

Measure API (jchaffraix)

- Working on a number of prototypes for measure together with Ian

Kilpatrick.

- github repo for prototypes. To allow layout callbacks a script

execution context might be needed. Runs js in snadbox since we cannot

modify the DOM. Would not have even read-only access to the DOM.

Needs to be able to access render tree in a safe way.

- Defining an API for the render tree, houdini style.

- slightlyoff would prefer to not add a new execution context that is

not workers. jchaffraix and slightlyoff to talk offline.

Line Boxes (hartmanng, szager) \[crbug.com/321237\]

- No real progress last week. Blocked on higher priority work to fix

high profile regressions.

Flexbox (cbiesinger) \[crbug.com/426898\]

- Working on updating flexbox implementation to match latest spec

revision.

Text (kojii, eae)

- Made RenderCombinedText less obstructive. 4 if-RenderCombinedText

removed, 1 moved to less frequent path. (kojii)

- Worked with dominik.rottsches to make vertical flow performance

test. (kojii)

- Started analyzing complex path performance. It looks like

characteristics varies by scripts and values of properties. I'll look

into CJK vertical flow first. (kojii)

- Fixed two text regressions on Mac causing incorrect rendering and/or a

render crash (due to an ASSERT). Required a Harfbuzz change and roll.

(eae, behdad)

- Added support for emoticons and emoji on Windows. (eae)

Line layout (szager)

- Fixed bug where shrink-wrapping of some content failed on Facebook,

caused by imprecision in LayoutUnit to floating point conversions.

Writing a test for this was tricky as layout tests run without

subpixel text on most platforms. (szager)

Going over the list of OKRs and discussing each one:

\[ Have telemetry benchmark suite running key_silk_cases monitoring layout times
with less than 1% variation between runs. \]

\[ Add 3 real-world pages with lots of layout to key_silk_cases. At least two of
which are mobile oriented. \]

&lt;benjhayden&gt; A couple of cases with large variance, most have low
variance.

&lt;benjhayden&gt; Layout measurement should fix this in most cases. Some tests
have a bi-modal behavior, triggering an extra layout in some cases.

&lt;nduca&gt; We want a link on the layout team site to a set of graphs by the
end of the quarter.

&lt;eae&gt; So we're looking pretty good, do we think we can get there by the
end of the quarter?

&lt;benjhayden&gt; Yes.

\* on track \*

\[ Speed up some key_silk_cases tests. \]

&lt;eae&gt; We've made a number of small improvements to complex text
performance and have ongoing work that should result in a 10% improvement.

&lt;eae&gt; Any other ongoing performance work?

\* at risk \*

\[ Create a Measure API prototype and write-up of lessons learned. \]

&lt;jchaffraix&gt; On second or third prototype, have a better understanding
since the Sydney convergence.

&lt;jchaffraix&gt; Github repo with prototype, working on write-up.

\* on track \*

\[ Support natural layout animations (subpixel layout during animation) \]

&lt;eae&gt; No real progress, have rough proof of concept prototype but nothing
concrete.

&lt;eae&gt; Might miss.

&lt;nduca&gt; Should be explicit if it wasn't a priority and what we did
instead.

\* at risk \*

\[ Finish root layer scrolling. \]

&lt;skobes&gt; Making progress, fixing issues as discovered.

&lt;skobes&gt; Handful of hard problems, some risk of slipping into Q2.

\* on track \*

\[ Move line layout to LayoutUnit. \]

&lt;hartmanng&gt; Distracted by P1 bugs, have made some progress.

&lt;hartmanng&gt; Complications with full-page zoom implementation.

&lt;hartmanng&gt; Should be possible if we get time to work on it.

&lt;szager&gt; Agreed.

\* on track \*

\[ Triage all clusterfuzz asserts and fix 50% of them. \[cbiesinger\]

&lt;cbiesinger&gt; Starting to realize that 50% might have been a bit
optimistic. Not all are reproducible.

&lt;cbiesinger&gt; Without minimized test cases it is really hard, given up on a
subset of them (without test cases).

&lt;eae&gt; What would a realistic goal be for Q1?

&lt;cbiesinger&gt; Triaging all reproducible ones and fixing the ones with
minimal test cases would be feasible.

\* at risk \*

\[ Have bugs automatically filed for new clusterfuzz asserts. \]

&lt;cbiesinger&gt; Need to work with someone on the clusterfuzz team, should be
easy.

\* on-track \*

\[ Render tree modifications during layout. \]

&lt;dsinclair&gt; first-letter is stabilized, list-marker is a lot closer.

&lt;dsinclair&gt; One tricky outstanding issue regarding descendants
(list-marker).

&lt;jchaffraix&gt; Full screen is a road block. Re-attach destroys renderer,
restarts plugins. Currently we do no reattach. We need to add a hoisting
mechanist to make it work. Prototype patch among those lines but it does break
plugins. Needs more work and is complicated.

&lt;jchaffraix&gt; Have not had time to dig into it, need to fix path.

&lt;dsinclair&gt; single-item-menu won't be done this quarter.

&lt;dsinclair&gt; On track according to plan, decent amount of risk.

\* on track \*

High level discussion:

&lt;dsinclair&gt; Renaming rendering to layout has been sucking up a lot of time
for everyone.

&lt;eae&gt; As has dealing with regressions and blocking bugs.

&lt;nduca&gt; Are we spending more or less time on work not captured in the OKRs
than expected?

&lt;eae&gt; Was expecting about a 50%/50% split between OKR work and
high-priority bug fixes. So for this quarter we've spent a bit more time on
bugs/regressions than expected.

&lt;nduca&gt; For Q2 OKRs should capture that work.
