---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-november-23-2015
title: Monday, November 23, 2015
---

Big update this weeks as we didn't have a meeting during BlinkOn and I
apparently forgot to send out notes for last weeks meeting.
Updates since last meeting (on Monday, November 2nd):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Unified animation curves across Blink and CC. They now share the same
curve. (skobes)
- Looking into RTL scrollbar placement for root layer scrolling.
(skobes)
- Re-started work on root layer scrolling after focusing on smooth
scrolling and bugs for a few weeks. (skobes)
- Landed fix for scrollbar clipping, basically scroll containers should
clip their scrollers but they don't. Chocking they didn't but now
works as expected. (skobes)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Fixed a number of regressions relating to recent changes in behavior.
(cbiesinger)
- Spent some time not fixing a flexbox bug for a change, had to do with
flexbox and scrollbars. cbiesinger came in and stole my thunder.
Thank you :) (szager)
- Fixed the release blocker szager was working on. Was investigating a
different bug that turned out to be the same. (cbiesinger)
- Fixed another release blocker, affecting google translate.
(cbiesinger)
- Moar release blocking bugs! (cbiesinger)
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- Simplified interface of GridResolvedPosition. (sergio)
- Grid sizing under min|max constraints with implicit sizes. (sergio)
- Added support for implicit grid before explicit grid. (rego)
- Absolutely positioned items inside grid adjustments. (rego)
- Landed a small refactoring of grid layout code. (rego)
- Gave talk at BlinkOn 5 about recent work on grid layout and what the
future holds. (javif)
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Bombing Levi with multicol patches as usual, and he's an unstoppable
reviewing machine.
- The way it looks now, we're one patch away from being ready to
introduce multicol for printing. Hopefully, I'll file that patch today
or tomorrow, and then add support for printing later this week. But
we'll see...
CSS Houdini (ikilpatrick)
- Writing design doc for worklets, will circulate for feedback at EoW.
- Custom line layout discussions and hack-a-thon in Tokyo two weeks ago,
we came up with two proposals that we're refining and will prototype.
(eae, shans)
- Planning to re-implement ruby support on top of said API, got input
from kojii and started experimenting. (eae)
Standards work
- Published 2nd CR of CSS Writing Modes Level 3. (kojii)
- Published UTR#50 Unicode Vertical Text Layout revision 15, supporting
Unicode 8.0. (kojii)
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Line layout, last patch got rid of it in LineText delayed due to
conflicting advice and opinions. (pilgrim)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- Plan to send Intent to Implement this week.
- Have pending spec patch that needs to be upstreamed.
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Everyone understands how it should work but managing the lifetime of
the objects are extremely hard is it involves relationships between
two objects with very different documents and lifetimes. Makes it very
tricky. You'd think oilpan would make it easier but so far it seems
that the opposite is true. There is an obvious leak and it's not clear
how to resolve that without having to do a lot of manual bookkeeping.
Trying to figure out a way to make it work in an oilpan world, still
not clear if that is even possible. Working with adamk to try to
figure it out, getting closer but haven't nailed it yet. Similar to
mutation observer in that it is really difficult not to leak. (szager)
- Once lifetime management has been resolved the plan is to get it into
the code base behind a flag. Then to have people give it a try and
kick the tires while, at the same time, continue to spec work and then
ship (unflag) early next year in beta, assuming spec work isn't
delayed. (szager)
Text (eae, drott, kojii)
- Spent time helping drott investigate an emoji joiner issue on android.
(workman)
- Fixed emoji joiners on Windows. (drott)
- Worked on regression due to the new shaping code where a secondary
font wasn't loaded correctly. Turns out to be a copyright protection
strategy used by some fonts. Used on
[virginamerica.com](http://virginamerica.com/) and a few other
high profile sites. (drott)
- Wrote and test emoji complex path on Webview_shell, merged to M47 and
M48 (Thanks to wkorman@ for sharing Android tricks and test phone),
reached out to WebView TL in LON regarding final verification on the
Beta build. (drott)
- Investigated T-crossbar issue: woff2 or freetype hinting issue,
[crbug.com/550523](http://crbug.com/550523). (drott)
- Prepared CL for font code memory infra instrumentation, instrumenting
word cache done, but larger remaining issue with tracking web font
blobs down to SkTypeface. (drott)
- Discussed deduplicating fallback font streams on linux with skia
folks, fmalita@ added API to SkFontconfigInterface, which on closer
inspection unblocks as, we can implement deduplication on Blink side
[crbug.com/524578](http://crbug.com/524578). (drott)
- Prepared harfbuzz roll, failed building on Mac, filed issue, fixed by
behdad, new release expected. (drott)
- Investigating Houdini custom line layout and ruby on top of it.
(drott)
- Investigating bidi-isolate by default for elements with dir attributes,
working with WebKit, Gecko, Edge, and W3C I18N WG. (drott)
- A few issues in bidi-isolate were found, working on them. (drott)
- Changed the life time of ShapeCache instances and the logic for cache
invalidation. Resulted in a massive layout speed up, more than
offsetting the regression we saw on a few micro benchmarks following
the switch to always use the complex text path. Unblocks the effort to
ship complex-by-default. (eae)
- Started looking into further tweaks to ShapeCache lifetime and
invalidation, more work to be done. Should reduce memory usage. (eae)
HTML Tables (dgrogan, jchaffraix)
- Investigating table bugs with jchaffraix. Looking into an issue where
there there are too many borders on our tables due to a bug in border
collapsing. (dgrogan)
- Helping dgrogan ramp up on tables. Here every Monday for that reason.
He has one patch up, looking good! (jchaffraix)
Bug Health
- Both eae and jchaffraix spent a lot of time last week doing bug triage
as per usual. Another two weeks or so should see our untriaged backlog
go town to zero! Yay!
- Total bug backlog, down by ~18%. If we keep this level of effort up we
should be able to hit our goal of a 20% reduction by EoQ. (eae)
Misc
- skobes, drott, ikilpatrick, and javif presented at BlinkOn. All their
sessions where well attended and very well received. Thank you!
Videos should be available within a week or two. (eae)
- Windows FixIt last week. (wkorman)
- Presented at QConf and Chrome Dev Summit. (ikilpatrick)
- Plan to get back to rebaseline bot on the bot this week. Looked into
file-locked bug where we cannot delete files due to them being locked.
Fun time untangling all the threads. (wkorman)
- Start flipped blocks work as per outlined earlier! (wkorman)
- Spent time last week working on a background image spriting bug that
we've had forever. We didn't get around to fix it back when we
implemented subpixel layout. Apple finally fixed it, in a way that's
very different from how we're approaching it. Taking a lot of time to
get things exactly right because we decide on the exact box to take
before making a final snapping decision during layout. Have patch that
is mostly ready, should be able to finish it this week. (leviw)
- Looking at a crashing big in frame loader. (skobes)
Logistic
- Looks like a quite week with everyone back in their home office.
- Short week in the US due to Thanksgiving (Thu-Fri off).
