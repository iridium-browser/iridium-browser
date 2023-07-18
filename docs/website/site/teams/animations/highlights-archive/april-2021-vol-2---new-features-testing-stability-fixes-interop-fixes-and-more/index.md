---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: april-2021-vol-2---new-features-testing-stability-fixes-interop-fixes-and-more
title: April 2021 (Vol. 2) - New features, Testing, Stability fixes, Interop fixes
  and more!
---

<table>
<tr>

<td>April 2021 (Vol. 2)</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>Chapter I: New features</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Composite background-color animation</td></td>

<td><td>xidachen@ resolved a few issues during this sprint.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/B_AGNBt9KFFBrKGrVF50Jlp1PqtXjwxYm2lsSP0YLcbaizx5xERcSuwz5phRvMf_HaqYCeTEAsmiZJb193hdCrcnfooYqfgwuVhtVi_--i51Q1mhIDMzaZR_iLU-lQcl9YX97BmB5g" height=92 width=283></td></td>

<td><td>The first one is completely decouple paint and compositing, which is shown in the above diagram. Specifically, we were passing a boolean from paint to the compositing stage and that could introduce technical debt in the future. Now we no longer require that boolean.</td></td>

<td><td>The second issue is to handle non-visible animations, which can happen in many cases such as an animation on a zero-sized element. There are a lot of discussions <a href="https://docs.google.com/document/d/1HtnP6oNFvcYIn91tHPhQR5n_8zhWLHfG_eXi4HG8Pzc/edit">here</a>, and we eventually decided that it is OK to composite these no-op background-color animations. The problem is fixed by this <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2795243">CL</a>. Fixing this issue also helped resolving an existing CSS paint worklet <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2841783">bug</a> which no-op animation.</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/Tl9i_d3F9PEiHRIPX4S_AirQlASHhy2-iRGCZMZTadEGsvNlC_58ko-6By-j7oFG4TAIM0gQ_BGkwaix3uWCaYK9iT7VOtRy1nDUNVBpAInsugo534oM0uLatt9K4iMN9KIgInQX0Q" height=64 width=283></td></td>

<td><td>The third issue is handling non-replace keyframes. As shown in the above code snippets, we should not look at the composite mode of the animation, but rather look at the composite mode of each keyframe. This is resolved by this <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2836907">CL</a>.</td></td>

<td><td>Capability Delegation moved to WICG</td></td>

<td><td>mustaq@ made awesome progress during this sprint, which includes</td></td>

    <td><td>Finalized <a
    href="https://docs.google.com/document/d/1VQiJBo_hBfgKfHN3lZnhHbs9ws74TCu5i-__y8_mdBU/edit?usp=sharing">Q2
    action plans</a> with Payments team (thanks to smcgruer@).</td></td>

    <td><td>Secured public approval from <a
    href="https://discourse.wicg.io/t/capability-delegation/4821/3">Stripe</a>,
    and thumbs-up from <a
    href="https://github.com/w3c/mediacapture-screen-share/issues/167#issuecomment-821290060">Mozilla</a>.</td></td>

    <td><td>Moved the proposal to <a
    href="https://github.com/WICG/capability-delegation">WICG/capability-delegation</a>.</td></td>

    <td><td>Updated/deprecated all related public threads/docs: HTML <a
    href="https://github.com/whatwg/html/pull/4369">#4369</a>, <a
    href="https://github.com/whatwg/html/issues/4364">#4364</a>. Crbugs: <a
    href="https://crbug.com/928838">928838</a>, <a
    href="https://crbug.com/925331">9253331</a>, <a
    href="https://crbug.com/931966">931966</a>. Docs and repos: <a
    href="https://github.com/mustaqahmed/user-activation-delegation">repository</a>,
    <a
    href="https://docs.google.com/document/d/1NKLJ2MBa9lA_FKRgD2ZIO7vIftOJ_YiXXMYfRMdlV-s/edit?usp=sharing">design
    doc</a>, <a
    href="https://github.com/mustaqahmed/autoplay-delegation/">follow-up attempt
    repo</a></td></td>

