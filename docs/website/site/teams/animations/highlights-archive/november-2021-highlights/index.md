---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: november-2021-highlights
title: November 2021 highlights
---

<table>
<tr>

<td>November 2021</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td colspan=2>Resolved major spec issues with user activation</td></td>

<td><td colspan=2>The HTML definition for “activation triggering events” has been incomplete since its inception, and that led to <a href="https://docs.google.com/spreadsheets/d/1DGXjhQ6D3yZXIePOMo0dsd2agz0t5W7rYH1NwJ-QGJo/edit?usp=sharing">significant inconsistencies</a> between browsers and redundancies in other specs that relied on HTML’s wording (e.g. in <a href="https://github.com/w3c/pointerlock/pull/70">PointerLock</a>).</td></td>

<td><td colspan=2>mustaq@ recently resolved the long-standing <a href="https://github.com/whatwg/html/issues/3849">HTML issue</a> by securing consensus on removing high-level events, adding missing keyboard events, and fine-tuning all low-level events.</td></td>

<td><td colspan=2><table></td></td>
<td><td colspan=2><tr></td></td>

<td><td colspan=2><td> Before</td></td></td>

<td><td colspan=2><td>After</td></td></td>

<td><td colspan=2></tr></td></td>
<td><td colspan=2><tr></td></td>

<td><td colspan=2><td><img alt="image" src="https://lh6.googleusercontent.com/9_UKpmUyXhKiMJNiMe1lmziVqYp1nMnyR3KdQI0e_d6Tj0eVKxD_fl6A8cIqVEUwADu8HatizK53sRGk865RY7YfjVtzBesDDZrhhIWlUiwHq5vDRrdFNriqz8wTV5RapstzSiXkk0rnINrw6aGXyov4KcwTOtFC7cVdCSskzy-BX6da" height=185 width=234></td></td></td>

<td><td colspan=2><td><img alt="image" src="https://lh3.googleusercontent.com/7QumKLInjQCBAW1rqLpYRWK3qmqNYlZoshE9QdoB-ngXO3npKJFmSP8VchRBcnLZURYCytTtX2EQWil3Gurp7M3TGhtJ1QdI3wZNYiZNu4E9oXlaPWEb-qaZEv9_EnSxoDUXqtL4s7_hmzGSUdAIj8Ei_hNtWYbq1PNCNvp5FA0YSU_H" height=118 width=324.82328482328484></td></td></td>

<td><td colspan=2></tr></td></td>
<td><td colspan=2></table></td></td>

<td><td colspan=2>The HTML spec is now clear and precise, and it is supported by 5 automated web-platform-tests (html/user-activation/<a href="https://github.com/web-platform-tests/wpt/tree/master/html/user-activation">activation-trigger-\*</a>). </td></td>

<td><td colspan=2>This HTML update immediately led to a <a href="https://github.com/w3c/pointerlock/pull/76">cleanup</a> in the PointerLock spec. Any other spec that had to patch over the past HTML gaps can now be fixed too.</td></td>

<td></tr></td>
<td><tr></td>

<td><td colspan=2>Scripted smooth scroll adjustments</td></td>

<td><td colspan=2>skobes@ has landed a patch that fixes scripted smooth scroll adjustments. A user smooth scroll now continues with adjustment when interrupted by an instant programmatic scroll. <a href="https://crbug.com/1264266">crbug/1264266</a></td></td>

<td><td colspan=2><table></td></td>
<td><td colspan=2><tr></td></td>

<td><td colspan=2><td>Before (Text is jumpy) </td></td></td>

<td><td colspan=2><td>After (Text scrolls smoothly)</td></td></td>

<td><td colspan=2></tr></td></td>
<td><td colspan=2><tr></td></td>

<td><td colspan=2><td><img alt="image" src="https://lh3.googleusercontent.com/142qKkDoajLbtfL7yzPbGqEZN8pAlIWWepdc8Qqf1c208pStk9I_1T45qniqs5FQOgBVhKSqxC_b9PLLW1GZIwmFijBGNIgCNc4POaH46e3wNBoeZaXu7PXnUlB1i3RtSCvxfUVxKOUhGiKk0Gb019QxHLBarfq0BQh5DtJsgaWaN9Ky" height=257 width=274></td></td></td>

<td><td colspan=2><td><img alt="image" src="https://lh6.googleusercontent.com/3fc4T-On-cu71DKzwB7nhKkEKv13d3KauaMyZ6u87W85J0vihxiYTasCBUY_tJJzyjvIjQyruC4hW4DU4phu93HEHYNFHaxgYYmebC0JDHxgnLMmlXEubR_yhLyjkyhymob2h8n6CbY-YMM4da4N7SomFBEXiCItXoceFsk30WUECQt7" height=257 width=274></td></td></td>

<td><td colspan=2></tr></td></td>
<td><td colspan=2></table></td></td>

<td></tr></td>
<td><tr></td>

<td><td colspan=2>Main thread scrolling reasons</td></td>

<td><td colspan=2><img alt="image" src="https://lh3.googleusercontent.com/eQgY9T3BlD0XhA5IFVBydHJTeyPH_KGqkUDgk0OM0wJzgwMZ1F95a5vxvqKep4XCZX3Me0Ttl1ADeo-PAvrVMfiSREfDPFtbb-PjPPSSX8fy56kPS5-LcWnbsN97Y_JwKGh27FjNuK4PnlZ1Y83myXTdkTYueB005g07du6VYfKCagVx" height=287 width=576></td></td>

<td><td colspan=2>skobes@ has landed a patch that changes reporting "main thread scrolling reasons" in a way that makes sense post <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=476553">Scroll Unification</a>. <a href="http://crbug.com/1082590">crbug/1082590</a></td></td>

