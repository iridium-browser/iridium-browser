---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: july-2019
title: July 2019
---

<table>
<tr>

<td>July 2019</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh3.googleusercontent.com/w9kTMoKBL0rbJTFvKmkNWwY3xG4Z3UqhW7iLHeK9aV4YsUitV1OdoR1WEsmHnz4jiwphbWtGyO31tp_j6lxpnlmRYvfAjdVTB5vJA_vDgjqAagHNh7Zb7kATsCecD8lVfYMmiqlj" height=283 width=565></td>

<td>One of three example effects that yigu@ created to show the potential of Group Effect. I'm staggered, really.</td>

<td>GroupEffect - Showing a need</td>

<td>When exploring new potential features, it is important to understand what usecases they enable and how ergonomic they would be for web developers to use. This sprint, Yi (yigu@) spent some time exploring the <a href="https://github.com/yi-gu/group_effect">proposed GroupEffect</a> concept. After <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1710156">building a prototype</a>, Yi created demos for three types of linked effects, such as the 'stagger' effect above. This work helps us understand the potential impact of GroupEffect, and also see how easy the proposed APIs would be to use.</td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/TweoRh0njZWyEESeBZAjj8CRazxBzSQYhiB3k72KH0tPLQRJMRdXx56EpBceCuuLHgExjuK_Vp7BrtfL6j2fH1Jg8eGNVcIsmnFYM7gIBtv4pckxtNZKzvFz8GJBxqqBKlLPakrP" height=184 width=271></td></td>

<td><td>Web Animations - towards an I2S</td></td>

<td><td><a href="https://drafts.csswg.org/web-animations-1">Web Animations</a> is a big focus for the team currently, and we continue to move towards our goal of being able to ship the full API.</td></td>
<td><td>This sprint included fixes for getAnimations (majidvp@, smcgruer@), AnimationClock (flackr@), animation events (kevers@), and CSS property serialization (gtsteel@) - resulting in 50 newly passing WPT tests!</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/4zapRR1JUKCEExT9Dy78adDfv0BZVOEics7z7LoQfb2gCZ6JRwukYMkSyCibRxMB8fjEfSw20riiW9mJkbKHPqa_B16Uso8QdhVJnNP29M3f9c5P3i812LKoxlqyCNaAPZFSs_Au" height=208 width=253></td></td>

<td><td>Viz hit testing V2 - heading to stable</td></td>

<td><td>Yi (yigu@) set his sights on Viz hit testing this sprint, with sensational results. He was able to close 12 hit-testing related bugs, including all known bugs blocking the 'V2' launch. As a result this new, faster hit testing model is now re-enabled for the M77 release and should be rolling out to stable sometime later this quarter.</td></td>

<td></tr></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/k4oS7E04Mz46HL5n13FzLh2ev0LCQNuNvg2VYkrzLEC807Bxurf32asE05BcHMaca1lPqr9zgIYEHWWKm6V_PT8c0DRUqhzeHBVqgGaP3nmsZNq1-Lo4mFsLOHxBbGj0HiHLnA4R" height=113 width=281></td></td>

<td><td>Complex Crash Bugs</td></td>

<td><td>Chromium is a big codebase, and sometimes simple looking crash bugs can turn out to have complex causes. Thankfully these bugs are no match for Rob (flackr@) and guest star Dana (danakj@) who tackled <a href="http://crbug.com/974218">such a bug</a> this sprint.</td></td>
<td><td>After multiple weeks of effort they tracked the crash to an old scheduler experiment which was unexpectedly re-ordering IPC calls! <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1692577">Disable the experiment</a>, and the crash was fixed. Magic, or perhaps just <a href="https://en.wikipedia.org/wiki/Clarke%27s_three_laws">sufficiently advanced debugging</a>.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/_d5BW7phRaKRlndSuxtEz8gidiwiNmYZ8NzvSpX5Iv9FplDwBinrCUcp1SgTKN1oazrAqRZYAvm3E68YfGx08KfLUEATZgD4wvIyuRd2LTGxTvOc21iS9aQIxwW-YQwC3oZmao53" height=121 width=286></td></td>

<td><td>Scroll Snap Interop</td></td>

<td><td>Blink's viewport style propagation for scroll snap was incorrect based on the latest <a href="https://github.com/w3c/csswg-drafts/issues/3740">spec changes</a>. Majid (majidvp@) dived in to fix this issue before Firefox's implementation ships, to ensure that the interop issue is addressed before usage grows and makes it more difficult to backtrack. </td></td>

<td><td>Since this is a breaking change, we needed to understand its impact on the web. To do so Majid performed an <a href="https://docs.google.com/document/d/1DxVjr3m02cPE81UrDMT_SN36bFDg5f4K3wESSwMOqck/edit#heading=h.a1k7isoapl5s">analysis</a> using <a href="https://docs.google.com/document/d/1FSzJm2L2ow6pZTM_CuyHNJecXuX7Mx3XmBzL4SFHyLA/edit">cluster telemetry</a> in conjunction with <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1715906">use counters</a> to confirm his findings - all clear, thankfully!</td></td>

<td></tr></td>
<td></table></td>

<td><img alt="image" src="https://lh3.googleusercontent.com/K6hFM4soJErRvy01DD6mh_CQBnhGKbyWG-ymrlIptHnSNUVT1A53TNDoE2zr7YSBkIsDxFd9sjymzqQQzof2xw86kL-Z8uwXlj_KSbiWRP1xzArmob40InxUv4wFvDPsYsEeJfqZ" height=207 width=596></td>

<td>Thanks to this demo by gtsteel@, it is easy to see why animating in the RGB color space is preferable to sRGB.</td>

<td>It's a colorful world</td>

<td>While discussing a <a href="http://crbug.com/981326">color interpolation bug</a> that Xida (xidachen@) was working on, he and George (gtsteel@) realized that by spec colors interpolate in the sRGB colorspace. This is unfortunate as sRGB is a non-linear space which results in less natural looking animations than the linear RGB space. George began investigating the possibility of adding RGB interpolation support to the various animation specs, however luckily we discovered that the <a href="https://www.w3.org/TR/css-color-4">css-color-4</a> spec is working on allowing web developers to <a href="https://www.w3.org/TR/css-color-4/#working-color-space">change the page's color space</a> which should also fix animations for developers who opt in.</td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | July 2019</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
