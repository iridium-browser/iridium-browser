---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: nov-dec-2019---blinkon-scrollsnap-lottie-hittesting-virtualscroller-websharedlibrary-wpt-and-more
title: Nov & Dec 2019
---

<table>
<tr>

<td>Nov & Dec 2019</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh4.googleusercontent.com/DWJAD7EpLKH2Ac19IePi2JmkPHgV1fsYia0LGCZ3mxpEGU6fJCY2k-1XInFTZj0Whh032Fdw-dIoCDEV0EDs45kwIWlsc2jsfZwt8LXPrRfFMfQZMs_oN6pw5TmeAfou7y53qGgS" height=253 width=270><img alt="image" src="https://lh6.googleusercontent.com/vK6GwgfvzXmjGedJa8KQ6pEW_UmVi7hYNjsk5h1ndge7kIp_oUwZhktXsLxjAtfGL2bpSWdhzSvQBeOgc6vs7Z92V0GqpEdF8zsaSZzVWaY8mumxS9Ldh48MgP_iRhMnHkCiBVAv" height=255 width=312></td>

<td>Left: the snapped element (white cat) is not preserved after layout changes.</td>

<td>Right: the resizing triggers re-snap to preserve the snapped element.</td>

<td>The Pursuit of Snappyness</td>

<td>Kaan(alsan@), our fantastic intern, made the web more appealing with the “snap after relayout” <a href="https://docs.google.com/presentation/d/1WUa6nFfkzXm2O1V70hr49vPFP4TOAbUh2q8fppkybJs/edit?usp=sharing">project</a>. Previously content or layout changes (e.g. adding, moving, deleting, resizing) did not pay attention to the content’s snap setting. With Kaan’s <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=866127">changes</a> landing in Chrome 81, scroll positions automatically update to keep snapped objects in the correct position. Galleries of kittens never looked so good!</td>

<td>Another cool use case unlocked by this work is the “Scroll By Clicking” effect shown below:</td>

<td><img alt="image" src="https://lh6.googleusercontent.com/fRyKl_faF-Bgdceo3OYErRenGuZzKBBs2n2xMKss4-7vRnDP5wvm7Gae0DmktSqdbx5fGwUT_j-ltLPIBeFtDu2NqsN4qr4Ns8p6QI0bhzHND0tFkR7n91LC-Al27tl4J-W96en7" height=233 width=297><img alt="image" src="https://lh3.googleusercontent.com/1nPkrBOk1Dm9i5mkPLt_jEk6uTF7Jhuo_7qKj1cRWZe8xpc9IPTmN8bXuzhSzGUubGyV3QhHTM-UY6-71fRyZnc_MY63zZjkwEdmhX72tBlB1eP6B1rC168XLrdwDA0cuhOxdMm8" height=237 width=280></td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/PZ4-_ZrbNzstAkLHnjWZPkwXGl4b0FZthhuQ8dgR0DVzOdfmXxez2SF3m4GLNN1S_Q2Latry8UZouikJFv3ZW9PUbJQXWjdK0TlbWyNJ3bfC3RvnocOZx3GffHgz44fjh43Esxyj" height=72 width=280></td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/S-nAodiRsv_xyxIngFbN86HJR-7JaCZdX7wzG1olre_TUBLKjKk5np7aJYfrwIltVXB7H8LcLVw1Z7rWEwuzSpKNeHX7C1yHoUqIXNe0MoUVnxmCllgfVHLiAnASux1dNQx89iRp" height=83 width=280></td></td>

<td><td>Lottie Demo - now more robust</td></td>

