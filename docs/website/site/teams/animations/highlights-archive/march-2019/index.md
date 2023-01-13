---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: march-2019
title: March 2019
---

<table>
<tr>

<td>MARCH 2019</td>

<td>Chrome Animations Highlights</td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/bT81a8VO0AgQk_m0F1KqBnoC5oYBcHNHpgknmtbpr3qLjsrHM3F0GE3K20VzUDlXjx47SUe7K1oV0GLC7Xcs1hEfMOc3hKuMCIbwR2XR1hrRBTMB8Q8pTK4EOp5YPIK3e0H6Qh21" height=181 width=185></td></td>

<td><td>Degenerate case in computing sign of quaternions. </td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/dvKfGyIqut3spwESpQ5gUXzdfMs8FvL5OndVdmtFSd9tynbci3E1z3xCO3G3lZA2Y2mq9Ypu1dU9SX_3yUjicYe0lGHi5iFKX8FPyVLNyVKARi0xqB0OBofd1vUlusUVi_ZuFrIF" height=181 width=185></td></td>

<td><td>Step 1: Fix quaternion calculations. Avoid degenerate edge cases.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/MYDxbzoQPU76BSHi8UQawTmlX9eDDWuEBD7kyjtvQUQv8Gy5_RCbaem7xEdbNAukqTiuyNWRvTJNG4RXp9KQgu_xphSZLuwaHU22Deo6e3T5879Rj-x9Jz8SXuCgl_DNesljbp8o" height=181 width=185></td></td>

<td><td>Step 2: Support 2D matrix decomposition.</td></td>

<td></tr></td>
<td></table></td>

<td>Improved 3D and 2D transform animations</td>

<td>We have continued to address 2D and 3D matrix decomposition issues that were causing transform animation and interop bugs. Above is one example but 16 more wpt tests are now passing in Chrome. The fact that there are two implementations (in Blink and Skia) of the same matrix operations is not helpful and we are considering options to consolidate them.</td>

<td>This work help identify inconsistencies and inaccuracies in the css-transform specification for which issues have been filed: <a href="https://github.com/w3c/csswg-drafts/issues/3709">1</a>, <a href="https://github.com/w3c/csswg-drafts/issues/3710">2</a>, <a href="https://github.com/w3c/csswg-drafts/issues/3711">3</a>, <a href="https://github.com/w3c/csswg-drafts/issues/3712">4</a>, <a href="https://github.com/w3c/csswg-drafts/issues/3713">5</a>.</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/BQeMOUyYU4rbjq7gr7IOZD-LfcTsBwYbnPWF5yfh-rAex7JNjMTeVC8hNQxrsl3Pawcx1y6TOutPkgG2jS5RRYFbI1-_8Du9YqX0uw-V3xpwcBG6sYw6D20VUCjN4POBqf2LdpV3" height=291 width=164></td></td>

<td><td>Existing implementation using scroll events.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/qP-hDF8ZwIY3DLAxCX4pXhO4Qc3OVVEHHcSnzsjJUP6LgBdCwWG1Kq6yZsNkXRulL8QkYhHoI-WQtl3aEOV4xUeapzwmxxKd7YvyB9PWmPdB2Suo91aD3H69x6X1F_pl133XX8Bm" height=286 width=160></td></td>

<td><td>New “Buttery Smooth” impl using Animation Worklet and Scroll Timeline.</td></td>

<td></tr></td>
<td></table></td>

<td>AMP Origin Trial Updates</td>

<td>AMP animation team has a near complete<a href="https://github.com/ampproject/amphtml/blob/master/extensions/amp-animation/0.1/runners/scrolltimeline-worklet-runner.js"> re-implementation</a> of their<a href="https://ampbyexample.com/visual_effects/basics_of_scrollbound_effects/"> scrollbound effect</a> system using Scroll Timeline and Animation Worklet. They have identified<a href="https://docs.google.com/document/d/1TZBb88apJ41Ibi3mued5ZB0Z9YY7h83UjAi2Q9b1fQM/edit#heading=h.eb8fxcagf7ig"> gaps</a> in the API which we collaborated on an<a href="https://docs.google.com/document/d/1XFPgy3g57njeesgJdbVSc2x8vZJQFAJBxUwuHLc3VXs/edit?usp=sharing"> action plan</a> to address (mainly by proposing changes to Scroll Timeline API). It was also decided to<a href="https://groups.google.com/a/chromium.org/d/msg/blink-dev/AZ-PYPMS7EA/kffOspnyBwAJ"> extend</a> the origin trial to allow AMP's new implementation to be gradually turned on for real users - which also helps us learn more about the performance of these features at scale.</td>

