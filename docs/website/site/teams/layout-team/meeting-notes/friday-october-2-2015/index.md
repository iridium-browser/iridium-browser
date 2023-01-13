---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: friday-october-2-2015
title: Friday, October 2, 2015 (Q3/Q4 OKRs)
---

Attendees: eae, cbiesinger, jchaffraix, leviw, ojan, skobes, and

szager (with ikilpatrick, drott, and kojii giving updates offline)

&lt;eae&gt; Good morning everyone. I know it feels like just yesterday we

set our Q3 OKRs but the quarter has already come to an end. Let's

spend the next 20 minutes going though our KRs and see how we did and

then use the rest of the time to discuss Q4.

\[ Discussing "Update flexbox implementation to match latest version of

specification" \]

&lt;eae&gt; Christian, this one falls on you. Would you mind giving an update?

&lt;cbiesinger&gt; I've updated some aspects of it to match the latest

specification and sent numerous "Intent to Ship" announcements to

blink-dev. For one of the biggest changes I added a use counter to

ensure that we don't break too many websites, now waiting for it to

reach a stable release to get numbers. As for the second big change I

didn't do a very good job prioritizing and all of a sudden it was

mid-October. Will carry over into Q4.

&lt;cbiesinger&gt; Spent a lot of time on importing css test suites which

wasn't captured in OKRs.

&lt;eae&gt; Sounds like a 0.5?

&lt;cbiesinger&gt; Agreed.

\[ Discussing "Remove page zoom" \]

&lt;eae&gt; This one falls on me, shared with the style team. We determined

to be feasible, vast majority of usage in the wild is for "zoom: 1",

i.e. a no-op used to trigger "has layout" in older version of IE. No

progress on implementation. Wasn’t a priority for either the style nor

the layout team. It's used extensively by layout tests so requires

quite a few tests to be updated before we can remove it.

&lt;eae&gt; Score: 0.0

\[ Discussing "Updated Unicode Bidirectional Algorithm" \]

&lt;kojii&gt; Had some progress on investigations, but not enough to write

up. Propose score 0.2?

&lt;eae&gt; Sounds reasonable.

\[ Discussing "Unprefix CSS Writing modes" \]

&lt;kojii&gt; All spec issues and critical code issues were resolved, design

doc and consensus to ship in blink-dev and in CSS WG done. Patches

are WIP.

&lt;eae&gt; Spec issues and intent to unprefix took longer than expected, not

due to any fault of ours. Let's call it 0.8.

&lt;kojii&gt; Good. Should ship in early Q4.

\[ Discussing "Improve font fallback" \]

&lt;eae&gt; Drott couldn't make this meeting but reported the following earlier.

&lt;drott&gt; Issue breaks down into better CSS font matching, better shaper

integration, and better usage of user preference and system fallback

APIs. Precondition, better CSS font matching done. Font fallback APIs

on all platforms surveyed, shaper integration see below. After shaper

integration is done, we can move to making better use of preference

font, perhaps add per-script fallback fonts and make better use of

system fallback APIs/Lists.

&lt;eae&gt; Was a bit more work than anticipated, making solid progress.

Will carry forward into Q4.

&lt;eae&gt; Score: 0.5

\[ Discussing "Spec work for houdini block & line custom layout" \]

&lt; ikilpatrick&gt; Mainly concentrated on the other aspects of Houdini

this quarter being custom paint and related specs. Groundwork is being

done for layout w.r.t. Isolated Workers & CSSOM 2.0.

&lt;ikilpatrick &gt; Had some more discussions with other browser vendors

w.r.t. what this API would actually look like. Expect that more spec

work will be done for layout in Q1/2 next year.

&lt;ikilpatrick&gt; 0.1?

\[ Discussing "Have all calls into block layout go through well defined API" \]

&lt;eae&gt; This one was shared between Levi and Ian K. Could you give an

update here Levi?

&lt;leviw&gt; So, we got line layout almost entirely buffered form block

layout. But we didn't get anywhere to style, editing or dom. I didn't

expect us to get to all of that but we're still making solid progress.

Have haven't started on the other vectors.

&lt;eae&gt; Should we continue full steam ahead or stop and evaluate the

line layout API before continuing?

&lt;leviw&gt; I think there is value in doing that once we've completely

finished the line layout API work, which we're close to, and have

include guards in place. Then we should spend a few days to see how

much we can do to clean up the API before going forward and maybe, if

it seems really difficult, we could take a step back. That's my take

on it.

&lt;leviw&gt; This ended up being a bigger API that I was hoping for. On the

other hand there are places where we can simplify it, a lot.

&lt;ojan&gt; I also think line layout is the chattiest one, right?

&lt;leviw&gt; Right, the other ones should be simpler. So, to answerer your

question, I think we should continue full steam ahead.

&lt;eae&gt; Great. How does 0.7 sound for a score?

&lt;leviw&gt; Sounds about right.

\[ Discussing "Have a clear understanding of the cost/complexity of

custom layout." \]

&lt;leviw&gt; We should continue. A lot of understanding the cost will be

figuring out what the API will be like. That's why we're planning to

sit down with the IE Edge team to help clarify the API. The ultimate

cost and complexity of getting it into our engine is entirely

dependent on what the API ends up being. So it's hard to say right

now.

&lt;eae&gt; Would it be fair to say that our plan is to continue our work on

a layout API boundary and have discussions with other browser vendors

to help steer and influence the custom layout APIs?

&lt;leviw&gt; Getting agreement and knowing the scope, yes. Beyond the work

we are doing with the API it's hard to get further ahead without broad

agreement,.

&lt;eae&gt; Are we doing everything we can to help this?

&lt;leviw&gt; The most important thing we can do is to engage other vendors.

We could always do more but we're in a good place.

&lt;eae&gt; So, it sounds like we've done all we can here even though we

don't yet have a clear understanding. 0.6?

&lt;leviw&gt; I'd say 0.5, just because the ultimate goal was only inched

towards, though we know the path to get there it has a lot of external

dependencies.

&lt;leviw&gt; All we can do is try and drive it as much as possible.

&lt;eae&gt; 0.5 it is.

\[ Discussing "Finish root layer scrolling" \]

&lt;skobes&gt; So we landed a bunch of stuff here, the two I'm most excited

about would be that it now works for inner and outer viewports on

Android. The second one being coordinates scrollbars which is pretty

awesome. So it's not done yet but made solid progress.

&lt;skobes&gt; Still a bit worried about a tree walk for overflow scroll. We

walk the entire deprecated paint tree underneath scrollable area for

overflow scroll. Tried to get rid of it but it's complicated.

&lt;skobes&gt; Would give a score of 0.7.

&lt;eae&gt; Sounds fair.

\[ Discussing "Remove simple text code path" \]

&lt;eae&gt; The performance of complex path now exceeds that of simple path

and all remaining functionality blockers have been resolved. All that

is left to do is flip the switch. Sadly that will involve updating

about 50 tests and rebaselining _all_ of our layout tests. So, we did

all of the hard work, what remains is easy but labor intensive.

&lt;eae&gt; Score 0.4?

\[ Discussing "Improve integration between Blink and Harfbuzz" \]

&lt;drott&gt; Implementation done, an issue with (anyway broken) small caps

and an edge case with ZWJ and ZWNJs for Arabic and AAT font remains so

far. Needs code cleanup, and performance evaluation, but functions

well. Then should be ready for landing in steps.

&lt;drott&gt; Perhaps 0.8-0.9?

&lt;eae&gt; Let's say 0.8.

\[ Discussing "Triage all incoming layout bugs within one week" \]

&lt;eae&gt; Next up, the first of our goals around bug health.

&lt;eae&gt; We have procedures are in place to triage all incoming bugs

within a week. Mostly a manual process for now due to lack of better

tooling but is working fine. For the latter part of the quarter we’ve

manage to adhere to the SLA and triage all bugs within five to six

days. Takes me about two hours a week which isn't too bad.

&lt;eae&gt; Score: 1.0

\[ Discussing "Reduce bug count by 30%" \]

&lt;eae&gt; So, this one was definitively more ambitious....

&lt;ojan&gt; How did we do? 3%?

&lt;leviw&gt; 2%?

&lt;eae&gt; We actually did a lot better than I feared. We reduced the total

bug count by 16%, just over half of our goal. We did much better for

high-priority bugs where the count was reduced by over 75%.

Total: From 2742 down to 2301, a reduction of 441, ~16%

Untriaged: From 860 down to 479, a reduction of 381, ~44%

P0/P1: From: 247 down to 56, a reduction of 191, ~77%

&lt;ojan&gt; That's really impressive.

&lt;leviw&gt; Reduction of over 400? Nice!

&lt; jchaffraix &gt; The fact that we still have over 50 P0/P1 bugs is

pretty telling though, probably means we're using the priority labels

wrong.

&lt;eae&gt; We have a bunch of larger blink team level goals around bugs in

Q4, we'll come back to that in a bit.

&lt;eae&gt; Does a score of 0.6 sound reasonable? We did a little over half

of as well as we set out but better for the bugs where it really

matters.

&lt;leviw&gt; Sounds about right.

\[ Discussing "Reduce unnecessarily forced layouts" \]

&lt;eae&gt; This was overly vague and we didn't really make any progress

here. Plan to work with dev-rel to get a couple of examples of

concrete cases where this is a problem on real world web sites and

then make specific goals around that.

&lt;ojan&gt; The goals should probably be around either reducing or making

the layouts cheaper.

&lt;eae&gt; That's a very good point, if we cannot avoid it than making it

cheaper/more predictable is certainly a worthy goal.

&lt;eae&gt; I'd score this as a solid 0.0.

\[ Discussing "Add UMA tracking & monitoring for different types of layout" \]

&lt;eae&gt; This is the work than Ben started before he left the team.

Dominik had signed up to take it on but he was oversubscribed last

quarter and this was deemed the lowest priority. Will carry over and

could use a new owner.

&lt;eae&gt; Score: 0.0

\[ Discussing "Brainstorm/plan devtools/rail score/layout integration" \]

&lt;eae&gt; We have some ideas here but implementation is blocked on the

actual rail integration into dev tools. This goal was about coming up

with ideas and thinking about what we might want to include, things

like forced layouts, layout costs etc.

&lt;eae&gt; Score 0.4

\[ Discussing "Trace layout on some popular websites in US, Europe,

Brazil, India" \]

&lt;eae&gt; We held a number of rather productive tracing parties in MTV,

SFO and WAT. Very useful and something we should continue doing.

&lt;eae&gt; Score 0.8.

\[ Discussing "Spin up effort around spatial" \]

&lt;ojan&gt; We spun up an effort. Most of the work is pretty low priority.

Have roadmap but it hasn't been staffed as it isn't considered to be

super high priority at the moment. We don't want to starve other

efforts.

&lt;ojan&gt; There is a set of use cases about where things are on a page,

mostly about delay loading things not on the screen. Other things on

the list include the CSS contain property, Will sent out a list. There

are a few smaller things.

&lt;ojan&gt; Mostly a bunch of P2 work, nothing that we HAVE to do next

quarter but if we don't do any then next year we'll be in a really sad

state.

&lt;eae&gt; Spun up, 1.0.

&lt;eae&gt; Thanks everyone. Before we start talking about Q4 goals let's

take a moment to think about things we did particularly well or

particularly bad.

\* silence \*

&lt;eae&gt; Please, don't all speak at once.

\* further silence \*

&lt;ojan&gt; I think that the steady progress we made on reducing the bug

count and fixing important bugs was great.

&lt;szager&gt; I like that we keep our weekly meetings short and to the

point. Is around 15 minutes which is about perfect.

&lt;szager&gt; It's been nice having people from other offices come up to SF

every now and then. Would be great if we could coordinate that better.

&lt;leviw&gt; We have BlinkOn coming which is a great excuse for everyone to

hang out here for a few days.

&lt;skobes&gt; Is that in SF?

&lt;leviw&gt; Yes, it was just announced.

&lt;eae&gt; Christian will be in SF for the full week of BlinkOn, as will

Dominik. He'll also be there the week before. Not sure about Koji yet.

Always great to have everyone in the same room. Especially given how

distributed we are.

\[ Discussing "Q4 OKR Ideas & Suggestions" \]

&lt;eae&gt; Let's move on. It sounds like we want to continue the flexbox

work, right Christian?

&lt;cbiesinger&gt; Yes.

&lt;eae&gt; And continue on fullscreen, bokan has recently signed up to take

that over.

&lt;jchaffraix&gt; Right, he'll take it over from Dan. Dan is about to land

his big fullscreen fix which is really exciting.

&lt;eae&gt; We should have an item about houdini.

&lt;leviw&gt; I put a tentative okr in there, should get Ian in on that

conversation. Will follow up.

&lt;eae&gt; Stefan, do you still want to drive the hyphenation work? We've

had a couple of meetings with the Android team and it looks like we'll

do a push to add hyphenation support and better line breaking in Q1.

Q4 is too much of a push. We'll still need to do some design work and

work with the Android text team in Q4 however.

&lt;szager&gt; Sounds sane, hyphenation is the number one user request in

Germany. Q1 sounds good, I'm fine with doing the required design work

in Q4.

&lt;skobes&gt; How about other browsers, do they all support hyphenation?

&lt;leviw&gt; All other browsers support it. We had some support but ripped

it out after the fork from WebKit.

&lt;eae&gt; Finish root layer scrolling?

&lt;skobes&gt; Yup.

&lt;eae&gt; Do you think we could finish it in Q4 or will it carry over into Q1?

&lt;skobes&gt; Think so.

&lt;eae&gt; Another idea we've been toying with is reimplementing ruby on

top of custom layout.

&lt;jchaffraix&gt; Isn't that blocked on implementing custom layout?

&lt;eae&gt; The idea is to use the internal API we've been working on and

then develop it in conjunction with the API. That way we ensure the

validity of the API.

&lt;eae&gt; To be clear, this goal is not about _shipping_ a custom layout

based ruby implementation. It's about seeing if it is feasible to use

some of the custom layout APIs to implement it internally and then, if

it is a successes, we'll see about shipping it. Would be nice if we

could as the current ruby implementation is a bit of a mess and isn't

complete. If we need to reimplement it I'd rather do it in a

extensible way. Having a custom for custom layout would be nice.

&lt;eae&gt; Next up, remove simple text. It's all about the rebaselines.

&lt;eae&gt; Font fallback, continue shaper integration and pref fonts and

system fallback improvements. Falls on Dominik and he wants to

continue this work.

&lt;eae&gt; As for bug health we've had a number of discussions in the

larger blink team and we've agreed to some common goals. What it comes

down to for us is the following:

- Continue to triage all bugs within one week

- Fix all (new) P0/P1 within one release

- Reduce untriaged bug count by 50%

- Reduce total bug backlog by 15%

&lt;eae&gt; The two last goals are ambitious but doable.

&lt;jchaffraix&gt; I'll be helping with bug triage in addition to documentation.

&lt;eae&gt; Thanks Julien! We should also make sure to have a goal around

documentation.

&lt;jchaffraix&gt; For those that haven't been following along, I've been

adding google style class level comments documenting the design and

implementation details for many of our core classes.

&lt;leiw&gt; I have a very long bug list, if we triage by just assigning

them I'll have an even longer list. That's not very helpful. Might

make more sense to have a bucket or label of sorts rather than

assigning more bugs?

&lt;ojan&gt; Does triage imply assignment?

&lt;eae&gt; No

&lt;ojan&gt; The way it used to work is that assigned means it's something

you will work on in the next week or so. Used to be standard practice

on Chrome, not any more. We should move back to it.

&lt;leviw&gt; I don't mind having them assigned, for these specific bugs I'm

a good fit and the right person to work on it.

&lt;ojan&gt; cc:ing would be fine for that, no?

&lt;leviw&gt; Yeah, should be fine. Hard to prioritize, I work off things

that are assigned to me.

&lt;szager&gt; I also think that, we use bug tracker for work items, bugs

and feature requests are intermingled.

&lt;ojan&gt; Sorry to derail the discussion, we could talk about this offline.

&lt;eae&gt; Not at all, this is super useful. We need to understand what

triage means and come up with a way to handle the backlog.

&lt;ojan&gt; How about we add a set of hot-lists or labels to help with this

without giving an illusion of forward progress?

&lt;leviw&gt; Would make me feel less like I'm drowning in bugs. Would be
helpful.

&lt;skobes&gt; I have the same problem with text auto size bugs, mix of

assigned and watching label. There are too many for me to look at,

need way to filter.

&lt;ojan&gt; How about you use a label for this? like
"important-text-autosizing".

&lt;skobes&gt; How about two separate triage passes, one to delegate and one

to set priority?

&lt;eae&gt; As for priority we need to have a team wide definition of what

P1 vs P2 vs P3 means, today P1 is essentially for blockers, P2 for

everything else and P3 means it'll never be looked at. Dru is driving

an effort to define the priorities.

&lt;eae&gt; We should continue this discussion and see what we can do to

make this work better.

&lt;ojan&gt; Who should drive this?

&lt;eae&gt; any volunteers?

\* crickets \*

&lt; jchaffraix&gt; I'm fine volunteering a little but seems to big for one
person.

&lt;ojan&gt; Might not need a formal owner.

&lt;leviw&gt; Let's try a few things and sync back in a month.

&lt;ojan&gt; Great, once we have something that works we can expand to

larger blink team. This is a team-wide problem but fixing it probably

needs to start in the sub-teams. Grassroots.

&lt;eae&gt; Let's talk about RAIL and the work we'd like to do in that space.

&lt;eae&gt; Implement CSS containment from Ojans spatial list seems like a

good candidate. Question is how to prioritize. Do you want to give a

sixty second summary Ojan?

&lt;ojan&gt; Sure, it's a feature where you can say contains: and a bunch of

values. Specifies that a div fully contains all of its children. No

position absolute can escape, no overflow outside the div. Also

doesn't auto-size. What this allows us to do is when you hit a contain

thing, if it's outside the screen, you can completely ignore doing

layout on the children. Might not even need style recald and certainly

no layout.

&lt;ojan&gt; Today you cannot do that without detailed knowledge of the

children. It's our proposal, it's has gone through the working group

and is ready to implement. Mozilla is about to start implementing it.

&lt;ojan&gt; Skipping style recalc work is tricky but possible. Skipping

layout is easy.

&lt;ojan&gt; V1 would not do the majority of optimizations, would implement

the correct behavior (no auto-size, reset list numbering, clip

content), Super easy. Small amount of work. The only optimization

would be the subtree layout root. Plugging in to existing system.

Easy. Get performance benefit form that. In future quarters we could

plug in more performance work. As web devs get aboard we get more

value from doing the performance work.

&lt;ojan&gt; Useful for delayed loading, infinitive list etc. Also useful

for things like text editors where typing in a text box doesn't

trigger layout for the full page. Only the text area.

&lt;skobes&gt; Positioned content?

&lt;ojan&gt; Changes behavior. Performance wise, nothing bad will happen.

&lt;skobes&gt; What if we have a lot of subree layout roots?

&lt;ojan&gt; Might require some fixes on our side, i.e. not having a flat

structure. Work should be guided by usage. Also, as you cannot have

inlines or auto sizer for this it naturally limits the places it can

be used.

&lt;ojan&gt; Not a p1, new feature. Helps in short term in building

libraries. Long game.

&lt;eae&gt; Also not much work, would be good to get started given the long

term goals and benefits.

&lt;ojan&gt; It's the one big thing I want to do from spatial in Q4.

&lt;eae&gt; Do you want to talk about smooth scrolling Steve?

&lt;skobes&gt; This is one of chromes oldest bug, issue 575, every other

browser has had this for a long time.

&lt;eae&gt; Do we have the pieces? Can we do this?

&lt;skobes&gt; All pieces are in place, doable in Q4.

&lt;eae&gt; Let's do it!

&lt;eae&gt; Finally, we should have a goal around forced layouts. Will

discuss specifics with devrel and come up with a set of concrete goals

that we can iterate on. Paul mentioned requesting window size as one

candidate.

\[ Wrapping up \]

&lt;eae&gt; Anything on this list you think we should _NOT_ be doing?

&lt;szager&gt; Maybe the rail work should be prioritized. Was mentioned last

but seems like the most important.

&lt;eae&gt; Ordering not indicative of priority, agree that it's one of the

top priorities.

&lt;eae&gt; How about the other way around, anything we should be doing that

we haven't talked about?

&lt;wkorman&gt; Flipped box writing mode that Levi and Ojan talked about?

&lt;wkorman&gt; The proposal to unprefixing vertical writing mode triggered

a discussion where some of the paint folks where saddened by the

complexity of vertical wiring mode combined with RTL combined with

flipped blocks. Could we simplify this around a sane path. Starting

thinking about it.

&lt;eae&gt; RTL isn't too bad but the others are a bit of a mess, also a

combinatorial explosion.

&lt;leviw&gt; Can't do much about RTL due to UBA, flipped blocks and

vertical writing mode shouldn't be as much overhead though.

&lt;leviw&gt; I think for somebody that would be a fun project to re-think.

&lt;wkorman&gt; Flipped blocks and vertical writing mode.

&lt;leviw&gt; Make it not suck.

&lt;ojan&gt; Please include me in the discussion.

&lt;wkroman&gt; How about document life cycle?

&lt;leviw&gt; I don't think anyone has worked on that in a long time.

&lt;ojan&gt; Mostly blocked on paint team, mostly compositor related.

Slimming paint v2.

&lt;wkorman&gt; There are some comments along we can come back to this node

multiple times, surprising but ok. We should fix these things and

enforce lifecycle. Editing code in particular is really quite bad.

Fixing might invoke adding another phase to the life cycle.

&lt;leviw&gt; Not a new phase, push computing rect until end of operation.

Could potentially do less work.

&lt;wkorman&gt; What about your high dpi work?

&lt;eae&gt; Captured in the windows teams OKRs, my involvement is mostly

advisory at this point now that we've decided on the design and

integration. Implementation is handled by the windows team.

&lt;eae&gt; Thanks everyone. I'll be following up with each one of you

offline over the next week or two.
