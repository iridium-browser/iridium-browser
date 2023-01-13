---
breadcrumbs:
- - /teams
  - Teams
page_name: animations
title: Animations Team
---

The animations team is an engineering team that aims to enable web developers to
deliver a smoother more responsive web.

The team is responsible for maintaining and improving the animations
infrastructure, correctly effecting threaded scrolling and related effects
(fixed/sticky position, scroll snapping, etc) in the composited property trees,
and ensuring that sufficient targeting and hit testing information is generated
from Blink.

See also our concrete [quarterly
objectives](/teams/animations/animation-objectives).

## Goals

*   Performance - Smooth user interaction and animations (reliable 60fps
            on today's hardware, 120 fps on tomorrow's)
*   Predictability - Interoperable animations APIs on the 4 major
            browser engines.
*   Capabilities - Enable rich scroll and input linked effects popular
            on mobile applications.

## Activities

The animations team is driving several ongoing efforts.

*   Houdini
    *   [Animation Worklet](/teams/animations/animation-worklet)
    *   [Paint Worklet](/teams/animations/paint-worklet)
*   Optimized hit testing
*   CSS Scroll Snap
*   Web Animations
*   [Scroll-linked Animations (Scroll
            Timeline)](https://drafts.csswg.org/scroll-animations)

We send out an ~monthly newsletter with our activities; feel free to [check out
our archives](/teams/animations/highlights-archive).

## Organization

### Mailing lists

The team uses a public mailing list for technical discussions, questions, and
announcements.

Email address: [animations-dev@chromium.org](mailto:animations-dev@chromium.org)

Web archives:
[animations-dev](https://groups.google.com/a/chromium.org/forum/#!forum/animations-dev)

We are also available in #animations on the [Chromium
Slack](https://docs.google.com/document/d/1nCqDQEF2pW5cUMNBBZPP20DZ7TVCu58ylhCk_Q8LqU4/edit).

### Weekly Meeting

There are two tri-weekly (once every three weeks) meeting held over video
conference on Tuesdays and another on Fridays for planning and going over
results. If you're interested in participating please reach out on the mailing
list or slack and we can share instructions.

The meetings follow the following schedule:

*   Week 1: Planning meeting on Tuesday 2:30pm EST, [meeting
            notes](http://bit.ly/animations-team-planning)
*   Week 3: Demo meeting on Friday 11:00am EST, [meeting
            notes](https://docs.google.com/document/d/15SH-FgMd0jPtUcPs_3A_JI9sOwmA1JrEZOHvIM8wIm0/edit?usp=sharing)

Highlights from the demo meetings are shared in our ~monthly newsletter, see
Activities above.

## Bug Triage

The animations team is responsible for bugs filed in the
[Blink&gt;Animation](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3ABlink%3EAnimation+&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids),
[Internals&gt;Compositing&gt;Animation](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3AInternals%3ECompositing%3EAnimation&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids),
[Internals&gt;Compositing&gt;Scroll](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3AInternals%3ECompositing%3EScroll&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids)
components.

The team has a daily triage to confirm, triage, and categorize incoming bugs in
these components.

## Related teams

[Input](/teams/input-dev)

[Rendering](/teams/rendering)
