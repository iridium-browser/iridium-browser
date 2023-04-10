---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: may-2019
title: May 2019
---

<table>
<tr>

<td>May 2019</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>
<td><img alt="image" src="https://lh5.googleusercontent.com/y55oEKmeWfBAk_jdj6Q_izzXpBOZT6wuau4DNg6VPlbK-sThHRNyJx-OLWmLi11KVTyFORLWHzUEFaMdOEizHaMLLCyEd60zW6RyGpQP56AVWVuvWjkRQJk6_yvWAC_LMfmW84mr" height=308 width=532></td>

<td>The slight decrease in 'interface' from Chrome 75 to 76 is due to newly added tests</td>

<td>Web Animations, road to shipping</td>

<td>Our investment in interop work for Web Animations continues to produce good results, largely thanks to hard work by Kevin (kevers@). Over the last two Chrome versions our pass rate for the <a href="https://github.com/web-platform-tests/wpt/tree/master/web-animations">Web Platform Tests</a> has increased by 45% (absolute) for the timing-model tests and a massive 70% (absolute) for the interface tests! Improvements like these allow us to continue to ship more of Web Animations - recently we shipped <a href="https://groups.google.com/a/chromium.org/d/msg/blink-dev/lTYK1HT47Qk/hmwmGm1ZBAAJ">Animation.updatePlaybackRate</a> and <a href="https://groups.google.com/a/chromium.org/d/msg/blink-dev/Gstf0GA7cbg/711ymCKKAAAJ">Animation.pending</a>.</td>

<td><table></td>
<td><tr></td>

<td><td>Code Health</td></td>

<td><td>Rob (flackr@) has been focused on improving the composited animations code in the new <a href="http://crbug.com/836884">post-BGPT world</a>. This sprint he landed a series of patches (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/1611762">1</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1609672">2</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1610304">3</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1461341">4</a>) to eliminate animation specific ElementId tracking in LayerTree. This simplification makes the architecture easier to understand, improves CPU usage and reduces memory usage - a triple win!</td></td>

<td><td>Scroll Snap</td></td>

<td><td>Scroll Snap is continuing to gain traction, with AirBnB being the latest partner to <a href="http://crbug.com/920482#c9">start using the feature</a> and Firefox <a href="https://bugzilla.mozilla.org/show_bug.cgi?id=1312165#c6">continuing to implement</a> the new spec. On our side we have shifted into supporting the product: Majid (majidvp@) has been busy triaging bugs, addressing partner requests, and polishing the code.</td></td>

<td></tr></td>
<td><tr></td>

<td><td><a href="https://uma.googleplex.com/p/chrome/histograms?endDate=20190515&dayCount=7&histograms=Blink.Animation.CompositedAnimationFailureReason&fixupData=true&showMax=true&filters=isofficial%2CEQ%2CTrue&implicitFilters=isofficial"><img alt="image" src="https://lh6.googleusercontent.com/IUKoi9dwyyJhEUdaoyDeGdtFoKLI_WO1j-Bkzk1s7RxSaP9ZZYTk6ugL1KHylCTX2IE5K6Vc8zvKUpKocBFDwF48OQJatYnjlXsJSSukICRCRvLKb6q2vVaHw_SM40RBFyXQDBtm" height=113.00000000000003 width=283></a></td></td>

<td><td>Understanding performance better</td></td>

<td><td>Stephen (smcgruer@) landed a new <a href="https://uma.googleplex.com/p/chrome/histograms/?endDate=20190519&dayCount=7&histograms=Blink.Animation.CompositedAnimationFailureReason&fixupData=true&showMax=true&filters=isofficial%2CEQ%2CTrue&implicitFilters=isofficial">UMA metric</a> this sprint to track why animations fail to run on the fast path. This allows the team to better understand where effort is needed to speed up existing animations on the web - vital for smoother user experiences.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/z-ajU372TxKnGgMOwOSQa30PmFdlpv-ZAWn_gZBkcvv91oDDyy9-Bl3W3t3Wz76F7OXc-VG3wxCP9PjyxClhdVKJ_fju9h25ymlvF0X3iSQxTQNEvzGHRYOqClEi9F-tWHW2zBrp" height=237 width=203></td></td>

<td><td>Smoothly animating Paint Worklets</td></td>

<td><td>One of the key goals of the <a href="https://docs.google.com/document/d/1USTH2Vd4D2tALsvZvy4B2aWotKWjkCYP5m0g7b90RAU/edit?ts=5bb772e1#heading=h.2zu1g67jbavu">Off-Thread PaintWorklet</a> project is to enable smoothly animating Paint Worklets even when the main thread is busy. Xida (xidachen@) has been working on a <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1601334">prototype</a> of the Animation integration and this sprint was able to show smoothly animating, raster inducing (!) Paint Worklets even as the main thread janked.</td></td>

<td></tr></td>
<td></table></td>

<td><img alt="image" src="https://lh6.googleusercontent.com/sRxCDjOAzL3r8SXmttXLNpN6Qv2eHz7ELPwsaSWGcNBfhPr-lYE0vfmMGuUSQh2O6l4vCBAPSKukS8vr5ErGy8AdZLMFNnBmBDhDUHyAXNQCa6ct9JHPukEztHaoy-SOjHR4uFRU" height=389 width=570></td>

<td>Animation Worklet - Pointer Events proof of concept</td>

<td>This sprint, Majid (majidvp@) created <a href="https://majido.github.io/houdini-samples/animation-worklet/touch-input/drag.html">a prototype demo</a> showing the power of off-thread input events combined with Animation Worklet. The ability to produce rich interactive effects which can be performance isolated from the main thread is very exciting, and we are continuing to invest in producing a spec for <a href="https://discourse.wicg.io/t/proposal-exposing-input-events-to-worker-threads">event delegation to workers/worklets</a>.</td>

<td>In other AnimationWorklet news, this sprint Jordan (jortaylo@microsoft.com) began <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1568315">landing support</a> to expose the full <a href="https://drafts.csswg.org/web-animations-1/#the-animationeffect-interface">AnimationEffect</a> interface inside the worklet. This is exciting both in terms of giving developers more power as well as welcoming a new contributor to the team!</td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | May 2019</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
