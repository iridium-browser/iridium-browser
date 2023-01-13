---
breadcrumbs:
- - /developers
  - For Developers
page_name: webkit-core-projects
title: Blink Core Projects
---

**This list is for Blink work that is about improving performance, infrastructure and/or the maintainability of the codebase. This is a list of high-priority projects that have minimal dependence on web standards bodies.**
**If you decide to take on one of these projects put your name at the beginning like \[ ojan \]. Some of the bigger projects could use multiple people working on them.**
**Talk to Ojan if you have questions about any of these or want to add more things to this list. If you discover other new projects that would fit well here we should add them!

## blink/core projects

*   **Please see the [Project Warden
            list](https://code.google.com/p/chromium/issues/list?can=2&q=label%3AProject-Warden&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&cells=tiles)
            for more maintainability tasks.**
*   Identify and fix scrolling performance problems on this page:
            <http://crbug.com/157016>
*   Moving widget-loading (iframes/plugins) out of the rendering tree
            [wkb.ug/45049](http://wkb.ug/45049)
*   Add more invariants during layout to ensure proper behavior (see
            <https://bugs.webkit.org/show_bug.cgi?id=49019> and
            <https://bugs.webkit.org/show_bug.cgi?id=64222>)

## Performance

*   \[META\] RoboHornet (and RoboHornet Pro) related performance issues:
            <https://bugs.webkit.org/show_bug.cgi?id=98798>
    *   - tag-based collections are invalidated on every add/remove of
                an element (25%+ win for RoboHornetPro)
                [wkb.ug/98823](http://wkb.ug/98823)
*   Improve performance of Dromaeo and dom_perf bencharks. For example,
            &lt;<https://bugs.webkit.org/show_bug.cgi?id=99197>&gt; makes JSC go
            40% faster on dom-traverse. Can we do something similar for V8?
*   Should report private memory on the perf bots:
            <http://crbug.com/163280>
*   Improve memory usage of the following benchmarks:
    *   <http://build.chromium.org/f/chromium/perf/xp-release-dual-core/memory/report.html?history=50&rev=-1&graph=commit_charge>
    *   <http://build.chromium.org/f/chromium/perf/xp-release-dual-core/moz/report.html?history=50&rev=-1>
    *   <http://build.chromium.org/f/chromium/perf/xp-release-dual-core/intl1/report.html?history=50&rev=-1>
    *   <http://build.chromium.org/f/chromium/perf/xp-release-dual-core/intl2/report.html?history=50&rev=-1>
    *   <http://build.chromium.org/f/chromium/perf/xp-release-dual-core/bloat-http/report.html?history=50&rev=-1>
*   Create (and then optimize) benchmarks that accurately represent
            memory usage in gmail, docs, etc.
*   Improve performance test suites:
    *   first and second tabs of dromaeo benchmarks show the same data:
                <http://crbug.com/160321>
    *   page cyclers should have subgraphs for each individual site:
                <http://crbug.com/161837>
    *   add performance trybots: <http://crbug.com/120380>
    *   Linux lowmem perf bot exhibits bimodal behavior:
                <http://crbug.com/159077>
    *   explore ways to reduce performance test variance:
                <http://crbug.com/161844>

## Infrastructure

*   rebaselining with virtual test suites involved gets confused:
            [crbug.com/237701](http://crbug.com/237701)
*   Switch chromium windows bots back to using Apache:
            <https://bugs.webkit.org/show_bug.cgi?id=101373>
*   Improve garden-o-matic:
    *   let you minimize failures so they're not in your face:[
                https://bugs.webkit.org/show_bug.cgi?id=97949](https://bugs.webkit.org/show_bug.cgi?id=97949)
    *   show the last revision process by every layout test bot and the
                last by every non-layout test bot:[
                https://bugs.webkit.org/show_bug.cgi?id=98088](https://bugs.webkit.org/show_bug.cgi?id=98088)
    *   garden-o-matic should run even if you don't have the server
                running locally: [crbug.com/241506](http://crbug.com/241506)

*   **Add a tool to garden-o-matic/flakiness dashboard/code review tool
            to make it possible to easily view the diff of an expected result to
            another platform's expected file (e.g. diff between
            platform/chromium/foo-expected.txt and
            platform/mac/foo-expected.txt).**
*   Rebaseline tests from the results.html page:[
            https://bugs.webkit.org/show_bug.cgi?id=86797](https://bugs.webkit.org/show_bug.cgi?id=86797)

*   ****Flakiness dashboard****
    *   gtests should output the same JSON format as run-webkit-tests:
                [crbug.com/247192](http://crbug.com/247192)
    *   flakiness dashboard links on the waterfall point to the wrong
                data: [crbug.com/246849](http://crbug.com/246849)
    *   Annotate the failing test results shown in the flakiness
                dashboard with the revision it last failed at. Something like:
                r12345
    *   Audio failures don't show up correctly on the flakiness
                dashboard: <https://bugs.webkit.org/show_bug.cgi?id=104632>

## LayoutTests

*   Create infrastructure for red-green layout tests. They have no
            expected result. If the bitmap has only green and white pixels, it
            passes, otherwise it fails.
*   Make the text caret not blink in tests:[
            https://bugs.webkit.org/show_bug.cgi?id=97558](https://bugs.webkit.org/show_bug.cgi?id=97558)
*   using js-test-pre.js should only require a DOCTYPE and
            js-test-pre.js:[
            https://bugs.webkit.org/show_bug.cgi?id=48344](https://bugs.webkit.org/show_bug.cgi?id=48344)
*   Hundreds of mac failures are due to a slightly different color
            repaint background: <https://bugs.webkit.org/show_bug.cgi?id=83400>
*   Hundreds of MountainLion-only reftest failures:
            [crbug.com/238093](http://crbug.com/238093)
*   checkLayout() should error out if no data-expected\* attributes were
            found: [crbug.com/233956](http://crbug.com/233956)
*   Inspector tests are crazy slow in debug:
            [crbug.com/238095](http://crbug.com/238095)