<td>Here is a<a href="https://drive.google.com/open?id=1WqNNyJC1r9yUoYTgATlRX4mYIPo8R3yU"> recording</a> of the<a href="https://amp-article.herokuapp.com/"> demo</a>. (To run locally enable experimental web platform feature flag and enable relevant AMP experiment using \`AMP.toggleExperiment('chrome-animation-worklet')\` in devtools console)</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/OyXkbEam5SYQ1dD04AR3HrX-hQ6z2fklO7BTRRPy5ZYFqVPwctNRQbXTcp4JgxA95sPL99cQUfixsSOWmr4g8mdbONwlKX2iN9oMBxAPPGD9Qlc4pkIKxXuYGhh70Pqk2vDaGWL9" height=125 width=283></td></td>

<td><td>Viz Hit-Testing Surface Layer</td></td>

<td><td>Finch trial is showing improvement over previous Draw Quad version. We are seeing 14%/8%/5% improvements in hit test requests answered in the fast path on <a href="https://uma.googleplex.com/p/chrome/variations/?sid=7bf43df040ce9f90cbea1e762c5c03d5">Windows</a>/<a href="https://uma.googleplex.com/p/chrome/variations/?sid=035b5f6d5f7ca030b7887f5b4e7f6b07">MacOS</a>/<a href="https://uma.googleplex.com/p/chrome/variations/?sid=e16a589895889a2310b4bc5f4387b43c">Android</a>, with 8% regressions on <a href="https://uma.googleplex.com/p/chrome/variations/?sid=2ffbcd8b59f4a74e05490d98f34cb579">Linux</a> (under investigation). Across all platforms, <a href="https://uma.googleplex.com/p/chrome/variations/?sid=5a3437665f748a4b4bcc679013900143">hit testing error rates</a> are lower than 0.05%.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/PdcNk4yzu3sVovMt1mQcuDwfoinb7N2EwvpdAgdC16SVQbeJOY-9snyuxO48NMCN2K0QrWH3S7uw7VJG-dWxEMJO_7b6Qpl6rrULnaV-garO2ER_s6st0NgQp01PECa9Yc3-HViY" height=123 width=283></td></td>

<td><td>Compositor Touch Action</td></td>

<td><td>Finch trial is showing promising <a href="https://uma.googleplex.com/p/chrome/variations?sid=4d04b0905ac90bafd4b63b39754ae20e">early results</a>. ScrollBegin latency improves 5% @50pct and 13% @99pct. ScrollUpdate latency improves 1% @50pct and 3% @99pct.</td></td>

<td></tr></td>
<td></table></td>

<td>Off-Main Paint Worklet</td>

<td>We made good progress by completing implementation of multiple sub-components: <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1426015">rasterization</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1479658">raster-caching logic</a> and <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1497032">lookup</a>. Work is in progress for <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1495445">dispatch logic</a>, and a new approach to share data between blink and cc for paint info (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/1497217">1</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1504229">2</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1506653">3</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1506818">4</a>). </td>

<td>Animation Worklet</td>

<td>Animation Worklet is now fully <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=791280">asynchronous</a> in CC with proper scheduling in place. With <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=915352">help</a> from Microsoft engineers our tests have been upstreamed to <a href="https://wpt.fyi/results/animation-worklet?label=master&product=chrome%5Bexperimental%5D&product=edge&product=firefox%5Bexperimental%5D&product=safari%5Bexperimental%5D&aligned&q=animation-worklet">web-platform-tests</a> and most critical flakiness issues are resolved. Specification has been updated (<a href="https://github.com/w3c/css-houdini-drafts/commit/3c1cd1b3babc49a92d7a5079f9de77df67d06775#diff-6547a8c38ca0bdc51fd63edeaa4b66b3">1</a>, <a href="https://github.com/w3c/css-houdini-drafts/commit/93ab72f8abee1ac008eb4b5292265633387381fc#diff-6547a8c38ca0bdc51fd63edeaa4b66b3">2</a>, <a href="https://github.com/w3c/css-houdini-drafts/commit/cf1a69a4c9dd165f53df7ba7d68c329287ab2659#diff-6547a8c38ca0bdc51fd63edeaa4b66b3">3</a>) and <a href="https://github.com/w3ctag/design-reviews/issues/349">TAG review</a> requested. Aiming to ship without Scroll Timeline in M75.</td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Updates | March 2019</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
