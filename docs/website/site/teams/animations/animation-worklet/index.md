---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
page_name: animation-worklet
title: Animation Worklet
---

Animation Worklet is a new primitive that provides extensibility for web
animations and enables high performance scripted animations that can run on
dedicated thread. The feature is developed as part of the [CSS Houdini task
force](https://github.com/w3c/css-houdini-drafts/wiki).

The Animation Worklet API provides a method to create scripted animations that
control a set of animation effects. These animations are executed inside an
isolated execution environment, *worklet* which makes it possible for browser to
run such animations in their own dedicated thread to provide a degree of
performance isolation from main thread. Worklet animations may be created and
controlled via Web Animations API. Animation Worklet combined with other new
features such as [ScrollTimeline](https://wicg.github.io/scroll-animations/),
[Input in
Worklets](https://github.com/w3c/css-houdini-drafts/issues/834#issuecomment-470579488),
can allow many currently interactive main-thread rAF-based animations to move
off main thread which improve smoothness.

    [Specification](https://drafts.css-houdini.org/css-animationworklet/)

    [Explainer](https://github.com/w3c/css-houdini-drafts/blob/master/css-animationworklet/README.md)

    [Design Principles and
    Goals](https://github.com/w3c/css-houdini-drafts/blob/master/css-animationworklet/principles.md)

    Tests:
    <https://github.com/web-platform-tests/wpt/tree/master/animation-worklet>

    Example codes [1](https://googlechromelabs.github.io/houdini-samples/),
    [2](https://aw-playground.glitch.me/),
    [3](https://houdini.glitch.me/animation)

    [Blink Design
    Document](https://docs.google.com/document/d/1MdpvGtnK_A2kTzLeTd07NUevMON2WBRn5wirxWEFd2w/edit?usp=sharing)

Current Status

<table>
<tr>
<td><b> Milestone</b></td>
<td><b> Status </b></td>
<td><b> Key features</b></td>
</tr>
<tr>
<td> Animation Worklet Prototype</td>
<td>Done</td>
<td> Scripted custom animation, single effect, only fast properties, off main-thread (using compositor thread).</td>
</tr>
<tr>
<td> Animation Worklet <a href="https://developers.google.com/web/updates/2018/10/animation-worklet">Origin Trial</a></td>
<td>In progress (<a href="https://docs.google.com/forms/d/e/1FAIpQLSfO0_ptFl8r8G0UFhT0xhV17eabG-erUWBDiKSRDTqEZ_9ULQ/viewform">signup</a>)</td>
<td> Scroll input (via ScrollTimeline), basic web-animation controls (play/cancel), move to dedicated thread, optimized performance.</td>
</tr>
<tr>
<td> Animation Worklet V1 - MVP</td>
<td>Scheduled for M75</td>
<td> Animate all properties (slow path ones runs in sync with main thread), sophisticated scheduling so we do not block compositor thread, improved scroll timeline integration. </td>
</tr>
<tr>
<td>Animation Worklet V2 </td>
<td>TBD</td>
<td> Pointer input via Events, multi-input animation, support GroupEffects, improved integration with Paint Worklet.</td>
</tr>
</table>
