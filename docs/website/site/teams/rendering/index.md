---
breadcrumbs:
- - /teams
  - Teams
page_name: rendering
title: Rendering Core
---

The rendering core team is a long-term engineering team that owns the overall
[rendering pipeline](/developers/the-rendering-critical-path) and most of the
core rendering stages. Specifically style, layout, compositing, and paint. The
team is also responsible for text, fonts, editing, canvas, images, hit testing,
and SVG.

The team is made up of contributors from many different companies and see
regular contributions from many more as well as from individual contributors.

## Team Charter

The rendering core team is focused on the architectural principles of
reliability, performance and extensibility of the core rendering technologies of
the web: HTML, DOM and CSS. We also make sure to satisfy top requests from
customers. Our primary customers are web developers and other teams within
Chrome which build features on top of rendering.

## Priorities

### Scalable Performance

*   Rendering update performance is proportional to the amount of
            change, and “amount of change” has an intuitive explanation.
*   Rendering performance of a component need not depend on where it's
            put within a containing document, or the size of that document.
*   Rendering performance of a document need not depend on the size of
            components contained within it.
*   Same goes for encapsulation - a component can be included without
            breaking a containing page, and a containing page cannot break a
            component.

### Reliability

*   Rendering features work correctly and have rational, understandable
            definitions.
*   Rendering features work the same on all platforms, and on all
            browsers.

### Extensibility

*   Web developers can extend the capabilities of Rendering in novel
            ways without performance or ergonomic penalties.
*   Chromium developers can extend or embed the Rendering code in new
            and novel ways without excessive effort or performance penalties.

## Ongoing Projects

List of ongoing major projects owned by the team or involving multiple team
members.

*   **CSS Containment**
    Ongoing work to optimize performance isolation for CSS containment.
