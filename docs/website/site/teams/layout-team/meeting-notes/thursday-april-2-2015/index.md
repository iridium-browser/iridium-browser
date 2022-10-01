---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: thursday-april-2-2015
title: 'Thursday, April 2, 2015: Q1/Q2 OKRs'
---

Attendees: eae, slightlyoff, benjhayden, bnutter, cbiesinger,
dsinclair, dglazkov, jchaffraixm, kojiim, nduca, skobes
Graded Q1 OKRs: <http://www.chromium.org/blink/layout-team/okrs/2015q1>
Q2 OKRs: <http://www.chromium.org/blink/layout-team/okrs/2015q2>
Q1 Evaluation
-------------------
\* eae presents draft document \*
&lt;eae&gt; Let's go through our Q1 OKRs and see how we did and what we can
learn.
\[ Have telemetry benchmark suite running key_silk_cases monitoring
layout times with less than 1% variation between runs \]
&lt;eae&gt; Variation between runs is about 3% with the new layout_avg
telemetry metric, down from about 10% for existing webkit performance
tests. Scored at 0.6.
&lt;benjhayden&gt; A way to reduce variance further would be to boost the
signal ratio by doing a lot more layout. Not sure if that is the right
thing to focus on. Perhaps we should focus on real world use cases
instead. This might be as good as we can get the tolerance with the
current setup.
&lt;benjhayden&gt; Have started rewriting metrics to expose more
information, this work will carry forward into Q2.
\[ Add 3 real-world pages with lots of layout to key_silk_cases. At
least two of which are mobile oriented \]
&lt;eae&gt; Ben added two new, real world, tests. Score 0.7.
&lt;benjhayden&gt; Perhaps we could add more polymer tests?
&lt;eae&gt; The Polymer Topeka tests is great as it is representative of a
real world app. Adding more like that could be useful but for now
we're pretty good.
\[ Speed up some key_silk_cases tests \]
&lt;eae&gt; We speed up a couple of webkit performance tests by over 10%,
most notably chapter-reflow and line-layout. We didn't manage to speed
up layout in any of the key_silk_cases tests though. Score 0.0.
&lt;jchaffraix&gt; No one owned the task, no one felt responsible and that
might be way.
&lt;benjhayden&gt; layout is already pretty fast
&lt;dsinclair&gt; need to pick specific tests and have actionable goals
&lt;jchaffraix&gt; lets talk about that for Q2 goals
\[ Create a Measure API prototype and write-up of lessons learned \]
&lt;eae&gt; We split this into text and element measure and have write-ups
for both. Score 1.0.
&lt;jchaffraix&gt; We probably scoped this too narrowly and achieved a 1.0.
Should we have been a bit more ambitious?
&lt;cbiesinger&gt; Could you add links to the write-up doc(s)?
&lt;julien&gt; Will do.
Note: Linked from graded Q1 OKR document;
http://www.chromium.org/blink/layout-team/okrs/2015q1
\[ Support natural layout animations (subpixel layout during animation) \]
&lt;eae&gt; Work was abandoned and depriotitized. Score 0.0.
&lt;jchaffraix&gt; Could you add some background as tho why it was
depriotitized?
&lt;eae&gt; Sure. It didn't seem worthwhile without perf guarantees from
something like layout boundaries. Focused on work to improve line
layout performance instead.
\[ Finish root layer scrolling \]
&lt;eae&gt; Steve made great progress here but didn't quite get to finish it
at all. Score 0.5
&lt;skobes&gt; Somethings worked, others didn't... Need another quarter to
finish.
\[ Move line layout to LayoutUnit \]
&lt;dsinclair&gt; We're getting close, but haven't made the switch yet. We
still need to look through all the test changes on Windows and see if
there are problems or if we just need to rebaseline. Score 0.7.
&lt;eae&gt; What about the performance problems Stefan found? Do we think it
mostly from the convenience wrappers used to make the transition?
&lt;dsincliar&gt; Not sure, Stefan has been focusing on that. I've been
putting out other fires.
&lt;eae&gt; Will follow up with Stefan (szager).
\[ Triage all clusterfuzz asserts and fix 50% of them \]
&lt;cbiesinger&gt; We didn't do a very good job on scoping, the goal was too
ambitious. Score 0.2
&lt;cbiesinger&gt; I switched focus to flexbox work half way through the
quarter. Doing both in parallel might have been better.
\[ Have bugs automatically filed for new clusterfuzz asserts \]
&lt;cbiesinger&gt; Auto-filing doesn't make sense without triaging. Score 0.0
&lt;cbiesinger&gt; All work is done, just needs to flip a switch.
\[ Render tree modifications during layout \]
&lt;dsinclair&gt; Looked at list markers specifically this quarter. Fixed
all of the crashers just ran into one issue where moving a renderer
further down tree causes issues. Score 0.7
&lt;dsinclair&gt; Think I know the way forward, just need to do it. List
marker work is close to done.
&lt;dsinclair&gt; There are other parts that we haven't started looking at,
like menus with only one item in them being a special case.
&lt;dsinclair&gt; Menus on mac/linux/windows special case when there is only
one item, has a single renderer. Android is different.
&lt;dsinclair&gt; Next up, full screen. Julien was looking at it for awhile.
&lt;eae&gt; Julien, do you want to give a quick update on full screen?
&lt;jchaffraix&gt; I wrongly though someone has fixed a prerequisite bug for
full screen. I spent some time doing tons of testing/rebaselining with
the assumption we could land the change. Realized that we couldn't as
the prerequisite bug wasn't fixed.
&lt;jchaffraix&gt; Blocked on complex problem around re-attach. When
toggling fullscreen you change state of render requiring re-attach
which is problematic.
&lt;jchaffraix&gt; Presto did work to save state when re-attaching to avoid
restarting plugins when re-attaching. Presto re-attaches more than
blink which prompted this.
\[ Speed up complex text by a factor of 2 \]
&lt;eae&gt; New goal added mid-quarter. We managed to speed up complex text
by a minimum of over 2 and up to 40x in some cases. Score 1.0
&lt;eae&gt; We might have been a bit too conservative here, we'll correct that
for Q2!
Non-OKR tasks
-----------------------
&lt;eae&gt; Let's talk about tasks we spent significant amounts of time on
that was not covered by our OKRs.
From draft doc:
- rendering/ -&gt; layout/ move and renaming
- bidi bugs
- mac text rendering follow up
- security bugs
- updating flexbox to latest version of spec
- import w3c flexbox tests
- css writing mode tests
- win_blink_rel deflaking
&lt;eae&gt; A few of those, like the rendering -&gt; layout rename and the
win_blink_rel work should probably have been added to the OKRs mid
quarter.
&lt;eae&gt; We cannot plan for the unexpected but if we have unforeseen work
that we expect to continue we should consider adding it mid-quarter to
the list.
&lt;jchaffraix&gt; Did we miss anything?
\* silence \*
&lt;eae&gt; I guess the silence means we have agreement, let's move on.
Q2 OKR Planning
------------------------
\[ O: Gain insight into real world layout performance. \]
\[ KR: Have a report characterizing layout performance for all layout
scenarios. - benjhayden \]
&lt;eae&gt; Could you explain what we mean by layout scenarios?
&lt;benjhayden&gt; That is part of the goal, we know load layout is
different from animation layout and incremental layout. Yet both can
do a lot of the same work. We're still trying to figure out what it
means.
&lt;jchaffraix&gt; Is this layout classes? Load vs animation?
&lt;benjhayden&gt; There are different types of animation layout, think
moves vs gets taller. Most layouts are a mix of many types, there is a
lot to figure out. It is a bit ill-defined at the moment.
&lt;dsinclar&gt; Could you add some words to make that more clear? Wasn't
obvious what it meant, I thought it was layout classes.
&lt;benjhayden&gt; That is part of it, we need to know what is text layout
vs block, load vs incremental etc. Lots of work to define.
&lt;jchaffraix&gt; This needs better wording.
&lt;dsinclar&gt; Should we split it into two KRs? One for moving on page,
other for what we call layout classes in blink?
&lt;jchaffraix&gt; We don't know the granularity.
&lt;eae&gt; Having a report split on a couple of different axis would be
useful, we don't know what the axis are yet though so being more
specific is hard.
&lt;benjhayden&gt; Having a report is a good key result, figuring out how to
present the data is part of the objective.
\[ O: Improve high-level layout design. \]
\[ KR: Have design document for grand node measure refactoring. \]
\[ - jchaffraix \]
\[ KR: List markers not modifying layout tree during layout (in dev). \]
\[ - dsinclair \]
\[ KR: Initial implementation of menu item not modifying layout tree
during layout (in canary). -dsinclair \]
&lt;jchaffraix&gt; For those that haven't been following along, we've
started talking about element measure and display outside/inside. Both
requires changes to how we handle the tree during layout in
significant ways. Wondering if we have to do this work up front to
support other goals or we can make the architecture better as we go
along.
&lt;eae&gt; This unblocks further work down the road, things in pipeline now
and things we want to do in the future.
&lt;jchaffraix&gt; Still early on, we're coming up with a broad proposal.
&lt;dsinclar&gt; I'd like to better understand the use cases for people that
ask about containment vs measure.
&lt;jchaffraix&gt; I'll share a document with use cases.
&lt;jchaffraix&gt; The KR here is to come up with a design document, guided
by the vision that we're still trying to define.
&lt;jchaffraix&gt; It might require that we remove all of the instances
where me modify the tree during layout.
&lt;bnutter&gt; This KR has a lot of owners, each one should have a single
person responsible for keeping track/be in charge.
&lt;eae&gt; Good point, first one listed is in charge.
&lt;jchaffraix&gt; I'm in charge of the design document one.
\[ O: Support standards efforts. \]
\[ KR: Ensure that custom layout spec is compatible with our vision. \]
\[ - jchaffraix, eae \]
\[ KR: Ensure that text measurement spec is compatible with our vision. \]
\[ - eae \]
&lt;eae&gt; These KRs are poorly worded, are about staying involved in the
process and making sure that the spec stays compatible with our goals
and that we can/want to implement them.
&lt;slightlyoff&gt; Who is in charge for the text measurement one? You Emil?
&lt;eae&gt; Yeah, text is on me.
&lt;slightlyoff&gt; Great, then I'm not worried.
&lt;bnutter&gt; This isn't really measurable.
&lt;slightlyoff&gt; Having a public github repo and discussion with other
vendors might be a better goal?
&lt;eae&gt; Makes sense to me.
&lt;slightlyoff&gt; Will be a demonstration.
&lt;dgalzkov&gt; As I understand it this is about us trying to be good
sports, clearing the way for houdini spec work.
&lt;eae&gt; Right, it is a bit hard to define what false on us vs houdini
and the other teams though.
&lt;bnutter&gt; I'm not concerned about who owns it. It doesn't have to be
an OKR, not all work you do needs to be. Writing a spec would be a
good goal but that doesn't seem to be what this is about.
&lt;eae&gt; Some very good points, this might not make sense as a set of
explicit goals. Let's talk about it offline and move along.
\[ O: Improve capabilities of the web platform.
\[ KR: Have an experimental out-of-tree node measure API. - jchaffraix \]
&lt;eae&gt; This might be a little bit too ambitious, might be blocked by
the high-level design re-architecture work.
&lt;jchaffraix&gt; The Q1 goals where scoped too narrow in this regard, I
think it is doable and want us to have stretch goals.
\[ O: Rationalize text rendering. \]
\[ KR: Make complex text as fast as simple text. - eae, szager, kojii \]
\[ KR: Remove the simple text code path. - eae, szager, kojii \]
&lt;eae&gt; Now this is a very ambitious set of goals, complex text is about
seven (7) times slower than complex at the moment.
&lt;eae&gt; Go big or go home, right?
&lt;cbiesinger&gt; Is this about making Arabic as fast as English?
&lt;eae&gt; No, it is about making English text going down the complex path
as fast as the same English text going down the simple path.
&lt;cbiesinger&gt; Ok, sounds more doable.
&lt;eae&gt; Still a ton of work and in practice this will significantly
narrow, if not completely close, the gap between scripts. Performance
will be more about whether ligatures are used or not. English can use
ligatures.
\[ O: Improve code health. \]
\[ KR: Move line layout to LayoutUnit. - szager, dsinclair \]
\[ KR: Finish root layer scrolling. - skobes \]
\[ KR: Fix 30% of clusterfuzz asserts. - cbiesinger \]
\[ KR: Ensure that bugs get automatically filed for clusterfuzz
asserts. - cbiesinger \]
&lt;eae&gt; Dan, do you want to take this one?
&lt;dsinclair&gt; Sure. As for line layout this KR is about finishing the
work we are doing now - carrying the Q1 goal forward.
&lt;skobes&gt; For root layer scrolling it is mostly things we are already
doing.
&lt;jchaffraix&gt; Sounds good. Moving on to clusterfuzz, is this work we
want to be doing? Should we continue with it?
&lt;eae&gt; I'm not sure.
&lt;cbiesinger&gt; Are you talking about stopping the work to fix asserts?
&lt;jchaffraix&gt; Is this work something we as a team thing is really
important? If not we should focus on broader team goals. Perf,
developer productivity and code health.
&lt;cbiesinger&gt; Debug builds are unusable due to assertions, hampers
developer productivity. Many of the asserts might be real bugs that we
don't know about.
&lt;jchaffraix&gt; We might have bigger fish to fry?
&lt;christan&gt; I want to keep doing it and think it is important. Perhaps
not at the highest priority?
&lt;dsinclair&gt; How about we drop fix 30% goal and file bugs for asserts
then fix them ad-hoc?
&lt;christan&gt; Good idea.
&lt;eae&gt; Sounds reasonable, I agree with Christian that it is important
and work we should be doing. Having Christian do all the work himself
and in bulk is not a good use of his time though.
\[ O: Improve web compatibility.
\[ KR: Update flexbox implementation to match latest version of
specification. - cbiesinger \]
&lt;dglazkov&gt; We should think about interoperability as function of
productivity, we need to thing about how much time a developer spends
on something like flexbox interoperability between implementations.
&lt;jchaffraix&gt; I raised the same problem, web compat can or cannot be
out of scope depending on impact on developer productivity. How do we
prioritize web compat vs other things we want to do.
&lt;cbiesinger&gt; I want to continue with flexbox spec changes, think they
are important. Firefox and IE are mostyl compatible now, we're very
different.
&lt;eae&gt; The other vendors are compatible, we are not. This makes life
hard for developers. I think this is really important, especially
since flexbox is one of the flagship layout primitives that we're
trying to push developers towards.
&lt;jchaffraix&gt; Sounds reasonable, just wanted to have the discussion.
&lt;eae&gt; I'm glad you bought it up, that is one of the key reasons we're
having these meetings.
\[ KR: Import w3c test suites. - cbiesinger \]
&lt;jchaffraix&gt; Could you explain this one Christian?
&lt;cbiesinger&gt; Relatively easy to do right now, we have a script that
does it. Need to figure out what to do about failing tests.
&lt;cbiesinger&gt; Perhaps we should just import with failures and add to
TestExpectations?
&lt;eae&gt; Josh might work on this so it could be out of scope for us. We
should sync up with him and his team.
\[ KR: Improve CJK vertical text support - kojii \]
\[ internal discussion, redacted \]
\[ KR: Raise the “Passed” rate of CSS Writing Modes Level 3 - kojii \]
&lt;eae&gt; Do you want to talk about writing mode as well?
&lt;kojii&gt; We do worse than most browsers, see
http://test.csswg.org/harness/results/css-writing-modes-3_dev/grouped/
- webkit is 82%
&lt;cbiesinger&gt; Is WebKit Blink?
&lt;kojii&gt; We've asked the working group to split it, for now the WebKit
number mostly means Blink.
&lt;jchaffraix&gt; What would this work entail? We need something we could
measure.
&lt;kojii&gt; Firfox is behind a flag, should launch soon. IE we have not tested
much.
&lt;eae&gt; Do we want to support all? What would be a good goal to aim for
here? Specific properties we want to support? Increase the number by a
couple of percentage points?
&lt;jchaffraix&gt; Is unclear. How much are we passing right now?
&lt;jchaffraix&gt; Never mind, found it. We're at 80.
&lt;kojii&gt; We used to be at 60, in Q1 i raised it to 80.
&lt;jchaffraix&gt; Should we aim for 100?
&lt;kojii&gt; Coverage goes down as tests gets add. Is 85% too conservative?
\[ internal discussion, redacted \]
&lt;eae&gt; Makes sense. Let's aim for 85.
\[ KR: Build the plan to support updated Unicode Bidirectional \]
\[ Algorithm - kojii \]
&lt;eae&gt; Updating the UBA is something that we've been punting for years,
the i18n team is really pushing for it.
&lt;kojii&gt; Top priority for BIDI team.
&lt;eae&gt; Will be a lot of work given that we have a custom version for
speed. Whether we update our custom one or improve the performance of
the new one it'll be a lot of work.
&lt;kojii&gt; Need to investigate, goal is to have a plan.
&lt;julien&gt; So the deliverable is to have a plan, not to do the work?
&lt;eae&gt; Yes.
