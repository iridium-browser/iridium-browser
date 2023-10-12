---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-september-28-2015
title: Monday, September 28, 2015
---

Updates since last meeting (on Monday, September 14th):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Getting closer to enabling coordinated scrollbars everywhere. Currently
enabled everywhere on Mac and Android but only for the main frame on
Windows and Linux. With these changes the main frame will no longer be
a special case and will instead use the overflow scroll
implementation. (skobes)
- Have two patches out for review that change how we paint scroll
arrows. Once those have landed we can turn on coordinated scrollbars
on all platforms. (skobes)
- Will spent some time doing bug triage this week. (skobes)
- Looking to land patch to web scrollbar theme painter, starting to have
a lot of dependent bugs, lots of of duplicate bug reports. (szager)
- Leading space char clusterfuzz bugs. (szager)
- Pushing forward to enable root layer scrolling for layout tests and
starting to triage failures. (szager)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Adding a use-counter for deprecated intrinsic sizing.
- Fixing up css working group test suite for flebox, hope patches will
be accepted/merged soon.
- Import css test suite into layout test. Plan is that the css working
group repository is the source of truth that we mirror periodically.
- Plan to upstream some more of our tests.
CSS Grid Layout (svillar) \[[crbug.com/79180](http://crbug.com/79180)\]
- No update since last week.
Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]
- Multicol balancing refactoring (do it after layout). The multicol part
of it has been ready for weeks, but before landing it, some changes to
strut handling in the LayoutBlockFlow neighborhood are required. I've
landed some simplification, clean-up and bugfixing CLs in the area on
my way. Hopefully it's soon sensible enough to hold my changes without
breaking under its own weight. (mstensho)
- Still working on pagination strut rewrite. Want to get it right and as
pretty as possible first before landing. (mstensho)
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Landed several large line layout patches, after one that is pending
review we will have gotten rid of all LayoutObject references in
InlineTextBox, i.e. we'll use go through the API for all layout calls.
(pilgrim)
- Working our way up the hierarchy to get rid of the remaining calls not
using the API in InlineBox. (pilgrim)
Text (eae, drott, kojii)
- Meeting with Docs team to discuss text metrics, line layout needs.
(eae)
- Working on fix for selection painting on high-DPI devices. (eae)
- Meeting with Android team to discuss sharing text layout
implementation or parts thereof. (eae)
Misc:
- Fixed a bug where the scrollWidth and scrollHeight where snapped
differently than clientWidth and clientHeight. (leviw)
- Working with Windows team on re-implementing our high-DPI support to
use our full-page-zoom logic instead of specialized code in the
compositor. Will allow for much better support for non-integer
device scale factors, reduce code complexity and (finally!) allow us
to snap to device pixels instead of CSS pixels. (eae)
- Supporting houdini hackathon last week. (leviw)
- Houdini/custom layout discussions. (shans, eae, leviw, kojii)
- Landed change to disable selection gap painting! (wkorman)
- Mac 10.10 baselines. (leviw)
- Supporting work to enable color emoji on Windows. (eae)
Logistics:
- eae in NYC Wed-Sat.
