---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: september-2021---new-features-testing-code-health-interop-fixes-and-more
title: September 2021 - New features, testing, code health, interop fixes and more!
---

<table>
<tr>

<td>September 2021</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Chapter I: New features</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Capability Delegation</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/MG0xgExux0IXLJhml1xYx4KnCibC4LI9_Q9qW04jIjlFaEkh7MKOdMe49olq5XfTKyiYwb8rEeOE6n-Y8G0LXu-6T_42-4vnwi4aRXHtxGyJKJCV_CImQrLO9C-fdDg3g0RuPSnQog=s0" height=221.4676 width=124.0124> <img alt="image" src="https://lh6.googleusercontent.com/0HyrhEBh-ZWRaAxhrkkA5U4LFwor0Zsy7J1U4MpsXJv3GLxZgDSOtThkHBPlAQvvQQDZmZ9-nTX3O_2eLElAIq0hlRS9mG1sS3FvlyiBO-Sv_EsITp7NfaRqOrVzV4tNq64F40EeVg=s0" height=126 width=126></td></td>

<td><td>mustaq@ has been working on the spec proposal for the capability delegation.</td></td>

    <td><td>We got official signal from Mozilla (annevk@): <a
    href="https://github.com/mozilla/standards-positions/issues/565#issuecomment-918137857">useful/reasonable</a>.</td></td>

    <td><td>Mozilla also <a
    href="https://github.com/WICG/capability-delegation/issues?q=is%3Aissue+author%3Aannevk+">started
    bugs & enhancements</a> discussions.</td></td>

<td><td>For payment capability delegation, the origin trial is now <a href="https://developer.chrome.com/origintrials/#/view_trial/640637046993453057">ready</a> for partners.</td></td>

<td><td>Composite BG-color animation</td></td>

<td><td>xidachen@ <a href="https://critique-ng.corp.google.com/cl/395582428">launched</a> the finch study for the feature CompositeBGColorAnimation on Beta channel, with 50% control vs 50% enabled. The preliminary <a href="https://uma.googleplex.com/p/chrome/variations?sid=15e5e4da675df567f60a113eb822ccde">result</a> looks very positive (Note that currently we have &lt; 7 days of data). Here are some highlights of the result:</td></td>

    <td><td>We do see performance improvement on Animations Smoothness
    metrics.</td></td>

    <td><td>There is no memory regression on Android</td></td>

    <td><td>There is no regression on FirstContentfulPaint and
    LargestContenfulPaint.</td></td>

<td><td>Elastic Overscroll</td></td>

<td><td>flackr@ made great progress on launching elastic overscroll.</td></td>

    <td><td>The feature is now <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/3150217">on
    by default on tip of tree</a>.</td></td>

    <td><td>Merged <a href="https://crbug.com/1240789">reduce stretch amount</a>
    and <a href="https://crbug.com/1232154">prefers-reduce-motion</a> behavior
    back to M93.</td></td>

    <td><td>Finch <a href="https://critique-ng.corp.google.com/cl/396373932">min
    version updated</a> to include two recent merges and <a
    href="https://critique-ng.corp.google.com/cl/396395915">launched to 1%
    stable</a>.</td></td>

    <td><td>Launch to 100% <a
    href="https://critique-ng.corp.google.com/cl/396900469">out for
    review</a>.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Testing</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Composite BG-color animation</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/yxjOLd0Oe3UdEwcVc0hKOxt85oJgTV_DpzaLzp910D8YinzafDO8M4I_O4Ju7mr2j2vxAryh6lGXu1qSqog61i6D3_LlmxsT0dlvEM20IoOZqORNugQEOgHin-EHoaqKb9NC_MsZFw=s0" height=49 width=283></td></td>

<td><td>xidachen@ fixed some very flaky paint worklet tests.</td></td>

    <td><td>Given that these are flaky tests, the auto bisect tool doesn’t work.
    We have to manually bisect it to find out that it is due to the launch of <a
    href="http://navigationthreadingoptimizations">this new feature</a> that
    makes navigation faster.</td></td>

    <td><td>With many times of try and error, it seems that the combination of
    paint worklet test + a simple div in ref.html somehow caused the crash. The
    root cause is unknown yet.</td></td>

