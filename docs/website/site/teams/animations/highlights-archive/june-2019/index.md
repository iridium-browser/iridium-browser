---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: june-2019
title: June 2019
---

<table>
<tr>

<td>June 2019</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh6.googleusercontent.com/m4coonQxOBK51yMEp5SzIVklqRrtqAYwfdXWZXoXSu3aes8t_mLkYnflh0fKVs4EXQPNeRLttrdBjC8R6GcDHMlc6YwMYc1ps2eKlq6H-RAMX6-rSukfKe23fQfzux059NrGHe28" height=151.34334224680958 width=593></td>

<td>Lottie Paint Worklet Renderer</td>

<td><a href="https://airbnb.design/lottie/">Lottie</a> is a popular framework from AirBnB that renders After Effects animations, allowing designers to create rich complex animations. Following up from the <a href="/teams/animations/highlights-archive/april-2019-volume-ii">previous proof of concept</a>, Rob (flackr@) <a href="https://github.com/flackr/lottie-web/tree/paint-worklet">created a proper renderer</a> preparing to send a PR to land the code upstream. The <a href="https://twitter.com/flackrw/status/1135714462546182144">response on twitter has been very positive</a> and with the demo <a href="https://flackr.github.io/lottie-web/demo/bodymovin/">publicly accessible</a> we have seen an <a href="https://chromestatus.com/metrics/feature/timeline/popularity/2385">increase in the usage of Paint Worklet</a>.</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/bQ1CSbp3EmX4fKEjkXY1dlKZ8MRvdtfBR4eoQt57owRNba-xw00y_2hGh90_o0LoSWECa5BT_ip3kJh0K988KWFfgKF1L_SzSQo3h-VutcgqTAY0sgFam3ndvD_OIsIf0oijOywi" height=121 width=198.1957328066892></td></td>

<td><td>Jump timing functions</td></td>

<td><td><a href="https://drafts.csswg.org/css-easing-1/#step-easing-functions">Jump timing functions</a> allow developers to choose the starting / ending behavior of the steps timing function. Thanks to Kevin’s (kevers@) <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1634722">hard work</a> chromium has <a href="https://groups.google.com/a/chromium.org/forum/?utm_medium=email&utm_source=footer#!msg/blink-dev/u65DesVOzmY/htUnPfFcBgAJ">shipped these functions in M77</a>!</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/BHmrokKErIb7eNeCsjGw4Ae6c-PDYX3YCjfB8ENHs70ELIQLDuL2nwSOza0XygjTYFQ5ZMKh8vmY11gBqmRZRv9FBbrh0aOmzD3dZcAhkIS9yYNZStUQle-EIFrjIKbhBM1x06eN" height=121 width=291.65571205007825></td></td>

<td><td>Scroll snap … stop!</td></td>

<td><td>A common use case for scroll snap is a paginated UI. Often, developers want users to be able to easily swipe to the next page. Thanks to Majid’s (majidvp@) efforts scroll snap stop has now <a href="https://groups.google.com/a/chromium.org/d/msg/blink-dev/bkUwigYHJDM/Bzvm8tkHAgAJ">officially shipped</a> in <a href="https://www.chromestatus.com/features/5439846480871424">M75</a> and used by <a href="https://drive.google.com/file/d/1D-xXO6wstu0HJJXvvwFjNpa2R1gzwuhG/view?usp=sharing">AirBnb</a>.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Houdini Face-to-face</td></td>

<td><td>Majid and Rob attended the <a href="https://wiki.csswg.org/planning/toronto-2019">CSS working group</a> to advance <a href="https://github.com/w3c/css-houdini-drafts/wiki/Toronto-F2F-June-2019">several Houdini spec issues</a>. Notable topics discussed include: <a href="https://github.com/w3c/css-houdini-drafts/issues/869">StyleMaps for Animation Worklet</a>, <a href="https://github.com/w3c/css-houdini-drafts/issues/877">cycle detection for Paint Worklet</a>, and <a href="https://github.com/w3c/css-houdini-drafts/issues/872">cheaply passing large data in the typed OM</a>.</td></td>

<td><td>Off-thread Paint Worklet</td></td>

<td><td>Support for running paint worklet is steadily approaching completion with Stephen (smcgruer@) adding <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1639079">asynchronous paint dispatch</a>, Xida (xidachen@) is designing <a href="https://docs.google.com/document/d/1a7gO6cBxsJhn53akuJuieUiXvB74vvEmFDyKww8NKdw/edit">animation integration</a>, and Adam (asraine@) <a href="https://crbug.com/948761">added fallback</a> for the cases we can’t support.</td></td>

<td></tr></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/r6Wkle2R-fnxzk4IwP-s_ssaMFE3-kmJP1FO20H9N2eJezr6Kc8yKo8JleHnY1czBkbVEhw-FKNAcb344bvyyntsfoiFH74OV5I37bmEM4wYQ1rBVHW6-zCHbPuz2DQFvJdaTr34" height=121 width=188.74074074074065></td></td>

<td><td>Group Effects</td></td>

<td><td>Majid and Yi (yigu@) have put together a new <a href="https://github.com/yi-gu/group_effects/blob/master/README.md">explainer</a> for Group Effect proposing changes to the existing draft design to make if more customizable and <a href="https://github.com/w3c/csswg-drafts/issues/4008">restarting</a> the effort to standardize the feature as part of WebAnimations level 2.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/cdBRl4mdiK_TkHcothUZSYhUXGbZ-NKD13Kx3P2-tcq_VjatfaOTov0Sd5AxGzPdr0ogyHw77mNKdZqfVh6_rRlVspirZmjkwB7LFuoZVJ_gXudDAgShHBRcR_QpL6aGqZLJ7ym5" height=126.00000000000023 width=202.27926371149522></td></td>

<td><td>Bugs, bugs and less bugs</td></td>

<td><td>We’ve continued our effort to stay on top of incoming animation bugs. Over the 3 week period 13 animation related bugs were closed, however 11 new ones were opened in that same time.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | June 2019</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
