---
breadcrumbs:
- - /teams
  - Teams
- - /teams/input-dev
  - Input Team
- - /teams/input-dev/input-objectives
  - Input Objectives
page_name: 2015-okrs
title: 2015 OKRs
---

## 2015 Q4 0.54

### Improve input latency and smoothness 0.40

## Ensure mean_input_event_latency time remains under 12ms on N5/One for 90% of
key_mobile_sites_smooth at least 90% of the time P2 1.00

## Get EventListenerOptions into official DOM event spec P1 0.70

## Implement and ship EventListenerOptions to support registering passive event
listeners. P1 0.30

## Ship UX (hung renderer dialog) for unresponsive sites on Android P4 0.00

## VSync-aligned touch input working with unified BeginFrame on Aura P4 0.00

## Reduce smoothness.pathological_mobile_sites.first_gesture_scroll_update on
the Nexus 5 by 20% P4 0.00

## Impl side hit-testing against property-tree and display lists. P4 0.00

### Improve understanding of real-world input performance 0.40

## Expose hardware input timestamps to the web P1 0.95

## Implement experimental input latency web API in M49 P1 0.00

## Expose scroll latency in DevTools somehow P1 0.30

## Concrete scroll latency API presented to web-perf WG at TPAC P1 0.10

## All touch latency UMA regressions have appropriate owners within 3 weeks. P3
1.00

## Add UMA metrics for determining potential benefit of passive event listeners
P2 1.00

## Add touch latency metric measuring until estimated vsync time. P2 0.00

## Better measure blink hit-test performance P4 0.00

## Use input-related perf insights data to drive performance investigations, 12
associated Hotlist-Jank bugs P4 0.10

### Eliminate key developer pain points around input 0.56

## Fix at least 150 Hotlist-Input-Dev bugs P1 0.60

## Implement root-layer-replacement mechanism for fullscreen and non-body root
scrollers P1 0.30

## Ship more rational mode of top control resizes P1 0.60

## Triage any new Blink-Input, Blink-Scroll bug within 7 days P1 1.00

## Spec and land API to control Chrome pull-to-refresh nav gesture behind a flag
P2 0.10

## Drive adoption of history Scroll Restoration API P2 1.00

## Implement unified OSK viewport behavior between Android and CrOS for M49
behind a flag P3 0.50

## \[STRETCH\] Allow "non-scrolling" apps to interact well with top controls
(behind a flag) P4 0.00

## \[STRETCH\] Implement new input modality media queries P4 0.00

## \[STRETCH\] Implement position: device-fixed behind a flag P4 0.00

### Improve interoperability between major browser engines 0.55

## Enable automated input testing on Chrome for W3C web-platform-tests P1 0.50

## Ship spec-correct body scroll API behavior P1 0.20

## Make all position-related APIs relative to Layout Viewport P1 1.00

## Make Chromium reliably send MouseLeave events to blink P1 0.60

## Ship KeyboardEvent.key in M48 P1 0.20

## Identify and fix 3 interop bugs with mouse events. P1 1.00

## Triage all Hotlist-Interop bugs and find appropriate owners every week P2
0.60

## Fix international keyboard events P3 0.20

## Ship HTMLSelect.open API P3 0.00

### Enable rich scroll effects 0.45

## Prototype snap-points implementation in M47 P1 0.40

## Enable scroll customization on UIWorker thread P1 0.50

## Ship scroll latching for touchpad behind a flag for one platform P4 0.10

## Smooth scrolling for wheel and keyboard scrolls in Win/Linux enabled on trunk
P2 0.90

## STRETCH: Ship scroll latching for touchpad on all platforms P4 0.00

## Prototype sticky implementation in M48 P1 0.40

### Continue PointerEvent API implementation 0.73

## Fire pointer events for all mouse events P1 1.00

## Add chorded button transformation to mouse-&gt;pointer event generation logic
P1 1.00

## Update MouseEvent (and so PointerEvent) to support fractional co-ordinates P3
0.20

## Extend WebMouseEvent to become a pointer event API P3 0.00

## Implement pointer-id ranges for all pointer types P2 1.00

## Make PointerEvents work for stylus in CrOS & Android. P2 0.70

## Force pointercancel firing with mouse off the page. P3 0.00

## Add pointer event handler tracking P3 0.00

## STRETCH: Explore design choices for explicit pointer capture support P4

### Evolve touch platform UI 0.56

## Ship unified touch text selection for native Aura UI P1 0.10

## Make check boxes and radio buttons touch friendly without depending on touch
adjustment P1 0.60

