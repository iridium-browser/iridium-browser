---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: may-2021---new-features-testing-stability-fixes-interop-fixes-and-more
title: May 2021 - New features, Testing, Stability fixes, Interop fixes and more!
---

<table>
<tr>

<td>May 2021</td>

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

<td><td>Scroll Timeline Documentation</td></td>

<td><td>kevers@ started a <a href="https://docs.google.com/document/d/1AyhFFWKuevKfKQU9uRKMMVMbCKdAwuEPmSBBghbhLFE/edit">doc</a> for I2S and devrel. The doc provides skeleton examples that highlights the use cases of scroll timelines and demonstrate the API.</td></td>

    <td><td>Parallax scrolling (from polyfill demo)</td></td>

    <td><td>Animating elements as they transition into or out of the viewport
    (from CSSWG issue)</td></td>

    <td><td>Auto-resizing header (bram.us)</td></td>

    <td><td>Scrollable picture story, which makes use of paint
    worklets.</td></td>

<td><td>Populating Touch click/contextmenu pointer id and pointer type</td></td>

<td><td>liviutinta@: click/contextmenu event from touch is triggered from GestureManager and there was no way to map a GestureEvents sequence to the corresponding PointerEvents sequence in order to obtain the pointerId associated to the PointerEvents sequence. The solution was to populate the unique touch event id of the first gesture in the GestureEvents sequence as the primary unique touch event id for all the GestureEvents in the sequence. This primary unique touch event id is then mapped to the unique touch event id of the pointerdown pointer event. When handling pointerdown, PointerEventManager would notify GestureManager about pointerdown’s unique touch event id and its pointer id, and in turn GestureManager would keep track of the association until the corresponding GestureEvent (for click or contextmenu) would be handled in order to populate the pointerId of the triggered click/contextmenu event.</td></td>

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

<td><td>kevers@ deflakes a set of scroll-snap tests:</td></td>

    <td><td>touch-scrolling-on-root-scrollbar-thumb: The problem is scroll
    gesture before ready and assert right after gesture. The solution is to use
    waitForCompositorCommit and scroll promise.</td></td>

    <td><td>scrollend-event-fired-to-\*: The problems with these tests are
    scroll gesture before ready, no cleanup and logic errors in listeners. The
    fix is to use waitForCompositorCommit, add cleanup and use scroll
    promise.</td></td>

    <td><td>fractional-scroll-height-chaining: The problem here is scroll
    gesture before ready, non-deterministic mouse position and rAF based wait.
    The solution is to use waitForCompositorCommit, mouseClickOn and
    waitForScrollEnd.</td></td>

    <td><td>mousewheel-scroll: The problem is non-deterministic mouse position
    and rAF based wait. The solution is to use mouseMove and
    waitForScrollEnd.</td></td>

<td><td>Fix scroll-unification tests</td></td>

<td><td>kevers@ also fixes some tests with scroll-unification enabled.</td></td>

    <td><td>scrollbar-drag-thumb-with-large-content: the problem is scroll
    gesture before ready, and the solution is to use
    waitForCompositorCommit.</td></td>

    <td><td>scrollbar-added-during-drag: The problem is scroll gesture before
    ready and single rAF before asserts. The solution is to use
    waitForCompositorCommit and scroll promise.</td></td>

<td><td>Fix single-thread mode with scroll-unification</td></td>

<td><td>skobes@ fixed scheduling in single-thread mode, which is the default mode for web tests.</td></td>

    <td><td>SetNeedsAnimationInput wasn't actually scheduling the animation,
    which caused tests with scroll gestures and scroll animations to
    hang</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Stability fixes</td></td>

<td><td><table></td></td>
<td><td><tr></td></td>

<td><td><td>Fix numeric overflow in CompositorKeyframeModel</td></td></td>

<td><td><td>kevers@ fixes a numeric overflow problem. The root cause is that AnimationTimeDelta is backed by a double whereas TimeDelta backed by 64-bit signed int. Using unsafe conversion from AnimationTimeDelta to TimeDelta, which can result in a numeric overflow.</td></td></td>

<td><td><td>The solution is to Update CheckCanStartAnimationOnCompositor to reject compositing an animation if currentTime is too large. Advancing current time by a few hundred years is just fine though.</td></td></td>

<td><td><td>SVG smil animation in throughput metrics</td></td></td>