*   **[LayoutNG](/blink/layoutng)**, issue
            [591099](https://bugs.chromium.org/p/chromium/issues/detail?id=591099).
    A new layout system for Blink designed with fragmentation, extensibility and
    interruptibility in mind.
    Phase 1 (block flow) launched in M77.
    Further layout modes (tables, flexbox, grid) and block-fragmentation support
    targeted for 2020.
*   **[Composite After
            Paint](https://docs.google.com/document/d/114ie7KJY3e850ZmGh4YfNq8Vq10jGrunZJpaG6trWsQ/view)**
            (CAP), issue
            [471333](https://bugs.chromium.org/p/chromium/issues/detail?id=471333).
    Previously known as Slimming Paint v2. Project to re-implement the
    Blink&lt;-&gt;CC picture recording API to work in terms of a global display
    list rather than a tree of cc::Layers. It will result in a drastic
    simplification of the way that composited layers are represented in Blink
    and cc, which in turn will yield improved performance, correctness and
    flexibility.
*   **src: local() matching**, issue
            [627143](https://bugs.chromium.org/p/chromium/issues/detail?id=627143).
    Font matching and IPC improvements to allow for spec compliant font matching
    and improved web font performance.

## Organization

Team organization and communication.

### Mailing lists

We use a set of public mailing list for technical discussions, questions, and
announcements. Access is currently limited to subscribers but anyone may join by
posting to the relevant list or following the web archives links below. Once
subscribed the full historic archives are available.

*   [rendering-core-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/rendering-core-dev)
    Primary list for the team. Used for non-technical and generic technical
    discussions as well as for announcements.
*   [dom-dev@chromium.org](mailto:dom-dev@chromium.org)
*   DOM team-specific list for technical and standards discussions.
*   [style-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/style-dev)
    Style (CSS) specific list for technical and standards discussions.
*   [layout-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/layout-dev)
    Layout, text, and font specific list for technical and standards
    discussions.
*   [paint-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/paint-dev)
    Paint, Compositing, and SVG specific list for technical and standards
    discussions. Also see [this site](/teams/paint-team).

### Bi-Weekly Meeting

There is bi-weekly meeting held over video conference every second Tuesday open to all team
members, the meeting notes of which are available below and sent out to the
public mailing list. If you're interested in participating please talk to
[Chris](mailto:chrishtr@chromium.org) and he'll share instructions.

**Current schedule:**

*   Every second Tuesday 10:00 PDT (13:00 EDT, 18:00 BST, 19:00 CEST; Wednesday 02:00
            JST, 03:00 AEST).

Meeting notes are public and are sent to *rendering-core-dev*, they're also
available in this document: [Meeting
notes](https://docs.google.com/document/d/1AK_PzaiMpSHrJRvMhcnK-FFYXc64Qx_sidx3umsHDRY/edit?usp=sharing).

### Slack

There is also a set of dedicated slack channels for the team. For logistical
reasons these are limited to team members and collaborators. Please talk to one
of the team members and they'll get you added as needed.

## Team Members

**Adenilson Cavalcanti** adenilson.cavalcanti - ARM San Jose - Performance

Anders Hartvoll Ruud andruud - Google Oslo - Style, Houdini

Chris Harrelson (lead) chrishtr - Google - San Francisco - All areas

David Baron - dbaron - Google Maryland (Remote) - Paint

David Grogan - dgrogan Google San Francisco Layout, Tables, Flexbox

Dominik Röttsches - drott Google Helsinki - Text, Fonts

Fredrik Söderquist fs - Opera Linköping - SVG

Frédéric Wang - fwang - Igalia Paris - Layout, MathML

Ian Kilpatrick (lead) ikilpatrick - Google Mountain View - Layout

Javier Fernandez jfernandez Igalia - A Coruña - Style, Layout, Grid

Joey Arhar jarhar - Google San Francisco - DOM

Kent Tamura tkent - Google Tokyo - Layout, Form controls

Koji Ishii kojii - Google Tokyo - Layout, Text, Fonts

Manuel Rego rego - Igalia Vigo - Layout, Grid

Mason Freed (lead) masonf - Google San Francisco - DOM

Morten Stenshorne mstensho - Google Oslo - Layout, Fragmentation, MultiCol

Oriol Brufau obrufau - Igalia Barcelona - Style, Layout, Grid

Philip Rogers pdr - Google San Francisco - Paint

Richard Townsend richard.townsend - ARM San Jose - Layout, Performance

Rob Buis rbuis - Igalia Hamburg - Layout, MathML

Rune Lillesveen (lead) futhark - Google Oslo - Style

Stefan Zager szager - Google San Francisco - Paint

Steinar H. Gunderson sesse - Google Oslo - Style

Vladimir Levin vmpstr - Google Waterloo - Async

Xianzhu Wang wangxianzhu - Google Mountain View - Paint

Xiaocheng Hu xiaochengh - Google Mountain View - Style

## Contributing

If you're interested in getting involved and contributing to rendering there are
many ways you could help and we'd love to have you. These range from filing good
bug reports to creating test cases, reducing and triaging failures, fixing bugs
and implementing new functionality.

Please see the chromium [getting involved](/getting-involved) guide for generic
advice and to help you get set up.

A good way to get started is to fix an existing bug. Bug fixes tend to be
limited in scope, uncontroversial, and easy to evaluate.

Going through the bug database to find a suitable bug is quite a daunting task
though. To make it a little easier we try to maintain a list of bugs that we
think are suitable starter bugs. Those bugs are marked with a *GoodFirstBug*
label. Use the following queries to see *GoodFirstBug* in the [style &
layout](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Component%3ABlink%3ECSS%2CBlink%3EFonts%2CBlink%3EFullscreen%2CBlink%3ELayout%2CBlink%3ETextAutosize+Hotlist%3DGoodFirstBug+&colspec=ID+Pri+M+Stars+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
and [paint &
compositing](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Component%3ABlink%3EPaint%2CBlink%3ECanvas%2CBlink%3ECompositing%2CBlink%3ESVG%2CBlink%3EImage%2CBlink%3EHitTesting%2CBlink%3EGeometry+Hotlist%3DGoodFirstBug+&colspec=ID+Pri+M+Stars+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
components respectively.
If you prefer, the following queries will show all open bugs in the respective
bucket: [style &
layout](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Component%3ABlink%3ECSS%2CBlink%3EFonts%2CBlink%3EFullscreen%2CBlink%3ELayout%2CBlink%3ETextAutosize&colspec=ID+Pri+M+Stars+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles),
[paint &
compositing](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Component%3ABlink%3EPaint%2CBlink%3ECanvas%2CBlink%3ECompositing%2CBlink%3ESVG%2CBlink%3EImage%2CBlink%3EHitTesting%2CBlink%3EGeometry&colspec=ID+Pri+M+Stars+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles).

## Documentation

For a high-level overview of the rendering pipeline please see the [Life of a
Pixel](https://www.youtube.com/watch?v=w8lm4GV7ahg) ([slide
deck](http://bit.ly/lifeofapixel)) talk that Steve Kobes gave a little while
ago. It gives a very good overview and explains how the different steps in the
pipeline work and interact with each other.

For more in-depth documentation about specific rendering stages see the relevant
markdown files checked into the main source tree. The README.md file in each top
level directory is a good starting point. Some of the key documents are linked
below.

*   [renderer/core/css/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/core/css/README.md)
*   [renderer/core/dom/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/core/dom/README.md)
*   [renderer/core/layout/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/core/layout/README.md)
*   [renderer/core/layout/ng/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/core/layout/ng/README.md)
*   [renderer/core/paint/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/core/paint/README.md)
*   [platform/graphics/paint/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/platform/graphics/paint/README.md)
*   [platform/fonts/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/platform/fonts/README.md)

[Debugging Blink
objects](https://docs.google.com/document/d/1vgQY11pxRQUDAufxSsc2xKyQCKGPftZ5wZnjY2El4w8/edit#heading=h.8x8n9x5rkr0)

[Debugging Firefox/Gecko
objects](https://docs.google.com/document/d/1TMckMPNHk_XGZJUCUQi8IW3rtr03-6LHs_4UGSG4H_M/edit)

## Design Documents

Each new feature and all major projects require a design document before the
implementation work may commence. These documents are updated during the
implementation phase and provides a detailed explanation of the feature or
project as well as the history and the motivation.

Please add new design documents to the bottom of this list. Make sure they're
world readable and, if possible, grant comment privileges to
*edit-bug-access@chromium.org* rather than *Anyone with a chromium.org account*
as not all contributors have *chromium.org* accounts.

*   [LayoutUnit & Subpixel
            Layout](https://trac.webkit.org/wiki/LayoutUnit) (2012)
*   [Line Box float -&gt; LayoutUnit transition
            plan](https://docs.google.com/a/chromium.org/document/d/1fro9Drq78rYBwr6K9CPK-y0TDSVxlBuXl6A54XnKAyE/edit)
            (2014)
*   [Blink Coordinate
            Spaces](/developers/design-documents/blink-coordinate-spaces) (2015)
*   [Eliminating Simple
            Text](/teams/layout-team/eliminating-simple-text) (2015)
*   [LayoutObject Tree API
            Proposal](https://docs.google.com/document/d/1qc5Ni-TfCyvTi6DWBQQ_S_MWJlViJ-ikMEr1FSL0hRc/)
            (2015)
*   [Using Zoom to Implement Device Scale
            Factor](https://docs.google.com/document/d/1CZSCPzOYujdUMyChocwzOBPKxYAoTsEoezMye30Hdcs/)
            (2015)
*   [Unprefix CSS Writing
            Modes](https://docs.google.com/document/d/1lGrcTSlKMDeOEKZbHqvdLnW_Soywn7oICci2ApBXB00/edit?usp=sharing)
            (2015)
*   [Slimming Paint](/blink/slimming-paint) (2016)
*   [LayoutNG](https://docs.google.com/document/d/1uxbDh4uONFQOiGuiumlJBLGgO4KDWB8ZEkp7Rd47fw4/)
            (2016)
*   [Scroll Anchoring](http:////bit.ly/scroll-anchoring) (2016)
*   [Shaper Driven Line
            Breaking](https://docs.google.com/document/d/1eMTBKTnWEMDu00uS2p8Xj-l9Pk7Kf0q5y3FbcCrWYjU/)
            (2016)
*   [Hyphenation in
            Blink](https://docs.google.com/document/d/1ZgMnNxYxvPJYMOeyxJs8MsfGMNFiDKrz64AySxlCzpk/edit?usp=sharing)
            (2016)
*   ["system-ui" generic font
            family](https://docs.google.com/document/d/1BI0OiWRUvsBOuPxPlF5J-_xtUZ49eVDUEXZXoF32ZcM/edit?usp=sharing)
            (2016)
*   [Using ICU BiDi in
            LayoutNG](https://docs.google.com/document/d/1GXtjMXE46IFDXUdzyUAY2_yzZ3tJoiuYPV15e5xtGeE/edit?usp=sharing)
            (2016)
*   [Root Layer
            Scrolling](https://docs.google.com/document/d/137p-8FcnRh3C3KXi_x4-fK-SOgj5qOMgarjsqQOn5QQ/)
            (2017)
*   [Baseline in LayoutNG](/) (2017)
*   [Offset Mapping between DOM/Canonical
            Text](https://docs.google.com/document/d/1voI52vA1_UdaDxJ0QNzbXKeu4MQQcsgaCKdWyECvwq8/)
            (2018)
*   [IntersectionObserver](https://docs.google.com/document/d/1MWXTJhtvB7FvF3SrlOD7IwLMVyj5zx_0NLDWNe0qYmI/)
            (2018)
*   [Style Update
            Roots](https://docs.google.com/document/d/1aMwREBUbPr-eE1_M4XuBGWZs3GuFrxWBr8f6o9-HaOs/)
            (2018)
*   [Caret Position Bidi
            Affinity](https://docs.google.com/document/d/1SyryaZ304uxVgwW2Hf3gGnq4_h1QqT3Jz9h-etqud1o/)
            (2018)
*   [Style Memory
            Improvements](https://docs.google.com/document/d/1cgtF8VVNGdXQmCdvOs34vLLA1bFWHPfZRZtm5nEY7bs)
            (2018)
*   [Paint Touch-Action
            Rects](https://docs.google.com/document/d/1ksiqEPkDeDuI_l5HvWlq1MfzFyDxSnsNB8YXIaXa3sE/view)
            (2018)
*   [Blink-generated property trees
            (BlinkGenPropertyTrees)](https://docs.google.com/document/u/1/d/17GKr2uIH2O5GthdTyvJpv1qZjoHYoLgrzvCkbCHoID4/)
            (2019)
*   [Unified ComputedStyle
            Storage](https://docs.google.com/document/d/12c8MKqitZBdhSORSVJuQOrar-XdY8VERthLKAO7muUk/)
            (2019)
*   [Flat Tree Order Style
            Recalc](https://docs.google.com/document/d/1tjKznu6K-B3TfRHxApsPYXYM7FyIXg9Xg2wWcOmStNU)
            (2019)
*   [StyleResolver
            Cascade](https://docs.google.com/document/d/1HrmPmcQBTUMouqQQG3Kww43I5aFW9-Q9tr-DEKZk09I)
            (2019)
*   [Skip Forced Style Update on Parsing
            Finished](https://docs.google.com/document/d/1KdMCXjHP6ynCV2vP84mJIBmnWPS5y99uDq7Fed3ogQA/edit#heading=h.v9as6odlrky3)
            (2019)
*   [Paint Non-fast Scrollable
            Regions](https://docs.google.com/document/d/1IyYJ6bVF7KZq96b_s5NrAzGtVoBXn_LQnya9y4yT3iw/view)
            (2019)
*   [LayoutNG block
            fragmentation](https://docs.google.com/document/d/1EJOdFesZKspvrU7uWtGl-8ab2jIrzRF6NKJhwYOs6hU/)
            (2019)
*   [Moving out from
            NGPaintFragment](https://docs.google.com/document/d/1O6u2BWhvjsT_dkqQZPftmqZ-qgBqxGlA2ZyLh_xBpgQ/edit?usp=sharing)
            (2019)
*   [NGFragmentItem](https://docs.google.com/document/d/10vJ6wdyEdeGkmcotKBZ9h3YtDzw5FIpDksa8rCHVFuM/)
            (2019)
*   [CSS min/max](http://bit.ly/31Yfkm6) (2019)
*   [MathML](https://docs.google.com/document/d/1biGEaWN8ThNTDtAbT1M5GIf6N5uQLWdxh2QhrG9uN5c/edit)
            (2019)
*   [@font-face Loading and Full Document Style
            Invalidation](http://bit.ly/35JjPmq) (2020)
*   [Jankless ‘font-display: optional’](https://bit.ly/36E8UKB) (2020)
*   [Reduce MediaQuery RuleSet
            changes](https://docs.google.com/document/d/1TMqAq4k3aTHNH1m2sYoj-5QAndNmEXLI1QeD-oQ7jWE/edit#heading=h.6kknmf22ixwc)
            (2020)
*   [FastBorderRadius](https://docs.google.com/document/d/1bYuKa_X9faxHTs6B5OjE4n92ytP8CrEToeuBE4Mr7hI/edit)
            (2019/2020)
*   [Improving UKM Client
            Counts](https://docs.google.com/document/d/1P49GrDe3mDDV9Q1yUm27ioWE4m1_Zz1FwRwVwGQh-ZI/edit?usp=sharing)
            (2020)
*   [Ensuring Rendering of Sub-pixel
            Borders](https://docs.google.com/document/d/1fAYkOFxp2Luh6OOoXxtwOehmvNRGNss58ibQtVXL0Tw/edit?usp=sharing)
            (2020)
*   [Metrics for HTML
            Parsing](https://docs.google.com/document/d/1zWlnQELDePFJBz105RgjjgPCkDN7quiVOHSWzNMf-DU/edit?usp=sharing)
            (2020)
*   [Improving Tracking of Web Test
            Changes](https://docs.google.com/document/d/1SCKmT1S7HmoaZFYThqXiqrQJvQPikUfV6mwwXbW7Bk4/edit?usp=sharing)
            (2020)
*   [Dependency-Aware
            MatchedPropertiesCache](https://docs.google.com/document/d/1uJSpTD9mAgFGljMaQzTvjFKzsvAv1yRJ15M0ZPzAVbo)
            (2020)
*   [Tree-scoped names in
            CSS](https://docs.google.com/document/d/1NY1GmSeAhpYMdZ2Jd581aT-Dr3DDIb8-pc5YGiBTm5E/)
            (2020)
*   [Base background for
            color-scheme](https://docs.google.com/document/d/1yTsrWTf5qWS7rVytSunSBEXrVO_j5gYu7xlzFsPENxk/)
            (2020)
*   [Interleaved Style and
            Layout](https://docs.google.com/document/d/1mpN2I0KYVmIoB8LfSV3pzPAoqGBIrubybHPzrhc2dxA)
            (2020)
*   [Resolving Container
            Queries](https://docs.google.com/document/d/17pymtoSq1WIP6mSQj-fSPU_3EdeB0AfjdTq38bYW574)
            (2020)
*   [SVG Text
            NG](https://docs.google.com/document/d/1GOPKXsAMyKBCcTlByEwGHdu6QuG6YTd_mDwOX8z9S2c/edit?usp=sharing)
            (2021)

Presentations

*   [Display
            Locking](https://docs.google.com/presentation/d/12IOGoBZS5kSX6CYb01904ZKuMhjKS2Q6O-hx9jTmvA8/edit#slide=id.p)
            (vmpstr, Jan 2020)
*   [Font
            performance](https://docs.google.com/presentation/d/123_mQWrDoNbpMQ4bXiHaJT_lli-Vy2DLY8aRp1aC2jU/edit#slide=id.p)
            (xiaochengh, Feb 2020)
*   [Canvas](https://docs.google.com/presentation/d/12nR0gKSynGIfmeT5iIEU9LOg-4nXe8xgQ9Q8VAFqL8w/edit)
            (fserb, Feb 2020)
*   [Compositing
            memory](https://docs.google.com/presentation/d/1_8PLdXVUPclq7aiWnTU7UES43NMjaUnRBbQYypmqrQQ/edit#slide=id.p)
            (pdr, Feb 2020)
*   [Portals](https://docs.google.com/presentation/d/1UYQe9jOysS2zX1yyiqP-LOxJdo-elC0grP-xUbVBtxY/edit?usp=sharing)
            (lfg, Feb 2020)
*   [FlexNG](https://docs.google.com/presentation/d/10e7bnBrkpNJj8aQXiofCJCslWpPtlxZyt7wZIw9NQrg/edit)
            (dgrogan, Mar 2020)
*   [Scrolling](https://docs.google.com/presentation/d/1cQZLTKzUWD2O0fUQhwaL4DbRckPPNiOjDqwfPYQzoDc/edit?usp=sharing)
            (samfort@microsoft, Mar 2020)

Google-internal design documents (aim is to migrate them to the list above; on
request we can try to make part/all public)

[Font Matching by Full Font
Name](https://docs.google.com/document/d/1yCZwVIF39S8WOgCUraT5OuUUaLSqWrxoG3mqdtnHnhs/edit#)
(2019)

## Bug & Triage Policy

The rendering core team is responsible for all bugs for the components listed
below, including sub-components . Our policy is that all new bugs are to be
triaged within a week of being filed and all P-0 and P-1 bugs are to be fixed in
time for the next release. Failures to meet the policy is tracked in our weekly
meeting and shared as part of the meeting notes

### Policy

1.  Triage all bugs within 7 days.
2.  Fix P0 bugs within 15 days.
3.  Fix P1 regression bugs within 15 days.
4.  Re-triage all bugs every 365 days.

### Links

*   [Tracking
            Spreadsheet](https://docs.google.com/spreadsheets/d/1mQ4b1cpy78wFKahKfzJl-4LUYeygAHyGaMGoZKnD_0U/)
*   [Process &
            Metrics](https://docs.google.com/document/d/1KncD2BSs9YnPt90DHSkoxttxyvG3yT79_9MXyMWN1Ok/edit#heading=h.mlb5a7a4vqj0)
*   [Test
            Flakiness](https://test-results.appspot.com/dashboards/flakiness_dashboard.html#testType=webkit_layout_tests)
*   [Working with release branches](/developers/how-tos/drover)

## Related Teams

*   [Input](/teams/input-dev)
*   [Animations](/teams/animations)
