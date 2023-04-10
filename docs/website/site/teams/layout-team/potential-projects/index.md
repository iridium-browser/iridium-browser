---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
page_name: potential-projects
title: Roadmap / Potential Projects
---

List of potential future projects (aka wishlist) for the layout team.
Relative priority in square brackets, based on impact, cost of implementing and
feedback from real-world web sites and web app teams.

## In Scope Projects

The backlog is now maintained in monorail, please see the
[layout-backlog](https://bugs.chromium.org/u/3593919584/hotlists/layout-backlog)
hotlist.

## Out of Scope

Potential projects that have been considered and rejected as being out-of-scope,
either due to the projects itself being premature, due to it being better suited
for a different team, or not considered important enough at the moment.

*   Viewporting and Infinite lists
    Too early, silk TLs need to discuss this further.
*   String of HTML into Fragment
    This is job of scheduler team.
*   When layout happens, calcDrawProps and sendInvalidations runs long
    Falls onto the [paint team](/teams/paint-team).
*   Repaint storm elimination & invalidation
    Tackled by [Slimming Paint](/blink/slimming-paint).
*   UIWorker / Animation Proxy
    Too early, silk TLs need to discuss this further.
*   Expose what triggered a layout in devtools
    Show in devtools which line of JS caused a layout, which nodes generated an
    invalidation, why it took as long as it did. Falls on the devtools team.
*   Element
            [onPaint](https://docs.google.com/a/chromium.org/document/d/1BmS4vT5RtTxg_HH_33bU1PNwVZR4ZsNpOagWjEsETA8/edit)
            callback
    Falls on the [paint team](/teams/paint-team).
*   HTML Canvas
    Falls on the GPU or graphics team.
