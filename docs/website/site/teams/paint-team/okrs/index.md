---
breadcrumbs:
- - /teams
  - Teams
- - /teams/paint-team
  - Paint Team (paint-dev)
page_name: okrs
title: Paint team OKRs
---

## Q1 2017

Code health
[#wp-predictability](https://easyokrs.googleplex.com/search/?q=wp-predictability)

Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   P1# of open bugs did not increase during the quarter
*   Owner: [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   Starting point: 1350 on Jan 3, 2017
*   P2Reduce # of failing layout tests, layout test fix SLA
*   Owner: [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   Starting point: 58 on Jan 3, 2017
*   P1No regressions reach the M57 or later stable channel
*   Owner: [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   Should this be M56 and later, given it has not gone to stable yet?

Improve quality of implementation
[#wp-predictability](https://easyokrs.googleplex.com/search/?q=wp-predictability)
[#wp-architecture](https://easyokrs.googleplex.com/search/?q=wp-architecture)

*   P2Top-starred bugs fixed
*   Fix at least the following"
    \* White flash: <http://crbug.com/470669> (chrishtr)
    \* Blurry fractional posn text: <http://crbug.com/521364> (trchen)
    \* Jittery images: <http://crbug.com/608874> (schenney)
    \* border-radius children: <http://crbug.com/157218> (schenney)
    \* imageSmoothingQuality not right: <http://crbug.com/618324> (junov)
    \* flickering dithered content: <http://crbug.com/226753> (fmalita)
    \* raster scale heuristics: <http://crbug.com/652448> (chrishtr)
    \* SVGTextContentElement.getSubStringLength(): <http://crbug.com/622336>
    (pdr)
    \* Mac controls painting bugs: 607438, 611753, 158426 (schenney)
*   P2Remove need for FrameView::forceLayoutParentViewIfNeeded in SVG
*   Owner: fs@opera.com

Understand, maintain and improve performance
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)

Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   P1Measure performance and prevent any performance regressions from
            reaching the stable channel
*   Owner: [wkorman](https://easyokrs.googleplex.com/view/wkorman/)

Improve paint/compositing interfaces
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)
[#wp-architecture](https://easyokrs.googleplex.com/search/?q=wp-architecture)

Owners: [ajuma](https://easyokrs.googleplex.com/view/ajuma/),
[chrishtr](https://easyokrs.googleplex.com/view/chrishtr/),
[paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
[wangxianzhu](https://easyokrs.googleplex.com/view/wangxianzhu/),
[wkorman](https://easyokrs.googleplex.com/view/wkorman/)

*   P1SPv2 works for all effects, including mask
*   Owner: [trchen](https://easyokrs.googleplex.com/view/trchen/)
*   P1SPv2 works for scrolling and scrollbars
*   Owner: [pdr](https://easyokrs.googleplex.com/view/pdr/)
*   P1SPv2 works for animation metadata and touch event regions
*   Owner: [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   P1PaintArtifactCompositor layerization as good as
            PaintLayerCompositor
*   Owners: [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/),
            [wangxianzhu](https://easyokrs.googleplex.com/view/wangxianzhu/)
*   P1Finish removing target knowledge from clip and transform trees
*   Owners: [ajuma](https://easyokrs.googleplex.com/view/ajuma/),
            [jaydasika](https://easyokrs.googleplex.com/view/jaydasika/)
*   P1Cache dynamically-determined clips
*   Owner: [jaydasika](https://easyokrs.googleplex.com/view/jaydasika/)
*   P1Stop requiring a cc::Layer for any property tree nodes
*   P2Build the RenderSurfaceLayerList from effect trees
*   Owner: [ajuma](https://easyokrs.googleplex.com/view/ajuma/)
*   P1Launch SlimmingPaintInvalidation
*   Owner:
            [wangxianzhu](https://easyokrs.googleplex.com/view/wangxianzhu/)
*   P1Design and prototype implementation of border-radius clipping in
            SPv2
*   Owners: [ajuma](https://easyokrs.googleplex.com/view/ajuma/),
            [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/)
*   Stretch additional goal: implement & land it

Support other teams' work

Owners: [chrome-gpu](https://easyokrs.googleplex.com/view/chrome-gpu/),
[paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   Promote more opaque scrollers and fixed-position elements in the
            presence of LCD text
*   Owners:
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
            [threaded-rendering](https://easyokrs.googleplex.com/view/threaded-rendering/)
*   threaded-rendering: implement
    paint-dev: support
    Fix discovered compositing bugs, re-enable fixed position promotion. Promote
    scrollers with border-radius, box shadow, and scrollers/sticky/fixed
    elements containing no text.
*   P1Apply border radius clipping to non-stacking descendants
*   Owners:
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
            [threaded-rendering](https://easyokrs.googleplex.com/view/threaded-rendering/)
*   crbug.com/157218
    paint-dev: implement feature
    threaded-rendering: implement tiled masks for efficiency
*   P2Implement color correction in blink
*   Owners:
            [chrome-gpu](https://easyokrs.googleplex.com/view/chrome-gpu/),
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)
*   ccameron: implementation
    paint-dev: support
*   P3Fully threaded scrolling with touch-action
*   Owners:
            [input-dev-eventing](https://easyokrs.googleplex.com/view/input-dev-eventing/),
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
            [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   paint-dev, input-dev: design for touch-action in SPv2
*   P3Simplify paint usage of Sk\*Shaders
*   Owners:
            [chrome-gpu](https://easyokrs.googleplex.com/view/chrome-gpu/),
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)
*   This is to support easier implementation of custom display lists

Code Health and Future Design

Owner:
[input-dev-eventing](https://easyokrs.googleplex.com/view/input-dev-eventing/)

*   P3Touch-action hit testing
*   Owners: [dtapuska](https://easyokrs.googleplex.com/view/dtapuska/),
            [flackr](https://easyokrs.googleplex.com/view/flackr/),
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
            [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   [#wp-architecture](https://easyokrs.googleplex.com/search/?q=wp-architecture),
            [#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)
    Have a concrete design of how SPV2 will information necessary for
    touch-action hit testing on the compositor.

Support other teams' work

Owners: [chrome-gpu](https://easyokrs.googleplex.com/view/chrome-gpu/),
[paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   Promote more opaque scrollers and fixed-position elements in the
            presence of LCD text
*   Owners:
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
            [threaded-rendering](https://easyokrs.googleplex.com/view/threaded-rendering/)
*   threaded-rendering: implement
    paint-dev: support
    Fix discovered compositing bugs, re-enable fixed position promotion. Promote
    scrollers with border-radius, box shadow, and scrollers/sticky/fixed
    elements containing no text.
*   P1Apply border radius clipping to non-stacking descendants
*   Owners:
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
            [threaded-rendering](https://easyokrs.googleplex.com/view/threaded-rendering/)
*   crbug.com/157218
    paint-dev: implement feature
    threaded-rendering: implement tiled masks for efficiency
*   P2Implement color correction in blink
*   Owners:
            [chrome-gpu](https://easyokrs.googleplex.com/view/chrome-gpu/),
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)
*   ccameron: implementation
    paint-dev: support
*   P3Fully threaded scrolling with touch-action
*   Owners:
            [input-dev-eventing](https://easyokrs.googleplex.com/view/input-dev-eventing/),
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
            [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   paint-dev, input-dev: design for touch-action in SPv2
*   P3Simplify paint usage of Sk\*Shaders
*   Owners:
            [chrome-gpu](https://easyokrs.googleplex.com/view/chrome-gpu/),
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)
*   This is to support easier implementation of custom display lists

## Q4 2016

Code health [#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   1.00P1# of open bugs did not increase during the quarter
*   Owners: [schenney](https://easyokrs.googleplex.com/view/schenney/),
            [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   Mid-quarter update: on track. 1383 -&gt; 1385, including addition of
            a new component.
    End of quarter: 1383 on Sept 29, 2016 to 1350 on Jan 3, 2017.
*   0.70P1No regressions reach the M55 or later stable channel
*   Owner: [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   Still on track.
    End of quarter update:
    2 Regression reached M54, not really part of OKR:
    <https://bugs.chromium.org/p/chromium/issues/detail?id=655941>,
    <https://bugs.chromium.org/p/chromium/issues/detail?id=662378>
    One regressions reached M55 specifically.
    M56 not to stable yet.
*   0.10P2Reduce # of failing layout tests, layout test fix SLA
*   Owners: [schenney](https://easyokrs.googleplex.com/view/schenney/),
            [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   1. Reduce # of steady-state failing layout tests by 50%
    2. Fix all new layout test regressions within 2 weeks
    Directories:
    LayoutTests/paint/...
    LayoutTests/compositing/... (except compositing/animations)
    LayoutTests/svg/...
    LayoutTests/hittesting/...
    LayoutTests/fast/images/...
    LayoutTests/css3/images/...
    LayoutTests/canvas/...
    Mid-quarter-update: Negligible overall reduction this quarter.
    TODO(schenney) check lifetime of new regressions
    End of Quarter: No significant reduction, total count increased
    significantly due to random order test flakiness. Regression tracking
    process failed.
    Non-zero score because we did go down except for factors beyond our control.
    35 -&gt; 34 before random-order jump.

<img alt="History" src="https://easyokrs.googleplex.com/images/history.png"><img
alt="Copy" src="https://easyokrs.googleplex.com/images/copy.svg">[<img
alt="Link"
src="https://easyokrs.googleplex.com/images/link.png">](https://easyokrs.googleplex.com/o/2336130/)

0.56P2Improve quality of implementation
[#wp-predictability](https://easyokrs.googleplex.com/search/?q=wp-predictability)
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   0.56P2Top-starred bugs fixed
*   Owner: [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/)
*   Fix at least these: 521364, 470669, 608874, 157218, 618324, 226753,
            652448, 468497, 622336
    Mid-quarter update: some progress, but needs work. 468497 is done. Review
    for 470669 currently in review. 521364 still in progress. 157218 in
    progress, close to patches landing. 652448 in progress, vmpstr doing good
    work to support.
    End of quarter:
    \* 521364: CLs in review
    \* 470669 fixed, bug blocked on fix for crbug.com/21798.
    \* 608874 no progress
    \* 157218 landed Dec 12, reverted Dec 13, re-landed January 5, perf issues
    and 2 bugs still open.
    \* 618324 no progress
    \* 226753 investigated, no longer repros on later OS X updates. Probably not
    going to fix.
    \* 652448 prototyped, but concluded design proposal is not feasible in SPv1
    \* 468497 fixed
    \* 622336 no progress
    Fixed/close to fixed: 3
    mostly there: 2
    WontFix: 1
    No progress: 3
    Total score: (1.0\* 3 + 0.75 \* 2 + 0 \* 3) / 8 = 0.56
*   1.00P2SVG filter hack in FrameView is gone
*   Owner: fs@opera.com
    Mid-quarter: on track! Code in review.
*   0.00P2Remove need for FrameView::forceLayoutParentViewIfNeeded in
            SVG
*   Owner: fs@opera.com
    Mid-quarter: ?
*   0.00P2Widget hierarchy is gone
*   Owners: [szager](https://easyokrs.googleplex.com/view/szager/),
            [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   crbug.com/637460
    Mid-quarter: not yet started.
    EOQ: no progress

1.00P1Understand, maintain and improve performance
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)

Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   1.00P1Measure performance and prevent any performance regressions
            from reaching the stable channel
*   Owner: [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   Mid-quarter update: on track. Only thing being tracked is the Linux
            regression on M54 stable.
    EOQ: still going well
*   1.00P1Composited opaque scrolling for low-DPI screens
*   Owner: [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   Mid-quarter update: basically done. Launching with M56.
    End of quarter: Landed for M56. Paint update time drops evident in UMA data.
    No outstanding open issues.

0.58P2Improve paint/compositing interfaces
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)
[#wp-architecture](https://easyokrs.googleplex.com/search/?q=wp-architecture)

Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   0.25P1SPv2 works all effects, including mask
*   Owner: [trchen](https://easyokrs.googleplex.com/view/trchen/)
*   Mid-quarter update: needs more work. implemented for filters. Masks
            not done.
    Overall, also blocked on creating render surfaces in SPv2.
    EOQ: some progress, but still a ways to go. Some design thought about masks.
*   0.00P1SPv2 works for scrolling and scrollbars
*   Owner: [pdr](https://easyokrs.googleplex.com/view/pdr/)
*   Mid-quarter update: needs attention. no progress this quarter.
    EOQ: no progress.
*   0.50P1SPv2 works for animation metadata and touch event regions
*   Owner: [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   Mid-quarter update: needs attention. no progress this quarter.
    EOQ: design investigation, early implementation.
*   1.00SPv2 compatible ancestor-dependent and descendant-dependent
            PaintLayer values
*   Owner: [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/)
*   Mid-quarter update: needs attention. no progress this quarter.
    EOQ: done! Design and implementation in place.
*   0.75P2SPv2 paint invalidation test infrastructure
*   Owner: [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/)
*   Mid-quarter update: landed changes to unify paint invalidation
            output format between SPv1 and SPv2. Tests not converted yet.
    EOQ: done! Design and implementation in place. Tests not converted yet, but
    that wasn't in the KR description.
*   0.60P1PaintArtifactCompositor property tree optimization
*   Owner: [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/)
*   Passes all compositing tests with equivalently performant
            layerization
    Mid-quarter update: needs attention. no progress this quarter.
    EOQ: designed implemented the optimizations. But not all compositing tests
    passing yet.

0.46P1Finish cc property tree implementation
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

Owners: [ajuma](https://easyokrs.googleplex.com/view/ajuma/),
[jaydasika](https://easyokrs.googleplex.com/view/jaydasika/),
[paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
[sunxd](https://easyokrs.googleplex.com/view/sunxd/)

*   0.75P1Finish removing target knowledge from clip and transform trees
*   Owner: [jaydasika](https://easyokrs.googleplex.com/view/jaydasika/)
*   Mid-quarter update: transforms done! Calculating clips done (behind
            a flag), just needs caching implementation.'
    EOQ: transforms done, clips getting close.
*   1.00P1Cache dynamically-determined transforms
*   Owners: [ajuma](https://easyokrs.googleplex.com/view/ajuma/),
            [sunxd](https://easyokrs.googleplex.com/view/sunxd/)
*   Mid-quarter update: done! TODO(ajuma): check if property update UMA
            metrics moved due to this.''
    EOQ done.
*   0.50P1Stop requiring a cc::Layer for any property tree nodes
*   Owners: [ajuma](https://easyokrs.googleplex.com/view/ajuma/),
            [jaydasika](https://easyokrs.googleplex.com/view/jaydasika/)
*   Mid-quarter update: on track. some progress, but work to do.
    EOQ: not done. still several references.
*   0.00P1Build the RenderSurfaceLayerList from effect trees
*   Owner: [ajuma](https://easyokrs.googleplex.com/view/ajuma/)
*   Mid-quarter update: at risk. no progress yet, still consumed with
            other work.
    EOQ: no progress
*   0.50Cache dynamically-determined clips
*   Owner: [jaydasika](https://easyokrs.googleplex.com/view/jaydasika/)
*   Mid-quarter update: no progress yet, work starting soon.
    EOQ: implementation getting close
*   0.00P2Simplify transform property nodes
*   Owner: [ajuma](https://easyokrs.googleplex.com/view/ajuma/)
*   1. Only one transform per node
    2. No caching (tracked via "Cache dynamically-determined transforms"
    however)
    Mid-quarter update: no progress.
    EOQ: no progress

P2New features

Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   0.10P2Drive tentative standards agreement on a raster scale spec
            [#wp-ergonomics](https://easyokrs.googleplex.com/search/?q=wp-ergonomics)
*   Raster scale defines the scale at which to raster composited layers
    Note: crbug.com/652448 tracks doing this automatically for background
    images, and is tracked in another
    KR.
    Mid-quarter update: no progress.
    EOQ: prototyping of automatic version for images didn't work out. Going to
    revert code. No progress
    any standards discussion.
*   0.00P2Fix all bugs blocking backdrop-filter launch
            [#wp-predictability](https://easyokrs.googleplex.com/search/?q=wp-predictability)
*   Owner: [jaydasika](https://easyokrs.googleplex.com/view/jaydasika/)
*   All dependent bugs of crbug.com/497522 fixed.
    Mid-quarter update: no progress so far.
    EOQ update: no progress.

0.93P1Improve paint invalidation correctness and performance
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)

Owners: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
[wangxianzhu](https://easyokrs.googleplex.com/view/wangxianzhu/)

*   Mid-quarter update: on track. lots of progress simplifying the code
            and fixing bugs.

*   0.75P1Launch slimmingPaintInvalidation
*   Owner:
            [wangxianzhu](https://easyokrs.googleplex.com/view/wangxianzhu/)
*   Mid-quarter update: on track. Close to a complete implementation.
    EOQ: launched to experimental status
*   1.00P1Implement incremental paint property tree update
*   Owners: [pdr](https://easyokrs.googleplex.com/view/pdr/),
            [wangxianzhu](https://easyokrs.googleplex.com/view/wangxianzhu/)
*   Mid-quarter update: on track. Code mostly done and design complete.
    EOQ: done!
*   1.00P1Implement all SPv1 invalidation features in
            slimmingPaintInvalidation mode, or deprecate them if obsolete
*   Owner:
            [wangxianzhu](https://easyokrs.googleplex.com/view/wangxianzhu/)
*   Mid-quarter update: on track. Lots of progress.
    EOQ: done!

## Q3 2016

## Code health [#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

## Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
[paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
[schenney](https://easyokrs.googleplex.com/view/schenney/)

*   ## 1.00P2# of open bugs did not increase during the quarter
*   ## Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
            [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   ## Baseline starting point: 1323 open bugs as of July 8 2016 for
            these labels:
    ## Blink&gt;Paint, Blink&gt;Paint&gt;Invalidation, Blink&gt;Compositing,
    Blink&gt;SVG, Blink&gt;Canvas, Blink&gt;Image
    ## Mid-quarter update: on track. Bugs down to 1295 (2.1% decrease)
    ## EOQ update: 1311 as of 9/29 (plus 54 for the new HitTesting component)
*   ## 1.00P17-day SLA for triaging all incoming bugs
*   ## Owners:
            [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/),
            [junov](https://easyokrs.googleplex.com/view/junov/),
            [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   ## Another owner: fs@opera.com
    ## Mid-quarter update: on track. Holding around zero.
    ## EOQ update: met goal.
*   ## 0.75P1No regressions reach the M52 or later stable channel.
*   ## Owner: [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   ## Definition: bug is a regression that ends up bisected to a commit
            after the M52 branch point, yet made its way
    ## to the stable channel.
    ## Mid-quarter update: at risk. We already have one; crbug.com/635724 might
    leak to M53 stable.
    ## EOQ update: crbug.com/646363 made it to stable M53. crbug.com/637556 made
    it to M52 stable. No bug reached stable due to a triage failure. Problem was
    late reporting of the bug.
*   ## 1.00P3Monitor # of failing layout tests
*   ## Owner: [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   ## Directories: fast/repaint/\*, paint/\*, compositing/\*, svg/\*,
            fast/canvas/\*, canvas/\*
    ## Mid-quarter update: on track.
    ## EOQ update: met
*   ## 1.00P2Document an official triage process
*   ## Owners:
            [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/),
            [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   ## Mid-quarter update: on track.
    ## EOQ update: met

## 0.40Improve quality of implementation
[#wp-predictability](https://easyokrs.googleplex.com/search/?q=wp-predictability)
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

## Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   ## 0.40P1Top-starred bugs fixed
*   ## Owner:
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)
*   ## Fix at least these: 521364, 470669, 608874, 157218, 618324,
            226753, 409868, 470608
    ## Update: at risk. Only one of these bugs is fixed so far.
    ## EOQ update:
    ## - 521364 is close to done
    ## - 470669 unblocked by dependent bug a few days ago
    ## - 608874 no progress
    ## - 157218 has a prototype fix
    ## - 618324 not clear how to fix yet
    ## - 226753 no progress
    ## - 409868 WontFixed
    ## - 470608 Fixed.
    ## Score: 2/8 done, 2/8 close to done, 4/8 not done: 2/8 \* 1 + 2/8 \* 0.75
    = 0.43 ~ 0.4
*   ## 0.60P2SVG filter hack in FrameView is gone
*   ## Owner:
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)
*   ## Ref: Document::hasSVGFilterElementsRequiringLayerUpdate
    ## Owner: fs@opera.com
    ## Update: at risk. More work upcoming.
    ## EOQ update: patch to finish it coming very soon; some good preparatory
    refactoring done.
*   ## 0.20P2Remove need for FrameView::forceLayoutParentViewIfNeeded in
            SVG
*   ## Owner:
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)
*   ## Owner: fs@opera.com
    ## Update: at risk. trchen is also helping with this one.
    ## EOQ update: some promising prototyping, but a ways t go.
*   ## 0.40P2Geometry code for layout and paint better-written and
            documented
*   ## Owners:
            [blink-layout](https://easyokrs.googleplex.com/view/blink-layout/),
            [szager](https://easyokrs.googleplex.com/view/szager/),
            [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   ##
            [#wp-predictability](https://easyokrs.googleplex.com/search/?q=wp-predictability)
            [#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)
    ## Shared between the paint and layout teams.
    ## Update: at risk. Plan in place for removing the Widget hierarchy, but not
    much progress yet.
    ## EOQ update: reworked flipped-block logic in
    mapToVisualRectInAncestorSpace(); initial exploration to rework
    Widget/FrameView trees; documentation of writing modes in layout/README.md
    in flight.

## 0.97Understand, maintain and improve performance
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)

## Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   ## 1.00P1Measure performance and prevent any performance regressions
            from reaching the stable channel
*   ## Owner: [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   ## Mid-quarter update: on track.
    ## EOQ: met
*   ## 1.00P2Composited scrolling for low-DPI screens
*   ## Owner: [schenney](https://easyokrs.googleplex.com/view/schenney/)
*   ## Mid-quarter update: on track. Lots of progress. Should be able to
            turn it on for M55.
    ## EOQ: met
*   ## 1.00P1Use compositor RTrees rather than cached SkPictures
*   ## Owner: [wkorman](https://easyokrs.googleplex.com/view/wkorman/)
*   ## Mid-quarter update: done!
    ## EOQ: met
*   ## 0.80Partial repaint telementry test implemented
*   ## Mid-quarter update: at risk. No progress yet, work upcoming this
            quarter.
    ## EOQ: in code review.

## 0.67Improve paint/compositing interfaces
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

## Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   ## 0.40P1New (SPv2) compositing integration works for scrolling and
            non-mask effects
*   ## Owners:
            [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/),
            [pdr](https://easyokrs.googleplex.com/view/pdr/),
            [trchen](https://easyokrs.googleplex.com/view/trchen/)
*   ## All scrolling (assuming root layer scrolls) and compositing tests
            pass up to masking.
    ## Mid-quarter update: on track for scrolling, at risk for effects.
    ## EOQ update: scrolling almost done. No progress yet on effects.
*   ## 1.00P1Paint invalidation tree walk within the pre-paint tree walk
            fully implemented
*   ## Owners:
            [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/),
            [wangxianzhu](https://easyokrs.googleplex.com/view/wangxianzhu/)
*   ## Mid-quarter update: done!
    ## EOQ update: met
*   ## 0.60P1GeometryMapper implements fast paint invalidation, paint
            clip rects, and interest rects.
*   ## Owner: [chrishtr](https://easyokrs.googleplex.com/view/chrishtr/)
*   ## Mid-quarter update: on track. Lots of progress.
    ## EOQ update: done for paint invalidation, CL almost done for paint clip
    rects. Interest rests not yet done.

## 0.58Finish cc property tree implementation
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

## Owner: [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/)

*   ## 0.90P1Finish removing target knowledge from clip and transform
            trees
*   ## Owners:
            [jaydasika](https://easyokrs.googleplex.com/view/jaydasika/),
            [weiliangc](https://easyokrs.googleplex.com/view/weiliangc/)
*   ## Mid-quarter update: On track for transforms, at risk for clips.
            Computation of transforms without in-tree target knowledge has been
            implemented behind a flag; for clips, there are unit test failures
            to sort out (where the "correct" answer isn't necessarily what the
            test currently expects), and work is being transitioned to new
            owners.
    ## EOQ update: done for transforms (1.0), nearly done for clips (0.8), so
    0.9 overall.
*   ## 0.40P1Cache dynamically-determined draw properties
*   ## Owner: [sunxd](https://easyokrs.googleplex.com/view/sunxd/)
*   ## Mid-quarter update: On track for transforms, at risk for clips.
            Currently evaluating performance for transforms, but clips are
            blocked on the previous KR.
    ## EOQ update: nearly done for transforms (0.8) and not started for clips
    (0) so 0.4 overall.
*   ## 0.00P3(stretch) RenderSurface determination on the impl thread
*   ## This depends on the previous two KRs being finished, but is not a
            lot of additional effort.
    ## Mid-quarter update: no progress.
    ## EOQ update: not done
    ## EOQ update: ?
*   ## 0.60P1Stop requiring a cc::Layer for any property tree nodes
*   ## Owners: [ajuma](https://easyokrs.googleplex.com/view/ajuma/),
            [jaydasika](https://easyokrs.googleplex.com/view/jaydasika/)
*   ## Mid-quarter update: On track. Most of the dependencies have been
            removed.
    ## EOQ update: Work for routing animation updates directly to property trees
    is nearly done; most of the direct dependencies of render surfaces on owning
    layers have been removed, but RSLL iteration still needs to be converted to
    use layer lists and effect nodes.

## 1.00New features

*   ## 1.00P2Support custom paint
            [#wp-future](https://easyokrs.googleplex.com/search/?q=wp-future)
*   ## Code reviews, design reviews, etc.
    ## Mid-quarter update: on track. (nothing to do)
    ## EOQ update: met
*   ## 1.00P2Unprefix -webkit-clip-path
            [#wp-predictability](https://easyokrs.googleplex.com/search/?q=wp-predictability)
*   ## owner:fs@opera.com
    ## Mid-quarter update: on track, code complete already. Sending an intent to
    ship soon.
    ## EOQ update: met

## Code health [#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

## Owner: [blink-layout](https://easyokrs.googleplex.com/view/blink-layout/)

*   ## P2Reduce life cycle violations
*   ## Owners: [atotic](https://easyokrs.googleplex.com/view/atotic/),
            [paint-dev](https://easyokrs.googleplex.com/view/paint-dev/),
            [wangxianzhu](https://easyokrs.googleplex.com/view/wangxianzhu/)
*   ## [#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)
    ## Reduce life cycle violations in the rendering pipeline.
    ## Shared between the paint and layout teams.

## Q2 2016

Code health

Owner: paint-dev

*   0.50P2Reduce total open bugs by 2%
*   Owners: chrishtr, junov, schenney
*   Mid-quarter update: actual result is up by 1%. Wont Meet.
    EOQ grading notes: actual is up by 1.8%. Score=0.6 for doing ok at fixing
    bugs, but lots more bugs keep coming in via triage from other labels.
*   0.90P1Triage all legacy bugs, and triage all new bugs within a 1
            week SLA. Enforce priority 1 meaning.
*   Mid-quarter update: triaging SLA is On Track. Priority 1 bug hygiene
            Needs Work.
    EOQ grading notes: we met the SLA almost every week. Blink&gt;Compositing in
    particular slipped a couple of times.
*   0.70P1Prevent any regressions from reaching the M51 or above stable
            channel
*   Mid-quarter update: M51 just went to stable, so no direct evidence
            yet of success or failure. Therefore On Track.
    EOQ notes: still not sure about M51. needs some bug digging. Tentative score
    of 0.7
*   0.66P2High-priority / most-starred bugs fixed
*   Fix at least these:
    <http://crbug.com/596382>
    <http://crbug.com/521364>
    <http://crbug.com/470669>
    Mid-quarter update: 596382 is fixed (with followup to re-enable safely). The
    other two bugs have some progress
    but Need Work.
    EOQ update: 596382 is fixed. 521364 is halfway done. 470669 is halfway done.
    Final score: 0.66 (averaging).

0.40P1Measure performance

Owner: paint-dev

*   0.00P1Implement a partial repaint Telemetry test
*   <http://crbug.com/527189>
    Mid-quarter update: no progress. Needs Work.
    EOQ update: nothing done.
*   0.80P1Monitor UMA stats for paint, raster, paint invalidation and
            compositing. Don't allow regressions into the stable channel.
*   Owner: wkorman
*   Mid-quarter update: monitoring is On Track, not allowing regressions
            Needs Work.
    EOQ update: Stats monitored on regular basis, no currently known stable
    channel regressions M51 (may need more investigation to be 100% sure?)

1.00Performance and complexity improvements

Owner: paint-dev

*   1.00P2Re-implement reflection using filters
*   Owner: jbroman
*   Launch by end of quarter
    Mid-quarter update: implemented and pretty much working. On Track.
    EOQ update: done, in M53. Score 1.0.
*   0.70P2Composite scrolling on low-DPI screens when possible
*   Owner: schenney
*   Launch by end of quarter. Tracked in crbug.com/381840
    Mid-quarter update: partially implemented. Need to fix bugs in current
    implementation and commit it. Needs Work.
    EOQ update: All the necessary work done to set the layer as composited, and
    resulting Blink bugs fixed, but blocked on enabling background and
    foreground painting into the same composited layer when the background is
    opaque in order to actually get LCD text on the layer.
*   1.00P1New DOM-order-based paint invalidation tree walk
*   Owner: wangxianzhu
*   Launch by end of quarter
    Mid-quarter update: finished on current PaintInvalidationState. Needs work
    to migrate to PrePaintTreeWalk (PaintPropertyTreeBuilder). On Track.
    EOQ update: Done. Score 1.0.

P1Property trees / SPv2 plumbing

Owners: paint-dev, pdr, trchen

*   0.50P1Property trees in Blink are complete up to the deficiencies of
            the paint artifact compositor.
*   Owners: pdr, trchen
*   Transform trees are complete (including backface visibility,
            flattening, and transform-style).
    Effect trees are complete (including reflection, clip-path, mask).
    Root layer scrolling supported using unittests.
    All of compositing/, fast/, svg/ passing or failing for known&labeled
    reasons like paint artifact compositor deficiencies.
    This includes multicolumn.
    Implement the scroll tree.
    Mid-quarter update: Needs Work. Some progress on transform tree semantics
    and SVG clip paint properties, but a lot more work to do.
    EOQ update: lots of measurable progress was made (transform edge-cases,
    filter tree preparation, html and svg difficult cases) but we still fail a
    significant number of tests (~3100/16728 of fast/ & compositing/ & svg/).
*   1.00P1All direct dependencies in cc on the cc::LayerImpl tree are
            gone, replaced with property trees
*   Owners: jaydasika, sunxd, vollick
*   Mid-quarter update: On Track. Lots of great progress.
    EOQ update: Done!
*   0.30P2RenderSurface determination is on the impl thread
*   Owners: vollick, weiliangc
*   Update render target info and draw properties after render surface
            determination.
    Mid-quarter update: Partial work. Rest blocked by target information still
    on clip/transform tree. Wont Meet
    EOQ update: blocked on other KRs, but also not done.
*   0.60P1Transformed layers can be sent as a layer list with property
            trees, rather than a hierarchical layer tree
*   Owners: ajuma, jbroman
*   Mid-quarter update: progressing, with a partial implementation. On
            Track.
    EOQ update: blink-side: score 0.5 can do trivial translations with a layer
    list (and trivial trees), but I'm still working on building non-trivial
    ones. cc side: 1.0. Total: 0.5 (most work is in Blink)
*   0.40P2SPv2 paint invalidation is in place and works behind the
            feature flag for SPv2
*   Owner: wangxianzhu
*   Mid-quarter update: no progress yet. Needs Work.
    EOQ update: Plumbed SPv2 paint invalidation onto spv1 paint invalidation and
    it basically works, but it's still far from the goal of spv2 paint
    invalidation.
*   0.50P1Remove target knowledge from Clip tree and Transform tree
*   Owner: weiliangc
*   Remove target_id and \*_in_target_space in Clip and Transform tree.
    Remove main thread cc's use to to_target transform, use to_screen transform
    instead (including visible rects calculation, sublayer scale, snapping)
    Dynamically calculate draw properties with target info from Effect tree.
    Blocks both Blink/cc API and RenderSurface determination on impl thread.
    Mid-quarter update: Progressing. Needs Work.
    EOQ update: progressing, but still needs plenty of work.
*   0.20P3Cache dynamically calculated draw properties
*   Owner: weiliangc
*   Blocked by Clip and Transform tree clean up and Render Surface
            determination on impl thread.
    Critical for performance, do not block API implementation.
    Mid-quarter update: Still blocked by dynamic calculation. Wont Meet.
    EOQ update: Only moved a small part of properties.

1.00P2Support goals of other teams

*   1.00Support custom paint
*   Mid-quarter update: On Track. All custom paint patches are
            committed.
    EOQ update: Done. Score 1.0.
*   1.00Support Blink -&gt; Chromium style change, and Onion Soup
            cleanups
*   Mid-quarter update: On Track. We're reviewing everything coming our
            way.
    EOQ update: Done. Score 1.0.

0.40P1Geometry code improvements for performance, less redundancy, simplicity
and correctness

Owner: paint-dev

*   0.70P1RTrees used to optimize layer rasterization, instead of
            SkPicture replay
*   Owners: chrishtr, wkorman
*   Mid-quarter update: down to about 12 failing layout tests, plus a
            small unknown regression in raster time. On Track.
    EOQ update: Close to landing for M53, work still underway on &lt; 10 failing
    layout tests.
*   0.40P2GeometryMapper implemented and used to implement features in
            Blink
*   Owners: chrishtr, wkorman
*   1. SPv2-compatible
    2. PaintLayerClipper
    Mid-quarter update: prototype partial implementation of GeometryMapper in
    progress. Needs Work.
    EOQ update: First patch committed. Still plenty to do. Score 0.4.
*   1.00P1Partial raster for Blink launched
*   Mid-quarter update: done, On Track.
    EOQ update: On track. Score 1.0.

0.30Improve SVG integration into the document lifecycle

*   0.50P2SVG filter hack in FrameView is gone
*   Ref: Document::hasSVGFilterElementsRequiringLayerUpdate
    Owner: fs@opera.com
    Mid-quarter update: some progress. Needs Work.
    EOQ update: some progress, but not quite there.
*   0.10P2Remove need for FrameView::forceLayoutParentViewIfNeeded in
            SVG
*   Owner: fs@opera.com
    Mid-quarter update: no real progress. Wont Meet.
    EOQ update: some analysis performed, but no work yett

## Q1 2016

Code health

Owner: paint-dev

*   Reduce total open bugs by 5%
*   Owners: chrishtr, junov, schenney
*   Document all new features in Markdown and in the code, as we write
            it
*   Implement pixel capture on Cluster Telemetry
*   Owner: pdr
*   crbug.com/536285
*   Triage all legacy bugs, and triage all new bugs within a 1 week SLA
*   Owners: chrishtr, junov, schenney

Measure and improve performance

Owner: paint-dev

*   Implement a partial repaint Telemetry test
*   Owner: weiliangc
*   crbug.com/527189
*   Improve performance of paint aggregated across Cluster Telemetry by
            a total of 10%
*   Owner: wangxianzhu
*   Monitor existing UMA stats at the bi-weekly meeting
*   Owner: wkorman

Progress towards Slimming Paint v2

Owner: paint-dev

*   Move RenderSurface determination to the impl thread
*   Owner: weiliangc
*   Finish by M51 branch point
*   Replace all direct dependencies in cc on the cc::LayerImpl tree with
            property trees
*   Owners: jaydasika, sunxd
*   Finish by M51
*   Complete implementation of paint property trees
*   Owners: pdr, trchen
*   Passes most compositing/ layout tests.
    Passes multicolumn tests.
    Passes scrolling tests for the root frame.
*   Implement a correct (but not yet performance-optimized) version of a
            new Blink compositing integration
*   Owner: jbroman
*   Passes most compositing/ layout tests.
    Passes multicolumn tests.
    Passes scrolling tests for the root frame.
    Passes video tests.
*   Common implementation of property trees
*   Owner: chrishtr
*   Designdoc completed. Code implemented and working integration with
            cc (may not yet be committed)
*   Make use of RTrees to optimize layer rasterization, instead of
            SkPicture replay
*   Owner: wkorman
*   Implement a SPv2-compatible paint invalidation tree walk
*   (..that is integrated into the paint properties or paint tree walk)

Support goals of other teams

Owner: paint-dev

*   Support custom paint implementation
*   Owner: chrishtr
*   Support paint-related loading metrics
*   Owner: chrishtr
*   Investigate OOPIF to see if there are paint-related design problems
            or bugs
*   Owner: pdr
*   Support conversion of Blink to Chromium style

Code quality and cleanup

Owner: paint-dev

*   Delete CDP
*   Owner: weiliangc
*   Delete selection gap code
*   Owner: wkorman

## Q4 2015

## Improve bug triage and fix rate (grade: 0.46)

*   ## This objective apples to the following labels:
            Cr-Blink-Paint-Invalidation, Cr-Blink-Paint, Cr-Blink-Canvas, and
            Cr-Blink-Compositing.

*   ## Reduce open bug count by 10%
*   ## Triage 100% of all new bugs with a 1-week SLA
*   ## Note: this goes along with a web-platform-wide OKR with the same
            goal.
*   ## Triage or re-triage 50% of all bugs filed before Q3 2015
*   ## Note: this goes along with a web-platform-wide OKR with the same
            goal.
*   ## Define and agree upon an ongoing triage process and standards for
            the paint team
*   ## This includes instructions on how to treat priority, Type, Cr
            labels, etc.

## <img alt="Revision History"
src="https://easyokrs.googleplex.com/images/history.png"><img alt="copy"
src="https://easyokrs.googleplex.com/images/copy.svg">

## Improve documentation of paint code (grade: 0.13)

*   ## Migrate all feature documentation on the sites page to Markdown
            files
*   ## Add Markdown files describing clearly major architectural pieces
            of Paint
*   ## This includes forward-looking documentation of the SPv2
            implementation, and important codebase-wide conventions.

## Launch components of SlimmingPaintV2 (grade: 0.68)

*   ## All KRs below are met if they are done and launched in M49.

*   ## Launch SlimmingPaintSynchronizedPainting
*   ## Launch SlimmingPaintSubsequenceCaching
*   ## Launch SlimmingPaintOffsetCaching
*   ## Launch compositor thread property trees
*   ## Includes deleting all of CDP
*   ## Launch removal of DisplayItemList-&gt;SkPicture replay
*   ## Launch the cc effect tree

## Implement the new compositing path for Slimming Paint v2 (grade: 0.2)

*   ## Implement the paint properties design
*   ## crbug.com/537409
    ## Includes: paint properties tree walk, definition and generating of
    property trees, and homogeneous DisplayItemList with just drawings.
*   ## Implement layer compositing in Blink on top of paint properties
            and paint chunks
*   ## (Stretch) Implement squashing and overlap testing in the
            compositor thread

## canvas.toBlob, callback version of the API (grade: 0.8)

*   ## Have an async implementation that has its scheduling controlled
            by Blink
*   ## Refactor the png encoder integration into skia (SkCodec) in a way
            that allows the workload to be divided into small units of work for
            the purpose of Idle scheduling.
*   ## Stress test should show that a long-running png image encode does
            not interfere with a lightweight 60fps animation on a low spec
            device (2 core CPU).
*   ## (Stretch) Refactor JPEG and WebP encoder integrations into skia,
            and pass the same test described above with these encoders.
*   ## (Stretch) Ship it

## ImageBitmap (grade: 0.94)

*   ## ImageBitmap is transferable to Workers
*   ## ImageBitmaps can be created on Workers
*   ## Calls to createImageBitmap always &lt; 5ms for 1k-by-1k canvas on
            Android One
*   ## Async response always &lt; 300ms for 1k-by1k canvas with no
            animation running (network delays notwithstanding) on Android One
*   ## (Stretch) Ship it

## Ship Display List 2D canvas everywhere (grade: 0.7)

*   ## Use Display lists in cases where GPU-acceleration is available
*   ## Google Sheets triggering display list mode on all platforms
*   ## (Stretch) Google Sheets stays in display list mode while
            scrolling the document

## OffscreenCanvas feature implementation (grade: 0.35)

*   ## ImageBitmapRenderingContext capable of displaying an ImageBitmap
            without making a copy of the image data
*   ## OffscreenCanvas can be created
*   ## (Stretch) 2D context can be created on an offscreen canvas
*   ## (Stretch) Functional ImageBitmap flow: OffscreenCanvas on a
            worker can transfer rendered content to an
            ImageBitmapRenderingContext on main thread

## Canvas product stability (grade: 0.40)

*   ## Eliminate all OOM crashes caused ImageData APIs
*   ## Reduce by 50% 2D context losses caused by GPU context resets

## Support work to start implementing custom paint (grade: 1.0)

*   ## Review code and designs for a new canvas rendering context
            interface for custom paint
*   ## (Stretch) Review code for implementation of custom paint
*   ## The Houdini team does not expect to do any work along these lines
            until December at the earliest.

## Support work to define time-to-first paint (grade: 1.0)

*   ## This is an OKR to support work by the loading team to improve
            time-to-first-paint metrics. Tracked in crbug.com/520410

## Improve telemetry testing of paint (grade: 0.0)

*   ## Add a telemetry test for incremental paint
*   ## Tracked in crbug.com/527189.
*   ## Add a telemetry benchmark to capture pixels
*   ## This will allow us to run pixel diff tests on the top 10k sites
            in Cluster Telemetry. Tracked in crbug.com/536285.

## Q3 2015

Launch compositor thread property trees (grade: 0.5)

*   Tracked in crbug.com/481585

*   CalcDrawProps not used at runtime
*   Launch in Chrome &lt;=47
*   Stretch: Delete CalcDrawProps and fix unit test dependencies
*   There is a substantial amount of cleanup here, which we should not
            block launch on.

Major progress finishing implementation of Slimming Paint phase 2 (grade: 0.24)

*   Tracked in crbug.com/471333
    Fully achieving this OKR will be a stretch!

*   New Blink-&gt;cc API complete
*   Tracked in crbug.com/481592
*   Implementation of basic layerization
*   Tracked in crbug.com/472842
    \*Not\* included: smart code to only re-raster or re-build only the parts of
    the layer tree that are needed.
*   Implement "full" display lists in Blink
*   Tracked in crbug.com/472782. Includes property trees, diff
            representation, pixel refs interest rects and RTrees.
*   LayoutObject-based invalidation

Product excellence: improve bug triage and fix rate (grade: 0.6)

*   Triage all paint bugs (with label Cr-Blink-Paint or
            Cr-Blink-Paint-Invalidation or paint-related compositor code)
*   Reduce number of unfixed paint bugs by 10%

Remove selection gap painting (grade: 0.40)

*   Implement representation of newlines in selection
*   Tracked in crbug.com/474759
*   Remove selection gap painting, & resulting code cleanup

Support implementation of stopping rAF / lifecycle for non-visible, cross-domain
iframes (grade: 1.0)

*   Tracked in crbug.com/487937. Primary implementation seems likely to
            come from scheduler-dev.

Implement OffscreenCanvas (grade: 0.13)

*   Synchronous mode of operation (using ImageBitmap) functional for 2D
            contexts.
*   Ship behind experimental flag in M47

Ship canvas filters (stretch) (grade: 0.38)

*   Can be used by Swiffy to improve efficiency of Flash-&gt;JS
            conversion of Flash code that uses filters.

*   No longer experimental in M47
*   Optimized to apply filter only on the region of interest, in all
            applicable use cases

Refactor 2D canvas (grade: 0.9)

*   Remove &gt;1000 lines of code by eliminating dependency on
            SkDeferredCanvas and simplifying ImageBufferSurface
*   Start using display lists for GPU-accelerated canvases by M47

Ship ImageBitmap (stretch) (grade: 0.15)

*   ImageBitmap no longer experimental
*   ImageBitmap is transferable to workers
*   ImageBitmap creation is truly async

Accelerate the use of fonts in 2D canvas (grade: 1.0)

*   When font was previously used (cache hit), setting the 'font'
            attribute should take &lt; 1 microsecond on all platforms, for all
            use cases

## Q2 2015

Launch Slimming Paint phase 1 (grade: 0.9)

*   Tracked in crbug.com/471332

*   Launched on all platforms, with no performance regressions
*   Makes the Chrome 45 cut.

Working implementation of Slimming Paint phase 2 (grade: 0.0)

*   Tracked in crbug.com/471333

*   Complete implementation of display lists
*   Complete crbug.com/472782 and crbug.com/471264.
*   Implement a working version of layerization in the compositor
*   Complete crbug.com/472842
*   Passing 90% of layout tests

Launch property trees for all cc main-thread computation (grade: 1.0)

*   M45 or die

*   Launched on all platforms, with no performance regressions
*   Complete crbug.com/386810

Design for Custom Paint (grade: 0.8)

*   Produce design document
*   Shared with Paint team
            (<https://easyokrs.googleplex.com/view/paint-dev/2015/q2/>). This
            should specify the API, its integration into Blink, fully worked out
            use cases, and an explanation of how it will benefit performance.
*   Flesh out implementation on a branch

Collaborators: blink-houdini, paint-dev

## Q1 2015

Overall Score

*   Solid progress towards a well-implemented Phase 1 launch in Q2.
            Phase 2 deliverables were way too aggressively stated.

0.50Implement DisplayItemList backing of existing cc::layers

*   0.60No performance regressions relative to prior system
*   (Score comment: raster performance appears to match, but recording
            is still 25% slower.)
*   0.00On by default in the Chrome Canary channel for at least the last
            week of the quarter
*   (Score comment: we didn't get this far.)
*   0.90Passes all layout tests
*   (Score comment: almost all layout tests actually pass, though we
            don't have all of them enabled yet.)

0.43Use property trees for all cc main-thread computation

*   0.00On by default in the Chrome Canary channel for at least one week
            at the end of the quarter
*   (Score comment: we didn't get there yet.)
*   0.30Finalized design of integration of property trees into display
            list representation
*   (Score comment: we have a design and partial implementation of
            transform trees.)
*   1.00Passes all layout tests
*   (Score comment: they all pass!)

0.00Make all composited layer decisions in cc

*   Stretch: include DisplayItemList diffs from Blink -&gt; cc
    (Score comment: we didn't get there.)

*   0.00Implement initial design behind a flag
*   We didn't get there.
*   0.00Pass layout tests in the compositing/ directory
*   We didn't get there.

1.00Consider proposals for paint callbacks and decide whether to support
implementation

*   Paint callbacks are part of the "explain css" proposals that are
            part of Glitter.)
    (Score notes: we discussed with the Houdini team about it, and now have much
    better clarity about its design and justification for ergonomics and
    potential performance, along with a Q2 OKR to create a detailed design
    document for the feature.

0.70DevTools timeline invalidation tracking ready for UI review

*   (Score comment: implemented but not yet committed.)
