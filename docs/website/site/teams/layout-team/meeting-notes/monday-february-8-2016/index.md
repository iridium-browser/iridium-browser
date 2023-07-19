---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-february-8-2016
title: Monday, February 8, 2016
---

Updates since last meeting (on Monday, February 1st):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Basic implementation of scroll anchoring landed behind the
--enable-blink-features=ScrollAnchoring flag. (skobes)
- Working on adding more test cases for scroll anchoring. (skobes)
- Adding about:flags entry so folks can try on mobile canary. (skobes)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Continued triage.
- Fixed main size calculation for overflow:auto.
CSS Grid Layout (svillar, jfernandez, rego, javif)
\[[crbug.com/79180](http://crbug.com/79180)\]
(since last grid update in late December)
- Fix auto track sizing with min-size:auto (svillar)
- Add support for repeat(auto-fill|auto-fit,) (svillar)
- Support for implicit grid before explicit grid (rego)
- Fix unknown named lines resolution (rego)
- Investigating issues with positioned items and RTL (rego)
- Layout tests refactoring (jfernandez)
- Investigating issues in orthogonal grids (jfernandez)
- Add "normal" for content alignment (jfernandez)
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Working on cleaning up mapLocalToAncestor() in order to make it easier
and cleaner to fix a multicol bug.
- Intent to ship discussions continues, we might be able to unprefix
multicol real soon after all!
- Some work on the break-after, break-before and break-inside
properties. More or less ready to file a CL, once the intentery gets
resolved.
CSS Houdini (ikilpatrick)
- Stay tuned for a separate blonk-wide update on Houdini progress.
Add API for layout (leviw, pilgrim, dgrogan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Converted BlockPainter to new line layout API. (pilgrim)
- Convert last use of RootInlineBox to the API. (dgrogan)
- Shim AXLayoutObject uses of InlineBox-&gt;layoutObject(). (dgrogan)
CSS Containment (leviw) \[[crbug.com/312978](http://crbug.com/312978)\]
- Discussions about containment at CSS Working Group meeting with
standard body and other browser vendors. (leviw)
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Fixed handling of zero-sized target elements for observations.
(szager)
- Fixed handling of cross-origin observations. (szager)
- Changed observers to use DOMHighResTimeStamp for notification time.
(szager)
Text (eae, drott, kojii)
- Meeting with the Android fonts team to discuss needs and possible
integration going forward. (drott)1
- Working on resolving emoji and multi-locale issues on Android, some
parts are tricky, some parts are easier. Will continue to work with a
android fonts team after the Tokyo workshop. (drott)
- landing the remaining parts of SymbolsIterator/EmojiSegmentation,
until now where I am hitting this issue of having to address graphemes
as a unit of fallback with Behdad first - as per my comment on the
last CL in the series. (drott)
- Working on font fallback for symbol/emoji fonts on Android with
bungman on the Skia team. (drott)
HTML Tables (dgrogan)
- No updates since last week.
Misc
- Finished work to make LayoutUnit construction explicit. (leviw)
- Fixed weird broken zooming logic in offsetTop/Left calls. (leviw)
