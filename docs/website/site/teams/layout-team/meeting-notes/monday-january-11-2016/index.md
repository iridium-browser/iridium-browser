---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-january-11-2016
title: Monday, January 11, 2016
---

The layout team is a long-term team that owns the layout code in blink.

See https://www.chromium.org/blink/layout-team for more information.
Updates since last meeting (on Monday, January 4th):
Scrolling (skobes, szager) \[crbug.com/417782\]
- Launched smooth scrolling! Top user request for year and one of our
oldest open feature requests. Congratulations! \[skobes\]
- Swamped with smooth scrolling issues. \[skobes\]
- Biggest issue is we can't tell wheel scrolling form trackpad scrolling
on windows, might end up enabling smooth scrolling for trackpads
too. \[skobes\]
CSS Flexbox (cbiesinger) \[crbug.com/426898\]
- The behavior changes in December caused a number of problems on high
profile websites. A lot of triage and bug fixing to follow.
CSS Multi-column (mstensho) \[crbug.com/334335\]
- Bug fixing for nested multicol and print support.
CSS Houdini (ikilpatrick)
- Plan to send intent to implement for CSS Paint this week.
- Spec work; draft of css custom layout api in time for houdini meeting
in Sydney at the end of the month.
- Meeting with MS and Apple in Seattle last week, very productive in
terms of getting on the same page with regards to custom layout and
suggesting something that all rendering engines can support.
Add API for layout (leviw, pilgrim, ojan) \[crbug.com/495288\]
- Wrapping up API around inline layout.
Intersection Observer (szager, mpb) \[crbug.com/540528\]
- Enabled on trunk, work continues.
Text (eae, drott, kojii)
- Tracking down AAT font slowness on mac. Looks like we have a great
patch that will improve it by at least an order of magnitude. We had
a couple of issues with 16s rendering for a simple page. Due to AAT
artificial fallback slowness, going through the fallback path up to five
times unnecessarily. (drott)
- Emoji segmentation, assisting fallback for emoji and punctuation.
Initial approach didn't quite work as expected due to complex cases,
have a new implementation that looks better and solves all the
known cases. (drott)
HTML Tables (dgrogan)
- We had an artificial limit for row-span, increased it to match the FF
artificial limit in response to developer bug reports.
Logistics:
- This is the last layout team meeting for jchaffraix as he is leaving
the team and blink as of the end of this week. Thanks for all your
work on Blink over the years Julien and best of luck on your next
projects.
- cbiesinger in Tokyo until Jan 20.

- eae, leviw in Tokyo next week.
