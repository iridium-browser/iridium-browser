---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: january-2020---off-thread-paintworklet-web-animation-scrolltimeline-virtual-scroller-scrollsnap-and-more
title: January 2020
---

<table>
<tr>

<td>January 2020</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh3.googleusercontent.com/YUOJZ1fWnO_xlHUDQwtYmpu-WxSH85_WGBxALF-sBbEKcuUmU9kYfM4n0HgfL9dLAQh9rm8CniR3f8MN1zSSq26Q-PQ1J2bur6eQSOWYbGIMri6MpQzwUutY8bN9tvF-fmYqG_fm" height=254 width=579></td>

<td>Off-thread PaintWorklet shipped!</td>

<td><a href="https://developers.google.com/web/updates/2018/01/paintapi">PaintWorklet</a> is a great example of Houdini's value offering, allowing developers to build complex yet compartmentalized controls like <a href="https://googlechromelabs.github.io/houdini-samples/paint-worklet/qr-code/">QR code generators</a>, <a href="https://googlechromelabs.github.io/houdini-samples/paint-worklet/ripple/">Ripple effects</a>, <a href="https://googlechromelabs.github.io/houdini-samples/paint-worklet/circle/">Custom</a> <a href="https://googlechromelabs.github.io/houdini-samples/paint-worklet/parameter-checkerboard/">background</a>, <a href="https://googlechromelabs.github.io/houdini-samples/paint-worklet/diamond-shape/">custom shape elements</a>, <a href="https://googlechromelabs.github.io/houdini-samples/paint-worklet/border-radius-reverse/">custom border effects</a>, etc. Off-thread PaintWorklet moves these effects off of the main thread, ensuring jank-free performance even under load. After a year of effort, Xida (xidachen@) proudly drove the feature to completion and turned it on by default in M81 on behalf of Rob (flackr@), Stephen (smcgruer@), Ian (ikilpatrick@) and the Animations team. The <a href="https://twitter.com/slightlylate/status/1225102256053182464">gif above</a> shows how the popular Lottie animation library is adapted to use PaintWorklet. Do pay extra attention to what happens when we inject artificial jank! There are a few caveats which will be addressed soon: it currently <a href="http://crbug.com/1046039">requires</a> will-change: transform etc to force a compositing layer and the animation <a href="http://crbug.com/1049143">must</a> start after the PaintWorklet is registered.</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/NvUQu5IYtvdtY7R45QmkSgu3nQeN-09U0OevkZGYHxYzevW-kSTXG9OWwvm5l_F6YIHpmeEvfRrhjjlyUSObcRycnh6E8ujOPZxY6aSXV65PUj8PkIqaz_0Q1HkEmlK-O2Cj-WP0" height=166 width=292></td></td>

<td><td>new animation created every mousemove</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/GvvT0tEqot8b8GNtWYK1gT4WAMuj2CInVNIoIKDYPYj0Y6W8Vrlj77L6vDKJ2As-oYs0NzTKc5BIAjU7bE3qR2zf9DdqoErxUPJvT3a0jnNyrOZM9cgNXqkR1ZIZpDE7CP3FySRb" height=166 width=265></td></td>

<td><td>finished animations automatically replace older ones</td></td>

<td></tr></td>
<td></table></td>

<td>Support replaceable animations</td>

<td>When a fill forward animation finishes, it remains in effect. If enough of these animations build up, they can negatively impact performance and leak memory. e.g. with the following code snippet we would create one animation per mouse move over and over until the memory runs out (left gif).</td>

