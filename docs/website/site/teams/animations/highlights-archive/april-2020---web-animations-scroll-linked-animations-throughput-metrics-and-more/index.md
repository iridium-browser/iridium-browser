---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: april-2020---web-animations-scroll-linked-animations-throughput-metrics-and-more
title: April 2020 - Web Animations, Scroll-linked Animations, Throughput Metrics,
  Performant Meet, Hackathon and more!
---

<table>
<tr>

<td>April 2020</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh5.googleusercontent.com/Asd0BAjki50O1Jd36jqK8YY87ftxZs4-UTyjagI2h_J-YXt_TCDKwOVcib-lSWoPZzyqhL9DmvaipvwzNVLkpwxr3acGYHXxUbY2m3xeNtuBRmz70cD3ii11lX7NceLqahtL_5Uf" height=244 width=317></td>

<td>Celebrate shipping Web Animation API<a href="https://jsbin.com/weqamosare/1/edit?js,output"> using the API</a></td>

<td><table></td>
<td><tr></td>

<td><td colspan=2>Complete Web Animation API in M84!</td></td>

<td><td colspan=2>The <a href="https://drafts.csswg.org/web-animations-1/">API</a> provides developers with a powerful way to create and control animations on the web including existing CSS Animations and Transitions. After years of effort by the Animation team, Kevin (kevers@) sent out the final <a href="https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/Wu4yPMznUw0">Intent to Ship</a> and turned on the feature <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2161345">by default</a> in M84! Here is a <a href="https://docs.google.com/document/d/1sWAEytrZDxQWnnqozMSzI2lHvIJ0kF3dzgy9Q1PbYjE/edit">showcase</a> of all the new features. This was a collaborative effort with other browsers which is why the same rich API would be available in <a href="https://webkit.org/blog/10266/web-animations-in-safari-13-1/">Safari</a> and <a href="https://hacks.mozilla.org/2020/04/firefox-75-ambitions-for-april/">Firefox</a> as well. There is still <a href="https://github.com/w3c/csswg-drafts/labels/web-animations-2">room for improvement</a> but let’s take some time to celebrate this big milestone in animations! Thanks to the dozens of developers across five+ organizations (Chrome, Microsoft, Firefox, Igalia, Opera etc.) who have contributed to this! </td></td>

<td><td colspan=2><img alt="image" src="https://lh5.googleusercontent.com/LqXcIohcW0fR8okipubge1E96AcHRzz3xbiq4OubalRtluI5uHTUXSmkFMPNNqy2tLIkW4sRA2sVT8Ugg_wxNNCxvLXMc7wVyhWzcv0tlcNba_EhRbQJKJlTMj_iGoeBzR0z28Ac" height=230 width=459></td></td>

<td><td colspan=2>A long journey of shipping Web Animation API</td></td>

<td></tr></td>
<td><tr></td>
<td></tr></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/YcuCWIgXIi7sSxu3o_nP5sEfCPm9JC3eFAe4-xHZ3KC3UXV5FkXsOZJqUUdZo7X9M8TcWU_CIC-ZhOZZjxiIgvZqI0jA6deOklMCJ3BgMLKhB7HBS0rw5g-aSp0Hj3_mWR6INTTp" height=40 width=273></td></td>

<td><td>Green Volume Meter</td></td>

<td><td>Google Meet usage has surged significantly, <a href="https://www.theverge.com/2020/4/28/21240434/google-meet-three-million-users-per-day-pichai-earnings">adding 3M users per day</a>. We’ve noticed that the green volume meter was unexpectedly re-rastered during animation, causing significant CPU usage. Rob (flackr@) came to the rescue and <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1074055">fixed</a> the issue by preserving the raster scale for animations with will-change: transform. This avoids unnecessary and expensive rasterization, and should greatly reduce CPU/power usage. </td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/xs4GcGU_ey6XxTBPJndhQgyTwZTMiNFuTX3bOTnxgImBUZGsXFIvlhTMbv-x28U93zhNiZP3TC168i_8MloXC2zT3iZxd-zWw6CLHSVYfijGqMQCHLqAwo5fNp6bT2sJes9VsL6o" height=152 width=280></td></td>

