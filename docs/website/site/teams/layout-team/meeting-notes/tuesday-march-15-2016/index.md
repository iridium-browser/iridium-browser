---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: tuesday-march-15-2016
title: Tuesday, March 15, 2016
---

Updates since last meeting (on Monday, March 7):
Scrolling (skobes)
- Fixed a smooth scrolling RTL bug and a smooth scroll text selection
bug.
- Fixed how we reset state between layout tests.
- Reached out to conops and came up with a plan to monitor and address
user feedback and concerns.
- This week, working through a few minor smooth scrolling bugs.
- Plan to resume work on scroll anchoring, getting it to work correctly
with different position attributes.
CSS Flexbox (cbiesinger)
- Been working on flexbox, big surprise, turns out that when I through I
had finally fixed all the scrolling issues I had not, in fact, fixed
all the scrolling issues. \*collective expression of sadness\*
There are a number of new regressions and release blockers that I'm
working to burn down. The bug that keeps on giving.
- Almost done with percentage children, patch out for review.
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- See mail to blink-dev for an update on Grid -
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- No update since last week -
CSS Houdini
- Devtools for main thread worklets. (ikilpatrick)
- Landed initial paint() function, (this does nothing, just parsing
code). (ikilpatrick)
- Plan to finally send out the patch for PaintRenderingContext2D for
review. (ikilpatrick)
- Might get to the Paint API class/instance management CL. (ikilpatrick)
- Started making progress on resize observer. (atotic)
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Layout API skeleton is done, looks like a bigger project than we
thought. LayoutView alone has over 100 references that need
converting. Have started migrating first HTML element class to serve
as a template for future work. (pilgrim)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- We're doing a prototype for layoutNG inside of out current layout
engine. Part of the motivation here is to address big architectural
issues in the layout engine itself and part of it is to support the
houdini effort to implement custom layout. Shoehorning custom layout
into out existing legacy layout would be very hard and require a lot
of hacks and isn't really something we want to do. Especially since we
already know that our layout architecture needs quite a bit of work.
Essentially we're trying a new approach to achieve many of the goals
identified during the big layout refactoring discussion (a.k.a. Moose)
last year.
What we're trying to see is if embedding layoutNG within the legacy
system is practical, determining whether we can support both in
parallel and use the new system for certain sub-trees.
Made more progress than expected, have a prototype that builds and
supports very basic block layout and paints. Will keep working for the
rest of the week, should have data to share by the end of the week.
Work is on a [chromium.org](http://chromium.org/) experimental branch, see bug
for URI.
(leviw, dgrogan, ikilpatrick, shanestephens)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- No update since last week -
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Intersection observer work is dragging on for much longer than
originally anticipated. Got a bunch of feedback on the intent to ship
that will be incorporated into the feature before shipping. (szager)
- Wrap up intersection observer work leading up to the branch point.
(szager)
Text (eae, drott, kojii)
- Landed two emoji fixes for line breaking and for skin tone modifiers,
these are now enabled and work fine on Android. Works with line
breaking and shrink-wrapping too. One remaining issue flagged by the
android team is with backspacing in content-editable where we delete
character by character rather than glyph by glyph. (drott)
- Helped out with font regression, tracked down the issue and worked
with windows team and testers to resolve it. (drott)
- Resuming work to fixing shaping across unicode ranges, should have
a patch ready by EoD. (drott)
- Resume small-caps work. (drott)
- Looking into line breaking performance, working on an implementation
that would take advantage of per glyph measurements to find the ideal
position and then scan backwards for the first preceding breaking
opportunity. Should performance a lot better than our current
implementation which always scans from the front of the string and
considers each break opportunity up until the end of the line.
Handling of collapsible white-space might be a problem, it's currently
implemented as a part of both the measure, line break and paint steps
rather than as a separate stage. (eae)
HTML Tables (dgrogan)
- Landed first patch, Table cell background painting. (atotic)
Misc
- Asyncathon last week, bringing together all the rendering leads to
discuss our strategy around async for the next year. See separate
update from Dru for details. (eae, ikilpatrick)
- Fixed rebaseline bot and re-worked it to run as a separate service as
opposed to as a client app on a desktop machine. (wkorman, szager)
- Helped the pain team diagnose a couple of top crashers. All have now
been resolved. More lifecycles issues. (leviw)
- Talked to julien and told him about my work on collapsed borders, he
liked what I did and agreed to review it. (atotic)
