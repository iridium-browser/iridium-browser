---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: september-update
title: September 2019
---

<table>
<tr>

<td>September 2019</td>

<td>Chrome Animations Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/animations-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh6.googleusercontent.com/sXy9qEAKUpZOSAjFMLPicre5Zj4uUdiukK-uBgf7lHbvH3comIpZuE30nOlH3Mmga03rJGC6Ctcjbg6K0KTjrrV37ALqs3-Cf0wnh0Sr9JuLwxPKaF_hChJuLsUJ--x95zyU725_" height=245 width=596></td>

<td>TPAC - Technical Plenary And Cookies</td>

<td>Several members of the team attended TPAC to make progress on key specifications. We did <a href="https://docs.google.com/document/d/1-y6rEBbOi3jXUtiuBW2ecmQSBKatCQu_ZC_N8yLY3DI/edit">pre-planning</a> ensuring key issues are filed and discussed with the right people. Full trip-report is <a href="https://docs.google.com/document/d/1S49ZrTaCV1KMkFK4hdtMwheM9Sjdd_eiyDqH0A-3q48/edit?usp=sharing">here</a> but some highlights from TPAC are:</td>

    <td>Agreements on deferring some small features (<a
    href="https://github.com/w3c/csswg-drafts/issues/4300">1</a>, <a
    href="https://github.com/w3c/csswg-drafts/issues/4299">2</a>, <a
    href="https://github.com/w3c/csswg-drafts/issues/4301">3</a>) to web
    animations level 2 paving the path for shipping level 1.</td>

    <td>Moving ScrollTimeline out of incubation with <a
    href="https://github.com/w3c/csswg-drafts/issues/4337#issuecomment-532120609">agreements</a>
    on remaining key issues.</td>

    <td>Progress on issues related to Group Effect and Matrix transforms.</td>

    <td>Kevin (kevers@) learned a lot about WPT and how we can use it more
    effectively which he <a
    href="https://docs.google.com/presentation/d/1hlweg5L2V6gyxtXnj6MeCTXqNHRMnvAILNDIPS8dnOc/edit?usp=sharing">shared</a>
    with the team.</td>

<td>All of these were a result of constructive discussions with collaborators from Microsoft, Mozilla and Apple.</td>

<td><table></td>
<td><tr></td>

<td><td>Scroll Timeline Polyfilled</td></td>

<td><td>As part of TPAC preparation Majid (majidvp@) proposed <a href="https://github.com/w3c/csswg-drafts/issues/4337">element-based targeting</a> for ScrollTimeline. Rob (flackr@) wrote a full fidelity polyfill for ScrollTimeline which implements the proposal. He used the polyfill to create a compelling <a href="https://flackr.github.io/scroll-timeline/demo/parallax/">demo</a> of key usecases. The demo clearly shows the improved ergonomics which greatly helped in <a href="https://github.com/w3c/csswg-drafts/issues/4334">convincing</a> CSSWG on merits of the proposal. It also helped us validate the proposal and find several awkward parts of the current API <a href="https://github.com/w3c/csswg-drafts/issues/4324">\[1\]</a> <a href="https://github.com/w3c/csswg-drafts/issues/4325">\[2\]</a> <a href="https://github.com/w3c/csswg-drafts/issues/4327">\[3\]</a> <a href="https://github.com/w3c/csswg-drafts/issues/4336">\[4\]</a> <a href="https://github.com/w3c/csswg-drafts/issues/4323">\[5\]</a>.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/LSLhqQDlrowYd1CDTj-9Yy_6eCEW3TaIVeb_RnZb_5EnHqK09s6jVoIALIqsDPjMA7RsRQ7MCckZylwQ_vZd8wkV9wS_0UUoGx6bRQPANzz9culKlBGkKYBzJIaTCRDVPezZnXD8" height=504 width=283></td></td>

<td></tr></td>
<td><tr></td>

<td><td>Excellent Scroll Snap</td></td>

<td><td>We continue to invest in making scroll snapping excellent. Kaan (alsan@) has made sure more wpt test pass in chrome <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1799387">\[1\]</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1799387">\[2\]</a> and pay some <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1801745">technical debts</a> while Majid worked on improving wheel scroll snapping <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1835012">\[1\]</a> <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1769044">\[2\]</a>. We also triaged and prioritized our bugs around interop after discussions with other browser vendors. </td></td>

<td><td>Web Animation Progress</td></td>

<td><td>Stephen (smcgruer@) spend time to <a href="https://docs.google.com/document/d/1YPgb85q9w3HGKuMb4YCgTNaUTcF9mkjcMmfb2AeD64I/edit#heading=h.luhqvvzi99ac">categorize</a> and file bugs for all remaining failures. Stephen also made <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=788440#c9">progress</a> on implementation of ‘composite: accumulate’ and several timing bugs <a href="https://chromium.googlesource.com/chromium/src.git/+/c004564dcd80e078136866d21ab942e78a93753a">\[1\]</a>, <a href="https://chromium.googlesource.com/chromium/src.git/+/73b9e0f4905111da5034767d7956eca0940aaac0">\[2\]</a>, <a href="https://chromium.googlesource.com/chromium/src.git/+/108ec45b194b5e9a16d91e1ef0cd4dd145b00ae6">\[3\]</a>.</td></td>

