---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-july-18-2016
title: Monday, July 18, 2016
---

Updates since last meeting (on Monday, June 27, 2016):

Back to our regularly scheduled programming after the chrome team wide
two no-meeting period.
Scrolling
- Work on root layer scrolling continues, will carry over into Q3.
(szager)
Scroll Anchoring \[[crbug.com/558575](http://crbug.com/558575)\]
- We now have a real spec for scroll anchoring \[1\]. (skobes)
- Sent Intent to Implement \[2\] to blink-dev. (skobes)
- Added support for the css overflow anchor property. (skobes)
- Plan to continue bug triage work this week.
CSS Flexbox
- Flexbox bug triage and bug fixes. (cbiesinger)
- Two known remaining issues for flexbox:
- Min-size auto for nested column flexboxes.
- Getting better metrics for new intristic size to see if it can
potentially be enabled by default.
(cbiesinger)
CSS Grid Layout \[[crbug.com/79180](http://crbug.com/79180)\]
- No updates since last week. See tracking bug for status.
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- No updates since last week. Morten out for the next few weeks.
CSS Houdini
- Pretty large update to the Houdini layout API spec, got into some
really gnarly issues like out of positions floats in the middle of
words and how to make the constraint space API sane for developers.
Check it out \[3\], comments welcome! (ikilpatrick)
- Going through and seeing what worklet stuff needs to be done for web
audio, been meeting with the TOK team for worklets and will continue
that this week. (ikilpatrick)
- Fixed zoom in CSS paint worklets. (glebl)
- Fixed a small regression with select elements complicated by a lack
of tests. (glebl)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- The design doc for LayoutNG has been updated and the feedback from the
layout team incorporated. Thanks for your help and input everyone!
It's all but finished and I plan to circulate among the wider blink
team later this week. (eae, ikilpatrick)
CSS Containment \[[crbug.com/312978](http://crbug.com/312978)\]
- Shiped in M52. No further updates will be provided. (eae)
Intersection Observer (szager, mpb)
\[[crbug.com/540528](http://crbug.com/540528)\]
- Shiped in M51 with the last few fixes and improvements shipping in 52.
No further updates expected for the time being. (szager)
Resize Observer (atotic)
- Got LGTM from szager, other reviewers posetive but suggested splitting
implementation into multiple patches. Working ongoing. (atotic)
- Hope to land initial Resize Observer implementation this week.
(atotic)
- Set goals around event loop targets for Q3. (atotic)
Tables (dgrogan)
- Fixing a class of bugs around table borders. (dgrogan)
- Plan to work with the Sydney-based style team with a couple of table
issues blocking the work to separate style resolution from layout tree
construction. (dgrogan)
- Next up is adding support for visibility: collapse to table rows and
columns. (dgrogan)
Text (eae, drott, kojii)
- Complex text enabled by default on Android again (yay!), there are a
number of small regressions with regard to both performance and memory
but nothing like the big regressions we've seen in the past. Still
deciding on strategy. (drott)
- Merged font manager change corresponding regression fix to M53.
(drott)
- Next, planning to go back to looking at the glyphToBoundsMap removal -
there were still regressions in SVG tests that I need to figure out.
(drott)
- Meeting with Docs team tomorrow to discuss what could be done in
Chrome to improve Docs. (drott)
- Fixed a couple of crashes involving LayoutTextControl. (eae)
Misc:
- Fixed SVGLength crash. (eae)
- Up until now eae has been doing all bug triage for the layout team.
Now that the triage backlog is zero it's time to start a rotation
among all team members. Expect it to start next week and to involve
one week long stint as a triager every two months. (eae)
- Wrote up instructions for bug triage \[4\]. (eae)
Logistics:
- szager out for the next two weeks.
- mstensho out for the next month.
1:
<https://www.google.com/url?q=https%3A%2F%2Fcdn.rawgit.com%2Fymalik%2Finterventions%2Fmaster%2Fscroll-anchoring%2Fspec.html&sa=D&sntz=1&usg=AFQjCNGmw2oJ64KP0QB3z2UM2UBvX0kCdg>
2:
<https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/TbuxbOtyZvk>
3: <https://drafts.css-houdini.org/css-layout-api/>
4: <https://www.chromium.org/blink/layout-team/bug-triage>