<td><td>Click as PointerEvent</td></td>

<td><td>liviutinta@ and mustaq@ collaborated on this work.</td></td>

    <td><td>Identified a regression in older <a
    href="https://bugs.chromium.org/p/chromium/issues/detail?id=1192449#c31">Esri
    software and APIs</a> which is widely used related to fractional coordinates
    for click.</td></td>

    <td><td>Disabled the <a
    href="https://critique-ng.corp.google.com/cl/369699904">Finch
    experiment</a>. Kept <a href="https://crbug.com/1192449">engaged</a> with
    developers until the problem is fully resolved.</td></td>

    <td><td>Reached the <a
    href="https://groups.google.com/a/chromium.org/g/blink-dev/c/bta50W_Hg24/m/YAYeAzCZAAAJ">final
    decision</a> about what to ship: click/auxclick/contextmenu as pointer
    events but with integer coordinates.</td></td>

    <td><td>Closed a 5-yr old debate in the <a
    href="https://github.com/w3c/pointerevents/issues/100">Pointerevents
    spec</a>.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Testing</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Scroll-snap: Deflake tests</td></td>

<td><td>kevers@ deflakes a few scroll-snap tests:</td></td>

    <td><td>The first set is snaps-after-scrollbar-scrolling-\*. The problem is
    rare TIMEOUTs. The solution is to accelerate animation timing. Fixes main
    thread testings only.</td></td>

    <td><td>The second is scrollend-event-fired-after-snap. The test fails due
    to incorrect event ordering. The solution is to reset to prevent scroll end
    during test reset from being triggered as an end before scroll is
    finished.</td></td>

<td><td>Mousewheel: Deflake tests</td></td>

<td><td>kevers@ also deflakes a few mousewheel tests. There are a few problems with the tests:</td></td>

    <td><td>Several mouse wheel tests are flaking.</td></td>

    <td><td>Focused on <a
    href="https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/web_tests/fast/scrolling/percentage-mousewheel-scroll.html;l=1;drc=e51dd5c377fd47393a171f6bdcd7c1a6a9a609c5?q=percentage-mousewheel-scroll&sq=&ss=chromium%2Fchromium%2Fsrc">percentage-mousewheel-scroll</a>
    which has highest scroll on flake dashboard.</td></td>

    <td><td>Scroll and wheel events getting dropped.</td></td>

    <td><td>Not differentiating between missed wheel event and incorrect
    result.</td></td>

    <td><td>Missing test cleanup.</td></td>

<td><td>The solutions to the above problems are:</td></td>

    <td><td>Set mouse position before triggering synthetic wheel
    event.</td></td>

    <td><td>Ensure that scroll event is received.</td></td>

    <td><td>Ensure wheel event is received.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Stability fixes</td></td>

<td><td>Fixed UAF due to promise resolution timing</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/K3v4CmTZiknQj2hNvwbhnT-A7daryFxJOBV_Oc8rF41LmZf6f_ngngIbjIySHjPtfJJVQKC3p57kQ8tba0WgoyckbH9iciVOGEvm-JRjFkCKVcY4eI4HcyIva3BYjvb1FVwejKJnKQ" height=207 width=262></td></td>

<td><td>flackr@ fixed a UAF problem. Specifically, we were synchronously resolving ready promise during <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1196781#c10">RunPaintLifecyclePhase</a>. Since promise resolution already <a href="https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/core/animation/animation.cc;l=2382;drc=c86618b300799fd70be83a72afbe7e15f124493d?q=Animation::CommitPendingPause">handles forbidden script context by posting a task</a> so the solution was to wrap UpdateAnimations call in ScriptForbiddenScope. We move ScriptForbiddenScope to entire lifecycle with explicit exceptions for locations where we expect/handle script execution.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: Interop fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Scroll-snap: Resnap to focused element after layout</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/pUAg3nUiQw_QiWhDPwX_-zf4Y5wK-iz41FcUKRHSaraaujjDbrsoftqCX0gQ2vFiWsOZofPDYeVbxSlhI6xkBagvhACUJsNTq57M2X8hsAK5ZBU_27reWwfoQPMIdNjsoYRNDC8bCg" height=127 width=283></td></td>

