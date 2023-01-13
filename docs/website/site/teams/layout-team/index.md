---
breadcrumbs:
- - /teams
  - Teams
page_name: layout-team
title: Layout Team
---

The layout team has been subsumed into the new [rendering
core](//chromium.org/teams/rendering) team. Please see the rendering core page
for details about ongoing style and layout work.
This page is preserved for posterity and will no longer be updated.

The layout team is a long-term engineering team tasked with maintaining,
supporting, and improving the layout capabilities of the Chromium Blink
rendering engine. This includes line layout, block layout, text, fonts, and
high-dpi support. It is distinct from [style](/teams/style-team) and
[paint](/teams/paint-team), both of which are handled by their respective teams.

## Team Charter

Our overarching goal is to **improve the experience for end users and platform
developers alike**.

*   By offering fast, correct, and compatible layout features supporting
            the majority of writing systems and scripts.
*   By providing layered and powerful APIs for developers.
*   By driving the layout capabilities of the web forward through
            collaboration with web developers, other browser vendors, and
            standard bodies.
*   And finally, by intervening when required. Thereby allowing a better
            user experience for existing and legacy content.

## Big ticket items for 2017

*   Schedulable, interruptible engine
    *   LayoutNG can render ~50% of web content.
*   Give framework authors the ability to extend the rendering engine
    *   Ship at least one CSS Houdini API.
*   High quality predictable platform
    *   Maintain bug SLA
    *   Full test and design doc coverage.
    *   Ship CSS Grid.
    *   Unify scrolling implementation (root layer scrolling).
    *   Ship OpenType variation support.
    *   Intersection Observer v2.
*   Improve rendering performance
    *   Improve line layout performance by 20%

A large number of other individuals not listed above routinely contribute to our
layout code and their impact and contributions should in no way be downplayed.
This merely lists the people that formally make up the layout team or have
committed to a certain task or project.

If you are looking for someone to review a change to the layout code anyone on
the list above should be able to help or connect you with the right person. For
questions about the design or implementation of a certain layout sub-system or
about layout in general the entire team may be reached through the
[layout-dev](mailto:layout-dev@chromium.org) mailing list.

For higher level questions or if you are interested in joining the team, please
reach out to [Emil](mailto:eae@chromium.org).

## Design Documents

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
            Proposal](https://docs.google.com/document/d/1qc5Ni-TfCyvTi6DWBQQ_S_MWJlViJ-ikMEr1FSL0hRc/edit)
            (2015)
*   [Using Zoom to Implement Device Scale
            Factor](https://docs.google.com/document/d/1CZSCPzOYujdUMyChocwzOBPKxYAoTsEoezMye30Hdcs/)
            (2015)
*   [Scroll Anchoring](http:////bit.ly/scroll-anchoring) (2016)
*   [Shaper Driven Line
            Breaking](https://docs.google.com/document/d/1eMTBKTnWEMDu00uS2p8Xj-l9Pk7Kf0q5y3FbcCrWYjU/edit?usp=sharing)
            (2016)
*   [Root Layer
            Scrolling](https://docs.google.com/document/d/137p-8FcnRh3C3KXi_x4-fK-SOgj5qOMgarjsqQOn5QQ/)
            (2017)

**Documentation**

*   [Out of flow
            positioning](https://docs.google.com/document/d/1Qbgfx7vh2CTxa8CsYVS25tQWtkGdrN-D6TzPmYGZNtc/edit#heading=h.ndf3qdi6efu4)

## Links

*   [Readmap & Potential
            Projects](/teams/layout-team/potential-projects)
*   [Team mailing
            list](https://groups.google.com/a/chromium.org/forum/#!forum/layout-dev)
*   [Profiling Blink using Flame
            Graphs](/developers/profiling-flame-graphs)
*   [Skia telemetry try
            bot](http://skia-tree-status.appspot.com/skia-telemetry/chromium_try)
