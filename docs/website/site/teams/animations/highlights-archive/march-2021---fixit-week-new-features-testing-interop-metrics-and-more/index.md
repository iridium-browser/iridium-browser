---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: march-2021---fixit-week-new-features-testing-interop-metrics-and-more
title: March 2021 - Fixit week, New features, Testing, Interop, Metrics and more!
---

<table>
<tr>

<td>March 2021</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Chapter I: Updates</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>News and headlines</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/8Rx9baQSC1D5OKFFterOr8Ep4YoxqFThALN73pUPewvq4fUaFUVlOksg2JThWYNqD2QU0BRVezKD9fDNXj0MxRN_zt-N7G4vHYsyjkqd35ob51icSH-87sJV-20hBNTPOAPYrrD2w0Ggrg7eVUtIUmR1reIvSKYemlzjfdfzvl4nysCh" height=140 width=139><img alt="image" src="https://lh4.googleusercontent.com/M_WQs-eMjr2z7Vo1B5MnwO02b2XhJRNzBPsaLddW8yBxbwu7AFSBm-tTfR-T1MBMMsT7gy6SQX95J8YEB_IMV3TJobogfhmBZpmgZS19VQ4UwO1Kr2y0pS162qw9jLAiuMnRdNwqV-8OQfbaXlL5jaue5_GBlUhvfyox1n9X-FQUF3Xg" height=153 width=124></td></td>

<td><td>The above pictures show:</td></td>

    <td><td>Una’s Chrome blog post: “<a
    href="https://developer.chrome.com/blog/hardware-accelerated-animations/">Updates
    in hardware-accelerated animation capabilities</a>”</td></td>

    <td><td><a
    href="https://twitter.com/ChromiumDev/status/1366422289441566736">Tweet</a>
    on Scroll-To...</td></td>

<td><td>Bug status update</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/baJ0Vl-hujPKzdTypC9eeqvk7ifk30XYovJ4uKFZkJakYRVVlj6OKrwwwWKebHate9YPgSail2_yQQla7zP0lkNl0f9uAALZYQGJpDtD0C0iu6rKr6Dd7wsbnIFj8c1VzybSQThgKefFBNFXRcXStkEohFZvUeCdMipFfADVUw7bT97y" height=77 width=140><img alt="image" src="https://lh5.googleusercontent.com/0nTwbFbyafdWtsV8h2di3l5lOqebkNcmCG6OfwwQi6SLLEsSA3dJCt-l3D_KqAn6cUbawHVyx77DHwvlpuzr7MGfv6ZsSiUR41JUbYVS9R5VRzaetVQ3JatAn-jgUfRRzYk91bQYptKWiD6_CC62pzqT-oF2VEvhy4Nmnq3XuIRMQrpy" height=76 width=140></td></td>

<td><td>Our team did an awesome work on bug fixing this sprint...and during perf too!</td></td>

<td><td>This is likely due to our fixit week effort which is proudly introduced in the next chapter...</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Fixit Week!</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td><a href="http://go/interactions-fixit-2021q1">Overall stats</a></td></td>

<td><td>Our team had a fixit week during Mar 1~5, and the team made tremendous progress!</td></td>

<td><td>Bugs we closed: <a href="https://bugs.chromium.org/u/2341872843/hotlists/Interactions-FixIt-2021Q1?can=1">40</a> including:</td></td>

    <td><td>P1s fixed: 4</td></td>

    <td><td>P2s fixed: 11</td></td>

    <td><td>P3s fixed: 7</td></td>

    <td><td>Others (WAIs, dups): 18</td></td>

<td><td>Bugs re-triaged: <a href="https://bugs.chromium.org/u/flackr@chromium.org/hotlists/Interactions-2021-Retriage?can=1">19</a> including:</td></td>

    <td><td>Fixed: 2</td></td>

    <td><td>Closed: 6</td></td>

    <td><td>Assigned to another team: 1</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>De-flake scroll snap tests</td></td>