<td><td>Cross-platform scroll-timeline tests</td></td>

<td><td>kevers@fixed some scroll-timeline tests. The issues are:</td></td>

    <td><td>Error tolerances were too tight.</td></td>

    <td><td>Sensitive to scrollbar width (platform specific)</td></td>

    <td><td>Magic numbers in tests (tough to infer correctness at a
    glance)</td></td>

    <td><td>Misleading calculations (e.g. scrollheight - clientHeight to compute
    scroll range in both directions)</td></td>

    <td><td>Mixup of logical units in RTL tests.</td></td>

<td><td>The issues were discovered when testing via polyfill implementation. A significant number of near misses in test failures.</td></td>

<td><td>The solutions are:</td></td>

    <td><td>Compute error tolerance for percentage calculations based on a half
    pixel error in the scroll position.</td></td>

    <td><td>Hide scrollbars to ensure that scroll-range is consistent across
    platforms.</td></td>

<td></tr></td>
<td></table></td>

<td>Chapter III: Code Health</td>

<td><table></td>
<td><tr></td>

<td><td>Triage scroll unification failing web tests</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/84j_rIrI0nOqe3JLW1dWNKILLWEg5r8mwaWkxzSwjW5VkZihaJQFPCDfnU0VK1_eyYQ_mQ26CGWRmiSTDE8FW6LqoAKWV3in60STJOVorEcjsDJrjG-bzWrYw-y-WpS8_PA0o9gi1w=s0" height=231 width=582></td></td>

<td><td>skobes@ triaged failing web tests for scroll unification and the details are captured <a href="http://go/su-web-tests">here</a>. At this moment, there are 30 failing tests</td></td>

    <td><td>13 failures from 4 functional regressions (3 P1s, 1 P2), bugs
    filed</td></td>

        <td><td>Cannot touch-drag custom scrollbars, resize corners</td></td>

        <td><td>Re-latch when scroller removed from DOM</td></td>

        <td><td>Scollbar arrow click scrolls by only 1px</td></td>

    <td><td>12 failures from bad test (waitForCompositorCommit, rebaseline,
    etc)</td></td>

    <td><td>5 requiring more investigation (3 plugin related)</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: Interop fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Scroll-timeline polyfill</td></td>

<td><td>kevers@ made more progress on scroll-timeline polyfill.</td></td>

    <td><td>Proxied AnimationEffect to override timing calculations</td></td>

    <td><td>Custom AnimationPlaybackEvents to report percentage based
    times</td></td>

    <td><td>Removed time-range from scroll timeline proxy</td></td>

    <td><td>Conversion between percents for API and times for internal
    use</td></td>

<td><td>Two problematic tests remain with high failure rates, reminder average over 90% pass rate.</td></td>

<td><td>The remaining work includes update timing and event phase.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter V: Bug Updates</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/O41TurQq6k183Wi1Bg7WiqMm86Zyryrh2N4SBe26qOI9tehJiGOXVrrYn0OhPZWadxhcnCfjoAhVAFFsa9SKIz1Kj0o_63_YoYv3yK2An9kUMzhUUHkUY4-jqP6HRq49mtpro0N6QA=s0" height=153 width=275><img alt="image" src="https://lh4.googleusercontent.com/AiPQ4O6ynfCIoBuYCEXeEWtQ9bQIKTE25lLZ9ilCJZ3wsgcDb4qFIHvMrRJyxwBxL6iL83ROQUgrREMKi3I7SRDcDH4f2AknTHPEpU1w9bLmOTrzabJDweeJK4bMwumGcJUOk-qbdQ=s0" height=152 width=279></td></td>

<td><td>Our team lost a bit of ground in P2s && P3s, but kept the P1s in check.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | September 2021</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
