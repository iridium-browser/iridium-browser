---
breadcrumbs:
- - /teams
  - Teams
- - /teams/input-dev
  - Input Team
- - /teams/input-dev/input-objectives
  - Input Objectives
page_name: 2017-okrs
title: 2017 OKRS
---

## ==2017 Q4==

**Eventing Sub-team**

### Improve Smoothness and Predictability of Events 0.25

### Resampled Events P1 0.30

### Early ACK touch events when we have a touch-action but all passive event
listeners P3 0.20

### Predictive Points P3 0.10

### Add Richness to Web Platform 0.63

### Low latency input mode for main screen canvas P1 1.00

### Prototype input events dispatched to Worker P1 0.15

### Spec for input for Workers P4 0.00

### Finalize Simple User Activation model P1 0.50

### Expose fractional coordinates for mouse P1 1.00

### Ship scroll options in focus P2 1.00

### Input Mode P2 0.75

### Pointerevent Spec V2: pass 10 failing tests P4 0.00

### InputDeviceCapabilities is Chrome-only P3 0.50

### Autofill focus spec P3 0.10

### Mouse Events forward/back buttons P3 0.80

### Get drag and drop on Android to work within a page P3 0.60

### Clarify screenX/Y and movementX/Y definitions P4 0.00

### Performance 0.86

### Hold the line on Input Performance P1 1.00

### Push other parties to fix the performance bugs P1 0.70

### Updating the existing metrics in UKM P2 1.00

### UKM Dashboard for input metrics or equivalent P2 1.00

### Rappor analysis and scripts P4 0.00

### Product Excellence 0.65

### Be responsive to our users P1 0.50

### Fix 8 regression bugs in Blink&gt;Input P2 1.00

### Fix 6 starred &gt; 10 bugs P2 1.00

### Touch Adjustment crbug.com/625198 P3 0.40

### Fix ARC++ trackpad events for the web. P3 0.50

### Remove legacy Touch detection P3 0.50

### Fix ordering of focus events P4 0.10

### Code Health 0.78

### Mojo Input P1 0.80

### Transform all WebTouchEvents to WebPointerEvent in Blink P2 0.90

### Support mus team in shiping new input routing mechanism. P2 1.00

### Latency info cleanup P2 0.40

### Nuke EventDispatchMediator P3 1.00

### Get rid of null initialization of UGIs P4 0.00

### Remove Legacy Touch Event Queue P4 1.00

## Scrolling Sub-team

### Performance 0.87

Wheel scroll latching and async wheel events P1 0.80

Implement phase information from ChromeOS P2 1.00

### Improve Interop 1.00

Publish “spec” how viewports and APIs work/should work 1.00

### Add Richness to Web Platform 0.51

Plan a path to shipping for document.rootScroller P1 0.70

Produce a draft spec for document.rootScroller P4 0.00

Make document.rootScroller work with OOPIF P3 0.40

Propose “resize-mode” API for OSK P4 0.00

Scroll Customization P3 0.35

### Code Health 0.60

Remove duplication between frame and layer scrolling P1 0.70

Browser side fling P2 0.40

Use scroll gestures for keyboard scrolling P2 0.20

Correct telemetry gestures P2 1.00

### Product Excellence 0.48

Windows precision trackpad implemented behind a flag P1 0.40

Enable subpixel precision scrolling in Blink P4 0.00

Scroll boundary behaviour P2 1.00

Close out remaining overlay scrollbar polish issues P3 0.30

Fix important/curated bugs P3 0.20

## ==2017 Q3==

**Eventing Sub-team**

### Improve Smoothness and Predictability of Events 0.63

### Align Events with Appropriate Lifecycle P2 0.80

### Resample input events on main thread and compositor P2 0.20

### Dispatch Mouse Events when hover changes due to DOM modifications P2 1.00

### Predicted points P3 0.20

### Make hover states and boundary events consistent across all the cases P4
1.00

### Add Richness to Web Platform 0.40

### Implement a simple user activation model that is good for cross-browser
implementation P1 0.45

### Define input events for Workers P1 0.40

### Support low latency stylus P1 0.20

### Move Pointer Event spec to CR P3 0.60

### Add Richness to Editing events P3 0.70

### Enable Input Automation in WPT P4 0.50

### Performance 0.86

### Hold the line on Input Performance P1 1.00

### Rappor and UKM metrics P1 0.50

### Ship Direct Manipulation Stylus on Windows P1 1.00

### Analyze 120Hz Hardware P2 1.00

### Product Excellence 0.52

### Be Responsive to our Users P1 0.80

### Solve non-consistent target issues with Touch Adjustment P1 0.20

### Take over ownership of Blink&gt;Input&gt;PointerLock issues P2 0.60

### Mus Input Routing P2 0.50

### Code Health 0.37

### Complete mojofication of input pipeline P1 0.70

### Complete switch to pointer events input flow in Blink Code P1 0.20

### Latency Info Cleanup P2 0.20

### Fix 5 low-hanging input-dev issues with Hotlist-CodeHealth P3 0.10

### Fractional Mouse Coordinates P3 0.60

## Scrolling Sub-team

### Add Richness to Web Platform 0.63

Smooth ViewportAPI/Inert Viewport Launch P1 0.90

