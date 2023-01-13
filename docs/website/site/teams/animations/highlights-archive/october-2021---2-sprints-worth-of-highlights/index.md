---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: october-2021---2-sprints-worth-of-highlights
title: October 2021 - 2 sprint's worth of highlights
---

<table>
<tr>

<td>October 2021</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><table></td>
<td><tr></td>

<td><td>CompositeBGColorAnimation finch </td></td>

<td><td>xidachen@ launched finch on the stable channel for 1% of the population (detailed <a href="https://docs.google.com/document/d/1Fkp7udbCgYqVtNf4gn-NXGVYamPrd_n0qKqalruTq6E/edit#heading=h.u1s0gwal1np6">result</a>). Overall, we got very good results on throughput metrics, but slightly more memory consumption on Windows (~1.5%). The increase in memory is explained by how garbage collection is scheduled. Most importantly, there was no regression on the first or large contentful paint.</td></td>

<td><td>A crash was discovered owing to the fact that not all colors are handled the same way. System colors behave more like references. For example, the “Field” color refers to the default background color for an input field. The correct color is resolved later in the pipeline, and cannot presently be used with a composited background color animation. These animations are now deferred to the main thread (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3216109">link</a>).</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/HHMbn-fB8tWKPra3VxfAvEBGErp-QAtbBAmzeyoyRMtvaB_5Ga_Tlz0ncKLQR5LTZYE7OnyH8zUTpyWlL-aKxrPJvMZtuZjVLLkrSh9j6fGyvcDzjh_9eoOV93pzuO5yVGIdVX3WKg" height=83 width=283></td></td>

<td><td>With this final known issue fixed, we’re ready to review performance metrics for launch.</td></td>

<td><td>Scroll timeline</td></td>

<td><td>kevers@ landed a number of spec changes to <a href="https://drafts4.csswg.org/web-animations-2/">web-animations-2</a> in support of <a href="https://css-tricks.com/practical-use-cases-for-scroll-linked-animations-in-css-with-scroll-timelines/">scroll-linked animations</a>. These spec changes largely address issues with timing in the API. Previously all timing was recorded in milliseconds. With progress-based animations, times are reported as CSSNumericValue percentages. The spec changes also address some edge cases for scroll-linked animations that are in the paused state as well as the effect phase when at limits of the scroll range.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/JacSGMav-KYulzw-o2qSwjnhif87ENbCnnElh-BoGEiXTArgrBbnlOaqgMOTfL0OLaCLcx46xUG0WqIU2Q4OGu2WAcl5sX1Gt6VuLD5kosGyilUdvC-EkzaLpk750h0LRMhdOizI3g" height=80 width=251></td></td>
<td><td>Pull request links: <a href="https://github.com/w3c/csswg-drafts/pull/6656">PR6656</a>, <a href="https://github.com/w3c/csswg-drafts/pull/6655">PR6655</a>, <a href="https://github.com/w3c/csswg-drafts/pull/6508">PR6508</a>, <a href="https://github.com/w3c/csswg-drafts/pull/6479">PR6479</a>. <a href="https://github.com/w3c/csswg-drafts/pull/6702">PR6702</a>, <a href="https://github.com/w3c/csswg-drafts/pull/6712">PR6712</a>.</td></td>

<td><td>kevers@ fixed some flaky scroll animation tests (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3169158">link</a>). One source of flakes was caused by slight platform dependencies combined with floating point rounding errors. By tweaking the tests to not depend on scrollbar width and to have nice integer expected values, these tests no longer flake. The flakiness was discovered while working on a <a href="https://github.com/flackr/scroll-timeline">polyfill</a> implementation of scroll timelines. Another source of flakes was due to improper assumptions when making a style change before an animation frame (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3226725">link</a>).</td></td>

<td><td>kevers@ fixed 2 clusterfuzz failures. Both were caused by unexpected input: malformed scrollOffsets (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3244932">link</a>), and unsupported effect delays (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3245202">link</a>).</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>\[Scroll Unification\] Web test burn-down</td></td>

<td><td>Scroll unification is an important code health project to streamline the scrolling process. Currently, there are multiple code paths, which need to be considered when addressing a bug or updating scroll behavior. This cycle, skobes@ has focused on burning down the number of test failures.</td></td>

    <td><td>lock-renderer-for-middle-click-autoscroll.html  (<a
    href="http://crrev.com/925854">r925854</a>)</td></td>

    <td><td>scrollbar-double-click.html             (<a
    href="http://crrev.com/925896">r925896</a>)</td></td>
    <td><td>background-attachment-local-scrolling.htm</td></td>
    <td><td>plugin-overlay-scrollbar-mouse-capture.html</td></td>

    <td><td>reset-scroll-in-onscroll.html                   (<a
    href="http://crrev.com/927008">r927008</a>)</td></td>

<td></tr></td>
<td></table></td>

<td> This reduced the number of outstanding web test regressions by 16%.</td>

<td><table></td>
<td><tr></td>

<td><td>Responsive composited animations</td></td>

<td><td>kevers@ fixed the responsiveness of composited animations to changes in the animation environment (<a href="https://chromium-review.googlesource.com/c/chromium/src/+/3205715">link</a>). For main thread animations, ConversionCheckers detect changes that would affect interpolations of properties during an animation. These changes now trigger an invalidation of the compositor keyframe snapshots. Some further tweaks were required to defer updating the compositor snapshots until after the ConversionCheckers have run.</td></td>

<td><td><table></td></td>
<td><td><tr></td></td>

<td><td><td>@keyframes spin {</td></td></td>
<td><td><td> 0% { transform:</td></td></td>
<td><td><td> scaleX(var(--scale))</td></td></td>
<td><td><td> rotate(0deg); </td></td></td>
<td><td><td> }</td></td></td>
<td><td><td> 100% { transform:</td></td></td>
<td><td><td> scaleX(var(--scale))</td></td></td>
<td><td><td> rotate(180deg); </td></td></td>
<td><td><td> }</td></td></td>
<td><td><td>}</td></td></td>

<td><td><td><img alt="image" src="https://lh6.googleusercontent.com/kInX8dX9QmbDrsLHGKAq0YXiT0CHnAr5ts0ud3AGUmYFsZsGsnCN3-9YzX353drQBISBBdkLQi8KcHb528Fi6frBU9uP_PiO_McqXVB6qMeV0iZlc_suQ1_C6MzmWHCv_JsnSPr4sQ" height=181 width=123></td></td></td>

<td><td></tr></td></td>
<td><td></table></td></td>

<td><td>Infinite user activation by extensions (mustaq@)</td></td>

<td><td>Extension messaging API needs to trigger an artificial <a href="https://html.spec.whatwg.org/multipage/interaction.html#tracking-user-activation">user activation</a> in background scripts to allow access to user-activation-gated APIs like Permissions and popup. This trigger caused a challenging <a href="https://crbug.com/957553">P1 security bug</a> reported by a user 2+ years ago (internally we knew it even before that).</td></td>

<td><td>The problem here is that a malicious site or extension can craft a delayed message-reply sequence to effectively extend the lifespan of the original user activation, and even repeat the sequence indefinitely to secure a “forever-active” state. That means, a single user click in one webpage could allow infinite popups from any tabs!</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/l0vhMbMctf57poYsl-uh5sgv7seC1o8DYejptY9Br5oTMupHsqEDh9DC7D-pKwr4USj280259hc2LKLByAhs2ZKnuex3hx1Qy4O7-Ueq1slSvvpjtbxdQ-dXGlhJTR9ByR1FcQ54zQ" height=220 width=208></td></td>

<td><td>In addition to being a challenging problem by itself, the bug “worsened” two other security bugs (on leaking autofill <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=966562">966562</a> and bypassing sandbox restriction <a href="https://crbug.com/1035315">1035315</a>) in the last two years!</td></td>

<td><td>After a long brainstorming through unactionable whiteboard drawings, <a href="https://uma.googleplex.com/p/chrome/histograms?sid=c4e21a8aa954807b89df16f4d2d8e9be">misleading UMA</a> discovery and stalled <a href="https://docs.google.com/document/d/1TKjjwFlQGh2LLm0_mOW6FJdmmwyOBMj_fdWJyAJ_Q50/edit?usp=sharing">design discussion</a>, we were able to find and <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3154195">land</a> a fix in Q3, yayy!</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Fixed overscroll glow position (flackr@)</td></td>

<td><td>Overscroll glow on android <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1253617">could appear in the wrong position</a>.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/eoyR3w60Cqa8PVbrNBA_9-jTOKg4on6uc6MSmCn4lAxrDCNlyY1H4Z59BVPs_fEwg6m7vJMCmylEAq3CF6UVFuxvOQQH-_4Lb5zis14Ox1erYLJLiP0gUd0G3Jx_44DhKMIVdRwO8g" height=153 width=283></td></td>

<td><td>The scrollable viewport bounds didn’t include the current bounds delta from active touch dragging. Fixed <a href="https://chromium-review.googlesource.com/c/chromium/src/+/3188708">using ScrollTree::container_bounds</a>.</td></td>

<td><td>Smooth scroll vs. JS scroll (skobes@)</td></td>

<td><td>Achievements: Learned a lot about how to handle JS scrolls in the middle of a user-triggered smooth scroll (wheel, keyboard). Both the main thread and the compositor thread were handling this improperly in different ways. A <a href="http://crbug.com/1264266">new proposal</a> enables "adjustment" behavior in virtual scrollers.</td></td>

<td><td><img alt="easeinout.png" src="https://lh4.googleusercontent.com/yGrqbqHLvUia7IuId5eOlAJfQBx_nDnxaIoHd3zkYY81tyIbm70sr05msy4z1E1VKMayvR5eN_AK49mDMAfUfmlpLmHKlgqOCYiNpw9GoSwuUkq6hmFltMGkvFXk4ybC-MZphtoMMA" height=145 width=148></td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Bug status update</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/cvdKwm9SlfBYA9JJ7ZCu8HW2mH09w6K4DA8E0G-dSw8KEZyzgZv3n7BYQcJ-Aw1RRTxbI4NyFCmhlqQa7_IbwDMZFVCBjYTgPTYbTOAm3KWGwJSiRpIxKiRoeDdD8DTvQBHzSnnJ8Q" height=283 width=582></td></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/E4JvRCP-MkjTLdZviSF1JP0GoQKf6eOo-EcjNwffwl9vb2xC1vurMJlK5P_xKXDSm21Act6-psqYt5c-R75tUc52VFLpa3OMvI9F18-OVU1md4y51HlQ5AhVdqbF17xYGf7KNn4O8Q" height=283 width=582></td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | October 2021</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
