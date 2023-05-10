---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: web-animation-scroll-linked-animation-snap-after-layout-throughput-metrics-and-more
title: Web Animation, Scroll-linked Animation, Snap after layout, throughput metrics
  and more!
---

<table>
<tr>

<td>February 2020</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh6.googleusercontent.com/Duuu09yTSsJWnz8tD3LTf_8Efkat2mPX-57vvznGOi2ztEqT4jaREe0m6l_HTxhef-ztUn4IM2QsVA5VRpqu1DAQuJF2IMnuzkgnY1pXx5UneEIkdF88GxKN6LhRkMB1_9WfN48Q" height=360 width=254><img alt="image" src="https://lh5.googleusercontent.com/Pq6YWdKed4hu4OrROqEtMyda4ULyRcRr_Vvf6OAs2OMibUkmCQrZ7khiug4QjV2pEEAwpmaKAMQjdUQLaNrKvHS3z5VvNQ0s0s-KbOTJ9Ww1FwFlcK1-RY_ff41-jU-zirkVrgWD" height=359 width=329></td>

<td>A further step of scroll-snap after layout</td>

<td>In January we turned on the feature by default in Chrome 81. To better communicate with developers to deliver the feature to users, Yi (yigu@) worked with Adam (argyle@) and Kayce (kayce@) from the Web DevRel team and published a <a href="https://web.dev/snap-after-layout/">blog post</a> on <a href="http://web.dev">web.dev</a> with fresh demos. As Rick (rbyers@) <a href="https://twitter.com/RickByers/status/1235318530565984257">pointed out</a>, for years web developers have been asking how to keep the scroller from a chat app reliably stuck at the bottom. With scroll-snap after layout, developers can easily implement it with pure CSS (left gif). Moreover, they no longer need to add event listeners to force resnapping after layout changes such as rotating a device or resizing a window (right gif)!</td>

<td><table></td>
<td><tr></td>

<td><td>function flip() {</td></td>

<td><td> anim.playbackRate = -anim.playbackRate;</td></td>

<td><td>}</td></td>

<td><td>Updating playbackRate caused unwanted jumping back and forwards, as an animation was not marked as outdated when resuming from a finished state.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/qwecDRWN-ff5APjwvnBG-lmKnBTblQKFibqIJ5yQYJHSSRfZ63ccR-CfQqoLbl65JRktpUKHNl56OtALLB5C7vaGrRKnfotZxaxwCPpfe58vxam0XFKjTDCiKlBz1gXOjMWWbnuO" height=147 width=265></td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td></tr></td>
<td><tr></td>
<td></tr></td>
<td><tr></td>

<td><td>document.body.addEventListener(</td></td>

<td><td> 'mousemove', evt =&gt; {</td></td>

<td><td> const animation = circle.animate(</td></td>

<td><td> { transform : 'translate(...)' },</td></td>

<td><td> { duration: 500, fill: 'forwards' }</td></td>

<td><td> );</td></td>

<td><td> animation.finished.then(() =&gt; {</td></td>

<td><td> animation.commitStyles();</td></td>

<td><td> animation.cancel();</td></td>

<td><td> });</td></td>

<td><td>});</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/z5MCQctf8obsWW1sjlgqFKVR3vS4cvO9hnmJ3ORHu-uxuQ4gNeW7zFmIcMfLcXDUrERJdXJRn6ZWz7sO8rB7dGd_7R1qqRcLVnh4jSyYikPLrFqVHMsbVL3r_28KYcHMpCJuPiJv" height=133 width=248></td></td>

<td><td>Require layout object when resolving style since style could be box size dependent.</td></td>

<td></tr></td>
<td><tr></td>

<td><td colspan=2># Animation.commitStyles did not correctly handle transforms</td></td>

<td></tr></td>
<td><tr></td>

<td><td colspan=2>Web Animation on the way</td></td>

<td><td colspan=2>The team has made solid progress on new features and bug fixes. Kevin fixed the two regressions above. George (gtsteel@) completed the work on pseudo-element animations on both <a href="https://github.com/w3c/csswg-drafts/pull/4616">specification</a> and <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2031986">implementation</a> sides. As usual, Hao (haozhes@) made our implementation more interoperable by passing more wpt tests!</td></td>

<td><td colspan=2><img alt="image" src="https://lh4.googleusercontent.com/qUQHx8YY3PrU0F3X-9OfErZR9IwlHsKsCElzfR8h0v9xGR824m708ECrqY_z6gvJHTRSIQWJH1pol7Sshj9rd2BkRqU0Q3jH8H8RSXVTZcXyO5NgXO4zigsDSUzncj_vUbXVvJAj" height=248 width=555></td></td>

<td><td colspan=2><img alt="image" src="https://lh4.googleusercontent.com/tg1Bvj-WJkkw_QW8zVhsgu5AEO-blW-gYIZZL_l-nqcDPxQO89L0l3JIREtJ0-XMhXmEMIy-xDC36QbWsi-LsbHRpy8-7p57ppOCoyI922ZnUvkn64INt1hyGlg_J9HbZzCkov1a" height=251 width=555></td></td>

<td><td colspan=2>A new path of throughput metrics</td></td>

<td><td colspan=2>Frame throughput measures the smoothness of Chrome renderer. i.e. higher is better. However, it makes the UMA timeline hard to read. e.g. usually 90th percentile represents the 90% users with better results but it’s opposite in the throughput metrics. To better align with the UMA timeline, Xida (xidachen@) inverted the metrics Throughput with PercentDroppedFrames which still measures the performance but in a more readable way. For example, the graph in the above shows that at 90th percentile we have ~80% frames dropped.</td></td>

<td><td colspan=2>Scroll-linked animations</td></td>

<td><td colspan=2>We have been collaborating with Microsoft engineers towards shipping scroll-linked animations. Both Majid (majidvp@) and Olga (<a href="mailto:gerchiko@microsoft.com">gerchiko@microsoft.com</a>) became the <a href="https://drafts.csswg.org/scroll-animations/">specification editors</a> which helps with driving specification discussions and updates. This month, Majid wrote a <a href="https://github.com/w3c/csswg-drafts/pull/4751">PR</a> for css syntax and reviewed <a href="https://github.com/w3c/csswg-drafts/pull/4750">PR</a> to remove ScrollTimeline.fill. Gene (girard@) and Rob (flackr@) helped with reviewing all the outstanding spec issues and prioritized them. On the implementation side, Olga made solid progress on implementing scroll offset <a href="https://wicg.github.io/scroll-animations/#avoiding-cycles">snapshotting</a>; Majid started to prototype <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2070673">element based scroll offset</a> and Yi started to add support for <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2084088">composited scroll-linked animations</a>.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | February 2020</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
