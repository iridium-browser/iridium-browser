---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-august-17-2015
title: Monday, August 17, 2015
---

Updates since last meeting (on Monday, August 10th):
Scrolling (skobes, szager) \[[crbug.com/417782](http://crbug.com/417782)\]
- Moved scrolled-by-user flag out of FrameView. (skobes)
- Custom scrollbars are now working with root layer scrolling. (skobes)
CSS Flexbox (cbiesinger) \[[crbug.com/426898](http://crbug.com/426898)\]
- Got go-ahead to unprefix intrinsic sizing keywords (min-content,
max-content, fill and, fit-content). Will go ahead and unprefix all
but fill (currently -webkit-fill-available) as there is still some
open questions around it and the CSS working group doesn't quite think
it's ready for prime time yet.
- Added use counter for prefixed intrinsic size keywords.
CSS Grid Layout (svillar) \[[crbug.com/79180](http://crbug.com/79180)\]
- Working on refactoring min/max ContentForChild calls to share more
code and logic.
Region-based multi-column support (mstensho)
\[[crbug.com/334335](http://crbug.com/334335)\]
- Landed initial support for nested multicol layout. Column balancing
still needs work but the most common use cases work.
- Plan to hook up printing code once I'm more confident about the
implementation. Will finally enable multicol for printing for the
first time ever in Blink. :) (mstensho)
Add API for layout (leviw, pilgrim, ojan)
\[[crbug.com/495288](http://crbug.com/495288)\]
- Landed several API patches and finishing up API conversion for box
model object. Estimate about a weeks worth of work remains. (pilgrim)
Text (eae, drott, kojii)
- Debugging font matching issue on windows, looks like Skia is not
returning the full font name. (drott)
- Implemented tab characters for complex path, one of the blockers to
unify simple path to complex path. (kojii)
- Landed font fallback cleanup to fix a crash bug with support from the
memory team. (kojii)
- Enabled Unicode Variation Selector for Chrome OS. (kojii)
- Eliminated the last remaining direct caller to HarfBuzzShaper for
better layering. (kojii)
- Fixed handling of invalid and unmatched UTF-16 surrogate pairs, we now
replace invalid pairs with a replacement glyph and keep processing the
rest of the text node while before we would abort after the first
invalid character and not paint the remaining text. (eae)
- Fixed handling of tabs in complex path in cases where tabs are not
supported. (eae)
Misc:
- Cleaned up PageBoundaryRule and nextPageLogicalTop. (mstensho)
- Fixed issue with auto-height table cells and percentage heights that
broke a couple of popular legacy websites. (mstensho)
- Issue with win10 bots where hundreds of tests where failing and tools
not yet updated to support win8 or win10 specific results, since
resolved. (cbiesinger)
- Got go-ahead to upstream layout tests to W3C. (cbiesinger)
- Discussion around layout test standards and guidelines, jsbell gave
update on plan to support web platform tests and will send out further
information when ready.
- Wrapping up custom properties. (leviw)
Logistics:
- cbiesinger gardening last week (Thu-Fri).
- leviw gardening this week (Mon-Tue).
- Pre-CSS F2F meeting in SF on Tuesday.
