---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: april-2019
title: April 2019
---

<table>
<tr>

<td>APRIL 2019</td>

<td>Chrome Animations Highlights</td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh4.googleusercontent.com/e4d0yeYg2osuLzXs4PZ-Mocx7ILQEqgXJKvI2K5Lopqki3LxuCKFywsPcH-9JDiVOYCB4kF2I_zqkY3mGDDjBrPibOst19SBWkmVM3a0DagRZYZgm6UB5jz0k-PujynP5S8w1YS1" height=359 width=462></td>

<td>It looks the same as you would see in Chrome Stable, but it's totally running off-thread. We promise!</td>

<td>Off-Main Paint Worklet</td>

<td>Work continues on our efforts to take Paint Worklets off the main thread and run them from the compositor. With the <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1525160">blink → cc communication</a> and the worklet thread<a href="https://chromium-review.googlesource.com/c/chromium/src/+/1531153"> dispatching logic</a> landed, we are able to render a Paint Worklet off the main thread for the first time! There's still some way to go, but this is a vital step towards smoothly animating Paint Worklets.</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/P0KFfWlfC7wj43S6F4IlkfQ5poIbTEFbtIxQWxRT-CEpYFR-0A-JfdpZzkE01Qw3I_TiiNB8KngKzhPQM-DCzweaW-escsvEDD3PMXnUFJxnQXv-a3ajsMTP1nDv0BukWpkEufeL" height=166 width=227></td></td>

<td><td>Scroll Snap</td></td>

<td><td>With sunyunjia@ sadly leaving the team, girard@ took over finishing up the work on scroll snap. Initial ramp-up went well; discovered <a href="http://crbug.com/944184">an issue</a> with scrollbar arrow buttons and scroll snap, file a <a href="https://github.com/w3c/csswg-drafts/issues/3752">spec bug</a> and <a href="https://github.com/w3c/csswg-drafts/commit/09e040f05e12bb4c303d5839e0ceee3f6bf58c67">patched the spec</a>, and Gene is now working on <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1534988">a fix</a> for the Chrome implementation.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/qTkpiW1DKDbFNM12EZdAmQ7OgdOCE8ZQPZtvqIb6Za4WC-mWlNDCmaQ3zc5Bdd1R2uEaD-dvClmRRVblyw5ChI_lRLJ3uLR6GhDp-wAYSc48ziHdOSY8F5MqRzFAF2njgYJycyle" height=172 width=172></td></td>

<td><td>Shipping more of Web Animations!</td></td>

<td><td>Long overdue, some more parts of the Web Animations API are going to be shipping soon - <a href="https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/1EQKF7il48U">AnimationEffect and (some of) KeyframeEffect</a>, and the <a href="https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/UMxgGkxhdCo">Animation constructor</a>. </td></td>

<td></tr></td>
<td><tr></td>

<td><td>BlinkOn is coming!</td></td>

<td><td>Our team has two presentations planned for BlinkOn 10. We also have planned a collaboration session with Microsoft engineers around animations and related topics.</td></td>

<td><td>BlinkGenPropertyTrees</td></td>

<td><td>We continue to support the paint-team's project to move property tree generation to Blink instead of the compositor. This has required significant changes in how element ids are treated in composited animations, and has taught us about where our tests fail to catch regressions.</td></td>

<td></tr></td>
<td></table></td>

<td><img alt="image" src="https://lh4.googleusercontent.com/SMf7AoFH-HS2DqUKTgH4zFTnynOaoDXioRMPyeFiO-I5bJFrT2X1NiaB6YeetcUQ0BOtLeIjwNGOWF9EkzBliu4oY1Nc6C3jZdY6XN_gVMQHA9JK9SJ3u_c375BOAZPzXimTsJKn" height=92 width=596></td>

<td>Viz Hit-Testing Surface Layer</td>

<td>Tackling blocking issues preventing promotion of new version to 100% of Beta users. LTHI::BuildHitTestData was reduced from ~400ms to 0.3ms in a pathological case (6000+ layers on a page), and a BlinkGenPropertyTrees masking bug was resolved. Working on tackling low fast-path rate on Linux; multiple fixes in progress which are expected to bring it in line with other platforms.</td>

<td><img alt="image" src="https://lh5.googleusercontent.com/BGpN4LK3CTdcACVHZk4Obno9IbfAq1oSy2a3-f-uUuEwkn0_6bmAigxmZgukpJtcPPZvPI2EnKSdcgBiV4yxBhKyCwvVpw0vkhJI8An_fLZS89nZfktPFRl2DErvnik2NGYucPPZ" height=127 width=283></td>

<td>Animation Worklet - the road to shipping</td>

<td>Animation Worklet continues to make progress on its road to shipping. Notable features this sprint included finally making fully-asynchronous Animation Worklets as <a href="https://drive.google.com/file/d/1jF3bQlFKkbv4Sz7Q6r2rPDGxewXdw0Zl/view">buttery smooth and jank free</a> as they deserve to be (multiple weeks of work, well done kevers@ and flackr@!), <a href="https://crrev.com/386583bbe5a57761ff36bf925066f281b57d9d26">proper i18 support</a> for naming worklets, and progressing on <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1488312">support for stateful animations</a>. The Animation Worklet Origin Trial has been <a href="https://groups.google.com/a/chromium.org/forum/#!msg/blink-dev/AZ-PYPMS7EA/Fo76_FuFBAAJ">extended until M75</a> to gather more data as AMP rolls out its AnimationWorklet-based solution to all users!</td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | April 2019</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
