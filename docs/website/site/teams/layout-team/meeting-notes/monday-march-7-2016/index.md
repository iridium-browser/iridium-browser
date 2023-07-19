---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-march-7-2016
title: Monday, March 7, 2016
---

Updates since last meeting (on Monday, February 29):
We had a new person join the meeting for the first time this morning,
Aleks Totic (atotic). He join Google about two months ago and his
starter project is Resize Observer. Welcome Aleks!
Scrolling (skobes)
- Have a change in review that fixes a bunch of things with scroll
anchoring, with this change it is pretty close to usable on real-
world websites. (skobes)
- Smooth scrolling bugs (have four of them now, yay). (skobes)
- Reacquainted myself with scrolling stuff, was looking into a scroll
related bug that skobes volunteered to take on. (szager)
CSS Flexbox (cbiesinger)
- Worked on flexbox percentage sizing, going well. Will continue this
week.
- Last week, re-landed the fix for scrolling that fixes everything after
a fix by kojii that caused a revert (layout scheduling at a stage
where one should not happen).
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last week -
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- No update since last week -
CSS Houdini (ikilpatrick)
- Kast week in Waterloo with compositor worker folks, had fun getting
compositor worker to work with worklets (that's a lot of work\* words!)
and with a cleaner API. Have a new API version, will try to move into
trunk sometime later this month.
- About to commit a small change for main frame worklets API.
- Want to get paint rendering context 2d check-in this week.
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Continued making progress on the box layout api, have sense of size of
problem. Basically every element will have to have a layout item
instead. About ~40% of them have their own element specific things
which means that we'll need element specific block element API.
Working through that list and figuring out the hierarchy. Projecting
how large of a project it is. (pilgrim)
- So far most element specific APIs have been pretty small with only a
method or two, on the leaf node level. a lot of the higher level
elements (like layout image, layout media, layout box etc) have more
complicated APIs that will take more time. (pilgrim)
- Spent much of last week burning down the list of tasks for the Line
Layout API. Only size invalid calls left, hopeful then we can change
the constructors and add include guards this week then call the Line
Layout API done. (dgrogan)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- No update since last week -
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- Landed Layout Containment! <https://codereview.chromium.org/1530303003>
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Landed a couple of intersection observer changes last week, have a few
more coming but no longer a full-time endeavor. (szager)
- Focus on bugs this week. (szager)
Text (eae, drott, kojii)
- Coming back from tok, working on remaining on emoji font fallback.
After coordinating with android team changed how we get the fallback
font. When android releases the SDK preview for N then we can activate
fitzpatrick modifiers. Required further segmentation fixes. (drott)
- Fixed line-breaking issues with emjoi combiners. Not yet in ICU,
implemented on top of it for now. Will move to ICU over time. (drott)
- This week, plan to add unit tests for line breaking and then land and
coordinate with Android team. (drott)
- Fixed how we query the operating system for font settings relating to
font smoothing and LCD subpixel rendering on Windows. Previously we
tried to read the registry values from the rendering which wouldn't
work with the sandbox lockdown, changed it to query the OS from the
browser process and pass it down to the renderer. Fixes issues where
we don't respect the users font settings and also ensures consistency
across chrome and blink. (eae)
- Discovered a pretty bad perf regression for break-all. Our performance
has always been bad but got worse with complex text. Investigating and
have some ideas as to how it can be fixed. (eae)
HTML Tables (dgrogan)
- No update since last week -
Misc
- Started working on how we render table backgrounds with collapsed
borders, pulled a thread and the whole thing started to unravel.
Discovered out that collapsed borders are painted in a crazy way,
trying to clean up. (atotic)
- Spent a full day helping wkorman with the rebaseline bot, makes me
realize how happy I am that I left infra and moved to blink where
things are sane and predictable. (szager)
- Still dong analysis work for writing modes. (wkorman)