## Support trackpad pinch-zoom on CrOS P2 0.80

## Show touch selection handles when scrolling on CrOS P3 0.00

## Close 10 text selection bugs P1 1.00

### Improve UI Events spec and tests 0.69

## Migrate UIEvent (and old Dom3Event) issues from Bugzilla to github P1 1.00

## Reduce spec issue count down to less than 10 P1 0.40

## Implement basic keyboard and mouse event tests in UIEvent repo P1 1.00

## Merge UIEvent tests into main W3C tests P2 0.00

### Improve code health

## \[Stretch\] Plan for reducing at least one abstraction layer from input
events P4 0.10

## 2015 Q3 0.63

### Improve input latency and smoothness 0.55

## Ensure mean_input_event_latency time is under 12ms on Nexus 4 for 90% of
key_mobile_sites at least 90% of the time P1 0.80

## Get consensus on non-blocking event listener API P1 0.70

## Implement non-blocking event listener API behind a flag in M46 P1 0.10

## Approved UX design for unresponsive sites P1 0.60

## Ship hit-test-cache eliminating redundant hit tests in GestureTap and PEP
shadow DOM P1 1.00

## Drive investigation of regressions in touch latency UMA data P1 0.80

## File 30 concrete Hotlist-Jank bugs (eg. from "perf parties") P1 0.30

## Concrete plan for forced non-blocking event listeners P2 0.20

## Better measure and improve blink hit-test performance P2 0.20

## Reduce smoothness.pathological_mobile_sites.first_gesture_scroll_update on
the Nexus 4 by 25% P2 0.60

## STRETCH: Ship experiment for validity-rect based hit-test caching P3 0.00

## Expose hardware input timestamps to the web 0.85

## Concrete plan for input latency API on the web 0.30

### Improve interoperability between major browser engines 0.63

## Enable automated input testing on Chrome for W3C web-platform-tests P1 0.70

## Ship Event.isTrusted P1 1.00

## Ship viewport scroll behavior matching IE in M46 P1 0.90

## Ship spec-correct body scroll API behavior in M46 P1 0.40

## Share interoperability tracking plan with blink-dev and other vendors P1 0.60

## Triage all Hotlist-Interop bugs and find appropriate owners every week P1
0.40

## Ship interoperable mouse-drag behavior across iframes P2 0.10

## Make Chromium reliably send MouseLeave events to blink P2 0.40

## Ship KeyboardEvent.code in M47 0.90

## Finish implementing KeyboardEvent.key behind a flag 0.75

### Eliminate key developer pain points around input 0.68

## Fix at least 100 Hotlist-Input-Dev bugs P1 1.00

## Ship history scroll state customization API in m46 P1 1.00

## Ship InputDevice and sourceDevice API in M46 P1 0.90

## Implement top-control hiding for full-page scrollable divs behind a flag P1
0.20

## Reduce untriaged Cr-Blink-Input and Cr-Blink-Scroll bugs to ~0 P2 0.90

## File and investigate concrete bugs for accessibility issues around full-page
scrollable divs P4

## Allow "non-scrolling" apps to interact well with top controls (behind a flag)
P2 0.20

## Full-featured mouse support on Android (text selection, context menu's,
etc...) P2

## STRETCH: Dynamic support for interaction media queries P4 0.00

## STRETCH: Implement unified OSK viewport behavior between Android and CrOS for
M46 behind a flag P4 0.00

### Evolve touch platform UI 0.79

## Ship remaining M input features (stylus zoom, floating toolbars, perf test
for smart select) P1 1.00

## Ship unified touch text selection in M46 for web contents in Aura P1 1.00

## Ship unified touch text selection for native Aura UI P3 0.10

## Make check boxes and radio buttons touch friendly without depending on touch
adjustment P2 0.75

## (STRETCH) Fix cross-site navigation GestureNav issue P4 0.00

## Support trackpad pinch-zoom on CrOS P2 0.70

## (STRETCH) Show touch selection handles when scrolling on CrOS P4 0.00

### Continue PointerEvent API implementation 0.67

## Measure multi-touch pointer event dispatch overhead P1 0.90

## Fire pointer events for all mouse events P1 0.65

## Update MouseEvent (and so PointerEvent) to support fractional co-ordinates P2
0.20

## Add chorded button transformation to mouse-&gt;pointer event generation logic
P2 0.20

## Start extending WebMouseEvent to become a pointer event API P1 0.70

## Add stylus support for Android P2 0.75

## Finish pointercancel support for touch P2 1.00

### Enable rich scroll effects 0.65