<td><td>kevers@ focused on deflaking scroll snap tests during fix it week. Here are some common causes of flakes:</td></td>

    <td><td>Files containing too many subtests triggering timeouts. We cannot
    always accelerate the scroll timing. The tests have a 6s budget and can take
    3x longer than expected when machines are under load.</td></td>

    <td><td>Incorrectly detecting the end of an animation. Waits on a gesture or
    key event resolve when queued and not when handled. This can result on the
    scroll end being detected before it has even started! To address, we now
    wait for scroll events.</td></td>

<td><td>Outcome:</td></td>

    <td><td>Landed 7 CLs</td></td>

    <td><td>Removed 14 lines from test expectations.</td></td>

    <td><td>Fixed 10 test files</td></td>

    <td><td>Closed 8 bugs</td></td>

<td><td>Fix pointer events flaky tests</td></td>

<td><td>liviutinta@’s focused on pointer events tests marked in TestExpectations file:</td></td>

    <td><td>Using sendkeys instead of keyDown, keyUp</td></td>

        <td><td>Landed 1 <a
        href="https://chromium-review.googlesource.com/c/chromium/src/+/2737916">CL</a>
        that fixed 4 tests.</td></td>

    <td><td>Marked in TestExpectations:</td></td>

        <td><td>For <a href="http://crbug.com/893480">crbug.com/893480</a> the
        remaining 8 tests in Test Expectations need implementation keyUp/keyDown
        as it allows for multiple keys pressed at once</td></td>

        <td><td>For <a href="http://crbug.com/1140611">crbug.com/1140611</a>
        there’s “Element click intercepted error” thrown from test driver (2
        tests)</td></td>

    <td><td>Few web tests flaky because pointermove coalescing was not taken
    into account (1 WIP <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2745276">CL</a>,
    2 tests, 5 related issues)</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Fixing clusterfuzz crashes</td></td>

<td><td>xidachen@ worked on fixing clusterfuzz crashes:</td></td>

    <td><td>They usually have a minimal test case, easy to diagnose</td></td>

    <td><td>These are crashes can happen in the real world</td></td>

    <td><td>Often times it is due to our coding missing edge cases</td></td>

<td><td>Outcome: Landed 2 CLs and fixed 2 bugs. In additional to that, xidachen@ also closed a few touch-action related bugs.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: New features</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Animations in display locked subtrees</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/gXn-G235ngNBZqnmVTdwJK5JsmZSu8eeCwhoRJs4TZQoAwU-F2PgGaOZb_uU8zV18Nifvs8SAWH96eny1NaJPWzhoAl3L2SjJ6kXcV9ZHr-OPGNsroy4-2v1UzenxwYZfrAQ0wBI9GGj2VxR-3oiab8F5vtaM0hd0Psp7ll7fWD0RcbW" height=239 width=283></td></td>

<td><td>kevers@ is working on this performance optimization, which effectively “pause” CSS animations/transitions that are not rendered due to content-visibility, while direct queries must still produce correct results.</td></td>

<td><td>The above demo shows that animation events stop firing while content is hidden, which is a direct consequence of not updating hidden animations due to the normal passage of time. Calculations requiring a fresh style update are correctly resolved on demand.</td></td>

<td><td>Composite background-color animation</td></td>

<td><td>xidachen@ fixed a crash which is due to missing repaint. He is also working on two known problems in order to restart finch experiment.</td></td>

<td><td>Scroll to element</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/py8m0A7cqlby60QF-ayhnnAx0D5qNgt5tXjcZch6XaSmea-TKeG4tEscN66ShBCpdiacNSN-mK93HZ0kJEvC4EWD-gPBSd49UjBWh4QkwPkYzK6pQcGXtCZHzNK5ZlvtSGeismWyBhfb4Iy3CDNG3IEk0TFVyqBEGOv6ApprIstxIxp_" height=159 width=283></td></td>

<td><td>flackr@ developed a <a href="https://chrome.google.com/webstore/detail/scroll-to/hjaaolhckkhdkamciipnogbbiafgbcil">scroll-to extension</a></td></td>

    <td><td><a href="https://github.com/flackr/scroll-to-extension">Source</a>
    code is available</td></td>

    <td><td>Scrolls to selected element</td></td>

    <td><td>Rejects <a
    href="https://github.com/WICG/scroll-to-text-fragment/blob/main/EXTENSIONS.md#css-selector-restrictions">restricted
    CSS selectors</a></td></td>

