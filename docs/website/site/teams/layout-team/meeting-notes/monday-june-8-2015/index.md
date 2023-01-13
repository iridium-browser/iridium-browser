---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-june-8-2015
title: Monday, June 8, 2015
---

Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Ran into problem with hit testing absolutely positioned objects within
a scrollable element. Hit testing code thinks the element is in front
of the scrollbar. (skobes)
- Plan to get back to viewport issue next, once the abs hit testing
issue has been resolved.
- szager getting ramped up on scrolling.
Line Boxes (szager) \[[crbug.com/321237](http://crbug.com/321237)\]
- Done and seems to be sticking, some cleanup/monitoring left but no new
major issues encountered.
Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Continuing to update to match spec, progress according to plan.
Misc Warden (dsinclair, pilgrim)
- Containing work to slim-dopwn LayoutObject. (pilgrim)
Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]
- Removed old multicol code, yay! (mstensho)
- Fixed multicol fuzzers, and also a completely unrelated fuzzer in the
CSS counters code. (mstensho)
Removing DeprecatedPaintLayer (chadarmstrong)
\[[crbug.com/260898](http://crbug.com/260898)\]
- Current plan is to move hit testing code to stacking node instead of
box model object. This prompted a discussion about the reason behind
the change and the relative merits of retaining the stacking context
concept vs moving hit testing to be self contained and on box model
object. chadarmstrong/jchaffraix/ojan to follow up offline.
Text
- Imported CSS Writing Modes test suites, minor fixes and feedback of
our run results to CSS WG continues. (kojii)
- Unicode variation selectors support in progress, close to complete the
basic support. (kojii)
- Investigating orthogonal writing modes issues. (kojii)
- Investigating abspos issues in vertical flow. (kojii)
Add API for layout (leviw, pilgrim)
\[[crbug.com/495288](http://crbug.com/495288)\]
- pilgrim and ikilpatrick working on getting initial API definition
landed.
Logistics
- dsinclair GPU sheriff (pixel wrangler) this week.
- Planning meeting for Moose on Wednesday, notes will be sent out to
layout-dev, talk to eae if you want to be included.
