---
breadcrumbs:
- - /teams
  - Teams
- - /teams/rendering
  - Rendering Core
- - /teams/rendering/okrs
  - OKRs
page_name: 2020-q1
title: 2020-q1 OKRs
---

*   Avoid double rendering and layout shift for ‘font-display:
            optional’. crbug.com/1040632
    *   EOQ score: 0.7
*   Reduce style invalidation/recalc on @font-face loading. Ship
            improvements in M82. crbug.com/441925
    *   EOQ score: 0.8
*   Fix remaining test failures for BlockHTMLParserOnStyleSheets.
            crbug.com/891767.
    *   EOQ score: 0.95
*   Prototype relaxing parser yielding heuristics. crbug.com/1041006
    *   EOQ score: 0.95
*   Add UMA metrics for cumulative parsing time, broken down by C++ and
            JS, and cumulative amount of time spent hit testing or
            forced-layout-ing before FCP
    *   EOQ score: 0.95
*   Experimentally determine impact of compositing cross-origin iframes.
            Use results to decide next steps for isInputPending and Multiple
            Blink Isolates
    *   EOQ score: 0.7
*   Add UMA and UKM for shaper font stack traversal depth.
            crbug.com/1045571
    *   EOQ score: 0
*   Decide whether we can ship prefer-compositing-to-lcd-text.
            crbug.com/984346
    *   EOQ score: 0.5
*   Support directly composited images with CompositeAfterPaint.
            crbug.com/875110
    *   EOQ score: 0.8
*   Ship smarter RuleSet media query invalidation in M83. Fix issues
            1014920 and 589083
    *   EOQ score: 1
*   Fix all remaining test failures when CompositeAfterPaint is on. This
            includes the remaining paint-related. crbug.com/471333
    *   EOQ score: 0.7
*   Improve perf of subsequence caching, removing CompositeAfterPaint
            paint regression. crbug.com/917911
    *   EOQ score: 0
*   Add support to IntersectionObserver for clipping an explicit root.
            crbug.com/1015183
    *   EOQ score: 1
*   Reduce failures to zero for FragmentItem. crbug.com/982194
    *   EOQ score: 1
*   Enable LayoutNG for form controls. crbug.com/1040826,
            crbug.com/1040828
    *   EOQ score: 0.7
*   Implement and ship PaintArtifactSquashing in M83. Fix
            https://crbug.com/548184.
    *   EOQ score: 0.5
*   Ship text-decoration-width & text-underline-offset in M83.
            crbug.com/785230
    *   EOQ score: 0.1
*   Remove all DisableCompositingQueryAsserts for CompositeAfterPaint.
            crbug.com/1007989
    *   EOQ score: 0.6
*   Compute correct wheel event handler regions for cc.
            crbug.com/841364.
    *   EOQ score: 0
*   Fix Mac OS 10.15 system font narrow rendering regression, ship to
            M81. crbug.com/1013130
    *   EOQ score: 0.8
*   Reliable layout testing for new Mac OS versions, fonts tests
            rebaselined for 10.14 bot. crbug.com/1028242
    *   EOQ score: 1
*   Ship plan to navigate in M82. crbug.com/1013385
    *   EOQ score: 0.9
*   Refactor of the line-breaking logic to allow breaking after spaces
*   Change CSS transform implementation and spec to match Firefox's
            implementation. crbug.com/1008483
    *   EOQ score: 0.25
*   Spec ink overflow concept for IntersectionObserver.
            crbug.com/1045596
    *   EOQ score: 0
*   GridNG: design doc, commits landed. crbug.com/1045599
    *   EOQ score: 0.1
*   Ship Root element compositing changes in M81
    *   EOQ score: 1
*   Ship Style Cascade project in M82. crbug.com//947004
    *   EOQ score: 1
*   Ship color-scheme CSS property and meta tag in M81
    *   EOQ score: 1
*   Ship selector() for @supports. crbug.com/979041
    *   EOQ score: 1
*   Ship ::marker pseudo element.crbug.com/457718
    *   EOQ score: 0.9
*   Implement and ship imperative slotting API in M83. crbug.com/869308
    *   EOQ score: 0.
*   Declarative shadow DOM. crbug.com/1042130
    *   EOQ score: 0.9
*   Form Controls Refresh
    *   EOQ score: 0.75
*   TablesNG. crbug.com/958381
    *   EOQ score: 0.75
*   FlexNG. crbug.com/845235
    *   EOQ score: 1
*   FragmentationNG. crbug.com/829028
    *   EOQ score: 0.7
*   Ship contain-intrinsic-size CSS property in M82. crbug.com/991096
    *   EOQ score: 1
*   Ship render-subtree: invisible & render-subtree: invisible
            skip-activation in M82
    *   EOQ score: 0.5
*   Origin Trial for render-subtree: skip-viewport-activation and
            activation event in M82
    *   EOQ score: 0.5
*   Bug, triage, stars metrics
    *   EOQ score: 0.4
*   Ship pixel-snapped ResizeObserver rects for Canvas
    *   EOQ score: 0.5
*   Prototype composited clip path animations. crbug.com/686074
    *   EOQ score: 0
*   Ship @property in M82. crbug.com/973830
    *   EOQ score: 0.1
*   Ship multiple parts in ::part() in M82
    *   EOQ score: 1
*   Fix font cache lifecycle, runaway font memory consumption during
            animation, stable variable font animations without OOM in M82.
            crbug.com/1045667
    *   EOQ score: 0.8
*   Ship minimal named pages support in M83
    *   EOQ score: 0.5
*   Prototype aspect-ratio CSS property. Partner with AMP and
            render-subtree project on use-cases. crbug.com/1045668
    *   EOQ score: 0.6
*   Ship CSS filters on SVG. crbug.com/109224
    *   EOQ score: 0.6
*   Ship clip-path etc via external SVG resource. crbug.com/109212
    *   EOQ score: 0.6
*   Ship ‘revert’ keyword. crbug.com/579788
    *   EOQ score: 0.9
*   Incremental progress implementing custom Layout API.
            crbug.com/726125
    *   EOQ score: 1
*   Implement flexbox row-gap and column-gap in legacy and NG.
            crbug.com/762679
    *   EOQ score: 0.1
*   Investigate possible APIs and solutions to container queries
    *   EOQ score: 0.1
*   Implement input.rawValue prototype, and send Intent-to-Prototype.
            crbug.com/1043288
    *   EOQ score: 0.75
*   Finish CSS Scoping study. Finish the “problem statement” document,
            the “proposals” document, and gather feedback on both.
            crbug.com/1045645
    *   EOQ score: 0.7