## Prototype snap-points implementation in M46 P1 0.50

## Enable scroll customization on UIWorker thread P1 0.50

## Enable main-thread scroll customization driven by touch input - behind a flag
P2 0.90

## Ship scroll latching for touch in M47 P2 1.00

## 2015 Q2 0.53

### Improve focus on reducing user pain around input on the web 0.60

## Reduce untriaged Cr-Blink-Input and Cr-Blink-Scroll bugs to 0 0.20

## Fix at least 80 Hotlist-Input-Dev bugs 1.00

### Improve touch web platform rationality 0.64

## Ship history scroll state customization API in m44 P1 0.60

## Implement sourceDevice.firesTouchevents for MouseEvent and TouchEvent behind
a flag in M44 P1 0.85

## Ship spec-correct body scroll API behavior in M45 P1 0.50

## Get consensus in w3c on InputDevice api spec P1 0.60

## Enable (and fix) mouseenter / mouseleave behavior for touch in M44 P1 1.00

## Ship async touchmove event coalescing in M44 P1 0.90

## Full-featured mouse support on Android (text selection, context menu's,
etc..., potentially behind a flag) P2 0.20

## Finish implementing UIEvent.sourceDevice for all scenarios in M45 (behind a
flag) P1 0.50

## Enable dynamic support for interaction media queries in X11 and Android in
M45 P3 0.00

## STRETCH: Ship InputDevice and sourceDevice API in M45 P4 0.00

### Rationalize viewport behvior 0.50

## Mature Virtual Viewport 0.80

## Ship viewport scroll behavior matching IE in M44. 0.60

## Allow "non-scrolling" apps to interact well with top controls (behind a flag)
0.00

## Implement unified OSK viewport behavior between Android and CrOS for M45
behind a flag 0.00

### Improve input latency and smoothness 0.65

## Ensure mean_input_event_latency time is under 12ms on Nexus 4 for 80% of
key_mobile_sites at least 80% of the time P1 0.80

## Avoid sending touchmove's to Blink when unnecessary, with associated metrics
to track efficacy P3 0.20

## Reduce mean_input_event_latency on Nexus 4 for
sync_scroll.key_mobile_sites_smooth by 10%, with blame analysis for current
pathological cases P2 0.60

## VSync-aligned touch input behind a flag for Aura P3 0.20

## Drive investigation of regressions in touch latency UMA data P2 0.70

## Ship hit-test-cache in M45 eliminating redundant hit tests in GestureTap and
PEP shadow DOM scenarios P2 0.40

## Stop sending mousemove events during scroll on desktop and add relevant perf
tests in M44 P1 0.85

### Enable scroll-synchronized effects (behind a flag) 0.20

## Approved design for unified scrolling, with prototype for compositor
scrolling P1 0.30

## Implement scroll-snap-points for impl-thread scrolling behind a flag in M45
P1 0.30

## Consensus within chrome team on initial design for sync-scroll jank
mitigation P1 0.20

## sync-scroll fully implemented (with jank mitigation on Android) behind a flag
in M45 P2 0.00

## Ship main-thread fractional scrolling in M44 P2 0.00

### Enable scroll customization (main thread only, behind a flag) 0.45

## Enable main-thread scroll customization driven by touch input - behind a flag
in M44 0.50

## Enable main-thread scroll customization driven by JS triggered scrolls - ship
behind a flag in M45 0.00

## Publish high quality demos, add telemetry perf tests 0.40

## STRETCH: Enable scroll customization on UIWorker thread 0.10

## STRETCH: Enable scroll customization driven by all input modalities - ship
behind a flag in M4? 0.00

### Begin Implementing pointer events API 0.60

## Fire pointer events for all touch input 1.00

## Add telemetry tests for multi-touch pointer event dispatch 0.00

## WG consensus on concrete plans for avoiding hit tests 0.00

## Support sending pointercancel on touch scroll/pinch 0.70

## Add devtools support for pointer event types 1.00

## Spec and implement new touch-action options behind a flag in M45 1.00

## (STRETCH) Fire pointer events for all mouse events 0.00

### Evolve touch platform UI 0.52

## Ship Android M text selection features (smart select, long press and drag,
floating action bar) P1 0.90

## Ship unified touch text selection in M44 for web contents in Aura P1 0.60

## Ship unified touch text selection in M45 for native Aura UI P2 0.10

## Make check boxes and radio buttons touch friendly without depending on touch
adjustment P2 0.00

## (STRETCH) Fix cross-site navigation GestureNav issue P4

## (STRETCH) Support trackpad pinch-zoom on CrOS P4
