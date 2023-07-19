---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: october-2019
title: October 2019
---

<table>
<tr>

<td>Oct 2019</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Before</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/t8_ODuCgZ_gNvLhDkOp43QFiXky2FxOciZsXyORhVlUvvrv1fpoM33qc_89HOeHGuKx0-7rNd2zcHT8nM5AqUuL9bOenugdtekgjic5HeY4zcCGHWzjwkuATrUQHn83MWrvhYMr6" height=120 width=582></td></td>

<td><td>October Update</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/yfD26OsVLnxH-_ToJy499nhllsQK1mdpYjQLFSugK2i3WxdL6c4X6hzZtstBE4L4hax21uM0wULmw39zdiWRjQ3dB9WNnwjs6S5Isxz1ye4Ap56Z9ft2SV1AvAVtLai5c58bm2Nn" height=108 width=582></td></td>

<td><td>Goal</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/Vbaqe3mFSBV7_VEY0g0v0WI7Ah2xafMSGGsK3CvOr3G9vYpk500fPNpqBRLq-E2K8C1RTB2Oaf6mG-a0_gFseXL3c9U-Z5uqSPIegh4Rmj96RqKVyjABnQ9tqU1YFeO7dMyo6nzl" height=123 width=582></td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/BpYL2k0L7gpATooCPKgGPelqLnVlj6LEsY1lGXz0tgvjmN1n3zFuUTWdy-Z45B4-3UDUhoT-bF8gWysNjlSDnGkjxZXgW5s98KQFM_NDth-_lPzxCY7VVZhCG9PHtSNg6FYaD11l" height=20 width=208></td></td>

<td></tr></td>
<td></table></td>

<td> Figure. Refactoring Blink animation logic to align with specification model.</td>

<td>Web Animations Microtasking</td>

<td>Kevin (kevers@) has made lots of progress in landing a major refactor of Blink animation logic to match the web-animation processing model. Main changes include:</td>

    <td>Now using microtasks for finished promises / events.</td>

    <td>No more lazy update of variables affecting current time.</td>

    <td>Improved handling of edge cases (e.g. zero playback rate).</td>

    <td>Lots of changes in the works to align with the spec.</td>

<td>Stephen (smcgruer@) <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=788440">implemented</a> accumulate composite for <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1829355">transform</a> and <a href="https://chromium.googlesource.com/chromium/src.git/+/cc208c1adc376fa59aaa07897ed0555cebece58e">filter</a> properties bringing us one step closer to feature parity with Gecko.</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/IzZLxwrwIoPRBF9ygS4puJajKlGLO9P7BX0Y5TSvcQw-DKVye-NF3V28hLL1Fob9EBldzn3ierpk5elJYbFe5rjgmjwjBu96bvvdFmrViEpjaok-QoehyxOehllq5OIvEtHbGyOj" height=141 width=282></td></td>

<td><td>Frame Throughput Metric</td></td>

<td><td>The initial version of the <a href="https://uma.googleplex.com/p/chrome/histograms?sid=85974f925706cadddcae47f545d649f7">Frame Throughput</a> metric has landed. We have already made a few rounds of bug fixing and corrections.</td></td>

<td><td>Gene (girard@) and Xida (xidachen@) have been <a href="https://docs.google.com/spreadsheets/d/1-KJDVN60XQNrMtMRTv_9GldGMEwMkCpdlBfWyqgdtOQ/edit#gid=0">evaluating</a> the metric to guide such refinements. </td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/2xiBdb-GDi7wBgSupyAy5TbFazbC4jt9a9Sm-Uviorw2PczfEA-gdObOhsM82bt_5biaQ__py9lhwpa3yCqnegvFAkIWiv4R301k7CzcWrpJ2WLm1PvFVS1liH3ogRLIXWdYLRqp" height=140 width=291>Group Effect Polyfilled</td></td>

<td><td><a href="https://yi-gu.github.io/group_effect/">Group Effect</a> proposal has now a new functional<a href="https://yi-gu.github.io/group_effect/polyfill/"> polyfill</a> thanks to Yi (yigu@). He has been using this polyfill to experiments with richer API for stagger effects (e.g., 2d staggering and delay easing) and creating cool demos.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Scroll Timeline & Animation Worklet</td></td>

<td><td>We (Majid, Yi from Chrome with Olga, Jordan from Edge team) have completed the <a href="https://docs.google.com/document/d/1laATsw0V4bibsADvps5vV2oVEyZgjXzs_LLxo7pzn00/edit?usp=sharing">design document</a> for the Scroll Timeline. We also worked on preparing a <a href="https://docs.google.com/presentation/d/12UNGCTJybiL5gEMAGY2f-05WxXARvNz4k-RS02qgNuU/edit">presentation</a> on it for BlinkOn to share its current status.</td></td>

<td><td>Majid continued on improving Animation Worklet specifications (<a href="https://github.com/w3c/css-houdini-drafts/commit/ed84d19b90c459cf405bfb0fc98e6bbfe9ee5ea8">1</a>, <a href="https://github.com/w3c/css-houdini-drafts/commit/69d3ab9f72b76b5517227d8dd859efbea057f510">2</a>, <a href="https://github.com/w3c/css-houdini-drafts/commit/8448e9812d93c959b26f73197d91c7295f6bbad7">3</a>).</td></td>

<td><td>Investing in Code Health</td></td>

<td><td>George (gtsteel@) made progress toward removing the <a href="https://docs.google.com/document/d/1khxyBn8PIhDWZUen3GIH0sW-qTeyKB8qHO-bJZdAGOs/edit#heading=h.z1k0oapk1qmm">CSSPseudo element</a> which is also important for launching web-animations.</td></td>

<td><td>Yi (yigu@) fixed an issue so we <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1825720">no longer</a> auto-composite scroll-linked animations.</td></td>

<td><td>Majid (majidvp@) worked on <a href="https://docs.google.com/document/d/1isnrFwAqTjBYi5YnHI_GbQ33Q79Kq6nwae-9HJoBUyc/edit">figuring out</a> why scroll animations takeover logic got broken without us finding out and proposed a fix.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Snap, Harder Better Faster Stronger</td></td>

<td><td>Kaan (alsan@) has been making many improvements (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/1865048">1</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1865048">2</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1865048">3</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1902008">4</a>) to our scroll snap code resulting in fixes of long-standing bugs and enables landing additional features. He also implemented <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1845673">tracking snapped elements</a> which is the first part of snap-after-layout.</td></td>

<td><td>Majid has reached out to our contact at Safari to help improve Scroll Snap interop. He worked to improve our wpt tests: upstreaming input driven scroll snap tests to wpt (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/1893142">keyboard</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1922950">touch</a>), <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1003055">fixing</a> an issue to have 8 tests pass on Chrome.</td></td>

<td><td>OT Paint Worklets Launch-ish </td></td>

<td><td>This cycle Paint Worklet was launched briefly but had to be reverted due to <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1020238">unforeseen interactions</a> with OOP-R. In preparation for this launch Xida added necessary <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1827246">metrics</a> and improved stability (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/1838674">\[1\]</a> <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1789828">\[2\]</a>). We are now working to fix the newly discovered issue, improve test coverage, and relaunch.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | Oct 2019</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>

<table>
<tr>
</tr>
</table>