<td><td>The problem is that when snapping, multiple candidates can be equal distance from the origin of the snapport. If one of these candidates is focused, it should be snapped after a relayout.</td></td>

<td><td>The solution is to make whether the element is focused a tiebreaker when selecting a snap target.</td></td>

<td><td>Scroll-snap: mousewheel scrolls skipped snap positions</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/wcoZ25PYAKWFr9ldrG3JhCVq15c_0fEQwWt193N-MvDHsrMOY5Jm7nnzASz3h1sEXgnJ84gqbRAs0gpwR0IUKdu6XgqPBY6hBdQUyLUMcGYG22T75aeWqjbH2_Vyd_BIwbWNgmZcMQ" height=193 width=283>.</td></td>

<td><td>Here are the problems:</td></td>

    <td><td>Scroll snap called when a scroll animation ends and on a gesture
    scroll end.</td></td>

    <td><td>Gesture scroll end delayed in anticipation of additional mousewheel
    ticks.</td></td>

    <td><td>Results in two directional scrolls, skipping over the nearest snap
    position.</td></td>

<td><td>The solution is to snap only on animation end. The next steps are initiate snap at gesture scroll begin for mousewheel scrolls.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter V: Bug Updates</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/asi8pz5cPdAdbRaT-2QaBXhimLiPNYUwLMuUWIN4eWnVg0wwsGzgKLpsrcnQMKLu1Z02oiSTuUoFz7_XAzp83nOPSUzagwURcAHpuvFTRPnHSmTUa3-Yc_ipIPZr7Wbi2YZ4OvYyzg" height=157 width=283> <img alt="image" src="https://lh4.googleusercontent.com/OdAwuP64GqE3kURtMFPLaBSsitA3sy6MKWaGib6vkg2Tn-1NvkJkKDgHOaooynN4ENovUyFfP5Z0UzHrGEyy3jZYThYEyEefnpqt1peP0aCyB5IDsC2Xz3UmM-nXTZbCs4A3Mf8YlA" height=154 width=276></td></td>

<td><td>Our team had a sizable influx at the start of this sprint. Great efforts were made to stabilize the amount of bugs and even keep P1 bug number drop.</td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/_abXC_M3LqFDCPe31DLX4hv01AyN19LMYlV3UDx7fPGchqAutCc9tGLKuDaM51LBUKc6SBcVYiQRjJCaznQzw16sJD8xir4Mqyq1RuBfJtk05ganqrhVTSqdVv0QKdCTkkmSGUCwSQ" height=196 width=318> <img alt="image" src="https://lh3.googleusercontent.com/iYcG2KtoT0Mt7qB3T7P-PzUhH-A6HYmLG5gw28veSDEhvvxp_O0DQ18EeJwRXqB4mqaclC1PJ6WZW1T0YrMiU7AAlHLUzLbDRCA721-B2d54YoNlo1TlsNWx83zDgrDjSqODzTG8kA" height=198 width=234></td></td>

<td><td>skobes@ fixed a scroll unification bug (<a href="http://crbug.com/1155655">crbug.com/1155655</a>).</td></td>

<td><td>Symptom: can't scroll sub-scroller in iframe. Root cause: compositor thread hit testing had an early exit if it saw no scrolling layers, which did not check for slow-scroll regions. Extra challenges:</td></td>

    <td><td>Couldn't repro locally until realizing bug was OOPIF-correlated
    (hint: rainbow layer border)</td></td>

    <td><td>Slow-scrollers can contain or be inside non-scrolling layer-promoted
    elements. Therefore, we need to check slow-scroll region on EVERY layer that
    is hit, not just the one in front.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | April 2021 (Vol. 2)</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
