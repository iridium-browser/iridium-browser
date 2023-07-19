---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: june-2020---web-animations-scroll-linked-animations-throughput-metrics-scroll-unification-webdriver-pointer-events-and-more
title: June 2020 - Web Animations, Scroll-linked Animations, Throughput Metrics, Scroll
  unification, WebDriver, Pointer Events and more!
---

<table>
<tr>

<td>June 2020</td>

<td>Chrome Interactions Highlights</td>

<td>Archives: <a href="http://go/animations-team-highlights">go/interactions-team-highlights</a></td>

</tr>
</table>

<table>
<tr>

<td><img alt="image" src="https://lh5.googleusercontent.com/qa4CnhPdM0cesetEwVq7R6MRWTpi_RtZkOAI2uteVUXnr4cSA_eUp7CW5wrayRElFCzMiko7wLECykKFllzcU0WWIcilwBWTyFro4uRcHxcVd3AMpHQfv4_FYkZDFrCz4fGH0o9fLw" height=343 width=406></td>

<td>Fix transition retargeting</td>

<td>The CSS transition property is used to create a smooth transition of a CSS property on a style change. If the property is currently being animated (via an existing transition or other animations), this needs to be accounted for when calculating the starting point of the transition to avoid abrupt jumps in the value of the transitioned property. This process is called transition retargeting. We currently set the starting point based on the computed value from the last frame.</td>

<td>Our team members (kevers@ and gtsteel@) have made significant progress on this issue. Per spec, we need a base computed style (without animations) and then apply active animations on top of the base style to get the before style change for retargeting. Fortunately, the style resolver already tracks the base computed style (conditionally) in order to accelerate style recalculations that are driven purely by animation ticks.</td>

    <td><a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2185509">Step
    1</a>: Add test cases for reversing modified transitions to WPT.</td>

    <td><a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2252642">Step
    2</a>: Always cache the base computed style in the style resolver, tracking
    when it can be used for style recalc. Highlights:</td>

        <td>Fewer flags to track</td>

        <td>Fix storing the cached style on the first frame of an
        animation.</td>

        <td>Reusable for transition retargeting</td>

    <td><a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2220263">Step
    3</a>: Compute the “before change” style by playing active animations on top
    of the base computed style.</td>

<td><table></td>
<td><tr></td>

<td><td colspan=2>A better fps meter</td></td>

<td><td colspan=2>xidachen@ has been working on measuring renderer’s smoothness and developed <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2244002">a better fps meter</a> that reflects smoothness. This new-looking fps meter is available on canary.</td></td>

<td><td colspan=2><img alt="image" src="https://lh4.googleusercontent.com/7X6mpuYlg8ai4-U3yphi45I7mEFYKW74K27X_2ekbw-_TXxaY51pIQ9elyjSu2tIzFlTmo8KFdIXrv56tTq_XS1US1Dl7_SUMK9U3PYFxA8X43nsq5vdmMLq4Xshjxd4rAqJ0OleEw" height=439 width=580></td></td>

<td><td colspan=2>Improved throughput UKM</td></td>

<td><td colspan=2>In the last sprint, we <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2112953">changed</a> the UKM reporting logic such that we report the median throughput of a page, which better reflects users’ browsing experience. Following graph shows the UKM curve where the red arrow points to the date when our change is landed.</td></td>

<td><td colspan=2><img alt="image" src="https://docs.google.com/drawings/u/0/d/sy-viUUomGinfT8KjKYZiDQ/image?w=270&h=335&rev=15&ac=1&parent=14Nvi8kAwj_lSQYSf2z6Ywhvz7TD4B8S9Z-LViAhB5LI" height=335 width=270></td></td>

<td><td colspan=2>Scroll-linked animations</td></td>

<td><td colspan=2>flackr@ made a lot of progress and discussions on <a href="https://github.com/w3c/csswg-drafts/pull/4890">progress-based animations</a>, particularly focused on procedure for converting time based animations. With this, developers won’t need to specify an arbitrary time duration for the animation:</td></td>

<td><td colspan=2><img alt="image" src="https://lh5.googleusercontent.com/pexwnQ-nbMSOhhDhEjfSJKinMbjJxbW3lLZWHk2DN4dl5vAJMEyZriNbYgFjAZ4lEMFXXZrXdTmHO6nkTYLwtn9aqws7V_yKfxYllK500DtZCB6kQJUVyK-7IpJ9RKP-xEhL8TspMw" height=124 width=408></td></td>

<td><td colspan=2>Scroll unification</td></td>