Prepare rootScroller for standardization P1 0.30

Unify ChromeOS and Android On-Screen Keyboard P2 0.00

Ship ScrollBoundaryBehavior P2 1.00

Ensure ScrollLeft/Top interop launch is a success P2 1.00

### Performance 0.60

Wheel scroll latching and async wheel events P1 0.60

### Improve Scrolling UX 0.60

Windows Precision Touchpad Support P1 0.40

Support Overlay Scrollbar Launch P1 0.80

### Improve Product Polish 0.40

Fix 6 important/long-standing user or developer-facing bugs P3 0.40

### Improve Code-Health 0.22

Progress Scroll Unification P2 0.00

Fling on Browser Side P2 0.50

Keyboard Gesture Scrolling \[stretch\] P4 0.00

**Threaded Rendering Sub-team**

### Enable rich custom effects 0.49

Finish implementation of PaintWorklet on main thread P1 0.90

Finish implementation of Snap Points P1 0.10

Start a origin trial Main-Thread Scroll Customization P3 0.40

### Make compositing excellent 0.53

Promote opaque fixed/sticky position elements into composited layers.
#wp-performance P1 0.00

Finish touch action implementation && measure performance impact P1 0.50

Close 25 non-feature bugs. P2 1.00

Finish the experiment of not compositing small scrollers P2 1.00

Analyze main thread scrolling reasons P4 0.00

Finish upstreaming position:sticky tests to WPT P2 0.80

Wheel listener rects P4 0.00

Reduce number of composited layers created for correctness. P1 0.60

### Improve metrics and code health 0.61

Fix regression bugs caused by composited border-radius scrolling P1 0.90

Identify / define metrics to monitor continued threaded rendering performance
excellence. P4 0.00

Improve documentation of the animation code P2 1.00

Measure metrics without compositing thread. P4 0.00

Remove all uses of layer ids or layer references in cc/animations P1 0.50

Progress or regression of bug open/close rate is visible to the team. P2 0.00

New issues are triaged within seven days. P3 0.90

### Enable performance isolated effects on the web 0.44

Animation Worklet - Draft specification for API #wp-ergonomics P1 0.70

AnimationWorklet - Land Milestone 1 P1 0.80

