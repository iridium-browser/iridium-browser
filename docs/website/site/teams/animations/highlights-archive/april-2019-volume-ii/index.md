---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: april-2019-volume-ii
title: April 2019, Volume II
---

<table>
<tr>

<td>April 2019, Volume II</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh3.googleusercontent.com/ijzv1FoeaoQqmakoVD-22LcixO6d-o_mKKFTGwYa3JKIFBecPJtDZjHLnSTEBFBCVdcQxHg2PUAToBolahoOUz0-DmLVOi1rZjT_mksxuZA20sPrkULJVO7H3SeYSyRxA_4uIt2x" height=367 width=596></td>

<td>One DIV, "7 lines" of script. Done! (Animation from LottieFiles: <a href="https://lottiefiles.com/433-checked-done">https://lottiefiles.com/433-checked-done</a>)</td>

<td>Lottie in a PaintWorklet</td>

<td><a href="https://airbnb.design/lottie/">Lottie</a> is a popular framework from AirBnB that renders After Effects animations, allowing designers to create rich complex animations. Its <a href="https://github.com/airbnb/lottie-web">web implementation</a>, however, is not very performant. This sprint, Rob Flack (flackr@) prototyped a port of the Lottie renderer to a PaintWorklet - driven by a standard Web Animation for input progress. Not only does this bring richer devtools integration, but with the ongoing <a href="https://docs.google.com/document/d/1USTH2Vd4D2tALsvZvy4B2aWotKWjkCYP5m0g7b90RAU/edit?ts=5bb772e1#heading=h.2zu1g67jbavu">Off-Thread PaintWorklet</a> effort we will soon be able to render Lottie animations performance-isolated from the main thread!</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/U9zrtaB0y4-AB2jh3ZIpZBM4h6Syh5L4ZqasUgSLmdez7iBL0r4TkUvBJczJGh7usdsZaoxdrPtLE2jMuYDa-c103TacHIZ2ghex6xyAaF9dhkTCX8dIsjTQu2iYdtcc1wD6Uivp" height=128 width=283></td></td>

<td><td>Viz Hit-Testing - Linux Performance</td></td>

<td><td>Xianda (sunxd@) sadly left the team this sprint, but not before he followed through on</td></td>
<td><td>his promise to bring the fast-path rate on Linux up to the same standard as other platforms.</td></td>
<td><td>The goal was to have 80% of hittests use the</td></td>
<td><td>fast path - Xianda overdelivered as usual with a fantastic 93% fast-path rate!</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/DYE-WxgyKi-r-aZ-7PNd8z9cWxufESkHmzVcqugM-wHBKU5pUjXqz2aT4v7XHQaDWXiRa6ZK0eqJKsQXkhskzkksQGQZ-yR_mmVnVFHU2IjEGiAqt-N9TrVpcL-8sZWFtxjmneD6" height=128 width=283></td></td>

<td><td>Stateful Animation Worklet</td></td>

<td><td><a href="https://drafts.css-houdini.org/css-animationworklet-1/#stateful-animator-desc">Stateful Animators</a> enable developers to keep local state in their Animation Worklet to perform richer effects (e.g. velocity based animations). Yi (yigu@) has lead the spec work on these, and this sprint he landed the corresponding code.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>BlinkOn has come - and gone!</td></td>

<td><td>Two thirds of the Animations team decamped to Toronto for BlinkOn, spending two days listening to (and giving) talks, syncing with colleagues from around the world, and getting to know some of our new collaborators from Microsoft. Both Stephen's <a href="https://docs.google.com/presentation/u/1/d/1WrzDN_PiKBfOVUrrbOV8gHQO-ODN0gdCpWE4xMYv29c/edit?usp=drive_web&ouid=105513761242358226829">Off Main Thread CSS Paint</a> and Majid's <a href="https://docs.google.com/presentation/d/1BCEbLCg-o_Ko65byel5QGnO7Cwf5aPZPjqnnMNbbA5E/edit">Event Delegation to Worker and Worklets</a> talks were well attended and well received.</td></td>

<td><td>Web Animations - Moar Interop</td></td>

<td><td>Kevin (kevers@) has been hard at work fixing bugs in our Web Animations implementation - over 50% of the <a href="http://crbug.com/772407">known WPT failures</a> are now fixed! This sprint has seen a focus on timing issues, with plenty of nasty floating-point boundary case bugs to squish!</td></td>

<td></tr></td>
<td></table></td>

<td><img alt="image" src="https://lh3.googleusercontent.com/5vUZFWkmaAi__ErtI9EwMBbnBx7aRTDa74k33Ya0BQ6hLcTQU5TGX4nB6OzlJWXcdY17QGEiZi8ADo5xXF5o0EtnCxx_Naw9Fj0dGMIT8GUvCwoM3G7DLMpvzOA_7XuBnYX_NgJz" height=286 width=353></td>

<td>Scroll Snap - supporting AMP</td>

<td>AMP team are excited to use scroll snap for their image carousel, but need paginated behavior. We previously implemented the <a href="https://developer.mozilla.org/en-US/docs/Web/CSS/scroll-snap-stop">scroll-snap-stop</a> feature to enable this. This sprint they <a href="http://crbug.com/823998#c15">reported a bug</a> where they were able to 'break' the snapping and cause their content to go flying! Majid leapt into action: <a href="https://drive.google.com/file/d/1Jlb1IlQ66-JbCi1lBn-L0zhDBoiZF_Qb/view">reproducing</a>, diagnosing, and <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1572286">fixing</a> the bug. This should clear the way for AMP to roll out scroll snap, and we are excitedly <a href="https://www.chromestatus.com/metrics/css/timeline/popularity/499">watching our metrics</a>.</td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | April 2019, Volume II</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