<td>document.body.addEventListener('mousemove', evt =&gt; {</td>

<td> const animation = circle.animate(</td>

<td> \[ { transform: \`translate(${evt.clientX}px, ${evt.clientY}px)\` } \],</td>

<td> { duration: 500, fill: 'forwards' }</td>

<td> );</td>

<td>});</td>

<td>Our team has worked closely to introduces a <a href="https://drafts.csswg.org/web-animations/#replacing-animations">solution</a> for this in the specification in the form of replaceable animations. This sprint Kevin (kevers@) implemented replaceable animations to tackle the issue. With this effort, animations that no longer contribute to the effect stack will be removed once they are finished (right gif). No more memory leak and performance degradation. YaY! See the <a href="https://www.chromestatus.com/feature/5127767286874112">I2P</a> for more details.</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/4qcWWtrg9phgX7dhxKdmlxP3U954Xmql0JSasLQXOyQIMhPCu7S-JFaJoJNsJJucARKfOZxd6wXs45LxzPdQw70EiT_Om0Y6MXhMeBwjjr8xPVCppUvkpgteZUsB02lq2M4fgVNy" height=157 width=290></td></td>

<td><td>--wpt_failure</td></td>

<td><td>Kevin, Hao (haozhes@) and George (gtsteel@) made awesome progress towards shipping Web Animation this sprint. 78 previously failed tests now pass.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/Uyg0fdPdGq6ODnXLgjZGnmNCFS5pO9P1ddwD01GGPmarYHpy_e84ZPhlbtVRtWC7LpelF30LzhyQU_-kTye4-vaiAAQ-yGrYnxBV_5GLRKWC7y4GABOXm9HvhTndtw1eM-GqawOt" height=163 width=292></td></td>

<td><td>Throughput metrics</td></td>

<td><td>Frame throughput is designed to measure the smoothness of Chrome renderer, which reflects the performance. In the past weeks, Xida, Rob and Sadrul fixed a bug where a large number (~20%) of Canary users reported 0% throughput. The fix was landed right before M82, and now the number has dropped to ~1.6%. As a result, the 20% users with the worst throughput now report ~23% in M82 (green curve) compared to ~0.95% in M81 (red curve).</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/9PzpLhk5NoA-WWwcSA57_H8WhpxEcniJ2gBW2tScCrN5W3wNJDcuRMkRBANs4zxTCEd7HNYlfjlBsCF8Np7lWldJzxPus9LxZRbbYSyuqKN7QjpzFDirgGFZ9f_SzlydnxHhmolG" height=144 width=137><img alt="image" src="https://lh4.googleusercontent.com/Lrl4uTP7YZpFBi4-ni3PQBrE59h_4a3d05jelqPXpmLBPLg2CCvAwvXYGb9r4azaCvrRU9SgCR1gaQrj8sQxyZcoJo32WfYdQkj45nnSA58IjkmvoLwvrCfebksvFAuYmb1ykRAb" height=141 width=135></td></td>

<td><td>++scroll_snap_after_layout_robustness</td></td>

<td><td><a href="https://docs.google.com/presentation/d/1WUa6nFfkzXm2O1V70hr49vPFP4TOAbUh2q8fppkybJs/edit#slide=id.g6c0755777a_4_441">Scroll snap after layout</a> frees developers from forcing a re-snap after layout changes with JavaScript. Yi (yigu@) fixed a ship-blocker this sprint, i.e. transform inducing resnap, and turned the feature on by default in M81!</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/nap0TfLZVJKZObpmhgBK24I9qfmyZz9SI4qKnGtFidKd0xQqOs025U1QPHJNazzGMWNnb-eK0iMFkAx82mFuETIEj7IQISFkoPoPCcO-DydKrwAXVQY_DGhB2PgpEGUp1t6yOiAr" height=135 width=124><img alt="image" src="https://lh3.googleusercontent.com/8I7tcRP5Nc4yU9FX1zwGwzjmTN67a7oCVh9VZkzT6sMUTtsaAc3fDlj5wuBFntOnn6X6b84H9XZqfmH47bQDtML1njBCKZkpCCTUeSARKAgTF2yRj44VSaCOScdFADHqfxDn1p0q" height=161 width=145></td></td>

<td><td>Free animations from pending state</td></td>

<td><td>When there are composited animations in the process of being started, we used to defer the start of main thread animations in order to synchronize the start times. This process could lead to main thread animations getting stranded in the pending state if composited animations are being continuously generated (left “gif”). With Kevin’s excellent work, main thread animations queued up in a previous frame no longer get blocked waiting to synchronize with fresh composited animations (right gif).</td></td>

<td></tr></td>
<td><tr></td>
<td></tr></td>
<td><tr></td>

<td><td colspan=2>Virtual Scroller</td></td>

<td><td colspan=2><img alt="image" src="https://lh6.googleusercontent.com/4V1EVsDDGrmPaAcKIvuQU3aCOU-3uycfFi91_Zso6_sMqJ5tfnoeoiCpXaXAAe9kuHa-xITdMgGR9Q9VjVIIlP-OC35-nGF1itJPN4bE05vlDpLVMMDSaC9rlcRTJSn846di0PI9" height=216 width=276><img alt="image" src="https://lh3.googleusercontent.com/rP5n2ImsLCT49zKMIU8Z_oqbyj9QbHqn3roRV94A0Kgi1_3y3V3PhOlwKugvbpfASuL8RGt-yktBhma9YVl1sxJBCTkRz7Mq6gMuBdvIo7nYD1aO3rGFaB_4oH28MYDQU9hqpMVX" height=216 width=280></td></td>

<td><td colspan=2>Rob worked with Vlad’s (vmpstr@) viewport activation, created a simple version of virtual scroller which exposed a <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1043674">scrollbar drag instability bug</a> (left). This was initially an open design problem - changed content sizes changes the scroll bar position and track length. While scroll anchoring keeps the current content visible the next drag tries to go back to the absolute position on the scrollbar. Rob applied a simple solution (right) - consistent with scrollers which load more content - treating scrollbar dragging as a delta from current position. To find a general solution, he started a <a href="https://github.com/WICG/display-locking/issues/109">discussion</a> and came to a tentative idea of locking the scrollbar area when drag starts.</td></td>

<td><td colspan=2>Scroll-linked animations</td></td>

<td><td colspan=2>The Animations team has been collaborating with Microsoft engineers towards shipping scroll-linked animations . This sprint we made solid progress on both standardization and implement work. Olga (<a href="mailto:gerchiko@microsoft.com">gerchiko@microsoft.com</a>) drove a <a href="https://github.com/w3c/csswg-drafts/issues/2066#issuecomment-568565738">discussion</a> with spec owners to settle some key concepts w.r.t. inactive timeline. She also made ScrollTimeline a first class citizen in AnimationTimeline. i.e. we now schedule frames for scroll linked animations only when scrolling changes instead of ticking them on every frame regardless. Yi did some fundamental refactor work on cc side and prepared to integrate ScrollTimeline with cc::Animation.</td></td>

<td><td colspan=2><img alt="image" src="https://lh3.googleusercontent.com/wjWoc8_v9g3xloeJG_hM20QMJTS7O0Ge5wwCu8z1hsbHQJPRs-aHcRzNuLIsm6aXO66S9BReqtvmsrc2O9xauzcPQ0ThsstMKabq-GS3Eq99cNQswP-JwW_n_UOwO_UogX1m9Ml7" height=389 width=580></td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | January 2020</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
