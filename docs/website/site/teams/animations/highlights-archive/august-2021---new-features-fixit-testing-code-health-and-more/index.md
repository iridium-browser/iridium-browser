---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: august-2021---new-features-fixit-testing-code-health-and-more
title: August 2021 - New features, Fixit, Testing, Code health and more!
---

<table>
<tr>

<td>August 2021</td>

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

<td><td>Scroll timeline spec</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/m5zIa5C1ZvdM09GFsx1cTUAA5aMCjRCMYDm5hT5ug7WGQqca86W_kSBc2bYuyFEc1y3cB7bDBg8RBBCX2RE_P--GbZUX0NxR4yG_d9tn0NryDCOmMYzutUnC6tpydCMaIfz06BHxxA" height=145 width=283></td></td>

<td><td>kevers@ changed the spec for the “setting the timeline of an animation” part.</td></td>

<td><td>Problem</td></td>

    <td><td>Scroll-timelines are now progress based and not time based. Two
    cases where we need to preserve the “progress”:</td></td>

        <td><td>Switching to a scroll timeline while paused</td></td>

        <td><td>Switching from a scroll timeline.</td></td>

<td><td>Solution</td></td>

    <td><td>Augment procedure to include calculation of previous
    progress</td></td>

    <td><td>Preserve position.</td></td>

<td><td>kevers@ is also changing the CSSNumberish current and start times. CSSNumberish is a double or CSSNumericValue, where CSSNumericValue has a value and a unit.</td></td>

    <td><td>The problem with current & start times remaining as doubles is that
    it requires inferring a different unit depending on the timeline. Animations
    associated with a scroll timeline are progress based and not time
    based.</td></td>

    <td><td>This would allow:</td></td>

        <td><td>scrollAnimation.currentTime = CSS.percent(30);</td></td>

        <td><td>timeAnimation.startTime =
        CSSNumericValue.parse(‘-30ms’)</td></td>

<td><td>Composite BG-color animation</td></td>

<td><td> <img alt="image" src="https://lh6.googleusercontent.com/YwLTJWcJyqJCA3Qp3Udo7jVtFHSpaW_DltCWoYfwHucQ5S6TsUlcI1s-OOCVq-EnGpuStDav3F26tpwZYB4lUXtkEa-_fJ6QTfX8nCmzr3t6-HuFSIDNMxVJQnjyI8FnosUWhwlRqg" height=124 width=283></td></td>

<td><td>xidachen@ launched the finch study for composite bgcolor animation. The preliminary <a href="https://uma.googleplex.com/p/chrome/variations?sid=9249d9466a749268e49631a32938b1bb">result</a> looks very positive.</td></td>

    <td><td>The above table shows the summary of the result with all platforms
    combined, canary + dev channels.</td></td>

    <td><td>Note that the blue ones show significant difference, the black ones
    are “not significant”.</td></td>

<td><td>Given the positive finch result, we will ship this in M94.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter II: Fixit</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

    <td><td>FixIt organizers are awarding skobes@ with a <a
    href="https://docs.google.com/presentation/d/1ahjM6k4TjGw6Pf_DGg1d7b30xROnav-7vV9ec-6CI1U/edit#slide=id.ge570983615_0_2467">High
    Achievement award</a> for fixing the <a
    href="https://crbug.com/43170">oldest bug</a>.</td></td>

    <td><td>We closed <a
    href="https://bugs.chromium.org/p/chromium/issues/list?sort=Pri%20-Stars&x=Status&y=Owner&cells=counts&q=label%3Achrome-fixit-2021%20owner%3Aflackr%2Cgirard%2Ckevers%2Cmustaq%2Cskobes%2Cxidachen%20status%3Afixed%2Cverified%2Cwontfix%2Cduplicate&can=1&colspec=ID%2BPri%2BStars%2BType%2BComponent%2BStatus%2BSummary%2BOwner%2BModified%2BOpened">15
    bugs</a> in total:</td></td>

        <td><td>Got rid of bugs with <a href="https://crbug.com/716694">52</a>,
        <a href="https://crbug.com/1148143">37</a>, <a
        href="https://crbug.com/43170">27</a> and <a
        href="https://crbug.com/61574">12</a> stars.</td></td>

        <td><td>Landed code to fix <a
        href="https://bugs.chromium.org/p/chromium/issues/list?sort=Pri%20-Stars&colspec=ID%20Pri%20Stars%20Type%20Component%20Status%20Summary%20Owner%20Modified%20Opened&x=Status&y=Owner&cells=counts&q=label%3Achrome-fixit-2021%20owner%3Aflackr%2Cgirard%2Ckevers%2Cmustaq%2Cskobes%2Cxidachen%20status%3Afixed%2Cverified&can=1">7</a>
        of bugs.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Testing</td></td>

<td><td><table></td></td>
<td><td><tr></td></td>