<td><td>Rob’s lottie demo exposed a number of PaintWorklet bugs, such as flashing at the end of the animation, incorrect background color, and flaws in high-dpi rendering (top gif). This month Rob (flackr@) and Xida(xidachen@) fixed some issues (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/1841530">high-dpi</a>, <a href="https://github.com/flackr/lottie-web/commit/8cea6ff27b03e6125c00e85d6ff15abff15c6e75">clipping</a>, <a href="https://github.com/flackr/lottie-web/commit/2aee64a69fb74c3c26f7435db9e0a804608e77ae">flashing</a>) in both the demo and the implementation to make it more appealing (bottom gif)!</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/SLQW5VgfbkGLfzMohnaHu0O_51WGPoMDlpIoX-Kzlw0e3kxairHQgkfLkwVtk0w7j6KW9ifGGc3WptW8b328XDsFwou9bWwOKMzbjbetT3OR5ofIG7MGjiQUunL2xPJmuHEQ0ZPD" height=176 width=278></td></td>

<td><td>Multi-Process Hit Testing Complete</td></td>

<td><td>Yi (yigu@) drove the viz hit testing v2 project to completion this month. It is now fully launched on all platforms! The new event targeting correctly targets events up to 6 times faster for site isolated configurations on <a href="https://docs.google.com/document/d/1AE02TKBg7NG63SiUbU-EiqlHgqsZY1tb3-Z4L0AZCSk/edit">Windows</a> thanks to the 32% more synchronous event targeting. </td></td>

<td></tr></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/NngtlrupseOCRsIOOr-Nn-UZbMX-MIdSiTk_Agpt7cFv-Qd5ieUwlIJXuZ482e_SOeWPip3uFwx6J-WIJ0XStDp4IQXxUhLZ9O6D59k2Q0oFphXTkbGV8AH5-TbAw0Xy2IHnV6jN" height=281 width=280></td></td>

<td><td>Virtual Scroller</td></td>

<td><td>Rob made solid progress on the exploration for the new web platform feature - virtual scroller control. He moved chromium virtual-scroller to github <a href="https://github.com/flackr/virtual-scroller">library</a>, added display: none fallback when rendersubtree is not supported and created a nice <a href="https://flackr.github.io/virtual-scroller/demo/">demo</a> with synthetic content and <a href="https://flackr.github.io/virtual-scroller/demo/?test=true">test</a> use cases.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/etS54zUr6juIFudSJaZ_4J0Tlp6Y9qxps7IENSmYxBBNx-5vCFMh9UugkPd9OZFpbvVMNM3seoFxoXe-JUoXQDpO_xfKT-Mu9Cw6FbNgCcBeFm9cpS4w2wqMme6pvg89s92sxvrJ" height=299 width=278></td></td>

<td><td>Port interpolation test to WPT</td></td>

<td><td>Our test suite for interpolated values does not work in other browsers. Since we deeply care about interop issues, we have ported as many tests as possible (<a href="https://docs.google.com/spreadsheets/d/1EtUV2IycJTgQSrtxfH0uQ8aUUbhz41-zhYj0tdCrT6A/edit#gid=0">121</a> to be exact!) to wpt. This work helped us to identify 17 new interop bugs in Chrome and Firefox. Thanks to Stephen (smcgruer@), Xida and Hao (haozhes@) for their effort to make this happen! </td></td>

<td></tr></td>
<td><tr></td>

<td><td colspan=2>Animations team at BlinkOn 11</td></td>

<td><td colspan=2>The animations team presented 3 breakout sessions and 4 lighting talks at BlinkOn 11. See below for details.</td></td>

<td></tr></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/mBS6Z_-mXCbDu_mhXaB-ZVvoi3yODetHPxtuNJmgXZrhohvNHMXdqK7rqAUNk9k3cUH0j7dQ7cW_IBrcwhE-A--NIIExJHzFEs0dlqrbCyW3OzUAu_Wk4o0tjb6_qKWURl8kQOvj" height=145 width=280></td></td>

<td><td>Web Shared Library</td></td>

<td><td>Web Shared Libraries attempts to improve website loading by sharing cache across sites. Rob’s presentation (<a href="https://docs.google.com/presentation/d/1CTC_BLpBqLf7B82b1ytIm0l84SjK45bk7UaCyRpyu5A/edit#slide=id.p">slides</a>, <a href="https://www.youtube.com/watch?v=cBY3ZcHifXw&list=PL9ioqAuyl6UI6MmaMnRWHl2jHzflPcmA6&index=23&t=0s">video</a>) at BlinkOn, kicked off a lot of constructive feedback. Gene (girard@) and Rob wrote up a <a href="https://docs.google.com/document/d/1lQykm9HgzkPlaKXwpQ9vNc3m2Eq2hF4TY-Vup5wg4qg/edit#heading=h.83t0eq3angjs">discussion pape</a>r capturing the opportunities and challenges involved, along with techniques to address the various concerns and metrics to evaluate impact.</td></td>

<td><td> <img alt="image" src="https://lh6.googleusercontent.com/1CFPALcatns9GWaoO89Tu2dvl1FraGY1RckXruqf3CaHW8AdyQUqsl47f7I3qWY3h2skZLRZxP2-HE0fiUUrlAgxt34acepBhzpW_yHvstWrlWR1QVChkfp5HuXUUr3PrmAKeKcU" height=200 width=262></td></td>

<td><td>Scroll-linked Animation</td></td>

<td><td>Yi and Majid (majidvp@) presented (<a href="https://docs.google.com/presentation/d/12UNGCTJybiL5gEMAGY2f-05WxXARvNz4k-RS02qgNuU/edit?usp=sharing">slides</a>, <a href="https://www.youtube.com/watch?v=nolDnHb_Sjo&list=PL9ioqAuyl6UI6MmaMnRWHl2jHzflPcmA6&index=4">video</a>) the current design for Scroll-linked Animation at BlinkOn. The updated design incorporates feedback on our origin trial from the AMP team. There was a F2F sync between Microsoft and Google to <a href="https://docs.google.com/spreadsheets/d/1tGJeicFqgRXK8MMIqscdwEiIPQN--1aQJ5EwQZ2nXzE/edit?usp=sharing">coordinate</a> future work and find solutions for outstanding spec <a href="https://github.com/w3c/csswg-drafts/issues/2066#issuecomment-557630869">issues</a>.</td></td>

<td><td>Rob also attached Scroll Timeline into the Paint Worklet Lottie demo for a cool lighting talk (<a href="https://docs.google.com/presentation/d/124zcoxveCQyXVB4-HWCXhQptrnaHDE9JIXyQ8B1N4ig/edit#slide=id.g742aacb70c_9_789">slides</a>, <a href="https://youtu.be/dJmWLQn9g2A?list=PL9ioqAuyl6UI6MmaMnRWHl2jHzflPcmA6&t=423">video</a>)!</td></td>

<td></tr></td>
<td></table></td>

<td>Last but not least</td>

<td>There were several other topics that the animations team presented at BlinkOn. Rob drove a discussion about measuring real world throughput and latency. The team gave several project updates during the Lighting Talks session including Off-thread PaintWorklet (<a href="https://docs.google.com/presentation/d/124zcoxveCQyXVB4-HWCXhQptrnaHDE9JIXyQ8B1N4ig/edit#slide=id.g70a5f3c77e_102_92">slides</a>, <a href="https://youtu.be/FOCHCuGA_MA?list=PL9ioqAuyl6UI6MmaMnRWHl2jHzflPcmA6&t=886">video</a>), Contribute to WPT (<a href="https://docs.google.com/presentation/d/124zcoxveCQyXVB4-HWCXhQptrnaHDE9JIXyQ8B1N4ig/edit#slide=id.g70a5f3c77e_102_0">slides</a>, <a href="https://youtu.be/FOCHCuGA_MA?list=PL9ioqAuyl6UI6MmaMnRWHl2jHzflPcmA6&t=1021">video</a>), Throughput Metrics (<a href="https://docs.google.com/presentation/d/124zcoxveCQyXVB4-HWCXhQptrnaHDE9JIXyQ8B1N4ig/edit#slide=id.g742aacb70c_289_204">slides</a>, <a href="https://youtu.be/FOCHCuGA_MA?list=PL9ioqAuyl6UI6MmaMnRWHl2jHzflPcmA6&t=1442">video</a>) and GroupEffect (<a href="https://docs.google.com/presentation/d/124zcoxveCQyXVB4-HWCXhQptrnaHDE9JIXyQ8B1N4ig/edit#slide=id.g742aacb70c_9_417">slides</a>, <a href="https://www.youtube.com/watch?v=z3EjkpvEWC4&feature=youtu.be&list=PL9ioqAuyl6UI6MmaMnRWHl2jHzflPcmA6&t=1455">video</a>).</td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | Nov & Dec 2019</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
