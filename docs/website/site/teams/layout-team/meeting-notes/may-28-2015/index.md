---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: may-28-2015
title: 'Thursday, May 28, 2015: Layout Moose post BlinkOn Sync Up'
---

a.k.a. the project formerly known as "the grand layout re-factoring" and briefly
as "layout slimming"

Attendees: eae, leviw, esprehn, jchaffraix, ojan, cbiesinger, dsinclair

&lt;eae&gt; Thanks for organzing Levi, I'd like to get a quick update, hear
concerns and discuss logistics.

&lt;jchaffraix&gt; I'm concerned about a lack of transparency, meeting notes not
getting out and key people being excluded from meetings.

&lt;ojan&gt; Been meaning to send out notes from BlinkOn sessions, oversight
that they weren’t. Not intentioanlly exlcuding anyone.

&lt;ojan&gt; How do we ensure everyone’s on board?

&lt;eae&gt; I’ve stayed out of the technical part of the discussion to avoid
having yet another cook in the kitchen. Now I want to make sure we have a plan,
circulate it, and ensure we’re moving forward.

&lt;jchaffraix&gt; We need to ensure we have a concrete roster and include all
steakholders.

&lt;eae&gt; We have that.

&lt;ojan&gt; We just didn’t have a chance post-BlinkON to send out the notes. We
weren’t trying to be exclusive.

&lt;ojan&gt; Dan, it sounded like maybe you wanted to be included too?

&lt;dsinclair&gt; I’m interested because it seems like the future, but couldn’t
go to the meetings because of timezones.

&lt;ojan&gt; Sydney + Waterloo is hard.

&lt;esprehn&gt; Ian K will be moving here so we won’t have to keep having Sydney
time zone involved.

&lt;ojan&gt; For future reference voice if you want to be involved. I’ll try to
keep sending emails to layout-dev to tell people ahead of time when there will
be meetings.

&lt;eae&gt; Things will be easier when discussion go from being predementantly
brainstorming and turn into more of a concrete plan. Circulating an actual
design will help guide the discussion.

&lt;ojan&gt; Let me summarize our discussions and current thinking:

- There are a certain set of features we want to achieve (measure, fragments,
async append).

- All these have require things that are hard to do in the current codebase

- We also want more sanity and understanding in the codebase.

- We took our brainstorm and turned it into a slightly more concrete set of
proposals (v1, v2, etc.)

- Added psuedocode

- Result proves some of our ideas, but not necessarily the feasibility of the
approach.

- Doc still isn’t really ready for external consumption

- Next step is to put current box layout code behind an API, then there’s the
controversial step of re-implementing that api behind a flag.

- Unsure of the sense of time required here (somewhere between 3 months and 3
years).

- Important to see the API requirements ahead of time to understand.

&lt;esprehn&gt; Implementing the various box layout algorithms is a relatively
easy exercise.

&lt;ojan&gt; We all agree what the first step is an API. We should do that then
see about next steps.

&lt;jchafrraix&gt; We don’t \*need\* that API.

&lt;eae&gt; We don't but I think we want it regardless.

&lt;jchaffraix&gt; Do we really want to put this in the critical path since
we’re now gating everything on this?

&lt;eae&gt; It fits with our overall goals of a layered platform and it isn't
necesarrily gating other work. Other than opertunity cost of work we could be
doing instead.

&lt;esprehn&gt; This will allow us to enforce sanity.

&lt;jchaffraix&gt; You don’t need to sell me on the API. My point is whether or
not this should be the priority.

&lt;eae&gt; That is a discussion for our OKR meetings. We don't need to resolve
that here today.

&lt;ojan&gt; What happened in Sydney was that we had had a hackathon meeting
where we started working on a Line Layout API. It’s possible that we’ll get
1/3rd the way through this and decide that it doesn't work and have to start
over. We ended up with a patch that moves a handful of Line Layout accessors
into the box tree into an API.

&lt;esprehn&gt; We should do Paint next.

&lt;ojan&gt; My perspective is the next step is to get someone to own the line
layout api patch and get it landed.

&lt;jchaffraix&gt; It seems odd we’re not designing this api, but just grabbing
the things we’re using.

&lt;leviw&gt; Designing it is step 2.

&lt;ojan&gt; We didn’t know exactly what methods are currently in use. The next
step is refactor how the API is used and make it sane.

&lt;eae&gt; Next thing I'd like to see is a 1-pager of our plan, shared with
layout-dev, and a high level overview of what we want in terms of an evental
API.

&lt;ojan&gt; That’s hard to plan ahead of time, we don't know what the API is
going to look like it.

&lt;eae&gt; This could be a living document, maybe that section is blank to
start. Having a document would be super useful as would having an owner.

&lt;ojan&gt; Someone should draft this document and share it around.

\* ojan stares at levi \*

\* levi is volunteered \*

&lt;eae&gt; Assuming this is something we want to work on in Q3, we should have
this document ready in the next couple weeks. It would also be useful to have
something like this for the “v2 rewrite the world” approach.

&lt;ojan&gt; On my todo list is to take the doc we have today and turn it into
something an outsider can read.

&lt;eae&gt; Great! Current doc unreadable unless you where in all the meetings.
Would be useful to have something more concise.

&lt;ojan&gt; Seperate tasks (from nduca); taking all our layout-related
rendering leads items and drawing a chart that tells us what we need to do to
implement all of them. Here’s this proposal that let’s us do them, here’s what
it would take to do it incrementally and how long, etc.

&lt;eae&gt; I agree but having the API doc is the highest properity for me.

&lt;esprehn&gt; Boundaries would exist between layout, paint, dom, editing APIs.
Woudln’t be super disruptive. Then we’d have a real list of where we’re actually
interacting with the layout tree.

&lt;dsinclair&gt; Sounds like someone else will own all clusterfuzz bugs --
hooray!

\* everyone hooray! We haz plan! \*

&lt;eae&gt; As for the API document, I'd really like to see it ready for
circulation in the next week or two to allow it to be circulated and reasoned
about in time for setting the Q3 OKRs.

\* eae looks at Levi \*

&lt;leviw&gt; This shouldn’t block anything.

&lt;ojan&gt; Mark pilgrim would be a good guy to run with the layout stuff once
it’s started.

&lt;ojan&gt; I keep hearing people say “Should I not do this because Moose” and
I say Noooooooooooo.

&lt;leviw&gt; Ties in with much of our, and Dans, previous work. Should not
block.

&lt;dsinclair&gt; Do we know what all our API boundaries are?

&lt;ojan&gt; We know the big ones. We don’t necessarily need to know \*all\* of
them. (line, paint, dom, editing)

&lt;esprehn&gt; Chris says they’re aiming for 45-46 to rip out paint stuff. Is
optimistic. Hope that by December we should be able to take out tons of
compositor-related stuff. Shouldn’t focus on that api surface. Editing will be
so gross we’ll have plenty to do without the compositor.

\* levi makes moose noise \*

&lt;eae&gt; That’s a wrap!
