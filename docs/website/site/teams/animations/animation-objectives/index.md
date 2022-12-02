---
breadcrumbs:
- - /teams
  - Teams
- - /teams/animations
  - Animations Team
page_name: animation-objectives
title: Animation Objectives
---

We define and track progress against our goals using
["OKRs"](https://www.gv.com/lib/how-google-sets-goals-objectives-and-key-results-okrs)
(Objectives and Key Results). Here are most of the OKRs for the animations team.
Note that these are **intentionally aspirational** and so we will be happy if we
deliver 60%-70% of them.

OKRs are given priority orderings, where P0 is the highest priority and P4 is
the lowest. Normally (for our team) OKRs exists between P1 and P3.

If you have any questions about our OKRs, please email our [mailing
list](/teams/animations).

**2020 Q1**

Modernize animations programming by shipping Web Animations v1 in M82
*#Implement*

P1 - Sign off on spec and I2S

P1 - Close all P1 Blockers

P2 - Triage and address P2’s

P2 - Address WPT gaps to improve interop *#Velocity*

Enable richer, jank-free web experiences *#Optimize*

P1 - Ship Off-Thread Paint Worklet

P2 - Validate and Promote PaintWorklet

P2 - Drive consensus on GroupEffect standardization *#Next*

Improve the scrolling experience *#Next*

P1 - Scroll Timeline: Implement and prep for Q2 Origin Trial

P2 - Launch Virtual Scroller and supporting primitives

Team Velocity *#Velocity*

P1 - Reduce [bug count](https://bit.ly/2RbjS1C) by 15%

\[Measure is: 1 - (bugs_at_end_of_quarter / (bugs_at_start_of_quarter +
bugs_opened_during_quarter))\]

P1 - Iterate on improving throughput metric

**2019 Q4**

Implement the existing standards 0.75

P1 - Implement Web Animations v1 in Q1 (M82) 0.6

P2 - Stabilize Scroll Snap 0.9

Explore and define the next generation of animation 0.75

P1 - Animation Worklet: Ship updated AW in M81 0.4

P1 - Scroll Timeline: Validate Spec through Origin Trial 0.5

P2 - Investigate Virtual Scroller, with community+partners 1.0

P2 - Drive consensus on GroupEffect standardization 1.0

Optimize Animations 0.85

P1 - Ship Viz Hit Testing V2 1.0

P1 - Ship Off-Thread Paint Worklet 0.7

P2 - Validate and Promote PaintWorklet 0.0

Team Velocity 0.3

P1 - Reduce [bug count](https://bit.ly/2RbjS1C) by 25% 0.2

Measure is: 1 - (bugs_at_end_of_quarter / (bugs_at_start_of_quarter +
bugs_opened_during_quarter))

P1 - Land “Throughput” metric, and validate w community 0.4

**2019 Q3**

**Implement the existing standards 0.71**

P1 - Implement Web Animations v1 by M79 0.7

P2 - (Prepare to) Land Web Animations v1 0.7

P2 - Hand Off Scroll Snap 0.75

P2 - Improve WPT Compliance (fix the tests) 0.7

**Explore and define the next generation of animation** 0.51

P1 - Animation Worklet: Spec Approved (and shipped!) 0.3

P2 - Scroll Timeline: Specification+Feedback 0.3

P2 - Input For Workers/Worklets - design use case for input to animation
worklets 0.5

P2 - Group Effect: Publish Explainer 1.0

**Optimize Animations 0.75**

P1 - Land Viz Hit Testing 0.8

P1 - Off-Thread Paint Worklet: enable custom property animation 0.7

**Team Velocity 0.75**

P1 - Reduce [bug count](https://bit.ly/2RbjS1C) 25% 1.0

*Measure is: 1 - (bugs_at_end_of_quarter / (bugs_at_start_of_quarter + bugs_opened_during_quarter)).*

P1 - Land top-level/user-facing metric for animation performance 0.7

P2 - End to end cc/integration testing 0.0

P2 - Animations Tracing - Define plan for exposing the complexity of animation
via dev tools or tracing 0.0

P2 - Animations Metrics Design Doc published, reviewed 0.7

**2019 Q2**

**Implement the existing standards 0.7**

P1 - Implement key missing parts of the Web Animations standard. 0.5

P1 - Ensure spec conformance and interoperability by driving down WPT test
failures 1.0

P2 - Land Scroll Snap 0.7

P2 - PaintWorklet: Implement Lottie (as a demo) off-thread using paintworklet.
0.6

P2 - PaintWorklet: Enable worklets on cc 0.6

**Explore and define the next generation of animation** 0.78

P1 - Ship Animation Worklet 0.8

P2 - Invest in partnerships/relationships with standards and browser partners
0.9

P2 - Continue to invest in adding value to Animation Worklet 0.8

**Optimize Animations** 0.38

P1 - Enable Viz layer based hit testing 0.3

P1 - Measure animations performance 0.7

P2 - Remove additional property nodes / compositing overhead due to animation
ElementId tracking 0.2

**Team Velocity** 0.63

P1 - Reduce [bug count](https://bit.ly/2RbjS1C) 25% 1.0

*Measure is: 1 - (bugs_at_end_of_quarter / (bugs_at_start_of_quarter + bugs_opened_during_quarter)).*

*End of quarter result: (134 / (128 + 55)) ~= 27%*

P1 - BGPT causes no P1 animation regressions 1.0

P1 - Animations Metrics Design Doc published, reviewed 0.4

P2 - Start animations based on ElementId existence 0.1

P3 - End to end threaded rendering unit test support 0.0

**2019 Q1**

**Correct animations and effects** 1.00

P1 - Reduce [bug count](https://bit.ly/2RbjS1C) 15% 1.0

*Measure is: 1 - (bugs_at_end_of_quarter / (bugs_at_start_of_quarter + bugs_opened_during_quarter)).*
*End of quarter result: (128 / (134 + 49)) ~= 30%*

P1 - Fix 5 of the top 20 animations [interop issues](https://bit.ly/2sFImqb) 1.0

*End of quarter result: Closed 16 Hotlist-Interop bugs incl shipping
prefers-reduced-motion + CSS transition events.*

**Improve animations code health** 0.2

P1 - Begin consolidation of
[blink::WorkletAnimation](https://cs.chromium.org/chromium/src/third_party/blink/renderer/modules/animationworklet/worklet_animation.h?l=37&rcl=a4f73cdaeddaee4969614ce2832ea8b997514242)
and
[blink::Animation](https://cs.chromium.org/chromium/src/third_party/blink/renderer/core/animation/animation.h?l=64&rcl=a4f73cdaeddaee4969614ce2832ea8b997514242).
0.1

P2 - Make a blink driven cc integration/unit test possible. 0.0

P2 - Consolidate transform interpolation between blink and cc. 0.5

*End of quarter result: Feasibility investigated + prototype attempted, but
sidelined in favour of AnimationWorklet work.*

P2 - Refactor code around [p](goog_624119910)[er-property
cc::Animations](http://crbug.com/900241). 0.3

**Smooth scrolling on all (i.e. incl/ low end) devices** 0.82

P1 - Ship Compositor Touch Action 1.0

*End of quarter result: Shipped in M75!*

P1 - Viz Hit Testing - continue to scale up Finch trial 0.9

*End of quarter result: 100% on canary/dev, 50% Finch trial on beta,
improvements in performance + correctness*

P2 - Implement paint-calculated Non-Fast Scrollable Regions 0.3

**Richer animation effects competitive with native experiences** 0.50

P1 - Ship first version of AnimationWorklet 0.8

*End of quarter result: almost all MVP features landed, TAG review requested,
not published as FPWD yet.*

P1 - Ship ScrollTimeline for AnimationWorklet 0.3

P1 - Improve overscroll-behavior implementation 0.3

P1 - [Off-Thread Paint Worklet](/teams/animations/paint-worklet): basic version
behind flag in tip-of-tree 0.8

*End of quarter result: Two CLs still in flight, but with those in a (partially)
working prototype exists behind --enable-blink-features=OffMainThreadCSSPaint*

P2 - Drive AnimationWorklet adoption 0.0

P2 - GroupEffect Implementation 0.0

P2 - Support snap points 0.8

*End of quarter result: scroll-snap-stop implemented to support AMP, some bugs
remain*

**Easier performant effects** 0.00

P3 - Educate developers on when their animations are slow and why 0.0
