---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: january-2021---code-health-new-features-testing-and-stability-fixes
title: January 2021 - Code Health, New Features, Testing and Stability Fixes
---

<table>
<tr>

<td>January 2020</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Chapter I: New Features</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Scroll timeline polyfill</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/wDUAhebw0vhXO9C35VZA1VuJY-pNYDGPaurAXLhap5vN_7mmeaAKs1Bv1feGheZzowhF0Q8pJAb0gBZq5pC7V9msHgBd4Xww5X6xOMjgvPOon7w_fCLXfyNDK5anNuG63XGj3jCg0g" height=175 width=280></td></td>

<td><td>Both kevers@ and flackr@ have been working extensively on this feature.</td></td>

    <td><td>Over 93% pass rate on the scroll timeline tests.</td></td>

    <td><td>Over 350 passing tests.</td></td>

    <td><td>Recent areas of development:</td></td>

        <td><td>Extended the scope of the polyfill:</td></td>

            <td><td>EventTarget</td></td>

            <td><td>Promises</td></td>

            <td><td>Tasks</td></td>

        <td><td>Support writing modes</td></td>

<td><td>Compositing percentage based transforms</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/s0yra2ZzXVDUViwMExbnMhRNT-wV5_1XN3BjCLKg8CahRv4X29TbHcZBvZ7hWBBEH-Bv31nuoQtG6Hj685Lxe8xNljDtpH-vB-C4Y07nh2v_T1nsUEqX-ZOb5MxJNmWY1MqQ6l9vVg" height=172 width=225></td></td>

<td><td>menu.hidden { transform: translateX(-100%) }</td></td>

<td><td>kevers@ re-enabled this feature in M89:</td></td>

    <td><td>The challenge is that size dependency could change while the
    animation is running. And we need to catch the change and restart the
    composited animation.</td></td>

    <td><td>The feature is in common use. One can write CSS only animations
    without the need to resort to JS for size calculation, such as the above
    demo.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Aspect ratio interpolation</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/J2Ao_wP4zetydT84MzFOI9sBrfbjXjfYsIwOwspHUn-66j0reQlqj_8CnMYqoJpYsU7sc4Gpmsi1PpDmdXhcsjE20NzhYkpDs1vcCLkEhc13WG27wY9ThX3B36qJ0Af3Yvh32foVGw" height=111 width=132> <img alt="image" src="https://lh4.googleusercontent.com/Jk1F0TdSK1OjQuphK-Z-h8sE-mB2beGCnzTRCU8VHcIkHwkqn7G_vps1dj4JPAgMR1M0FDGYEUvjrTAdu5c8vB5mf1hLCshite8C66pKVJNF0cVqHWvDJTdQfK3oy5So2iheBH1erw" height=110 width=141></td></td>

<td><td> <a href="https://output.jsbin.com/niyuref/quiet">Demo</a> Before <a href="https://output.jsbin.com/niyuref/quiet">Demo</a> After</td></td>

<td><td>flackr@ implemented aspect ratio interpolation, and enabled it by default in M90.</td></td>

    <td><td>Recent spec <a
    href="https://github.com/w3c/csswg-drafts/issues/4953">discussion</a>
    decided that aspect ratios should be interpolated by the log of their value,
    to preserve the same visual speed whether they are wide or narrow.</td></td>

<td><td>Composite background-color animation</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/Cr89iFEq8k2axei93_YW0wmhvqmaiXE4XaIF50OVncHHUZqcDP23r2TeDsnSP_2nzuqP4_1N0gUqTIBoRPW0cS_ccxgsE5f3pY3wHJ2_0bWDDANBgbfsm4RBfZjLY-pnX9X3JnKGaQ" height=203.57052631578944 width=217.23208556149734></td></td>

