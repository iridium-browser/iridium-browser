---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
- - /teams/animations/highlights-archive
  - Highlights Archive
page_name: february-2021-new-features-testing-stability-fixes-code-health-and-more
title: 'February 2021: New Features, Testing, Stability Fixes, Code Health and more!'
---

<table>
<tr>

<td>February 2021</td>

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

<td><td>Content-visible CSS animations & transitions</td></td>

<td><td>kevers@ has been working on this feature and fixed some problems.</td></td>

<td><td>Problem 1: Wasteful to run animations on hidden content (content-visibility hidden or auto + offscreen). </td></td>

<td><td>Problem 2: Wasteful to check each frame if the conditions apply.</td></td>

<td><td>Solution:</td></td>

    <td><td>Document has some global state for display locks.</td></td>

    <td><td>Added time of last lock update</td></td>

    <td><td>Use time of last lock update to determine if locally cached lock
    state is stale.</td></td>

    <td><td>Unblocks remaining steps in developing the feature. (<a
    href="https://docs.google.com/document/d/1Enj8nD-y2vgCp2A6-M2QFoUKf1-nxMX023ervOoXi50/edit?usp=sharing">Design
    doc</a>)</td></td>

<td><td>Composite background-color animation</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/T329-TOc6C-VtU5u5Dry6zo-250r722wLDaPyE7QrioBJWEzmLezgu9OpHZP3X7Us9GSJc-0L2bcU2IABFNziUPI-wl19ECPiL0rcAHy1CSMFKq0vArDbw4qlOG8Q_amwpp9JFUXfN1CK0ibHfhXywQO6xl-YjWk7q99dff4MSE34Y9R" height=203.57052631578944 width=217.23208556149734></td></td>

<td><td>xidachen@ landed some CLs to complete the implementation for this feature, and then started a finch experiment.</td></td>

    <td><td>The first finch study had a lot of crashes, xidachen@ <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2675160">landed</a>
    fix for that.</td></td>

    <td><td>The second finch try crashed a small number of users, xidachen@ <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2692609">landed</a>
    fix.</td></td>

    <td><td>Currently, there are two known problems, and we are actively working
    to resolve them.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Animation Validation</td></td>

<td><td>xidachen@ worked on validating main-thread animation detection logic, as part of the core web-vitals effort.</td></td>

    <td><td>Here is a <a
    href="https://docs.google.com/document/d/1iz0YdNKHpcObTe3UrM7uY6fsGv8xOlwY35m5dZzIoR8/edit">doc</a>
    that describes how to detect different types of main-thread animations from
    traces. And here is a sample <a
    href="https://drive.google.com/file/d/1IdObZMCYRMraUIYcM5RSQo2L9u2Uhqw9/view?usp=sharing&resourcekey=0-rTeQ2zu0ajWxMtGGhFAKng">video</a>
    of output.</td></td>

<td><td>Impulse animations</td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/28lZWBVSb1BQHulAMjKveaDG9I3uaNXJzPzD5ZdwTS2KhNWn85CZdnDZL-e_f2P6BT1PLiIYvCw6nzpmFsCmiKtJsEwBWBJhxctZDOb4-U16TuvW6j5ursJPg7H0T5EEiGztd1LShDKjzfIs02ulLhPAVhJAV_Ydrsj1lSQzPV7KB5VO" height=92 width=134> <img alt="image" src="https://lh4.googleusercontent.com/3SRHx6N3NrBhVlYRrGv_d70qwqH_zQyQL_0PXr2D3HI4tTRXYr8RzirbqcOKtBj6Xg4TPfWZtwwUfSk8nNekZ8Yi5CSkiqDta1-I3bpSBpn0MRo8OYXnQPrWbXp8cCK6NSULireHlRgvC-7ji80BER6HBB12jqNgaVproz-4RXMykV7Z" height=90 width=133></td></td>

<td><td>flackr@ supported <a href="mailto:arakeri@microsoft.com">arakeri@microsoft.com</a> to implement this feature, which has a faster initial impulse when scrolling. This is shipped in M90.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Click/Auxclick as pointer event</td></td>

