---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-february-2
title: Monday, February 2, 2015
---

Performance Tracking (benjhayden)
- Running perf tests on Android, have preliminary data.
- Loading fonts takes up a lot of time for some specific tests, these
tests have very simple layout though so font-loading is the only
"slow" thing.
- Most test have a more even distribution that looks very similar to
linux.
- Added histogram for layout times to collect user data. Might add more
detailed ones (text layout vs block layout etc) later.
Rename Rendering -&gt; Layout \[[crbug.com/450612](http://crbug.com/450612)\]
(dsinclair, bsittler,
jchaffraix, eae)
- Moving rendering to layout. Using spreadsheet to coordinate.
- Moved hit testing last week, moving compositing now (dsinclair)
First-letter (dsinclair)
- More Clusterfuzz issues.
Line Boxes (hartmanng, szager) \[[crbug.com/321237](http://crbug.com/321237)\]
- Continuing on float to layout unit conversion. Looks like a few more
classes will need to move to layout unit.
- hartmanng and szager to sync up on status and strategy.
Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Started working on updating flexbox implementation to match latest
spec revision.
- Coordinating with the firefox and IE teams to ensure compatibility.
Scrolling (skobes) \[[crbug.com/417782](http://crbug.com/417782)\]
- Background painting bug has been fixed (yay!)
- window.scrollTo wired up for root layer scrolling.
- Working on issues with position: fixed.
Text (eae)
- Held text workshop with android and i18n teams last week. Will try to
share hyphenation implementation (or at the very least dictionaries)
with android. szager volunteered to lead this form the blink side,
likely in Q2.
- Looks like the i18n team has dictionaries with compatible licenses
for at least a handful of languages.
- WIP patch that removes an extra copy of the text and avoids
unnecessary work for 8bit. Needs a new version of harfbuzz which is
problematic.
Logistics
- hartmanng is perf sheriff last week.
- jchaffraix and eae in Sydney for planning meeting.
- szager out much of last week.
