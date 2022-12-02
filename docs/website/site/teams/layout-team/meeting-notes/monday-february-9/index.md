---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-february-9
title: Monday, February 9, 2015
---

\[ eae gave high level update on planning meeting; separate
announcement on that to follow \]
We added three new team members to this week which is very exciting!
Welcome aboard, we're glad yo have you!
- James Maclean (wjmaclean) in WAT, new to project warden.
- Paul Meyer (paulmeyer) in WAT, new to blink and warden.
- Koji Ishii (kojii) in TOK, new to the team but has already been helping
out with text rendering performance for awhile.
Performance Tracking (benjhayden)
- Refined android profiling pipeline, similar to existing linux one.
- Results match that of linux with a few exceptions, more time spent in
powf() and loading fonts. Almost 3% of total layout time is spent in

powf() translating color spaces for fonts.
- Started to look into FontCache on android which doesn't seem as
effective as on other platforms.
Scrolling (skobes) \[[crbug.com/417782](http://crbug.com/417782)\]
- Smooth scrolling now works the same for overflow divs as for frames.
win_blink flakiness (dsinclair)
- Worked to reduce win_blink flakiness by triaging flakiness and
timeouts.
- Flakiness went from 40% -&gt; 10% (!), it is now one of the faster bots.
- Much of remaining flakiness due to font antialiasing/subpixel
rendering differences.
Rename Rendering -&gt; Layout \[[crbug.com/450612](http://crbug.com/450612)\]
(dsinclair, bsittler,
eae, hartmanng)
- All non-prefixed files renamed. 188 prefixes files remaining.
(dsinclair)
- Most of the linbox tree has been moved. (hartmanng)
List markers (dsinclair)
- Trying to figure out how to deal with an ancestor being moved. Opera
has some ideas and there might be multi-col logic that can be reused.
Text (kojii, eae)
- Make RenderCombinedText less obstructive to the line breaker and
paint code. Mostly [donecrbug.com/433176](http://donecrbug.com/433176),
selection paint issue
[crbug.com/438852](http://crbug.com/438852) and a bit more cleanup still
remains. (kojii)
- Complex path perf issue that had been stalled due to the slow legal
permission is finally clear now, hope to re-start with Dominik this
week. (kojii)
- Folded line_break into TextBreakIteratod, removed unused logic and
de-templetized a bunch of functions. (eae)
- Difference in preferred width between text and containing element
caused regression on Facebook due to strict overflow handling.
Has fix but writing layout test is difficult due to simplified
text-rendering pipeline for tests. (szager)
- Fixed ZWJ handling on simple text path on Mac. (eae)
