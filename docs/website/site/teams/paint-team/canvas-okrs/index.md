---
breadcrumbs:
- - /teams
  - Teams
- - /teams/paint-team
  - Paint Team (paint-dev)
page_name: canvas-okrs
title: Canvas team OKRs
---

## Q4 2016

Ship OffscreenCanvas
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)
[#wp-ergonomics](https://easyokrs.googleplex.com/search/?q=wp-ergonomics)

Owners: [chrome-canvas](https://easyokrs.googleplex.com/view/chrome-canvas/),
[junov](https://easyokrs.googleplex.com/view/junov/),
[xidachen](https://easyokrs.googleplex.com/view/xidachen/),
[xlai](https://easyokrs.googleplex.com/view/xlai/)

*   Lead: xlai

*   implement convertToBlob
*   Owner: [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   Make placeholder canvas elements work as image sources (drawImage,
            toBlob, etc)
*   Owner: [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   Ship it in M57
*   Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
            [xidachen](https://easyokrs.googleplex.com/view/xidachen/),
            [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   This includes achieving full spec conformance.
*   Make a convincing performance demo, and support devrel with launch
            PR
*   Owner: [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   Land the specification against whatwg
*   Owner: [junov](https://easyokrs.googleplex.com/view/junov/)
*   Get canvas filters to work in a Worker
*   Owner: [fserb](https://easyokrs.googleplex.com/view/fserb/)
*   Make OffscreenCanvas commit flow works in the Android WebView
*   Owner: [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   Write tests that verify that it works, fix if necessary.

<img alt="Copy" src="https://easyokrs.googleplex.com/images/copy.svg">[<img
alt="Link"
src="https://easyokrs.googleplex.com/images/link.png">](https://easyokrs.googleplex.com/o/2361706/)

OffscreenCanvas testing and tracking
[#wp-predictability](https://easyokrs.googleplex.com/search/?q=wp-predictability)

Owners: [chrome-canvas](https://easyokrs.googleplex.com/view/chrome-canvas/),
[junov](https://easyokrs.googleplex.com/view/junov/),
[xidachen](https://easyokrs.googleplex.com/view/xidachen/),
[xlai](https://easyokrs.googleplex.com/view/xlai/)

*   Lead: xidachen

*   Contribute to web platform tests
*   Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
            [xidachen](https://easyokrs.googleplex.com/view/xidachen/),
            [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   Achieve complete test coverage: Each statement in the spec should be
            covered by tests, with xrefs to the spec.
    Upstream the tests to web platform tests repo, test other implementations
    for conformance
    Main owner/coordinator is xidachen. xlai and junov are contributors.
*   Add telemetry performance tests for OffscreenCanvas
*   Owner: [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   Must cover commit() and ImageBitmap flows, cover all permutations of
            GPU/CPU rendering and compositing, run on GPU bots.
*   Add usage metrics histograms.
*   Owner: [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   Must track:
    \* OffscreenCanvas feature usage
    \* mode of operation (commit vs transferToImageBitmap)
    \* commit() code path
    \* CPU run time for each commit() code path (on the thread where commit is
    called)
    \* commit() latency (time elapsed from commit() call to UI compositor
    update)
*   Have the fastest implementation
*   Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
            [xidachen](https://easyokrs.googleplex.com/view/xidachen/),
            [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   Write animometer-style benchmarks for OffscreenCanvas (commit and
            imageBitmap flows, 2D and WebGL). Compare to other implementations.
            Stay on top
    Main owner/coordinator: xidachen. junov and xlai to contribute to fixing
    perf issues.

requestAnimationFrame in Workers
[#wp-ergonomics](https://easyokrs.googleplex.com/search/?q=wp-ergonomics)

Owners: [chrome-canvas](https://easyokrs.googleplex.com/view/chrome-canvas/),
[junov](https://easyokrs.googleplex.com/view/junov/)

*   Implement it
*   Spec it
*   Ship it in M57

Product excellence
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

Owners: [chrome-canvas](https://easyokrs.googleplex.com/view/chrome-canvas/),
[xidachen](https://easyokrs.googleplex.com/view/xidachen/),
[xlai](https://easyokrs.googleplex.com/view/xlai/)

*   Lead: xidachen

*   Weekly product excellence report
*   Owner: [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   Publish weekly report to chrome-canvas mailing list.
    Bug stats: outstanding bug count (number fixed, number of new bugs), list of
    high-profile outstanding bugs (crashers, security, regressions, highly
    starred, p0-p1), high profile bugs resolved since last report. Keep report
    history in a google doc.
    Delegate reporting when on leave.
*   Performance metrics report
*   Owner: [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   Twice in the quarter, present to paint team show salient
            fluctuations in key performance metrics.
*   Prevent bug count from creeping up.
*   Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
            [xidachen](https://easyokrs.googleplex.com/view/xidachen/),
            [xlai](https://easyokrs.googleplex.com/view/xlai/),
            [zakerinasab](https://easyokrs.googleplex.com/view/zakerinasab/)
*   (104 outstanding bugs as of October 5 2016)
*   Refactor Canvas and OffscreenCanvas dependencies on cc
*   Owners: [xidachen](https://easyokrs.googleplex.com/view/xidachen/),
            [xlai](https://easyokrs.googleplex.com/view/xlai/)

Misc canvas features
[#wp-ergonomics](https://easyokrs.googleplex.com/search/?q=wp-ergonomics)

Owner: [chrome-canvas](https://easyokrs.googleplex.com/view/chrome-canvas/)

*   Make experimental implementation of canvas colorspace work
*   Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
            [zakerinasab](https://easyokrs.googleplex.com/view/zakerinasab/)
*   Experimental implementation of text rendering in workers (stretch)
*   Owner:
            [zakerinasab](https://easyokrs.googleplex.com/view/zakerinasab/)
*   0.5 for getting text to render using only the default font.
*   Ship DOMMatrixInit-based canvas APIs (stretch)
*   Owner: [xlai](https://easyokrs.googleplex.com/view/xlai/)

## Q3 2016

OffscreenCanvas feature complete
[#wp-future](https://easyokrs.googleplex.com/search/?q=wp-future)

Owners: [chrome-canvas](https://easyokrs.googleplex.com/view/chrome-canvas/),
[junov](https://easyokrs.googleplex.com/view/junov/),
[xidachen](https://easyokrs.googleplex.com/view/xidachen/),
[xlai](https://easyokrs.googleplex.com/view/xlai/)

*   0.80P2commit() flow working for 2D canvas
*   Owner: [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   Mid-quarter status: Stalled for about 3 weeks due to dependency on
            mojo improvements. Still on track for EOQ
    EOQ status: Mostly implemented except software compositing cases
*   0.80P2commit() flow working for WebGL
*   Owner: [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   Mid-quarter status: Not started, still possible by EOQ.
    EOQ status: Mostly implemented except software compositing cases
*   1.00P3GPU-accelerated 2D canvas in a worker
*   Owner: [junov](https://easyokrs.googleplex.com/view/junov/)
*   Mid-quarter status: WIP CL almost done. On track.
    EOQ status: done.
*   0.50P1Finish specification for OffscreenCanvas (rolled over from Q2)
*   Owner: [junov](https://easyokrs.googleplex.com/view/junov/)
*   Mid-quarter status: iterated on proposal some more, still attainable
            by EOQ, moderate risk of slipping.
    EOQ: Draft complete, not yet publicly shared.
*   0.80P3Canvas readback APIs + tainting for 2D
*   Owner: [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   getImageData + toBlob + toDataURL working on OffscreenCanvas on main
            thread + Workers (stretch)
    Mid-quarter status: WIP, tainting for 2D completed, the rest partially
    implemented, at risk of slipping into Q4
    EOQ status: done, except for toBlob which is not yet implemented

<img alt="Copy" src="https://easyokrs.googleplex.com/images/copy.svg">[<img
alt="Link"
src="https://easyokrs.googleplex.com/images/link.png">](https://easyokrs.googleplex.com/o/2239617/)

0.10P3Canvas color space support
[#wp-future](https://easyokrs.googleplex.com/search/?q=wp-future)

Owners: [chrome-canvas](https://easyokrs.googleplex.com/view/chrome-canvas/),
[junov](https://easyokrs.googleplex.com/view/junov/)

*   Mid-quarter: Slow progress so far this Q, work on colorspace
            implementation is ramping up with
            [zakerinasab@](https://teams.googleplex.com/zakerinasab)

*   0.00P2Converge spec proposal
*   Owner: [junov](https://easyokrs.googleplex.com/view/junov/)
*   Close all unresolved issues about the proposal on the W3C WICG forum
    Mid-quareter status: No progress. Still attainable this Q
*   0.00P3Write spec (stretch)
*   Owner: [junov](https://easyokrs.googleplex.com/view/junov/)
*   Commit CanvasColorSpace spec against WHATWG HTML spec.
    Mid-quarter status: No progress, not likely this quarter.
*   0.30P4Prototype/experimental implementation in Blink
*   Owner: [junov](https://easyokrs.googleplex.com/view/junov/)
*   Mid-qarter status: Partially landed. Experimental API is there,
            color space plumbing in the compositor is there, not functional due
            to several missing pieces in skia, blink and compositor.

0.74P1Product Excellence
[#wp-health](https://easyokrs.googleplex.com/search/?q=wp-health)

Owners: [chrome-canvas](https://easyokrs.googleplex.com/view/chrome-canvas/),
[junov](https://easyokrs.googleplex.com/view/junov/),
[xidachen](https://easyokrs.googleplex.com/view/xidachen/),
[xlai](https://easyokrs.googleplex.com/view/xlai/)

*   0.80P2Reduce bug count
*   Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
            [xidachen](https://easyokrs.googleplex.com/view/xidachen/),
            [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   Go from 121 to under 100 open bugs (Type = Bug + Bug-Regression +
            Bug-Security)
    Mid-quarter status: 106 outstanding bugs. On track!
    EOQ status: 104 bugs.
*   P1Resolve all memory leak-like issues related to canvas
*   Owner: [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   Mid-quarter status: Out of our hands: dependency on V8 team GC
            triggers.
*   0.80P12 week SLA on fixing crashers and security bugs
*   Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
            [xidachen](https://easyokrs.googleplex.com/view/xidachen/),
            [xlai](https://easyokrs.googleplex.com/view/xlai/)
*   Mid-quarter status: On track.
    EOQ status: crasher 624718 is lagging behind. Otherwise we did good.
*   1.00P21 week SLA on triage
*   Owner: [junov](https://easyokrs.googleplex.com/view/junov/)
*   Mid-quarter status: on track
    EOQ: done.
*   0.50P3Be the best browser for all canvas-related Animometer
            benchmark scores
*   Owner: [junov](https://easyokrs.googleplex.com/view/junov/)
*   (Includes work by intern sebastienlc)
    Mid-quarter status: implemented but not enabled. Not on track because no one
    is currently owning follow-up on intern's work. Postponing.
    EOQ status: some heuristic for falling back to SW have been implemented,
    thus preventing critical performance cliffs. Global approach is not ready to
    ship.
*   0.60P2Fix top starred issues
*   Owners: [junov](https://easyokrs.googleplex.com/view/junov/),
            [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   crbug.com/618324 - Experimental \`imageSmoothingQuality\` property
            doesn't provide expected quality
    crbug.com/7508 - We don't anti-alias &lt;canvas&gt; drawImage
    Mid-quarter status: 7508 is fixed, 618324 has been investigated but will
    probably not be fixed this quarter.
    EOQ status: 1/2 bugs fixed + bonus bug 424291

Misc Features [#wp-future](https://easyokrs.googleplex.com/search/?q=wp-future)
[#wp-performance](https://easyokrs.googleplex.com/search/?q=wp-performance)

Owner: [chrome-canvas](https://easyokrs.googleplex.com/view/chrome-canvas/)

*   0.70P3Ship ImageBitmap resize
*   Owner: [xidachen](https://easyokrs.googleplex.com/view/xidachen/)
*   Mid-quarter status: on track
    EOQ staus: implementation complete, but not shipped
*   0.90P2Ship ImageSmoothingQuality
*   Owner: [junov](https://easyokrs.googleplex.com/view/junov/)
*   Mid-quarter status: done, but there are outstanding complaint about
            the quality of the mipmaps used for high quality filtering.