<td></tr></td>
<td><tr></td>

<td><td>Google Meet FPS optimization</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/lSNmrAN5LzgMhOS2CppDDmbPza5nWwaPmqI0fgEBEblh7q2FbNV5iij-Kt_UkKQp2qOjt3xqEl5NH_rKGePmxJdxYC7jLFxxRZAqfZ2zmK6QxLjhYyHnpQ8QMSIE__2Ywnn1emJB_RXDBZeEHdw82iS2SCogBXfgZH2GR-qWcE9Ep2Qq" height=405 width=283></td></td>

<td><td>zmo@ plumbed minimum tick interval of animations, which requires</td></td>

    <td><td><a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2716656">Calculate
    minimum tick interval</a></td></td>

    <td><td><a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2730045">Used
    in tick rate heuristic</a></td></td>

<td><td>After the above change, flackr@ found that meet still wasn’t showing 60fps. More investigation shows that meet had incorrect animation. Specifically 0.6s 3 keyframe animation with steps(18) should be 30fps, but 18 animation-timing-function steps is applied between each pair of keyframes. flackr@ is currently working with meet team to further optimize this.</td></td>

<td><td>Penetrating context menu image selection</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/3WSJTjHaw94sUraj0T8JhdTWDJwtGtJpEijrluAiFSX1fsZCK4RwhgbVYnTxO5sxJu4oRogVdOo-Q9JDnQoM9Zt_taRYYXV2YBK-WUUcDIDiSbmszSq5EY3AGY4r2-zHhqx5VbT3Z8YwinxWir0Rofi5oDy1QUmsdKYe37T2EsI2yEY5" height=215 width=283></td></td>

<td><td>The feature is implemented by benwgold@ where flackr@ is an active reviewer. Specifically, this feature allows right click / long press to find images below targeted element.</td></td>

<td><td>In this <a href="https://output.jsbin.com/rucoyak">demo</a>, an image label element completely covers the image. Without penetrating image selection, one could not save the image without inspecting the page.</td></td>

<td><td>Disable double tap to zoom</td></td>

<td><td>liviutinta@ landed <a href="https://chromium-review.googlesource.com/q/bug:1108987">CLs</a> disabling DTZ for meta viewport tags such as:</td></td>

    <td><td>&lt;meta name=”viewport” content=”device-width”&gt;</td></td>

    <td><td>&lt;meta name=”viewport” content=”initial-scale=1”&gt; , for any
    initial-scale &gt;=1 we’ll disable DTZ</td></td>

    <td><td>&lt;meta name=”viewport” content=”minimum-scale=1”&gt;,
    minimum-scale &gt;=1 implies initial-scale &gt;= 1</td></td>

    <td><td>Combinations of the above</td></td>

<td><td>The <a href="https://groups.google.com/a/chromium.org/g/blink-dev/c/dXztlK096rs/m/6DKc6nhcCQAJ">I2S</a> API owners approved, and we have started finch experiment for 50% canary.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: Testing</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Move virtual scroll-snap tests</td></td>

<td><td>The problem with virtual/threaded is that it doesn’t guarantee that scroll snap tests run on the compositor. Thus kevers@ moved the tests to threaded-prefer-compositing, which ensures that the composited path is being used. This change exposed that we had limited testing of scroll snap with composited scrolling and temporarily introduced 7 new entries in the TestExpectations file. These failures have now been addressed.</td></td>

<td><td>Automate WPT test using testdriver action API</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/y5VfiIiUaDOm-G9ldk2gNytVwyx9DgTmvVSZZKzqvdkUBQ5JEV5z40t9YfledkhNAgqYpbskH9vTP5VCnryyOJMIRzz0ak45JZmMW29ZedQQGpVYzR7cg8NHxYQfHtJM3J6-C4o9ePXJBXFBszOwpeUxUfvoJh6mH49V_uQwhdBM5Jpe" height=185 width=283></td></td>

<td><td>lanwei@ kept working on automate WPT test and making good progress.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter V: Stability fixes</td></td>