<td><td>liviutinta@ made some great progress in this sprint.</td></td>

    <td><td>Bug hunting - no bugs identified.</td></td>

    <td><td>Expanded finch experiment to 10% stable.</td></td>

    <td><td>Landed Accessibility click <a
    href="https://chromium-review.googlesource.com/c/chromium/src/+/2679278">CL</a>,
    part of which was relevant to Click as pointer event.</td></td>

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

<td><td>In order to fix some flaky tests, kevers@ made some changes to gesture_utils.js. Work was already underway to replace use of waitForAnimationEnd with waitForAnimationEndTimeBased. </td></td>

<td><td>The problem with the former method is that the timeout is expressed in animation frames and for our fast tests these trigger every 1ms instead of the usual 16ms. </td></td>

<td><td>The later method is a step in the right direction, but tends to flake when test machines are under load. The reason for this is that the timing starts when the gesture event is queued and not when scrolling begins. Also, since we snap after the initial scroll completes, we are queued up a second smooth scroll, which can timeout while queued. </td></td>

<td><td>A third approach, waitForScrollEnd, is being introduced to address these problems. This method can also deprecate the waitFor method (same timeout issue as waitForAnimationEnd). By using scroll events, we avoid flakes due to queuing delays and handle chaining of scroll events better, while at the same time tests complete faster by avoiding unnecessary waits once the target position has been reached. </td></td>

<td><td>TestDriver Action API</td></td>

<td><td>lanwei@ kept improving the TestDriver action API and made more WPT tests automatic.</td></td>

<td><td>The following two pictures show the wpt dashboard for the TestDriver action API.</td></td>

<td><td><img alt="image" src="https://lh6.googleusercontent.com/N4_vKH1JiDu3pf13Ng6QfmjemCO1fSzV_QyfO6vHmB_AnaWTK8SPDoiLYag3lmb5FfmQfbW_4rdzbZiccNbQlv1j-JKigVmajoNNXbNciRwh1isPOfOTDfanvP6vcaHCq9VBtFWGMe-1l7CsDqDQuk4jWp_ROYO2R5Ah1u3sxcpLJcVK" height=102 width=140> <img alt="image" src="https://lh6.googleusercontent.com/vH3v_xKD--xwrrRcy4UosP2hEkBrVoK9WXRRZ5rAqIhi8vzlhN9COtKbNXeOkcXk5BvTNELRXJQlxVmk0rOlcDlvOkO2y_zN7AJZ3hiJBimD7R--jjIx9x1D9B7YMrHp32x4VY966SH9cKbaFhZOgeRYn3gXMPlyQBc8d5Q85ie5FQm5" height=71 width=135></td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter III: Stability Fixes</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Fixed crash in set current time</td></td>

<td><td>xidachen@ fixed a crash when the timeline time is Infinity or -Infinity and that timeline is attached to an animation. The solution is to special case when the current time is Infinity or -Infinity.</td></td>

<td><td>Fixed crash in null animation timeline</td></td>

<td><td>The problem occurs when we set an animation timeline where its previous timeline is null. xidachen@ resolved this by loosing the condition of some DCHECKs to ensure that we can handle the case where the previous timeline is null.</td></td>

<td><td>Prevent user from exiting fullscreen</td></td>

<td><td>mustaq@ fixed this issue which has complicated initial repro and that misguided our initial investigation. mustaq@ spent hours to narrow down the root cause, and it is due to the print dialog on another tab halts the main thread. Moreover, now we have a minimal repro of 12 lines of HTML + JS.</td></td>

<td><td>Fixed crash length interpolation</td></td>

<td><td>The crash happens when the “from” and “to” value of the interpolation is interpreted as Infinity. xidachen@ fixed it by only DCHECKing when the interpreted value is finite.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter IV: Code Health</td></td>