<td><td>Better Frame Throughput Metrics</td></td>

<td><td>Frame Throughput is severely impacted during initial page load, often dominating the metric. Xida (xidachen@) modified the sampling logic to measure from 5s to 10s, resulting in more accurate and meaningful values. One site’s dropped frames moved from 84% to 15% at 50th percentile and 99% to 75% at the 95th percentile.</td></td>

<td></tr></td>
<td><tr></td>

<td><td colspan=2>Scroll-linked Animations</td></td>

<td><td colspan=2>It’s been a productive month for scroll-linked animations. Majid (majidvp@) and Yi (yigu@) wrote a full-fledged <a href="https://github.com/w3c/csswg-drafts/blob/master/scroll-animations-1/EXPLAINER.md">explainer</a> for scroll timeline discussing many of the design trade-offs and showcasing multiple examples. Olga (<a href="mailto:gerchiko@microsoft.com">gerchiko@microsoft.com</a>) landed the spec change and implementation for using “zero” as initial start time for scroll-linked animation. Majid landed implementation for <a href="https://github.com/w3c/csswg-drafts/issues/4337">element-based scroll offset</a> and Yi added full support for running scroll-linked animations on the compositor. We now have enough implemented to see the <a href="https://majido.github.io/scroll-timeline/demo/parallax/">demo</a> we built using polyfill now works natively in Chrome Canary with the flag enable-experimental-web-platform-features.</td></td>

<td></tr></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/DrrQOx_U_ImfrYijLwAGdIm_khjezvah8V_DwBsvjgErlu-JBozu03cJ4oBy2k3yoGby56gfExyyGK210bfPnF5gMshGzXO4TuWhJ46fgW6N_GCAdG1oJ83s9JbXtDRLSkWRymYM" height=330 width=257></td></td>

<td><td>The timeline start/end offsets are computed based on position of element on the page. When we increase the margin of the element the offsets get updated which affects currentTime and subsequently the animation output.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/7yFkBLxKEQnOW-Sdk10vdZluDxgGPQthrGPZG247xYSdHJiTDpmwuxk1mm9jNKFAR1orKDHaFO9abHThe-cdtZd20DjlRb4hdC0085ODhLqnzabP9PkpvtuiC5sYslmxtj5SFilB" height=329 width=254></td></td>

<td><td>Scroll-linked animations show up in devtools and can be scrubbed on-demand!</td></td>

<td></tr></td>
<td><tr></td>

<td><td colspan=2>Animations Team Hackathon</td></td>

<td><td colspan=2>To better understand the Web Animation APIs we’re using and experience the awesome ergonomics of them, and/or pain points, Gene (girard@) initiated the first ever Animations team 1-day hackathon. We created lots of fun experiments that exercise the new APIs: <a href="https://majido.github.io/animation_jam/app/index.html">interactive animation editor</a>, <a href="https://codepen.io/shengha2/pen/NWGWLvq">tetris</a>, <a href="https://codepen.io/yigu/pen/dyYPoLY">ping pong</a>, <a href="https://codepen.io/george-steel/pen/zYvYWxO">got it game</a>, <a href="https://jsbin.com/tanuzonixo/1/edit?html,css,js,output">pendulum </a>(<a href="https://docs.google.com/document/d/1VImYZIzh6mSRwBhSgwiUsB8Mj9NCASqMYzXEa4JXXk4/edit?usp=sharing">explainer).</a></td></td>

<td><td colspan=2><img alt="image" src="https://lh4.googleusercontent.com/T_bcxoaClVBZ9n8k-zQlEaI9eMHlIamrMH4ydr7WSvu0hmvTec4RwW9ZsjCEc42w1VrhCY6QHwNXRFnUDmSkjHGvNBAKNXEb_x0C5qF2KegFbmFY2j01q_r4sBIbQnOVYdw66L4b" height=241 width=245></td></td>

<td><td colspan=2>Constructing the perfect pendulum: an exercise in using the web-animations API</td></td>

<td></tr></td>
<td><tr></td>
<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | April 2020</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