<td></tr></td>
<td><tr></td>

<td><td rowspan=2>Fix Flaky animation tests</td></td>

<td><td rowspan=2>Kevers addressed a number of flaky animation tests. In one batch of tests, the underlying problem was actually an infrastructure failure since results from one test were being compared with expectations for a different test. Nonetheless, some of these non-WPT tests lacked equivalent test coverage in WPT, and the failures presented a golden opportunity to migrate them to WPT. As part of the migration effort, tests were updated to make full use of the web-animations API.</td></td>

<td><td rowspan=2><a href="https://chromium-review.googlesource.com/c/chromium/src/+/3269980">CL 1</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3270068">CL 2</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3272694">CL 3</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3273014">CL 4</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3275545">CL 5</a></td></td>

<td><td rowspan=2>Numerical precision reared its ugly head again. </td></td>

<td><td rowspan=2><img alt="image" src="https://lh5.googleusercontent.com/_IwuC55nDhhuQW4SRNqh14TCkIAWlReVyej8SdzZkjy3w4yGXSCtgnFKSv0v3KW8aBY9sEuN2mGfkCbRc2nNt_bkipKrsEahHare2VVHPr-17y1BPBmKdQjwbWfiCNEFUvJLGb0dFfqLJq-c859RNCYaJzd1zt6Hiuli604Rz0oHCb7I" height=17 width=279></td></td>

<td><td rowspan=2>Fortunately, the web-animations-1 spec is clear on the required precision for animation timing, and this was an easy fix by simply changing to the already supported assert_times_equal, which has the specced 1 microsecond of slack (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3270992">CL</a>).</td></td>

<td><td>Animation Construction</td></td>

<td><td>Kevers resolved a crash on Android that was triggered by calling an animation constructor with a missing or destroyed execution context. It should be perfectly fine to trigger an animation in such cases. We simply can’t run script (e.g. event handlers or promise resolution code). The issue was fixed by conditionally setting the execution context (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3278735">CL</a>).</td></td>

<td></tr></td>
<td><tr></td>

<td><td>ScrollTimeline</td></td>

<td><td>Kevers added support for progress based animations in our ScrollTimeline Polyfill (<a href="https://github.com/flackr/scroll-timeline/pull/37">PR</a>). Doing so required adding a polyfill for several sub-components since in the web-animations-1 spec, all timing is in ms, whereas in web-animations-2 timing may be in ms or expressed as a percentage. Polyfills to AnimationEffect and AnimationPlaybackEvent now support percentages. With the fix, the WPT tests now have a 94% pass rate!!</td></td>

<td><td>The Blink implementation of ScrollTimeline was out of sync with the spec in regard to one of the timeline property names. The deprecated property scrollSource was replaced with the updated property name, source (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3261899">CL</a>). Though largely a mechanical change, it required updating 41 WPT tests. Landed this change unblocked a third party contributor from landing the equivalent fix to the Polyfill (<a href="https://github.com/flackr/scroll-timeline/pull/35">PR</a>). Thanks Bramus for diving in and contributing.</td></td>

<td></tr></td>
<td><tr></td>

<td><td colspan=2>CSS Selector Fragments</td></td>

<td><td colspan=2>mehdika@ has landed a <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3172857">CL</a> that implements <a href="https://github.com/WICG/scroll-to-text-fragment/blob/main/EXTENSIONS.md#proposed-solution">CSS Selector fragments</a> behind this flag blink::features::kCssSelectorFragmentAnchor.</td></td>

<td><td colspan=2><a href="https://en.wikipedia.org/wiki/Cat#:~:selector(type=CssSelector,value=img%5Bsrc$=%E2%80%9Dwhiskers_cat.jpg%E2%80%9D%5D)">https://en.wikipedia.org/wiki/Cat#:~:selector(type=CssSelector,value=img\[src$=”whiskers_cat.jpg”\])</a></td></td>

<td><td colspan=2><img alt="image" src="https://lh6.googleusercontent.com/M5w0K4JhTXJPKC5sjZgDSmT-wQUEKXadYT4_pGWiTepxGhSnHsWdWFqI4r7u4m99W8pQVWucGAyccRXyS8Ul0_VgJYNdR4VMcVGiAgqEddlc2gmAjjaMcbcwkFWUjmkWXvMsTNePIrHBTUkqRWktJuMKviTmTDTOFrdhBGDCi-DBklm_" height=225 width=447></td></td>

<td><td colspan=2>At this point the element is scrolled into the middle of the viewport. In the next step the element will be highlighted as well.</td></td>

<td></tr></td>
<td></table></td>

<td>Bug Status Update</td>

<td><img alt="image" src="https://lh6.googleusercontent.com/yK_7AqZ_uk5AjVjn86eSaBZ04rGF7E6ZN50DubuNIkOatP7y0CyDmliuJeYIgaJyLMYWrInbCXOQ2SPXo-GxCsudLGxuIPil8BjmL5jASvbVTMs-77EpXUBKTG3jbVlDjSP37vI1jF6rh5dXCBhM1-MpYktBoMpxjRNB5XQy8awrInN_" height=279 width=505></td>

<td><img alt="image" src="https://lh5.googleusercontent.com/8nkguJhKYxlkdIAAZo2yRzwDbvv4t8GEn5DGLwECoSD-TXqSwfXy268QOc0Sr7Vj5rlv5bpJgDnW_VTtyPDqf1QhBjYW91TUJQRR6PR5M7urHt6mKLitYzuXX2qBkWfh4MO0fCpANCYejBMRm7zYXtYUfeChGXw7wu5glhR7p4wzBzCr" height=264 width=479></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | November 2021</td>

</tr>
</table>