<td><td>kevers@ fixed a problem when running user code (Javascript) that can result in the destruction of an execution context. The solution is to double check if the context is used downstream of script execution.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter VI: Interop fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Sticky interop investigation</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/xCplj7tBIokIzfY3bN1pO4H07CEWAZq9qG8C9p8DmlfYfor-6CGO4h9OdyDZsR03gCf5esw4S1xIsIvYiyLRBcxvS1C7k6zIDFvS_111zX_QJjBHotjN4mBbY_Gj7VMV51LgF9b9OkIw1SU2Tld6sb6IpufwOub1V2YeJK-ShNRd0XPB" height=167 width=283></td></td>

<td><td>flackr@ created <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1185163">meta tracking bug</a> for this. During this sprint, <a href="https://bugs.chromium.org/p/chromium/issues/list?q=commentby%3Aflackr%40chromium.org%20sticky%20closed%3E2021-2-22%20closed%3C2021-3-12&can=1">3 issues</a> were closed. Moreover, a <a href="https://wpt.fyi/results/css/css-position/sticky/position-sticky-scrollIntoView.html?label=experimental&label=master&aligned">WPT test</a> has been landed to track new spec scrollIntoView behavior.</td></td>

<td><td>Fixed falsely overconstrained stick positioning</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/xtsXFqA5IqyV2xz6JWUDk25Ee5dkse6mUKh_5FTcPYpISbkPWErpJR5k3fMnUMI9qnyzgLJSziuxiFSOPImhoGdqX-fZ4NWry2y1Jivg6v_KZ5E75YtqmMTl-vw8OVl7xAa8VJmY7dVCm6VwrsrmLkbG3vK_A-VH1453sQTMChlYha0U" height=137 width=137><img alt="image" src="https://lh4.googleusercontent.com/BG8InXEt7-3mnsMMdOjt8vPJ_Ff8aexfkJ2xiBSV3JryUb501rzf6JUkU2ELO8H0bUHzSU5zYg_gS8FeLzm0DQsjWUr_WlMZKvOJtfhK4O3KmArm4vROemwz6axHO3l66GcIXtbzKtAA_WHOJZYEPPvUQE-BcoIlh7yun3UZUoXyrMbj" height=136 width=136></td></td>

<td><td>flackr@ fixed a stick position <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=841432">bug</a>. In the above demo:</td></td>

    <td><td>The left one is the wrong behavior where the bottom constraint is
    ignored.</td></td>

    <td><td>The right one shows the correct behavior. Both bottom and top
    constraints are used creating reveal and collapse effect.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Overscroll-behavior propagation</td></td>

<td><td>xidachen@ made some progress towards fixing this <a href="http://crbug.com/954423">bug</a>, which includes</td></td>

    <td><td>Found ~15 google internal sites that might break by the incoming
    fix</td></td>

    <td><td>Started this <a
    href="https://docs.google.com/document/d/1PGBW3PDWTenS8i6-k6Xsv1_MuKxqEfX8o1jVGmxxnFk/edit#">doc</a>,
    which provided guidance on how to change their sites to avoid
    breakage.</td></td>

    <td><td>All google internal sites listed in the doc have made
    changes.</td></td>

    <td><td>Unfortunately the percentage of sites that might break is not going
    down (<a
    href="https://uma.googleplex.com/p/chrome/timeline_v2/?sid=29901cc34c7b4b624f5a759bda0b5a34">Beta</a>:
    ~2.2%, <a
    href="https://uma.googleplex.com/p/chrome/timeline_v2/?sid=706cc2310b84fa2bdb3e87b52b1c052b">Stable</a>:
    ~0.5% of page loads)</td></td>

<td><td>Scroll snap behavior with scrollbar arrow keys</td></td>

<td><td>Resolved the scroll snap behavior for interactions with the scrollbar arrow keys during the last sprint cycle for main thread scrolling. Fixed for composited scrolling this sprint.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/VJhePxtBeo6Irdt8t9FxdTl8hvkvHcAAGVyxvmtUeehNqlThCATmCB3WWuLXGGfOP-7rg-4uihVZN1VoosN0Tpbq8Iekx1J55WTrQqZp8J-TeZxDq0U6-Iqo2PcM8zT-EudBCQpYRkjXdqlv78FjI_GilXNHHyy1Iu2yen5ZOhsgoywR" height=97 width=283></td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter VII: Metrics</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Finch study: Composite relative keyframes</td></td>