<td></tr></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/F6s9bCRSGPI3ox1LoaVo_us5MdXRLJ5gCkqrzeF20F_go4G9UT5eZonbRBwF1y0gE9MLmVF2jv1O25KOU24O9HJnq4JjRZdVJn8G09zur5yGrefzXV4GQy4oraN7LHK4Xht_yoHc" height=143 width=283></td></td>

<td><td>Off-Thread Paint Worklet 3..2..1..🚀</td></td>

<td><td>"If the intersection between weird and usable is your thing, you’ll feel right at home with paint worklets!" </td></td>

<td><td>This is a quote from the <a href="http://cssconfbp.rocks/speakers/jeremy/">Paint Worklet presentation</a> in CSSConf Budapest which comes with some neat <a href="https://paintlets.herokuapp.com">demos</a>. All the demos works flawlessly off the main thread (with --enable-blink-features=OffMainThreadCSSPaint). This sprint Xida (xidachen@) ensured Off-thread Paint Worklet has a <a href="https://docs.google.com/document/d/1XzfgvEE7B-RZId7vKPO3a7jzcCYqoc_0D80ggotAiQo/edit#">launch plan</a> as we get very close on enabling it on ToT. He also landed <a href="https://chromium.googlesource.com/chromium/src.git/+/80da06275c6afa7b54e8bac39e138b823538c7a1">metrics</a> and fixed multiple crashes <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1828063">\[1\]</a> <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1831790">\[2\]</a> <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1789828">\[3\]</a>.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/64I_PDKy4pCiACJrwKM_0GJSrxwaH3brwuyujUBCIajbcEoxb765OQ-8AG99o-7-d51IKaIVnPS85H5uPOnl289GVIehkgk09fsXebobI6s9KGOU-8IxTHt3UeK4kzGH5O0euSju" height=139 width=283></td></td>

<td><td>Viz-HitTesting launched 1% on Stable</td></td>

<td><td>This sprint Viz hit-testing V2 was launched 1% on Stable for non-CrOS. This was a boring launch thanks to many bug-fixes landed beforehand. On <a href="https://docs.google.com/document/d/1BJK_lcOnY6W5_Gjex44dVxc-sKDBpw4YU0zJlKQlkao/edit#heading=h.1ba74q72laoc">Windows</a> it shows that we are 32% better than the existing behavior on how many hit tests are handled synchronously.</td></td>

<td><td>The performance on CrOS in 78 beta is fascinating. <a href="https://docs.google.com/document/d/1YZ6NsxiiC3g6D6TTBcoKlCAgttoH2TA2PNX9R_Cxrkw/edit#heading=h.vlq2ywuvza4d">96%</a> hit tests are synchronous. \\o/</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Code Health and Interop</td></td>

<td><td>Stephen (smcgruer@) worked on improving web interoperability and compatibility by <a href="https://groups.google.com/a/chromium.org/d/msg/blink-dev/dxDGBFKvO3A/A2gajDMjAwAJ">shipping</a> ontransition event handlers. He spent time understanding webkit-prefixed versions usage and devised a plan for removal or standardization (<a href="https://github.com/whatwg/compat/issues/118">whatwg</a>, <a href="https://chromium.googlesource.com/chromium/src.git/+/85c09dce313fac83a250fd035b85fa7606f8b080">code</a>). George (gtsteel@) <a href="https://github.com/w3c/csswg-drafts/pull/4306">improved</a> css transitions specification and added new and cleaned up existing relevant tests <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1807297">\[1\]</a>, <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1809508">\[2\]</a>. Yi (yigu@) <a href="https://chromium-review.googlesource.com/c/chromium/src/+/1825720">removed</a> special compositing logic for ScrollTimeline making it simpler and more general.</td></td>

<td><td>Team Changes</td></td>

<td><td>Sadly our amazing Stephen (smcgruer@) is leaving the Animations team to join Ecosystem infrastructure team. While we are sad to see him leaving our team, we are happy that his passion, skills and leadership are going to have a large impact on the Web making it more interoperable. To ensure a smooth transition we have marked all bugs assigned to him as Hotlist-Interop ;). On the good news front, we have a new intern Kaan (alsan@) who is going to make Scroll Snapping more excellent, and noogler Haozhe (haozhes@) joining the team.</td></td>

<td></tr></td>
<td><tr></td>
<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Animations Highlights | September 2019</td>

<td><a href="http://go/animations-team">go/animations-team</a></td>

</tr>
</table>