Prototype raster-inducing scroll (https://crbug.com/715106) P1 0.20

AnimationWorklet - Land Milestone 2 P2 0.20

Animation Worklet - Schedule and give animations a deadline to avoid janking
compositor thread. #wp-ergonomics #wp-performance P2 0.10

\[stretch\] Ship Animation Worklet origin trial P4 0.00

\[stretch\] Ship PaintWorklet off thread P4 0.00

## ==2017 Q2==

## Eventing Related 0.52

### Improve Smoothness and Predictability of Events 0.38

## Align Events with Appropriate Lifecycle P1 0.65\[0.3\] Ship rAF aligned mouse
events \[0.4\] Address regressions with vsync aligned gestures 100 \[0.3\] Ship
Vsync aligned gestures 100

## Teach scheduler about non-blocking input events P1 0.10\[0.5\] Simplify
notification of input events to render scheduler \[0.5\] Notify scheduler to
adjust user model based on non-blocking event

## \[Punted\] Resample input events on main thread and compositor P2 eirage@
will take over. \[0.3\] Draft design doc for resampling algorithm and matrix
\[0.3\] Implement resampling behind a flag \[0.2\] Launch Finch Trial and
analysis data \[0.2\] Ship input resampling

### Product Excellence 0.67

## Be Responsive to our Users P1 0.85\[0.5\] Triage all new issues within 7 days
\[0.2\] Triage all open issues that have not been modified in 180 days \[0.3\]
Fix 7 top starred &gt; 10 stars issues

## Drive down regression bugs P1 0.70Close 33% (9 bugs) of our open regressions.

## Solve non-consistent target issues with Touch Adjustment P1 0.40\[0.1\] Write
touch/pointer tests to expose interop issues \[0.3\] Propose a solution and
draft design doc \[0.4\] Implement behind a flag \[0.2\] Ship new Touch
Adjustment <https://crbug.com/625198>

## Take over ownership of IO&gt;PointerLock issues P2 0.50\[0.5\] Fix 5
IO&gt;PointerLock issues \[0.5\] Triage and prioritize all issues.

## Study & design a sensible hover/active solution for touch+mouse P2
1.00\[0.4\] Write a doc describing and comparing hover behavior on major
browsers. \[0.4\] Propose a design based on (possibly) decoupled active & hover
states (no new APIs) \[0.2\] Implement a prototype based on the design

## Fix Chrome’s non-standard/interop APIs related to input. P2
0.60<https://bugs.chromium.org/p/chromium/issues/list?can=1&q=summary%3Astandardize%2Cremove+Hotlist%3DInput-Dev+opened&lt;2017-03-31+modified&gt;2017-03-31+reporter%3Afoolip%2Clunalu>
\[1.0\] Finalize standardize/remove decision for 5 non-standard web APIs.

### Add Richness to Web Platform 0.42

## Experiment with a simple user activation indicator that is good for
cross-browser implementation P1 0.50\[0.3\] Prototype a simple bit-per-frame
user activation and perform lab-test.
[crbug.com/696617](http://crbug.com/696617) \[0.2\] Propose a detailed design
for implementation. \[0.3\] Implement the design behind a flag, covering all
user activation clients. \[0.2\] Experimentally switch away from current
UserGestureIndicator in M59/60 beta. \[stretch\] \[PUNTED\] Publish a report on
observed regressions and next steps.

## Define input events for animation worklet P1 0.20\[0.3\] Create a
polyfill-based touch-drag prototype to aid design discussion. \[0.4\] Design doc
on event model for animation worklet, reach consensus on final design goals.
\[0.3\] \[PUNTED\] Complete an experimental plumbing to route input events to a
worker thread.

## Enable Input Automation APIs in WPT P1 0.20\[0.3\] \[PUNTED\] Add key
injection APIs in GPU Benchmarking (blocked on
[crbug.com/722921](http://crbug.com/722921)) \[0.3\] Add input injecting input
API with consensus from other vendors \[0.2\] Plumb the input injecting API to
enable the manual tests to run on Chrome bots \[0.2\] Update at least 3 manual
tests to use the new input injection API

## Expose fractional co-ordinates for PointerEvents of mouse type P2 0.00\[0.2\]
Plumb fractional mouse co-ordinates in Windows (browser side) \[0.1\] Plumb
fractional mouse co-ordinates in Android (browser side) \[0.1\] Plumb fractional
mouse co-ordinates in Mac (browser side) \[0.2\] Plumb fractional mouse
co-ordinates in Linux and ChromeOS (browser side) \[0.3\] Remove WebMouseEvent
coord truncation, fix regressions. \[0.1\] Expose fractional coordinates for
PointerEvents of mouse type (blink side)

## Make Pointer Events spec & tests ready for L2 P2 0.70\[0.3\] Drive solving
all of the 4 current L2 Blocking issues. \[0.7\] Resolve all of the current 29
tests with less than 2 passing browsers from failure list.

## Champion Auxclick P3 0.80\[0.7\] Merge auxclick spec into UI Event spec
\[0.3\] Get at least Edge or Safari to implement auxclick

## Predicted points API P3 1.00\[0.7\] Put together challenges and different
solutions we had for this API in a doc \[0.3\] \[PUNTED\] Put a prototype
together in Chrome to demonstrate this feature

## Drive TouchEvents-to-PointerEvents switchover for Google apps P3
1.00#internal \[0.5\] \[PUNTED\] Support Google Maps API switchover to
PointerEvents. \[0.5\] Make Google Maps usable with Android mouse.

## Keyboard Input Mode Consensus P4 \[0.5\] Continue with driving consensus in a
design internally \[0.5\] Publish design externally

## Add Richness to Editing events P3 0.60\[0.3\] Make sure InputEvent was
shipped without substantial issue \[0.4\] Rewrite all layout tests into WPT
through Input Automation API \[0.3\] Follow and fix incoming bugs

## \[PUNTED\] Synthetic pointer actions in WebDriver P4 \[0.3\] Design doc for
exposing the synthetic pointer actions in webdriver \[0.4\] Implementation of
the synthetic pointer actions in webdriver \[0.3\] Exposing the functionality in
WebDriver Js

### Code Health 0.71

## Target input events to either the RenderFrame or RenderView not both P1
1.00Make input event targeting on Chrome IPC consistent. 21 messages are sent to
the RenderFrame, and the remaining goto the RenderView on two different IPC
channel identifiers.

## Point person for MUS hit test API P1 0.70\[0.4\] Propose and agree on Mojo
Interface \[0.6\] Implementing the interface

## Move input messages to be mojo based P2 0.70\[0.4\] Define a mojo
InputHandler interface to the compositor thread and convert existing messages
\[0.4\] Define a mojo InputHandlerHost interface and convert existing messages
\[0.2\] Cleanup existing layering not necessary

## Complete switch to pointer events input flow in Blink code P2 0.45\[0.4\]
Replace all occurrences of WebTouchEvent in Blink with unpacked WebPointerEvent.
\[0.3\] Move grouping of touch events to the end of pipeline using raf signal
\[0.2\] Align WebMouseEvent attributes with WebPointerEvent \[0.1\] Merge
WebMouseEvent into WebPointerEvent

## Using WM_POINTER for all input types on Windows 8+ P2 0.40\[0.5\] Listen to
WM_POINTER to handle all mouse events to replace WM_MOUSE on Windows 8+ \[0.5\]
Listen to WM_POINTER to handle all touch events to replace WM_TOUCH on Windows
8+

### Performance 0.42

## Hold the line on input performance P1 0.50No regressions in key metrics that
last a milestone and monitor usage of touch-action

## Add richer set of metrics to input pipeline P1 0.30\[0.3\] Add
Event.Latency.Scroll(Begin|Update).(Touch|Wheel).TimeToScrollUpdateSwapBegin2 to
UKM \[0.2\] Do the initial round of analysis on RAPPOR data gathered last
quarter for input metrics and file at least 2 bugs for slow sites found via
RAPPOR \[0.2\] Prepare a document explaining a flow for analyzing UKM data from
dashboard or raw data \[0.3\] Compare the Rappor and UKM and decide what we will
be doing with RAPPOR metrics \[Towards the end of the quarter gathered data was
deleted due to mistakenly gathering some reports in incognito mode. We will have
more data next quarter for analyzing.)

## Quantify accuracy of Expected Queueing Time metric P2 0.50Measure how closely
actual queueing time correlates to expected queueing time. End-of-quarter update
- patch up here: <https://codereview.chromium.org/2954473002/>.

## \[PUNTED\] EGL frame timestamps for input latency metrics P4 \[0.7\] Utilize
Android O frame-timing data \[0.3\] Compare the new metric with the old
TimeToScrollUpdateSwapBegin2 metrics and write a doc explaining the results

## Scrolling Related 0.58

### Add Richness to Web Platform 0.60

## document.rootScroller P1 0.50https://crbug.com/505516 \[0.5\] Finish an
implementation that’s usable for AMP case \[0.2\] Preliminary spec describing
the new implementation \[0.2\] \[PUNTED to 61\] Ship origin trial in M60 \[0.1\]
Create example demos

## scroll-boundary-behavior P1 0.50\[0.4\] Implement scroll-boundary-behavior
for viewport (i.e. to prevent navigation) in M60 behind a flag \[0.4\] Implement
scroll-boundary-behavior for inner scrollers (i.e., chaining) on both cc and
main thread \[0.2\] Ship the final API in M61

## \[PUNTED\] Unify On-Screen Keyboard Resize Behavior \[Stretch\] P4
https://crbug.com/404315 \[0.2\] Determine if existing implementation is
sufficiently flexible to support app-like scenarios \[0.3\] Propose any missing
APIs needed for PWA like scenarios \[0.3\] Create GitHub explainer and demos
(similar to URL bar resizing) \[0.2\] Fully implemented by M61 feature freeze

## Rationalize viewport coordinates on the web P2 1.00Note: Added at mid quarter
check-in \[0.3\] Create WPT suite for visual viewport API \[0.2\] Ship
ViewportAPI in M61 (<https://www.chromestatus.com/feature/5737866978131968>)
\[0.5\] Ship inert visual viewport in M61

### Improve Scrolling UX 0.74

## Ensure Overlay Scrollbar launch is Successful P1
0.90<https://crbug.com/307091> \[0.3\] Land in M59 \[0.4\] Address all P0/P1
implementation issues \[0.3\] All polish items in the spreadsheet are addressed

## Supoort Windows precision touchpad P3 0.10\[0.6\] Use Windows API to mark
events coming from a precision touchpad \[0.4\] Support gesture navigation on
windows touchpad crbug.com/647140

### Performance 0.60

## Touchpad Scroll latching P1 0.60<https://crbug.com/526463> \[0.3\] Latching
logic should work for OOPIFs \[0.3\] Improve test coverage: unittests of all the
classes with the flag must cover both latching and propagation cases, and no
flaky layouttests \[0.2\] Enable the flag on the waterfall by default and
address any regression bugs. \[0.2\] \[Punted to 61\] Ship a finch trial in M60

## Async wheel events P1 0.60<https://crbug.com/706175> #WP-Alignment \[0.3\]
\[Punted to 61\] Finish the implementation behind a flag in M60 \[0.2\] Design
doc for wheel scroll latching and async wheel events \[0.3\] Enable the flag on
the waterfall by default and address any regression bugs. \[0.2\] Ship a finch
trial by EOQ

## \[Punted\] MouseWheel listeners on scrollers out of the scroll chain
shouldn’t block scrolling P4 <https://crbug.com/700075> \[0.2\] Add metrics to
have an approximation of cases that scrolls are blocked on listeners that are
not part of the scroll chain. \[0.6\] Implement tracking the wheel event per
individual scroller by EOQ \[0.2\] Ship a finch trial

### Improve Code Health 0.38

## Unify main and compositor scrolling paths P2 0.90<https://crbug.com/476553>
\[0.4\] Create design doc outlining a path to scroll unification between main
and impl threads \[0.2\] Create detailed implementation plan/design doc for
reraster-on-scroll \[0.4\] Share with relevant teams, iterate and come to
consensus shared vision

## Use scroll gestures for all user scrolls P2 0.00\[0.5\] \[Punted\] Keyboard
Gesture Scrolling \[0.2\] Autoscroll \[0.3\] Scrollbar Gesture Scrolling

## Unify Mac scrollbars P3 0.00\[0.4\] Remove scrollbar paint timer \[0.4\]
Decouple ScrollAnimatorMac from painting \[0.2\] \[Stretch\] Paint Mac
Scrollbars from WebThemeEngine

## Decouple Scrollbars from ScrollableAreas in Blink P3
0.50<https://crbug.com/661236> \[0.6\] Move all scrollbar generation/management
code out of ScrollableArea \[0.2\] Move scrollbar creation for main frame to
happen in RootFrameViewport \[0.2\] Fix blocked issues
<https://crbug.com/456861>, <https://crbug.com/531603>

### Improve threaded rendering code health

## \[Punted\] LTHI Scroll logic refactoring P2 \[0.75\] Design doc \[Punted\]
\[0.25\] An internal scroll manager that isolates scrolling logics in LTHI
\[Punted\] Bokan@ has a new idea, so the unifying work does not make sense
anymore.

## Threaded Rendering Related 0.53

### Enable rich scroll effects 0.60

## Finish position:sticky P1 0.80#wp-ergonomics #wp-predictability \[0.6\] fix
all current P2+ bugs (5 bugs) \[0.4\] Upstream web platform tests for spec’d
testable exposed behavior

## Scroll Snap Points P1 0.20Land the spec-matching version (behind flag)

## CSS OM Smooth Scroll P2 1.00Ship CSSOM View smooth scroll API (enabled by
default)

## \[Punted\] Scroll-linked animations polyfill P3 #wp-ergonomics wait for the
existing (external) editors to have something ready for review, then look at an
implementation and/or polyfill strategy at that point \[0.6\] Implement JS
polyfill \[0.3\] Implement CSS polyfill \[0.1\] Publish on shared team github
\[PUNTED\]. Waiting for Mozilla people to push this.

### Enable performance isolated effects on the web (AnimationWorklet) 0.46

## Implement AnimationWorklet M1 features behind flag P1 0.50\[0.2\] Implement
blink::WorkletAnimation (single timeline, multiple effects) \[0.2\] Implement
Animator and Animator definition in Worklet scope \[0.2\] Implement
cc::WorkletAnimationPlayer \[0.1\] Plumb mutation signal and current time to
worklet \[0.1\] Plumb WorkletAnimation blink =&gt; compositor =&gt; worklet
\[0.1\] Proxy Keyframe effects in Worklet scope and plumb local times worklet
=&gt; compositor \[0.1\] Get required changes to web-animation spec upstreamed
\[stretch\] Implement ScrollTimeline in blink \[stretch\] Run animation worklet
on dedicated thread Result: progress made on first 4 items. Will continue
working on in Q3.

## \[Punted\] Schedule animation calls at commit deadline P2 This needs to be
rolled into the animation worklet plan as the new API and integration with
animations will change how we schedule updates.

## \[Stretch\] Ship origin trial P3 0.30- amp or fb as potential clients Result:
flackr@ implemented a polyfil that can be used to experiment with API while
implementation is not complete. Blocked on majidvp@ updating the documentation
and making contacts.

### Make compositing excellent 0.58

## Resolve at least as many Hotlist-ThreadedRendering bugs as incoming P1 1.00

## Promotion of opaque fixed (and sticky) position elements P1 0.20\[0.6\]
Resolve consistent scroll position between composited and non composited
elements \[0.4\] Enable by default, 4% more composited scrolls

## Enable compositing border-radius with tiled masks P1 0.90\[0.2\] Investigate
if mask tiling can be disabled on AA bugs - Done \[0.5\] Implement demoting
logic - Done \[0.2\] Disable mask tiling on bugs and rare cases (AA, filters) -
Done \[0.1\] Stretch - Antialising bug (both RPDQ and content quads, but on RPDQ
it’ll be a regression) - not urgent if tiling is disabled. - Done Discuss with
paint team on whether blocked by a pre-existing paint bug. - Done Landing CLs
that enable the flag. - Reverted due to regression.

## Draft plan shared with team for raster inducing animations P2 Punted for now.

## Prototype compositor side hit testing (shared with paint team) P2
0.70Documented plan for touch action. Needs more work on browser side when
handling the white-listed touch actions.

## Triage - track incoming vs redirected / fixed / closed P2 0.00The app-engine
tool path looks a lot longer than it should.

## Enable reporting subpixel scroll offsets back to blink P3 0.00- Fix issue
456622 - bokan@ can provide consulting

### Identify opportunities / improve understanding of threaded rendering limitations in the wild 0.46

## Metrics about interactions with scrollers P2 0.80\[0.5\] Land UMA metrics
regarding size of scroller on page load and on scroll \[0.3\] Use results from
the metrics to launch a Finch trial to see the differences w, w/o compositing
small scrollers \[0.2\] Document and plan for the next step

## Analyze CPU/GPU cost due to layer explosion P3 0.50\[0.3\] Use current UMA
metrics to analyze CPU cost \[0.4\] Land new metrics regarding GPU memory cost
due to layer creation \[0.3\] Analyze GPU cost

## Analyze main thread scrolling reasons; from uma data added in previous
quarter P3 0.20\[0.2\] Resume the non composited main thread scrolling reasons
recording \[0.4\] Identify and resolve top remaining reason after opaque fixed /
sticky promotion. \[0.2\] Work on the case that transform with non-integer
offset cases \[0.2\] Work on the case that translucent element without text

## Save composited filter result (18% pages have filter according to CSS
popularity) P3 0.30\[0.4\] How often do we have a composited filter, land UMA
metrics \[0.3\] Timing metrics \[0.3\] Stretch - Figure out a way to apply the
animated-blur machinery in chromium to improve the perf

## Perf party P3 0.60\[0.4\] File 3 bugs. \[0.4\] Identify resolution of those
bugs. \[0.2\] Fix them.

## Publish three UI pattern blog posts P3 0.00\[0.25\] animated blur \[0.25\]
directional shadow \[0.5\] hero transition in scrolling content (google music)

### Improve threaded rendering code health

## \[Punted\] LTHI Scroll logic refactoring P2 \[0.75\] Design doc \[Punted\]
\[0.25\] An internal scroll manager that isolates scrolling logics in LTHI
\[Punted\] Bokan@ has a new idea, so the unifying work does not make sense
anymore.

## ==2017 Q1==

## Eventing Related 0.70

### Improve Smoothness and Predictability of Input Events 0.65

Align Events with Appropriate Lifecycle P1 0.50#wp-performance 0.3 Interpolate
touchmove events on the main thread 0.2 Analyze rAF Aligned Touch finch trial
data 0.2 Ship rAF Aligned Touch Input 0.3 Compositor vsync aligned input finch
trial

Intervene to improve scroll performance P2 1.00#wp-alignment #wp-performance 0.2
- Decrease fraction of page views which prevent default touch events without
having a touch-action from 0.66 to 0.57% 0.8 - \[PUNTED\] Land and evaluate
finch trial forcing event listeners to be passive if the main thread is
unresponsive. (<https://crbug.com/599609>)

Hold the line on input performance P2 0.85#wp-performance 0.7 No regressions in
key metrics that last a milestone 0.3 Monitor usage of touch-action

Measure Latency of Keyboard Input P2 0.50Report Latency of Keyboard Input via
UMA Did a bunch of cleanup making this easier. Patch in progress here:
https://codereview.chromium.org/2756893002/

Handle Input Events as soon as possible P3 0.50#wp-performance 0.5 Place all non
event based IPC messages in the main thread event queue so we don’t have to rely
on message pumps to process input events 0.5 Coordinate with the blink scheduler
so it can ask the main thread event queue if it has work to do.

### Add Richness to Web Platform 0.78

Ship Coalesced Points API P1 1.00#wp-ergonomics 0.5 Ship Coalesced Points API in
M58 0.5 Adding at least 3 tests in WPT for different aspect of the feature

Drive Pointer Events L2 to Recommendation P1 0.80#wp-ergonomics 0.3/0.5 Drive
solving all the new L2 Blocking issues; end quarter with zero issues 0.5/0.5
Prepare the full test result for all vendors and the explainer doc for the
failures

Make mouse a first-class event in Android P1 0.750.1/0.1 Address major user
concerns around mouse rerouting by M56 release. <https://crbug.com/675339>
0.3/0.3 Fix text selection triggering with mouse. <https://crbug.com/666060>
0.0/0.2 Enable page zoom through mouse. <https://crbug.com/681578> 0.2/0.2
Update old-style mouse click code. <https://crbug.com/669115> ---/0.2 \[PUNTED\]
Unify mouse & gesture triggered text selection in Android.

Input Automation for testing P2 0.40#wp-predictability 0.5 Get consensus on
input automation APIs 0.3 Implement the plumbing to use the input automation in
wpt serve 0.2 Make all Pointer Event tests to make use of this API

Round out stylus support on all platforms P2 0.900.3 Ship PointerEvent.twist and
.tangential on Mac <https://crbug.com/679794> 0.1 Complete Wacom Intuos Pro
support for Mac <https://crbug.com/649520> 0.4 Switch Windows (&gt;= 8)
low-level path to use WM_POINTER <https://crbug.com/367113> 0.2 Add missing
plumbing for stylus properties in Windows <https://crbug.com/526153>

Add Richness to Editing events P2 0.70#wp-ergonomics 0.3 Get a consensus
internally how to ship beforeinput/input 0.4 Finish cleanup and remaining bugs
0.3 Intent to Ship

Drive Adoption of auxclick P3 1.00#wp-predictability 0.5 Auxclick is implemented
by at least one other vendor 0.5 Click event is no longer fired for middle
button by at least one other vendor

Consensus on inputmode design P3 0.30#wp-fizz 0.5 Write up options and
distribute and build consensus 0.5 Build consensus in external WICG;
<https://crbug.com/248482>

### Product Excellence 0.78

Be responsive to our users P1 0.90Triage all new Input, Scroll issues within 7
days Triage all Hotlist-Input-Dev &gt; 10 starred issues every 90 days to ensure
we drive resolutions. Fix 5 top starred issues <https://goo.gl/fMt74P>
specifically <https://crbug.com/161464>, <https://crbug.com/25503>,

Fix touch interactions hiccups with PointerEvent P1 1.000.5/0.5 Fix multi-finger
panning with touch-action <https://crbug.com/632525> 0.4/0.4 Disable touch slop
suppression for touch-action:none. <https://crbug.com/593061> 0.1/0.1 Fix CrOS
device issues with touch-like stylus. <https://crbug.com/682144>,
<https://crbug.com/691310>

Address misc implementation issues with PointerEvents P2 1.000.3/0.3 Video
default event handler problem with PointerEvents. <https://crbug.com/677900>
0.3/0.3 PointerEvent should set movementX & movementY.
<https://crbug.com/678258> 0.4/0.4 Fix 5 other chromium bugs with PointerEvents.

Ignore clicks on recently-moved iframes (crbug.com/603193) P2 0.000.4 Add tests
and fix bugs in prototype 0.4 Land prototype behind a flag 0.2 Extend prototype
to handle OOPIF

Address long standing mouse event quirks P3 0.60#wp-predictability 0.5
Consistent behavior of zero-movement mousemoves plus hover states.
<https://crbug.com/488886> 0.5 MouseLeave on all platforms working correctly.
<https://crbug.com/450631>

### Code Health and Future Design 0.47

Simplify event processing P2 0.20#wp-architecture 0.5 Remove touchmove
throttling in the touch event queue 0.5 Remove fling curve generation from the
main thread

Continue removing artificial layering in event pipeline P2 0.80#wp-architecture
0.6 Remove PlatformMouseEvent and PlatformTouchEvent 0.4 Cleanup
PageWidgetEventDelegate

Improve Blink event bookkeeping P2 0.500.0/0.4 Event-handling states on frame vs
page. <https://crbug.com/449649> 0.3/0.3 InputRouter event queue cleanup:
<https://crbug.com/600773>, <https://crbug.com/601068> 0.2/0.3 Design doc on
PointerEvent driven event handling in Blink: <https://crbug.com/625841>

Make coordinates consistent for all Web pointer-type events P3 0.700.3/0.4 Make
WebMouseEvent coordinates fractional <https://crbug.com/456625>. 0.4/0.4 Remove
refs to deprecated location data in WebMouseEvent <https://crbug.com/507787>.
0.0/0.2 Normalize coordinate systems between WebMouseEvent and WebTouchPoint.

Update pointer/hover media query API & internal usage P3 1.000.5/0.5 Collect
data & possibly drop support for hover:on-demand. <https://crbug.com/654861>
0.5/0.5 Update mouse/touch detection code to use media queries.
<https://crbug.com/441813>

MUS Main Thread Hit Testing P3 0.00Support MUS team by providing API for hit
testing against the blink tree.

Touch-action hit testing P3 0.00#wp-architecture, #wp-performance Have a
concrete design of how SPV2 will information necessary for touch-action hit
testing on the compositor. EOQ update: no design doc in place.

## Scrolling Related 0.58

### Eliminate key developer pain points 0.44

Polish document.rootScroller design P1 0.40#wp-fizz \[0.5\] Ship
document.rootScroller origin trial in M57 \[0.2\] Provide demos and motivating
cases on how to use document.rootScroller \[0.2\] Create WICG repo to host
standardization process and engage with other vendors \[0.1\] Publicize the
experiment via developer outreach channels #wp-devx

Fix layout vs visual viewport discrepancy in window APIs P2 0.20Description:
#wp-predictability \[0.2\] Publish doc describing the issue and summarizing the
current situation \[0.4\] Get feedback from a senior Safari engineer on whether
they’re likely to change their viewport model and if they like the “inert”
viewport mode. \[0.4\] Ship either “inert visual viewport” or change “client”
coordinates to be visual in M58

Ship Overscroll​ ​Action​ ​API P1 0.60\[0.4\] Land drafted overscroll-action API
in M58 behind a flag \[0.2\] Get consensus for new scroll-boundary API \[0.3\]
Implement new scroll-boundary API \[0.1\] Ship a final API in M58.

Ship unified OSK model on Android in M58 P3 0.40\[0.2\] Document behavior of
other browsers \[0.3\] Verify interactions with fullscreen, orientation, split
screen, web app mode. Fix bugs and write web-platform style tests. \[0.5\] Ship
in M58

### Improve wheel scroll performance 0.63

Support of touchpad scroll latching on all platforms behind a flag. P1
0.70#wp-performance \[0.5\] Implement touchpad latching behind a flag on all
platforms in M58 \[0.5\] Ship a finch trial by EOQ (not a finch trial?)

Support of async wheel events with only the first event cancellable behind a
flag. P2 0.30#wp-performance \[0.7\] Implement async wheel events behind a flag
in M58 \[0.3\] Ship a finch trial by EOQ

### Code Health 0.00

Remove scrollbar code duplication P3 0.00#wp-architecture \[0.5\] Mac Scrollbars
painted in same paths as Aura/Android (<https://crbug.com/682209>) \[0.5\] Move
scrollbars out of ScrollableArea (<https://crbug.com/682206>)

### Understand how users scroll 0.55

Collect UMA stats for how users scroll P1 1.00\[0.5\] Measure how often users
scroll with scrollbars in M57. \[0.3\] Measure how often users scroll using
keyboard, wheel, touch in M58. \[0.2\] Measure how often users scroll with auto
scroll in M58.

Process is in place to analyze Rappor metric results for scroll latency P1
0.10#wp-performance \[0.4\] Get Rappor metric data analyzed and aggregated in
UMA or some other database \[0.4\] Generating appropriate charts in the Rappor
dashboard or Locally \[0.2\] Use Rappor data to investigate at least one related
issue if anything comes up

### Reliable Web Platform Scrolling 0.40

End quarter with 5 fewer Pri=0,1,2 scrolling bugs P2 0.40#wp-predictability
Currently at 99, query: http://go/input-dev-scrolling-bugs

### Implement Aura Overlay Scrollbars 0.95

Turn on Overlay Scrollbars in CrOS in M59 by default P1 1.00

Prioritise and fix all polish related issues P2 0.80Issues tracked in
<https://docs.google.com/spreadsheets/d/13pt4tM4Prm7WSVL_bAtdGN6XKHPhvEYoYSvjwH-CGTU/edit#gid=0>

Be responsive in working with PMs and UI review team 1.00

## Threaded Rendering Related0.45

### Accelerate more CSS animations 0.15

Accelerate transform animations containing percentages P2
0.10<http://crbug.com/389359> MQU: Discussed with Paint team, no potential
problems found. Otherwise, no progress EOQ: No further progress

Accelerate independent transform properties P2 0.20MQU: Investigation done,
seems good to go. Needs design doc, no implementation yet. We now support the
value 'none', and updated spec and Blink (main-thread) for smooth animation
to/from none. EOQ: No further progress.

Accelerate background-position P3

### Enable more rich scroll effects 0.47

Polish position sticky P1 0.700.7 - all P1 bugs older than 30 days fixed in M58
\[100\] 0.3 - shared test suite uploaded to csswg-tests \[50\]

Implement Scroll Snap Points for composited scrolling in M58 behind a flag P2
0.400.2 - design what information needs to be sent to CC 0.2 - match the new CSS
properties and box alignment model 0.2 - Implement snap info on main and send to
CC 0.4 - Implement snapping for touch scrolling on compositor

Scroll-linked animation polyfill P2 0.50Polyfill of proposed API written to
support / inform the spec. A demonstrating demo. Polyfill WIP.

Support main thread scroll customization P3 0.00Implement declarative API to
allow main thread scroll customization without affecting all scrolling. This is
required for an original trial experiment. \[PUNTED\]

Implement CSSOM Smooth Scroll P2 0.80Implement CSSOM Smooth Scroll and collect
UMA metrics along with implementation

### Improve performance-isolation on the web. 0.36

Fix known bugs in existing AnimationWorklet / Compositor Worker implementation
P1 0.25Fix any remaining bugs and polish the interface and performance: -
document.scrollingElement is not working (<https://crbug.com/645493>) -
occassional renderer freeze (<http://crbug.com/647035>)

Batch property updates to compositor proxies enabling running on a different
thread. P1 0.40

Fix known bugs in AnimationWorklet spec P1 0.50We have a number of ship-blocking
issues outstanding. Drive resolution to have an MVP animation worklet draft spec
by EOQ

Implement new AnimationWorklet API P3 0.00This is gated on resolving
specification issues and is therefore marked as stretch (we can use the polyfill
in the interim).

\[stretch\] Document / share plan for receiving input in animation worklet P4
0.50With plans for how this will work in Salamander. A plan was shared at
blinkon to no objections for a rudimentary WIP API but objections have been
raised regarding sharing this more broadly at houdini f2f. Need more feedback
externally.

### Improve Threaded Rendering Code Health 0.10

Factor cc scrolling logic out of LTHI P3 0.00It should be possible to factor
scrolling logic out of LTHI 0.5 Write design doc and create consensus around
doing this clean up 0.5 Implement ScrollingHost to be responsible for scrolling
logic in LTHI

Investigate flakiness in threaded-rendering tests P3 0.40We have heard from
animation team that some test are failing when used with threaded compositing.
We need to investigate why and decide if it is worth more effort and formulate a
solution

Refactor AnimationWorklet plumbing to use animation machinery 0.00Adapt existing
machinery for web-animation where appropriate reaching stage #3 as described in
this initial design doc:
https://docs.google.com/document/d/1q8ubhpeOvDOQk-BwthZzOcaZOvhWj7D-dTVYxPOvHYM/edit

### Make compositing excellent 0.51

Fix 16 open Hotlist-Threaded-Rendering bugs P1 0.80

Track whether we're closing more Hotlist-Threaded-Rendering bugs than are opened
P1 0.00

Scroll on compositor thread more frequently on desktop. P1 0.60Increase the
percentage of scrolls on the compositor thread by 5%. Ideas include making it
possible to promote opaque fixed position elements, non text containing elements
, border-radius (without memory hit, 80% completed). Blocking bug for mixing
composited and non composited content was not resolved.

Experiment not compositing small scrollers P2 0.74We have seen during perf
parties that small composited scrollers can cause layer explosion. We should
launch a Finch trial tracking memory and performance tradeoff.

### Identify / demonstrate best performance web practices 0.67

Implement and publish 2 new UI patterns P1 1.00Expect to get ~40k views per
post. #wp-devx

Identify effects used in 5 native applications which should be web apps P3 0.00

Analyze one existing framework and identify potential pitfalls / improvements
\[stretch\] P3 0.00

### Improve understanding of the limitations and tradeoffs of threaded rendering in the wild. 0.38

Analyze CPU costs as layer count increases based on UMA data P2 0.20

Fix metric measuring difference between compositor frame rate and main thread
frame rate P2 0.20tdresser@ found that
scheduling.browser.mainandimplframetimedelta2 was not accurate. We should fix
this so we can accurately track the upper bound on slippage. tdresser
investigating whether rAF aligned input fixes this. We found particular
instances of slippage in a trace which reported no slippage, but code looks like
it should be correct. Needs further debugging.

Measure time to move compositor frame from renderer to display compositor P2
0.80- Did measurement on Daisy CrOS and local machine. - Added Microbenchmark. -
Added UMA for aggregation and draw frame in display compositor.

Perf Party: compositing edition P2 0.40Case study specific to GPU/compositing w/
write-up and action items. 2 this quarter.

Measure additional GPU cost due to layer promotion P3 0.20Ensure we can track
regressions due to layer explosion.