<td><td colspan=2>lanwei@ <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2171955">replaced</a> eventsender with gpuBenchmarking.smoothScrollByXY (<a href="https://docs.google.com/spreadsheets/d/17NdL_750nAk51FN-dUprUNpBsp0_QCZZwKFJGENMXPE/">test list</a> 12/62 remaining)</td></td>

    <td><td colspan=2>Unblocks scroll unification, because after we finish the
    scroll unification, the scrolls happens mainly on the compositor thread, and
    the scroll code in the main thread will be</td></td>

<td><td colspan=2>removed. eventSender sends the scroll events to the main thread, so it would not work after the scroll unification. (<a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1047176">Issue</a>)</td></td>

    <td><td colspan=2>Tests full event delivery path</td></td>

<td></tr></td>
<td></table></td>

<td>WebDriver Actions Spec</td>

<td>lanwei@ is adding wheel source type to the spec and associated WebDriver tests. In particular, the spec change pull request is <a href="https://github.com/w3c/webdriver/pull/1522">here</a> and <a href="https://docs.google.com/document/d/1DdoNXbGspv4H5rmeCTgoiNPN5JGCitYbzVFd8FjSJiU/">design doc</a>.</td>

<td><table></td>
<td><tr></td>

<td><td>{"actions": \[</td></td>

<td><td> {"type": "wheel",</td></td>

<td><td> "actions": \[ {</td></td>

<td><td> "type": "scroll",</td></td>

<td><td> "x": 0,</td></td>

<td><td> "y": 0,</td></td>

<td><td> "origin": element,</td></td>

<td><td> "delta": 30,</td></td>

<td><td> "direction": "y"} \] \] }</td></td>

<td></tr></td>
<td></table></td>

<td>Pointer Events altitude/azimuth attributes</td>

<td>liviutinta@ sent a Pointer Events pull request to fix <a href="https://github.com/w3c/pointerevents/pull/323">problems with default values</a> for altitude/azimuth/tiltX/tiltY attributes. The associated <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2165457">CL</a> by liviutinta@ is under review too. mustaq@ sent a follow-up pull request to fix <a href="https://github.com/w3c/pointerevents/pull/324">boundary value conversion errors</a>.</td>

<td>The first <a href="https://github.com/w3c/pointerevents/pull/323">pull request</a> above adds important missing details to existing azimuth and altitude attributes: </td>

    <td>Support C++ code generation from pointer_event.idl and
    pointer_event_init.idl files. This was a blocker for C++ implementation in
    this <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2165457">CL</a>.
    As a result the default values for tiltX, tiltY, azimuthAngle, altitudeAngle
    were removed from pointer_event_init.idl. This change allows one of the
    angle pairs to be specified in the PointerEvent constructor and the other
    pair will be automatically calculated by the user agent. </td>

    <td>Change the default value for altitudeAngle from 0 to /2 to better align
    with default values for tiltX and tiltY.</td>

    <td>Specify the rounding method when computing tiltX, tiltY from
    azimuthAngle, altitudeAngle </td>

    <td>Specify what happens when only some of the angle values are provided for
    untrusted pointer events: the user agent will never overwrite a value
    provided by the author even if the values are wrong. If the values provided
    for tiltX, tiltY, azimuthAngle, altitudeAngle are not consistent, it will be
    considered an authoring issue.</td>

    <td>Specify that for trusted pointer events the user agent will populate
    both sets of angles. </td>

<td>Example specifying tiltX, tiltY in PointerEvent constructor and being able to use altitudeAngle, azimuthAngle afterwards.</td>

<td><table></td>
<td><tr></td>

<td><td>var event = new PointerEvent("pointerdown", {tiltX:0, tiltY:45});</td></td>

<td><td>console.log(event.azimuthAngle); // will print the value Math.PI/2</td></td>

<td><td>console.log(event.altitudeAngle); // will print the value Math.PI/4</td></td>

<td></tr></td>
<td></table></td>

<td>Example specifying azimuthAngle, altitudeAngle in PointerEvent constructor and being able to use tiltX, tiltY afterwards.</td>

<td><table></td>
<td><tr></td>

<td><td>var event = new PointerEvent("pointerdown", {azimuthAngle:Math.PI, altitudeAngle:Math.PI/4});</td></td>

<td><td>console.log(event.tiltX); // will print the value -45</td></td>

<td><td>console.log(event.tiltY); // will print the value 0</td></td>

<td></tr></td>
<td></table></td>

<td>Visual representation of azimuth and altitude for a pen <a href="https://www.raywenderlich.com/1407-apple-pencil-tutorial-getting-started#toc-anchor-006">here</a>.</td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | June 2020</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