<td><td>kevers@ launched this finch study and collected some meaningful results.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/uyULOys9B0l5Qkk1G8ZDUoqr-ps8pQ1ZpPMkJO4T0S9ipY6aLjCX6IMcuCfceaxBS98cfu3o-Q6OBUUZ0A_owKwozS_ctf0gbXEtUoi3Xru7ghDxCOE4uTYu26aFmclzqbnoh7aLZNAjc-q-kObkIGNXqhTKzRyUyVA1WctLKb8ZPQB0" height=123 width=283></td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/xoghGSmjwTGotF0VxXGa_XGkMHaKNQvIOvSx4CRqVpN900oCWoCYZwmMxbaz7IptN0gLl00CrW0XRgwSbhuOB3PZEjC4n1eDX3OqRybkLOhor4IUKXrFVqsTTyJNjMJmtfPHjIlYGAEtJdgIdluiKurKJJV6DahQdzEaNNQdvAwR1rgl" height=117 width=283></td></td>

<td><td>Finch study: <a href="https://uma.googleplex.com/p/chrome/variations?sid=950bc8883d9b9c8ce6469b5592a5e81a">Scroll prediction</a></td></td>

<td><td>flackr@ is launching this finch study on behave of a Microsoft engineer.</td></td>

<td><td>VisualJitter</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/mT74G-gB6SrpkOE9Psf3G700HUcy0Shav9Q-Z1dFcxJ8XTthGh8iB4p1MWRrcM3KVVG6zm_mEKBR4vMPej_vqBcnp_Qb4RDw9TkRYxLrdNi2yFbva-qai4RJWpwLc9KqVwjee-7UBB06Z7VmNYVo01pASlLvSHqEOKkXkW6feRDIAbtD" height=113 width=283></td></td>

<td><td>AverageLagPresentation</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/lcCu72bAUBMq3KtAO5_46zwl8GDtC5R9RjnOypXBrRzH-c5iQWzrSbnCnBC9EL-rM4JT02kNe_cCzWOlAHTPiJ2CIM4sbq0XTCzwanEt5PjQjXTndj89sdexYXYLd1I4-EwXg6UVbzlLyD3CO-ksuVCDLhfLvCU7QSodgt4ZSm4O18BV" height=113 width=283></td></td>

<td><td>Legend (f=# frames): -5ms 0.375f 0.5f 0.625f 1.217ms 3.3ms 5.383ms</td></td>

<td><td>The data of “VisualJitter” suggests that more prediction means more jitter. The data of “AverageLagPresentation” suggests more prediction increases lag.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Smoothness Metric</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/MKJsf1GYBhT0doAMEpwkenRs76RjwpmXg7qR50gqFH7ntVCI6OUJ3qLoVl_oLfpZ-kl9nN3gMprxubQ4GWQHSvfg_wCAAZ-l8Vfb_Np9HcdrEXuKa-WhcDmyxEy1Gm71f-gGyIvZV6xOFfB1jSrHXeHRxQX8fhlpGN6miqwvOrAqw2fc" height=143 width=283></td></td>

<td><td>lanwei@ explored in the smoothness metric field and did some data analysis.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter VIII: à la carte</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>PointerEvent spec reviews</td></td>

<td><td>mustaq@ has almost done reviewing two significant changes from the external spec editor:</td></td>

    <td><td><a
    href="https://github.com/w3c/pointerevents/pull/349">Reword/expand
    touch-action definition</a></td></td>

        <td><td>Added direct suggestions to clarify that “touch-action:
        manipulation” excludes double-tap zoom.</td></td>

    <td><td><a href="https://github.com/w3c/pointerevents/pull/350">Major
    refactoring: refer to “direct manipulation” rather than
    “touch”</a></td></td>

        <td><td>A long and tricky edit to officially include (some) pen pointers
        in touch-action.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | March 2021</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
