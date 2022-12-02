---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: '20150107'
title: Q1 OKRs - Wednesday, January 7, 2015
---

Attendees: benjhayden, cbiesinger, dsinclair, eae, jchaffraix, nduca, szager,
skobes

... Pleasantries, going over the list of proposed OKRs ...

\[ Discussing the proposed "Have ten layout benchmarks with less than 1%
variation between runs" OKR \]

&lt;benjhayden&gt; For the benchmark OKR are you referring to to descriptive
benchmarks?

&lt;eae&gt; Not necessarily, the goal is to have reliable benchmarks to guide
our work. Short term that likely means silk-style measuring layout times.

&lt;jchaffraix&gt; We do want to move to descriptive benchmarks (new type of
benchmark using recipes) eventually.

\[ Discussing the proposed "Improve two benchmarks by 10%" OKR \]

&lt;cbiesinger&gt; How will we choose which tests to improve by 10%?

&lt;jchaffraix&gt; Any of the tests really.

&lt;eae&gt; Will be guided by our findings when adding benchmarks and doing
profiling.

&lt;cbiesinger&gt; We should make the objective more precise, what kind of
benchmark?

&lt;nduca&gt; Make it two key_silk cases

\[ Discussing the proposed "Support subpixel layout during animation" OKR \]

&lt;skobes&gt; What is involved with subpixel layout during animation?

&lt;eae&gt; Today we snap to CSS pixels during layout, which ensures crisp
rendering, but during animations this causes animations to move on full pixel
boundaries which isn't ideal.

&lt;nduca&gt; It's all about enabling natural animations.. without that layout
animations looks bad.

&lt;eae&gt; ...really bad.

&lt;dsinclair&gt; Do we have any idea how hard it will be?

&lt;eae&gt; Technically all we need to do is not pixel-snap during animations
but doing that without complicating the code and introducing a separate code
path for animations could be tricky.

&lt;dsinclair&gt; We don't want to add "if (animation)" checks everywhere.

\[ Discussing the proposed "Move line layout to LayoutUnits" OKR \]

&lt;eae&gt; How is the work to move line layout to LayoutUnits coming along, is
finishing it in Q1 a realistic goal?

&lt;benjhayden&gt; Should be doable, making good progress. Stefan has started
helping out.

&lt;dsinclair&gt; A lot of it is figuring out rounding errors and burning down
the list of issues.

\[ Discussing the proposed "Prototype and propose Measure API" OKR \]

&lt;nduca&gt; Is the Measure API OKR about Element.measure

&lt;eae&gt; It is really too early to tell. We don't know Element.measure is the
right API yet, hence the need for a prototype.

&lt;jchaffraix&gt; The idea is to create a prototype, possibly based on
Element.measure, implement it and see if it solves the problem.

&lt;nduca&gt; So basically talk to Tab, spec the API and send out an "Intent to
Implement"? It doesn't have to be faster, it just needs to be separate from
layout.

&lt;eae&gt; An intent to implement might be premature, we need to figure out if
we can support the proposed API first and if it actually works as expected.

&lt;nduca&gt; We have a lot of requests for this and it is really important.
Nothing is sure or has been decided but we need to do something in the space.

&lt;eae&gt; Julien and I talked about it earlier and think it is important to
have a prototype and have someone from Docs or another client experiment with it
to make sure it fits their use cases.

&lt;nduca&gt; This isn't for Docs or desktop apps, they already have working
solutions and use native apps on android. It is more about animations between
layout states.

&lt;nduca&gt; Check with \[ internal team, redacted \]

&lt;nduca&gt; This is important for people doing transition animations from one
layout to another. They have to do the new layout and measure it, doing that
today requires jiggling and hacks when all they need to know is the final state.
We don't have to make it faster, just needs to support doing the measurements
without invalidating the universe.

\[ internal discussion, redacted \]

&lt;eae&gt; If that is the end goal there are other ways we could approach it.

&lt;jchaffraix&gt; Let's change the OKR to "Create a prototype and write-up of
lessons learned"

\[ Discussing the proposed "Finish root layer scrolling" OKR \]

&lt;eae&gt; Is finishing root layer scrolling this quarter a realistic goal?

&lt;skobes&gt; Making good progress, should be able to finish this quarter.

\[ Discussing the proposed ClusterFuzz OKRs \]

&lt;skobes&gt; How many clusterfizz asserts are you seeing now?

&lt;cbiesinger&gt; About 4 pages, burning down, not sure if more will surface.
Some are on the chrome side and out of scope but there are a good number of
them.

&lt;cbiesinger&gt; Actually, make that 18 pages, 10 per page...

&lt;eae&gt; The OKR might be a bit too optimistic then, just triaging that many
will take quite a while.

&lt;dsinclair&gt; Triage and fix 50%?

&lt;cbiesinger&gt; More reasonable.

&lt;dsinclair&gt; Can we do the auto triage?

&lt;cbiesinger&gt; Abhishek said the ClusterFuzz team would help with that.

&lt;jchaffraix&gt; How about team warden specific OKRs?

&lt;eae&gt; We have 'scrolling' and 'line layout'. Do we need more?

&lt;julien&gt; Ok. What other warden tasks are ongoing?

&lt;dsinclair&gt; Fullscreen and single item list selectbox (also on android)
are ongoing tasks in that realm.

&lt;nduca&gt; Should we have a code health objective that captures what you are
doing Dan?

&lt;nduca&gt; Next quarter the the boundary between layout and style needs to be
better defined, we should help with that.

&lt;julien&gt; By defining an API?

&lt;nduca&gt; Code health and new features are separate concerns. We can make
our lives and the life of the style team easier by being healthier, paying down
technical debt. Making people faster is good investment.

&lt;dsinclair&gt; Agreed.

\[ Discussing position sticky \]

&lt;eae&gt; Gmail, docs doesn't see it as top priority.

&lt;nduca&gt; Mobile first matters for this, apps do not... ads, mobile apps
matters.

&lt;eae&gt; Seems to have fizzled out from a year ago, is it still relevant? Do
we have anyone asking for it?

&lt;nduca&gt; We can likely punt it for bit longer but it will come up. We might
have to bite the bullet and do it eventually.

&lt;nduca&gt; The extensible web approach (layout hooks) might be better.

\[ internal discussion, redacted \]

&lt;eae&gt; Agree that solving the larger problem would be better. Let's revisit
position sticky in Q2.

&lt;dsinclair&gt; Wouldn't layout hooks disable the compositor?

&lt;nduca&gt; By delegating to js we would lose compositor hotness.

&lt;skobes&gt; Already moving in that direction with delayed scrolling, other
concepts. Certain features disables the compositor wins. Trade-offs.

\[ Discussing the proposed "Improve two tests by 10%" OKR \]

&lt;skobes&gt; Who would be responsible for the 10% perf wins?

&lt;benjhayden&gt; Would be everyone? I have some things that would imrpove
performance. I think Emil can deliver some wins though the text optimization
work. Text shows up prominently in profiling runs.

&lt;skobes&gt; Is that simple vs complex text?

&lt;eae&gt; not really, in the short term. Long term there are a lot of things
we can do to speed up complex text and (eventually) move to one text path. In
the short term there is a lot of room for improvement by simplifying things and
avoiding unnecessary work.

&lt;eae&gt; Some of Ben's work, like short-circuiting the float handling in
layout, could have a big perf impact.

&lt;nduca&gt; Everyone should try.

&lt;eae&gt; Agreed, with new the new tests it is easier to track.

&lt;nduca&gt; How about having a weekly meeting to keep on track?

&lt;dsinclair&gt; We have one already, Mondays at 11. Re-purposed the Team
Warden stand-up.