<td><td><td><img alt="image" src="https://lh6.googleusercontent.com/iZk5QFsOzBhO9kf5qQwxRDo_4OgpecDuod7BwMzWZQ5pwFiu9z9n1ef-O8W-7WhKOeHE2iT6tjGsidtKQ3XZ68D4TnTWojdQBZgIijpbIWMrQLuLninU8DQjaL7aoeDifRekLVeHgQ" height=158 width=276></td></td></td>

<td><td><td>Before</td></td></td>

<td><td><td><img alt="image" src="https://lh4.googleusercontent.com/gMEwHQQ97x0Jlsqwi66w_eF7AeU0yUjaSZrXPwrdZsYVYOdZ7nx2-98l2WxoWgAr1hJu5B4DCcz8-c7X2-kRJ7caeUbe1-GYGtJ0JTfAC-xOZOJMwskGygBK3VXSVV9kntF-O8lkSw" height=156 width=277></td></td></td>

<td><td><td>After</td></td></td>

<td><td><td>xidachen@ <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2863204">fixed</a> an issue where SVG smil animation is not counted as main thread animation in the throughput metrics.</td></td></td>

<td><td><td>The above demo shows that before the fix, there is no sample detected in Graphics.Smoothness.PercentDroppedFrames.MainThreadAnimations, while it is detected after the fix.</td></td></td>

<td><td></tr></td></td>
<td><td></table></td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: Interop fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>flackr@ <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2892623">fixed</a> the metrics for counting overscroll-behavior potential breakage.</td></td>

    <td><td>Before: we will hit a UMA recording whenever the overscroll-behavior
    property is different comparing the html and body element.</td></td>

    <td><td>After: we’d hit a UMA recording only if the body is the viewport
    defining element, and that the overscroll-behavior property is different
    comparing the html and body element.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter V: Bug Updates</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td><img alt="image" src="https://lh5.googleusercontent.com/MHKcz_eBTvaoEMvqtpGDxweELZwCGAEm7xdV4D89dz-1T_asGIKvFg2CL6O7ILhfZ_g6I4KFxpy-nes3cVhiFfAKGgxiQNsPF6lXQ8NOAGO9JjUrGSnuKigT25helY0xJ-PJSA3S-g" height=150 width=273> <img alt="image" src="https://lh6.googleusercontent.com/I7aNfDvFvhYarIPRGEYMVMSkFFNB8yPVjHStk4q9AeYvyrh6TolUXxSAAFZPsYrqHEtwBNvMRwk7a_sbFpg8ky_gGcaUn8jKxymnneXvxmi5UGPZCxSBh_ealFjDY-73bE5RXlb6mA" height=150 width=270></td></td>

<td><td>Our team closed fewer bugs this sprint, but we did get a <a href="https://youtu.be/m0JrspsaVDk?t=1779">chance</a> to <a href="https://youtu.be/VrEP7SPfQVM?list=PL9ioqAuyl6ULbse3njxmvJgJArp_-pKxY&t=3208">have</a> a <a href="https://youtu.be/VrEP7SPfQVM?list=PL9ioqAuyl6ULbse3njxmvJgJArp_-pKxY&t=3370">bunch</a> of <a href="https://youtu.be/m0JrspsaVDk?t=78">great</a> <a href="https://youtu.be/m0JrspsaVDk?t=201">conversations</a> at Blinkon... The rate of incoming bugs was relatively consistent.</td></td>

    <td><td>Xida Chen <a
    href="https://youtu.be/VrEP7SPfQVM?list=PL9ioqAuyl6ULbse3njxmvJgJArp_-pKxY&t=3208">Composite
    background-color animation</a></td></td>

    <td><td>Jordan Taylor: <a
    href="https://youtu.be/VrEP7SPfQVM?list=PL9ioqAuyl6ULbse3njxmvJgJArp_-pKxY&t=3370">Scroll
    Timelines</a></td></td>

    <td><td>Liviu Tinta: <a href="https://youtu.be/m0JrspsaVDk?t=78">Disable
    Double Tap to Zoom</a></td></td>

    <td><td>Rob Flack: <a href="https://youtu.be/m0JrspsaVDk?t=201">Scroll To
    \*</a></td></td>

    <td><td>Liviu Tinta: <a
    href="https://youtu.be/m0JrspsaVDk?t=1779">click/auxclick/contextmentu as
    PointerEvents</a></td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | May 2021</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
