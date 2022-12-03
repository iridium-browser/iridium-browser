---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: tuesday-february-16-2016
title: Tuesday, February 16, 2016
---

Updates since last meeting (on Monday, February 8th):
Scrolling (skobes, szager)
- Working on scroll anchoring, investigating a crash where it looks
we're holding stale pointers. (skobes)
\[[crbug.com/558575](http://crbug.com/558575)\]
- Smooth scrolling is under control one outstanding bug where
we're not handling the interaction between man and cc correctly. (skobes)
- Started looking at a scrollbar-renders-weirdly
bug, most likely it's the compositor pointing to a stale Scrollbar,
no fix yet. (szager) \[[crbug.com/553860](http://crbug.com/553860)\]
CSS Flexbox (cbiesinger)
- Have quite a few release blockers. All the same issue, related to
autoflow auto. Finally figured it all out. Stefan made a change to delay
overflow calculation to fix a scrollbar bug awhile ago. It turns out the
fix wasn't quite correct in regards to flexbox Working on a better
implementation of. Going to make a patch to do that and then to clear
the flexbox main cache which should be the right approach.
Specifically, flexbox has code, actually in LayoutBlock used for
flexbox, that handles overfow auto is after layout, called something
like updateOverflorAfterLayout. That then checks if we overflow and if
auto then adds scrollbars and triggers another layout. But if flexbox
we don't do it for every flexbox change as that wouldn't work for
nested flexboxes, we delay until after last flex. The new fix is to
do relayout without updating the cache for the flebox.
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last week -
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- No update since last week -
CSS Houdini (ikilpatrick)
- Last week was working on Worklet stuff, working with AudioWG to see
how it interacts with audioWorklet and their needs.
- This week will start on PaintRenderingContext2d, which is the subset
of canvas for custom paint.
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Removed remaining uses of deprecated InlineBox::layoutObject()
accessor and made it private. (dgrogan)
- Had to add many methods to the API. (dgrogan) :(
- Started removing uses of shim-like LayoutObject\* LineLayoutItem::
operator(). (dgrogan)
- Started working on next part of api work, have a little like starter
patch out, just need to hit CQ button. (leviw)
- Going to work on a poof of concept work to see if we can build
LayoutNG and have it co-exitst with and interact with our current
layout code. Will do work on branch but in public. (leviw)
- Have a patch out for for LineLayoutTextSVG class. Dave identified as
necessary, working through Levi's feedback. (pilgrim)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- Style containment landed this morning, yay!
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Trying to get the spec solid so we can get feedback from other
browser vendors. Getting other browsers on board is going to be the
long pole in shipping. (szager)
- Probably not going to make M50. Spec work needs to occur. Not getting
reviews as quickly as I'd like. (szager)
Text (eae, drott, kojii)
- Fixed orthogonal writing mode issue, working on following fixes.
(kojii)
- Support accept-language in font fallback for Windows and Android.
(kojii)
- Investigating snap-height property. (kojii)
- Investigated Chrome's coverage of font-variant-\* sub properties. Only
font-variant-ligatures supported, the rest rather looks like CSS 2.1.
Filed bugs about font-variant being in a state of a very old version
of the spec. (drott)
- Experimented and started a true small-caps implementation (non-
synthesized), checked for AAT small-caps feature in fonts on Mac.
(drott)
- Sent Intend to Implement for font-variant-caps. (drott)
- Review of Koji's multi-locale font-fallback Android CL. (drott)
- Working on an issue involving the handling of combining emoji (two or
more emoji joined together with a zero with joiner) where the metrics
cache has incorrect measurements causing extra line wrapping. (eae)
- Various layout release blockers. (eae)
HTML Tables (dgrogan)
- Fixed regression around width:50% handling for tables.
\[[crbug.com/244182](http://crbug.com/244182)\]
- Fix incorrect handling of percentage heights inside table cells,
cased a small issue for hangouts but got them to fix it on their end.
\[[crbug.com/353580](http://crbug.com/353580)\]
- Fixed a renderer crash. \[[crbug.com/570139](http://crbug.com/570139)\]
Misc
- Next week or two will take stab at flipped blocks approach.
News at 11! (wkorman)
- Correctness fix for paint performance with regards to VisibleRect. We
added VisibleRect which is used for paint performance, not yet live.
VisibleRect is essentially the CullRect but in the space of the
backing graphics layer. Needed for CC interaction. It didn't correctly
handle subpixel accumulation before. (wkorman)
- Tons of release blockers (leviw), will try to rebalance. (eae)
- Beginning to look at forced layout, (potentially reducing time spent
in forced layout / number of forced layouts). (cbiesinger)_
Logistics
- drott going to Tokyo on Thursday. drott/kojii/behdad plus people from
Mozilla meeting for a text workshop.
