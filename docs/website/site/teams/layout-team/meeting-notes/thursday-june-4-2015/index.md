---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: thursday-june-4-2015
title: 'Thursday, June 4, 2015: Mid-Q2 OKR Check-in'
---

Attendees: eae, cbiesinger, dsinclair, jchaffraix, nduca, skobes, szager

Q2 OKRs: <https://www.chromium.org/blink/layout-team/okrs/2015q2> (updated

with latest status, as is overview at team landing page

<https://www.chromium.org/blink/layout-team/>)

Q2 Mid-quarter check-in

-----------------------

&lt;eae&gt; Good morning/day/afternoon everyone. We're half way through the

quarter so I figured a check-in was in order to see where we

stand.

&lt;eae&gt; Let's go through our list of key-results and see whether we're on

track to achieve them by the end of the quarter.

\[ Have a report characterizing layout performance for all layout \]

\[ scenarios \]

&lt;eae&gt; Ben isn't here this morning, I'll follow up with him offline.

\[ Have design document for grand node measure refactoring \]

&lt;jchaffraix&gt; So the grand node refactoring has been superseded by Moose.

Moose has some design to it but not a finished design doc.

&lt;jchaffraix&gt; I don't really know how we want to scope that. Or if we

should just ignore it.

&lt;eae&gt; I think it's fair to say that the grand refactoring has been

superseded by Moose. At the same time the scope has also increased

dramatically.

&lt;jchaffraix&gt; We have early design ideas for Moose. Need to split it up

into concrete proposals and sync up with esprehn and ojan. They took

over the project when I went of vacation and ran with it, need to

reclaim it and come up with a plan.

&lt;eae&gt; The first phase of the moose work, to create an API, is

relatively well defined, uncontroversial and limited in scope.

&lt;jchaffraix&gt; The whole idea behind doing the API first is to avoid the

hard questions about differences of opinion relating to the strategy.

&lt;eae&gt; Yes. We need to figure that out but this is not the time nor the

venue. We need to discuss this, with all stakeholders, prior to setting

the Q3 OKRs.

&lt;nduca&gt; Everyone should be there when we talk about it.

&lt;jchaffraix&gt; Moose is more than required, lets continue this discussion

offline and with all the relevant people.

\[ List markers not modifying layout tree during layout (in dev). \]

&lt;dsinclair&gt; Done!

&lt;eae&gt; Awesome!

&lt;dsinclair&gt; It's in beta even.

&lt;dsinclair&gt; Turns out it _just_ missed beta, it's in dev though.

&lt;eae&gt; That was the goal though, you're golden.

\[ Initial implementation of menu item not modifying layout tree during \]

\[ layout (in canary) \]

&lt;dsinclair&gt; Done, also in dev.

&lt;eae&gt; You're on fire! What are you going to do the rest of the quarter?

&lt;dsinclair&gt; Code reviews. Tons of code reviews.

\[ Have an experimental out-of-tree node measure API. \]

&lt;jchaffraix&gt; This is from before I left on vacation. We want to push it

out of the roster, not a priority any more. Gated on refactoring work.

&lt;jchaffraix&gt; We have a prototype and a design but stopped working on it

to focus on refactoring work.

&lt;eae&gt; So we decided to focus on the broader scope refactoring work

instead of the limited out-of-tree measure implementation?

&lt;jchaffraix&gt; Yes, there where also concerns about supporting the API in

the future given the narrow use cases for it. Not necessarily worth the

complexity.

&lt;eae&gt; Let's revisit this at the end of the quarter.

&lt;jchaffraix&gt; Ok.

\[ Update flexbox implementation to match latest version of \]

\[ specification \]

&lt;cbiesinger&gt; Some parts done, some parts to be done.

&lt;eae&gt; On track?

&lt;cbiesinger&gt; Should be possible to finish before EoQ.

&lt;ndcua&gt; Did we make any progress on debugging?

&lt;cbiesinger&gt; Haven't heard anything about it.

&lt;nduca&gt; Can you follow up, the new behavior is really hard to debug (for

web developers).

&lt;cbiesinger&gt; I'm not quite sure how to shove it into the inspector.

&lt;nduca&gt; Not super important but a nice side narrative and it might be a

fun project. Would be be great for users.

&lt;cbiesinger&gt; Agreed, not sure where to start though.

&lt;nduca&gt; Only if you want to. Entirely up to you. Work with dev tools

team and nudge them in right direction. No need to do any UI work.

&lt;cbiesinger&gt; I'll put it on my todo list to reach out to the dev-tools

team and expose the information.

\[ Improve CJK vertical text support \]

&lt;eae&gt; kojii isn't here which is understandable given that it's 2am in

Tokyo at the moment. I'll follow up with him this afternoon.

\[ Make complex text as fast as simple text \]

&lt;eae&gt; So this is on me. I have a plan and have started working on it.

Still think I'll be able to get the complex path to be at least as fast

as the simple one by the end of the quarter, will by close though.

Involves changing how we cache things are reducing repetitive and

redundant work.

\[ Remove the simple text code path \]

&lt;eae&gt; This is obviously gated on the previous goal of getting complex

text to be as fast as simple text. We're quickly running out of time

this quarter so this is at risk of slipping. We should be able to at

least have a flag to disable it but removing it entirely seems a bit

optimistic at this point.

\[ Build the plan to support updated Unicode Bidirectional Algorithm \]

&lt;eae&gt; Also on kojii, will fallow up online.

\[ Raise the “Passed” rate of CSS Writing Modes Level 3 to 85% \]

\[ including new tests. \]

&lt;eae&gt; Also on kojii, will fallow up online.

\[ Move line layout to LayoutUnit \]

&lt;szager&gt; Done!

&lt;eae&gt; Want to expand on that?

&lt;sazager&gt; No. It's in dev and so far it looks good.

&lt;eae&gt; Congratulations and well done!

\[ Finish root layer scrolling \]

&lt;skobes&gt; Making good progress, with sazager helping out it should be

possible to get pretty close to finish it this month.

&lt;eae&gt; Any new unforeseen problems? It's grown in scope quite a bit since

it started.

&lt;skobes&gt; Nothing too major, just a lot of work.

\[ Ensure that bugs get automatically filed for clusterfuzz asserts \]

&lt;cbiesinger&gt; Done.

&lt;eae&gt; Awesome.

Wrapping up

-----------------------

&lt;eae&gt; Looks like we're on track or even ahead of the curve for the

majority of our objectives. Well done everyone! Perhaps we need to have

a bit more ambitious goals for Q3?

&lt;eae&gt; Speaking of which, it would be great if everyone could start

thinking about what they want to do next and what you want to work on

for the next quarter or two.

&lt;eae&gt; We have a big list of our backlog and potential projects at

<https://www.chromium.org/blink/layout-team/potential-projects>, please

add projects and tasks you think are missing and see what you think we

should focus on next.

&lt;eae&gt; We'll have a Q3 planning discussion in a few weeks.

&lt;eae&gt; Any questions about our OKRs or concerns that we need to talk

about?

\* silence \*

&lt;eae&gt; Thanks everyone!