<td><td><img alt="image" src="https://lh4.googleusercontent.com/jyVUQx4WQb7YDctRAjafGXZE-w7qxcUmoGFWxzW_ujPxx7YlWgKFZiF_Y982jngBopZsRX-hW70W5fDTdXcAhFwTWa15TZ8Tq-29VLl2PmiCu1FQE1DnoXfcatmbzlq1c_YqpaKuOf2gV2UeZ2MkJ47vkjPsi3xKxRAMjC7YZPJkG869" height=152 width=282> <img alt="image" src="https://lh3.googleusercontent.com/wEPBpfUYaObQqvjrBwCsfE7sIQ0bSJTIUS0345K6O9QICMbn1PPbIQdu0d8ApDFGSqUWPgPt43sJ8EweAgRR2o1Wt9OZWSOK5bO5uWMkyfu8njZ5nFa_DK5VLudJBfcSffh8AW2oYLJ2_RExeqCndVT4yyhNfwWGnqRxKRqwXNrGEvK5" height=152 width=278></td></td>

<td><td>Our team holds ground on P1 bugs. In terms of bug tracking, we switched the y axis from “issues” to “bugs”, which more accurately reflects goals to improve product excellence.</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Chapter V: Miscellaneous</td></td>

<td></tr></td>
<td></table></td>

<td><table></td>
<td><tr></td>

<td><td>Tabs</td></td>

<td><td>girard@ started this Tabs <a href="https://docs.google.com/document/u/0/d/18C_W5SRsuPfCyyXbEsCjiHOGc8OWJXRlKvKKN-rlYsQ/edit">One-Pager</a>, and a lot of people are contributing to it now.</td></td>

<td><td>flackr@ and nsull@ took a first pass at an <a href="https://docs.google.com/document/d/1HcQ75iRhO-dT7EHB6JrjmMATa9XlSCYZKWrXbzakexQ/edit?resourcekey=0-kYHpL3r3jY3Q8wtTaOa6aA">explainer</a>.</td></td>

<td><td>Disable double tap to zoom for mobile viewport</td></td>

<td><td>liuviutinta@ has a <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2679278">CL</a> in review, and still working on writing tests for Andriod.</td></td>

<td></tr></td>
<td><tr></td>

<td><td>Accessibility click indistinguishable from real click</td></td>

<td><td>liviutinta@ landed a <a href="https://chromium-review.googlesource.com/c/chromium/src/+/2679278">CL</a> to fix this <a href="https://bugs.chromium.org/p/chromium/issues/detail?id=1150979">bug</a>. The fix included:</td></td>

    <td><td>Added pointer events up/down.</td></td>

    <td><td>Populated mouse events coordinates appropriately.</td></td>

    <td><td>Cleaned up the code by using SimulatedClickCreationScope as argument
    for <a
    href="https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/core/dom/element.h;drc=763c071aa91f87c5e404edff98bc5bac8075d4a1;l=600">Element::AccessKeyAction</a>
    instead of bool send_mouse_events to clarify where the simulated click
    originates from</td></td>

    <td><td>Added wpt tests for clicks from accesskey and clicks from
    enter/space keys to ensure interop</td></td>

<td><td>Pointer lock: pointerrawupdate coordinate jumps </td></td>

<td><td><img alt="image" src="https://lh3.googleusercontent.com/5O-XkmmvjyGYYQDme-3cP1Jgn2AjV2DE7szj47ylMZKvthruyejkLyCsC5XlO_CafCOkJVrVrD1dkPGjXfrxgZ7JigZFxv4A_su8UR_EldHtTJQ5fdDWJv2t8bhe5WYWeBMIymwE7cmXdr23S_Xtvyl1xon4aJEVoPOV5xTvPXd1RrA6" height=224 width=247></td></td>

<td><td>musta@ discovered a regression due to a code “improvement”.</td></td>

    <td><td>In the code shown above, the Create() function does more than what
    the name applies!</td></td>

    <td><td>Regression has been fixed, a test has been added.</td></td>

<td></tr></td>
<td></table></td>

</tr>
</table>

<table>
<tr>

<td>Chrome Interactions Highlights | February 2021</td>

<td><a href="http://go/interactions-team">go/interactions-team</a></td>

</tr>
</table>