<td><td><td>Deflake scrolling tests</td></td></td>

<td><td><td>kevers@ fixed quite a few flaky scrolling tests.</td></td></td>

<td><td><td>mouse-autoscrolling-on-deleted-scrollbar:</td></td></td>

    <td><td><td>Timeout: fixed by deferring start until ready</td></td></td>

    <td><td><td>Position mismatch: fixed by waiting for scroll event before
    checking position</td></td></td>

<td><td><td>wheel-scroll-latching-on-scrollbar</td></td></td>

    <td><td><td>Timeout: fixed by deferring start until after
    onload</td></td></td>

    <td><td><td>Position mismatch: fixed by allowing for fractional
    offset.</td></td></td>

    <td><td><td>Note this was the top Blink&gt;Scroll flake!</td></td></td>

<td><td><td>mouse-scrolling-over-standard-scrollbar</td></td></td>

    <td><td><td>Position mismatch: fixed by using established way of determining
    scroll thumb position and waiting on scroll event.</td></td></td>

<td><td><td>Deflake an animation layout test</td></td></td>

<td><td><td><img alt="image" src="https://lh5.googleusercontent.com/QPnqYwNJLp7eAqU1gLSfLhImusPZX2D2kIs1E0j_IglvO4r7DOSfHCIVBfyA5mV3pNcH5O5i_WfhegxFsuuoaBqfZXUO0L7HmmXnbiD7tQm-omNhSlSx-47ZUCCQIASa-fHRGS-hCA" height=39 width=277></td></td></td>

<td><td><td>xidachen@ fixed a top Blink&gt;Animation flake. The root cause is that we do “A==B” when we compare two AnimationTimeDelta, and that the precision issue caused flakiness.</td></td></td>

<td><td><td>The fix is shown above, which is by introducing an epsilon when comparing two AnimationTimeDelta.</td></td></td>

<td><td></tr></td></td>
<td><td></table></td></td>

<td></tr></td>
<td></table></td>

<td>Chapter IV: Code Health</td>

<td><table></td>
<td><tr></td>

<td><td><table></td></td>
<td><td><tr></td></td>

<td><td><td>Remove use of DeprecatedAtOrEmptyValue in animations</td></td></td>

<td><td><td><img alt="image" src="https://lh4.googleusercontent.com/ui6bPPR-paFdFEVPGiMhTMZFVQbPfxyYsjpq3EElvbp7Z3EfAZZ8SawdFzxoyn5B25f-eEN4QumftjcjYPjtGQUrSZJcYgSq7hB-T42eTh1OjVkamB8HxpuP-x-Av1Uxj44uJ95sQA" height=21 width=277></td></td></td>

<td><td><td>kevers@ removed the usage of DeprecatedAtOrEmptyValue in the animations code base.</td></td></td>

    <td><td><td>Here is the <a
    href="https://docs.google.com/document/d/18JIiajErikZaBzCtZvl-wwAJShqZsg0jT9wVyPBhSdU/edit">design
    doc</a> for WTF::HashMap&lt;&gt;::at() refactor.</td></td></td>

<td><td><td>Cleanup of CompositorKeyframeModel constructors</td></td></td>

<td><td><td>kevers@ cleaned up the CompositorKeyframeModel constructors.</td></td></td>

<td><td><td>The issues are:</td></td></td>

    <td><td><td>3 public and 1 private constructor. All public versions end up
    calling the private constructor</td></td></td>

    <td><td><td>Unnecessary if-else construct</td></td></td>

    <td><td><td>Opportunity to improve efficiency with move-value
    semantics</td></td></td>

<td><td><td>Resolution:</td></td></td>

    <td><td><td>Single constructor that takes a KeyframeModel::PropertyId
    argument</td></td></td>

    <td><td><td>Add move constructor and move assignment operator to
    TargetPropertyId</td></td></td>

    <td><td><td><a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/3060658">Negative
    line count CL</a>.</td></td></td>

<td><td><td>Refactor Native PaintWorklet</td></td></td>