<td><td>The above background-color animation <a href="https://output.jsbin.com/xocoroh">demo</a> runs on the compositor thread with the experimental flag. xidachen@ has made great progress on this feature, including:</td></td>

    <td><td>Setup infra for animating on the compositor thread.</td></td>

    <td><td>Implement the interpolation logic.</td></td>

    <td><td>Handle transition keyframes.</td></td>

    <td><td>Handle multiple offsets in keyframes.</td></td>

    <td><td>Implement main-thread fallback logic.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Click/Auxclick as pointer event</td></td>

<td><td>liviutinta@ and mustaq@ co-worked on this feature and shipped it to stable. Here is the major work:</td></td>

    <td><td>Finch experiment: 50% on canary + dev + beta.</td></td>

    <td><td><a
    href="https://groups.google.com/u/1/a/chromium.org/g/blink-dev/c/bta50W_Hg24">Intent
    to ship</a> has been approved with 3 LGTMs.</td></td>

    <td><td>Enabled for stable at 1% starting with M88.</td></td>

    <td><td>Improvement of the feature is still on-going, such as the <a
    href="https://bugs.chromium.org/p/chromium/issues/detail?id=1150979">simulated
    clicks</a>.</td></td>

<td><td>Capability delegation with payment request</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/5MDu8JUMJOsu95kCUwcMhSlfw5kd8uU6vwXSDPG_ycd6mSBoDWnU-EfRY9ssODQuDPXG3C076zhHaO_OrmqKJK-uwzAG41pCoLJJ-1SGmahYvrK_v0JbkUm4-1zVRiqFtEUOrOghyg" height=194 width=256></td></td>

<td><td>mustaq@ built an initial prototype that allows PaymentRequest in subframe provided a token is passed into it. The PaymentRequest now consumes user activation.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Scroll unification</td></td>

<td><td>liviutinta@ kept working on this feature, and has made progress.</td></td>

    <td><td>Landed <a
    href="https://bugs.chromium.org/p/chromium/issues/detail?id=1113863">scrolling
    the &lt;select&gt; popup</a>.</td></td>

    <td><td>Designed <a
    href="https://docs.google.com/document/d/1w_Pr4sKPXfHz03ZUxVSZL23xjQy-TiKAKf8rmbcW1Fc/edit#">metrics</a>
    for scroll unification and LGTMed by the metrics team.</td></td>

    <td><td>Landed scroll bubbling from OOPIFs to ancestor frame</td></td>

    <td><td>Schedule frames for popup widgets in tests using compositor
    thread.</td></td>

<td><td>Scroll to images (and videos)</td></td>

<td><td>flackr@ has created and uploaded the <a href="https://github.com/WICG/scroll-to-text-fragment/blob/master/EXTENSIONS.md">explainer</a> for this feature.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Testing</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Deflaking tests</td></td>

<td><td>kevers@ addressed some test flakes in animations and scroll-snap. These flakes were largely triggered due to common gotchas like antialiasing, floating point precision, or test timing. </td></td>

<td><td>Animation timing checks have been updated to accommodate 1 microsecond accuracy. This fix was not a pure test change as there was a lurking issue with numerical precision and setting the finished state of an animation. It is not just strict equality checks that are problematic when working with floating point numbers.</td></td>

<td><td>Several layout tests in animations make use of an internal pause for testing API, which predates and is largely deprecated by the web animations API (WAAPI). One test flake was addressed simply by switching over to WAAPI. </td></td>

<td><td>Several timing flakes were fixed in scroll-snap tests, by injecting a listener for scroll events. Unlike web animations, scroll animations do not fire an event to indicate completion of the scroll operation, and the end is inferred based on an absence of position updates over time. Unfortunately, this can lead to flakes when testing machines get bogged down and tests start running slower than expected. Simply waiting on an initial scroll event before starting the countdown, largely addresses this type of flake in scroll-snap tests. Once scrolling starts, the number of animation frames (rAFs) required to complete the scroll has a strict upper bound.</td></td>

<td><td>xidachen@ also fixed quite a few paint worklet flaky tests by using the standard animation API. </td></td>

