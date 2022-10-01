---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/meeting-notes
  - Layout Team Meeting Notes
page_name: monday-august-29-2016
title: Monday, August 29, 2016
---

Updates since last meeting (on Monday, August 22, 2016):
Scrolling
- Work on root layer scrolling continues. (szager)
Scroll Anchoring \[[crbug.com/558575](http://crbug.com/558575)\]
- Support for SANACLAP (Suppress if Anchor Node Ancestor Changed Layout-
Affecting Property) landed. Should help reduce the number of hacks
required to support the feature. \[<http://bit.ly/sanaclap>\] (skobes)
- Aim this week is to get it (SANACLAP) into canary to gather data.
- Dealing with a devtools scroll anchoring bug.
CSS Flexbox
- Incoming bug rate for flexbox is going down, most issues have been
fixed and we aren't detecting many new ones. Yay. (cbiesinger)
- Some lingering reports and concerns about the abs-pos change that
will require outreach/evangelism. Not nearly as much as previously
thought though. Microsoft paved the way and did a really good job of
reaching out to web authors. (cbiesinger)
CSS Grid Layout \[[crbug.com/79180](http://crbug.com/79180)\]
- No updates since last week. See tracking bug for status.
CSS Multi-column (mstensho) \[[crbug.com/334335](http://crbug.com/334335)\]
- Still working on paint layer issues triggered by an investigation into
incorrect behavior for getClientRects. Issues mostly around writing
mode and paint layer interaction. Paint layer is a mess when it comes
to coordinate spaces. (mstensho)
CSS Houdini
- Lots of work around Worklets for Web Audio and Web Animations in
preparation for TPAC last week. (ikilpatrick)
- Plan to spend more time preparing the Worklets and CSS Custom Layout
specs for TPAC this week. (ikilpatrick)
- Working on event loop spec. (glebl)
LayoutNG \[[crbug.com/591099](http://crbug.com/591099)\]
- Converted every backing fragment and constraint space to physical
coordinates instead of logical. (ikilpatrick)
- Might attempt to add initial support for floats this week.
(ikilpatrick)
- Started on inline layout support. (eae)
- Initial layout opportunities implementation. (eae)
- Implemented state machine for async layout. (cbiesinger)
- Added fragment builder class to help with fragment construction.
(cbiesinger)
- Start working on margin collapse. (glebl)
Resize Observer (atotic)
- Finally landed and is looking good for M54. (atotic)
Tables (dgrogan)
- Continuing work on fixing high priority table bugs. (dgrogan)
Text (eae, drott, kojii)
- Complex text on Android is looking good and is on track for M54.
(drott)
- Investigated a shaping regression on old OS X versions. Due to a few
previous shaping fixes we now crash in certain cases due to the font
fallback cascade on old versions of OSX. Looking into options. (drott)
- We don't support type1 (bitmap) fonts on Linux however they show up
in the font selection UI. Fixed this and in doing so also broke the
dependency on Pango. (drott)
- Made a change to skia to avoid picking type1 fonts during fallback,
thereby reducing the number of attempts thus improving performance.
(drott)
- Preparing for ATypI. (drott)
Interventions
- Iterated on data collection methodology for a potential third party
iframe intervention. Missed the branch point but got a patch that
I'm happy with. (dgrogan)
Misc:
- Helping Physical Web team with Google infrastructure integration.
(glebl)