<td><td><td>xidachen@ refactor the native paintworklet code.</td></td></td>

    <td><td><td>Detailed design <a
    href="https://docs.google.com/document/d/12g1OLIxZk9ayLNbOI87ru_yoUUWdxcKewDLRR4tqzi8/edit#">doc</a>
    here.</td></td></td>

    <td><td><td>The refactor reduced a middle layer, and made the entire
    workflow simpler.</td></td></td>

    <td><td><td>Landed 3 CLs. (<a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/3016115">Part1</a>,
    <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/3044499">Part2</a>,
    <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/3067315">Part3</a>)</td></td></td>

<td><td><td>Magic behind move-value</td></td></td>

<td><td><td>kevers@ learned something about std::move.</td></td></td>

<td><td><td>Move constructor</td></td></td>

    <td><td><td>Foo::Foo(Foo&& other): other is a temporary object that may have
    its contents reset as a result of the move. Note the r-value ref cannot be
    const.</td></td></td>

<td><td><td>Move assignment:</td></td></td>

    <td><td><td>Foo::operator=(Foo&& other): same thing. Other is temporary and
    may be reset.</td></td></td>

<td><td><td>Foo foo = CreateExpensiveObject(...)</td></td></td>

<td><td><td>In this case, no std::move is required since RHS is already an r-value.</td></td></td>

<td><td><td>Foo expensive_foo_instance = TakeOwnership(std::move(expensive_foo_instance));</td></td></td>

<td><td><td>std::move is required to take advantage of move-value semantics since expensive_foo_instance is an l-value. Adding std::move converts to an R-value reference.</td></td></td>

<td><td><td>Useful instead of const & when not able to share an instance but can pass ownership. <a href="http://thbecker.net/articles/rvalue_references/section_01.html">Further reading</a>. See also <a href="https://docs.google.com/spreadsheets/d/1U8byWhb9_vGWVzYtnh8UoH0Xv0vpEyfumUnh2GpOruw/edit#gid=0">pkastings C++ 201 talks</a>.</td></td></td>

<td><td></tr></td></td>
<td><td></table></td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter V: Stability/security fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Fake user activation from Extension Messaging</td></td>

<td><td>Interactions team and Extension team together made a concrete plan about an old P1 security issue with fake user activation (<a href="https://crbug.com/957553">Issue 957553</a>).</td></td>

<td><td>Here is a brief history to see why this is important:</td></td>

    <td><td>The problem was known 3+ years ago, and we got the security bug 2+
    years ago.</td></td>

    <td><td>We posted solution ideas but compat risks held us back. Our <a
    href="https://docs.google.com/document/d/1TKjjwFlQGh2LLm0_mOW6FJdmmwyOBMj_fdWJyAJ_Q50/edit?usp=sharing">design
    doc</a> in early 2020 didn’t get enough traction for the same
    reason.</td></td>

    <td><td>We added UMA in late 2020 to slice the problem but got <a
    href="https://docs.google.com/presentation/d/1xyyeMLNFFPNlMkulSb_nVvmpOZJ6OpYyE1TNsZvFyZQ/edit#slide=id.ga4b082cc16_1_0">confusing
    results</a>.</td></td>

    <td><td>We committed to look again in Q3 this year, and coincidentally got
    an <a href="http://crbug.com/1233544">escalation</a> from fbeaufort@ and the
    Media team just in time!</td></td>

    <td><td>Brainstorming in early Q3 by mustaq@, flackr@ and rdcronin@ led us
    to a solid plan, finally!</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter VI: à la carte</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>\[Scroll Unification\] Scheduling investigation</td></td>

<td><td>skobes@ investigated on the scheduling improvements for scroll unification.</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/KzoJ1wz9JNBOE4niu0yJi1m8kRTEnT7NbNJXLJnJIaZuOrjCTmHd15yxkvHBf8AVDg-S2shF3__QGBN-4Gp0N5DUot3_M3VXppGLPGpZjQ0AF911GQJZ89ZybDi3yNZWb5cgD4NZqA" height=119 width=283></td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/nE2WWtZkK69TinInED4oHRiMojIbDnoQucrmY9F3qRVTfZLC9XIg4z78xCQDfyQQlKv1z-gP6vnodL4jXpwNRARabUEeQNkRXKbrenRd4mZ3Exdy8ttXEyn6UBNcPBR4_510lAWVow" height=112 width=283></td></td>

<td><td>Less motion, plz!</td></td>

<td><td>Remove animations option in settings disables a lot of system animations including stretch overscroll.</td></td>

<td><td>In Chrome, remove animations sets prefers-reduce-motion:</td></td>

    <td><td>This <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/3046841">CL</a>
    plumbed prefers-reduced-motion from blink to compositor and wired it up to
    input_proxy_client.cc to disable elastic overscroll (on Android
    only)</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter VII: Bug Updates</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/yx34TxFllV0ep-DPg1EYjWu52FJDaFuXyFk5bjznHkSCdSVfVWputqfO3rJkZau0pzpeiexbNcpgd3QbpqNG-i-Mr9TSpOKJEv8uJk8UwvgRUMSMqdqqr3JFxcvPLzNYYzgMY42a-g" height=156 width=287> <img alt="image" src="https://lh6.googleusercontent.com/7asOshHrb-Mi9gCdsGobedpUlzBQgvlWAsLj4BSr8VDN6tZ9FuMrH4Zy2eL2vmH-IcUMXGDTYf8py1pFocLtCPnul_Ljgg-HWToEjP5V-a9VUrWKmbHm9T94Z8hrtsF7fl1yRaRuuA" height=157 width=285></td></td>

<td><td>Our team lost a bit of ground in P2s && P3s, but kept the P1s in check.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | August 2021</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