<td><td>Before: we are taking a screenshot at rAF, which doesn’t guarantee the start of the animation.</td></td>

<td><td>Now: with the animation ready API, we take a screenshot when the animation has started which prevents the flakiness.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/Dx0Odk5DcCkNfcAetXSysZmPSIIcC198hA3S56TcCbGBm2hLF1Tn-gBC4fuzitR4kSWVmBzblZcL5g9Pwq1XHcJ3pZhcho-i99oSHnJ5NRSbJnw79M6TgXSoZzfisUJnm6AgeLyWcg" height=72 width=283></td></td>

<td><td>Before</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/FDMhgeEl1cuWOajwHO1P7dzgH09IAE06j0e_vGeUbyAXBa3jOc7xvfLBmcYwSsnNZUb9r0vjQhSn_m4VQGStucPhTcpxNHYeJxR1cuoBwyTxTSnqZLI8ob80dqnr56gKf7u8hEYeZw" height=71 width=283></td></td>

<td><td>After</td></td>

<td><td>WPT test coverage</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/rdOjKXaS2ht4sGwGCgrOuB1JQFVuOqvNiFumSW_zh89KsUBu1PenVeuy1oN3HJwAwF5bsTNUscPo_pLKIqgcHzWvYaKWyPRG1KnF-XcoQgxRtBpctW-TtxAiWv83RatSHvAxQ6EHvw" height=55 width=283></td></td>

<td><td>kevers@ and girard@ <a href="https://docs.google.com/spreadsheets/d/1aUnU5Igiescq93oY01bBPJ77TBb3oy5Y-jvUf7u_600/edit#gid=15396280">measured</a> test coverage in WPT for input/scroll/animation. The data can help us understand which category of tests needs more attention.</td></td>

<td><td>TestDriver Action API</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/rcHUPkETRQuNvGLbl_2x0o9KA2PhTUnHISjRsr0Oon68LKo9LtgnUynF997aTlPbj18znph4sfrEeZhNzyPlUA7oINLMWQT_oyemx_m_oCRxjzvUqZKkbXpIK1eUYT0torK1rqRiEg" height=178 width=237></td></td>

<td><td>lanwei@ kept improving the TestDriver action API and made more WPT tests automatic.</td></td>

<td><td>The following two pictures show the wpt dashboard for the TestDriver action API.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/VZ_e8LDAUVDSGmSXU8OcOjoqUtEQdpk2Xm9wbtCohj3nk2Dc-cVxFvyrg4hMQxwME1gRVzdlnOAiVs_qRkNqpvSqreNLBW7-U3xjXbUj-jHeF2BkbFjnAzC2UGbzaMZdiCouHcO8Rw" height=101 width=144> <img alt="image" src="https://lh6.googleusercontent.com/uCRgxqbX_Vp5mUsc3z0umBfFrdZtMBYySftCvpx8vQCy81r8Ljgftfw594GnAipZBazHUK7D59iNF5s2BGSTiZBK3vVrmZFM2ZdaB245E7Qb-vj294VLRF7MR3jwxTpf1Qg4ZDYZHA" height=108 width=127></td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Stability Fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Animation security fix: use after poison</td></td>

<td><td>kevers@ fixed a crash in Animatable::Animation. The problem is that running user code (JS) can result in the destruction of an execution context. We cannot simply check and forget about the execution context. The solution is to double check if the context is used downstream of script execution.</td></td>

<td><td>Float-cast-overflow in MouseEvent::PageX</td></td>

<td><td>liviutinta@ fixes a float-cast-overflow bug that is caused by static_cast&lt;int&gt;(double value) when the value is out of range. The solution is to usd std::trunc and clampToInt instead.</td></td>

<td><td>Paint worklet regression</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/LRW7Vq-NI5lusgpbWRv6HVUKTn0mZQztrsHkqtw9Yh-AtGXPD5pKQPJNtIY_0BcSEdKnBykk2KoZ_afKqK17-PbFJDWUfV91I0tzte96FktV2V67DsPIPTrsRD0UchD0uYyExNKkYA" height=222.4665071770335 width=284.0179856115108> <img alt="image" src="https://lh4.googleusercontent.com/PRq4eiC7ydMEjQ3gF6CKwFT_AdhAYjsxJ9vODczpmLtVRwjToS0UdnaJ5thn0jnKuzqTh7TqtJt_cSGd8IfurJ4DJp7eETLIRHVhBogoKEcJINDTwxFX-0fMARFxj8krVA2iINYVFQ" height=223.95238690885697 width=281.9892857142857></td></td>

<td><td> Stable Canary</td></td>

<td><td>xidachen@ fixed a regression due to a misuse of hashmap. In the above demo, the same animation is applied to both elements. In other words, the animations of the two elements should be in sync. However, the stable version is wrong. The canary version has the fix in it, and it is correct. </td></td>

<td></tr></td>
<td><tr></td>

<td><td>Crash in PointerEventFactory</td></td>

<td><td>mustaq@ untangled a mixed-up <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1164347#c4">crash report</a> that becomes release-blocker-stable at the last moment. The effort of fixing the crash includes:</td></td>

    <td><td>Determine the root cause, landed the <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2488181">fix</a>
    on ToT.</td></td>

    <td><td>Merged the fix back to M88 and M89.</td></td>

<td><td>Paint worklet crash</td></td>

<td><td>xidachen@ fixed a crash where the root cause is that our code path hits a DCHECK that is no longer applicable. The solution is to remove the outdated DCHECK.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: Code Health</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/ewdZ0ZzCsU5ix0iB-HZ3b9Rd_D2cpoVJTrlRAMh9lgyhQ96jCl2qEG2PLS_He6sp2Og_8BVheiG83LFJqDG1Bf4XwSyb1NxlJ_fEbzXlNV3hD-jsYHOdgSJkdIIa3WEeK9UQcpNprA" height=162 width=292></td></td>

<td><td>Our team had a great start to the year by closing over 70 bugs in this sprint:</td></td>

    <td><td>Specifically, we closed more bugs than opened in P1 and P2
    categories.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/-hszaQS6D49VBAmVoXwrtim2_Z_vFlVFNJH4L0YtdLHFo_4kQPX9dmwBrW3kQPpHv1nQOceCi6QWOGQtyq3HLpBysTtD57n2zUbsU9fM3_br_re6EfeEqGbe1VptGOzNdv0GUUYHuQ" height=136 width=217> <img alt="image" src="https://lh3.googleusercontent.com/h8ggB6_4DB7Mov_O0H4AJhF7gxhzAjxbo7Abgjz-6dzH9bVz1-mnXG7leBnPB0CF2fQb_kPIezzcEartcLY4TV9HG8OCHLFsZSxfvq35pAk389xH3Oa95Yg_4LWXUbyc8RNe5WEA5A" height=140 width=227></td></td>

<td><td>girard@ collected our team bug data and generated graphs to visualize it.</td></td>

    <td><td>Our P1 and P2 bugs are less than 40% of total bugs.</td></td>

    <td><td>~40% bugs have owners assigned.</td></td>

<td><td>girard@ re-enabled “<a href="https://chromium-review.googlesource.com/c/chromium/src/+/2643444">MissedTOUCHEVENTF_UP</a>” which tracks a hack in windows touch. We would like to <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=811273">tear out the hack/workaround</a> once usage drops.</td></td>

<td><td>liviutinta@ removed the <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2595636">AzimuthAltitude flag</a> since it has been turned on by default since M86.</td></td>

<td><td>lanwei@ and liviutinta@ fixed input failing tests on the new WPT bots.</td></td>

<td><td>mustaq@ landed Mouse/PointerEvent related code cleanup.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | January 2020</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
