---
breadcrumbs:
- - /teams
  - Teams
- - /teams/layout-team
  - Layout Team
- - /teams/layout-team/okrs
  - Objectives and Key Results
page_name: 2016q1
title: 2016 Q1
---

List of objectives and key results for 2016 Q1.

## Overview

*   Objective / Key Result Owner(s) Status
*   Measure and improve performance
    *   [Ship CSS Containment as experimental feature to stable.](#o0r0)
                leviw
    *   [Gather data on number of third party iframes loaded but never
                viewed.](#o0r1) szager
    *   [Collect metrics and do analysis on forced layouts.](#o0r2)
                cbiesinger
    *   [Stretch: Reduce number of forced layouts for one common
                trigger.](#o0r3) cbiesinger
    *   [Ship intersection observer to stable.](#o0r4) ojan, szager
*   Interop (Interoperability, Compatibility, Conformance)
    *   [Spec for custom layout.](#o1r0) blink-houdini, ikilpatrick
    *   [Finish up worklet implementation.](#o1r1) blink-houdini,
                ikilpatrick
    *   [Socialize resize observers specification.](#o1r2) atotic
    *   [Collect data on percentage margin usage.](#o1r3) eae
    *   [Ship updated absolute position behavior for flexbox to
                stable.](#o1r4) cbiesinger
    *   [Upstream 75% of our flexbox test to w3c.](#o1r5) cbiesinger
    *   [Make progress on text autosizing spec](#o1r6) pdr
    *   [Evangelize CSS containment and get another browser to commit to
                it.](#o1r7) leviw, shanestephens
*   Code health
    *   [Root layer scrolling passing all unit tests.](#o2r0) skobes
    *   [Design doc for internal layout api](#o2r1) leviw
    *   [Line layout API analysis document](#o2r2) dgrogan, pilgrim
    *   [Stretch: Ruby on top of custom layout as experimental web
                platform feature on trunk.](#o2r3) kojii
    *   [Ship emoji & punctuation fallback overhaul on all
                platforms.](#o2r4) drott
    *   [Feasibility study for replacing flipped blocks writing mode
                implementation.](#o2r5) wkorman
*   Product excellence
    *   [Ship improved web typography to beta.](#o3r0) drott
    *   [Triage all incoming bug reports within one week and eliminate
                backlog.](#o3r1) eae
    *   [Fix all new P1 bug reports within one release.](#o3r2) eae
    *   [Start to make vertical rhythm on the web easier.](#o3r3) kojii
    *   [Prototype scroll anchoring on layout.](#o3r4) skobes

## Details

### Ship CSS Containment as experimental feature to stable.

Owner(s):leviw
Score:

### Gather data on number of third party iframes loaded but never viewed.

Owner(s):szager
Score:

### Collect metrics and do analysis on forced layouts.

Owner(s):cbiesinger
Score:

### Stretch: Reduce number of forced layouts for one common trigger.

Owner(s):cbiesinger
Score:

Common triggers include: - I'm an iframe and need to know my size
(clientHeight/width) - I'm an iframe or component and need to know if i'm
visible. (scrollTop, etc) - I want to lazyload images or components when the
user gets closer (scrollTop) - I'm a metrics script and need to report the
window size (window.clientWidth, etc) - I need to set my dimensions based on the
computed width/height of something else. (clientHeight/Width) - Need to set
focus(), because new component is active. (elem.focus()) - I'm jQuery or
Modernizr and use computed style to identify features or bugs.

### Ship intersection observer to stable.

Owner(s):ojan, szager
Score:

Stretch: pay down technical depth by moving things to use the intersection
observer infrastructure, such as: - render throttling for offscreen - video auto
play

### Spec for custom layout.

Owner(s):blink-houdini, ikilpatrick
Score:

### Finish up worklet implementation.

Owner(s):blink-houdini, ikilpatrick
Score:

Help Audio & CW folks transition as needed.

### Socialize resize observers specification.

Owner(s):atotic, blink-houdini
Score:

### Collect data on percentage margin usage.

Owner(s):eae
Score:

### Ship updated absolute position behavior for flexbox to stable.

Owner(s):cbiesinger
Score:

### Upstream 75% of our flexbox test to w3c.

Owner(s):cbiesinger
Score:

### Make progress on text autosizing spec

Owner(s):pdr
Score:

(20% project)
https://docs.google.com/document/d/1hlg1TworBMsvSMdLz4n5DxFr8Bex7S8eB_p0GpFrdVU/edit

### Evangelize CSS containment and get another browser to commit to it.

Owner(s):blink-houdini, leviw, shanestephens
Score:

### Root layer scrolling passing all unit tests.

Owner(s):skobes
Score:

### Design doc for internal layout api

Owner(s):leviw
Score:

Including feasibility study for supporting a custom-layout style API in
conjunction with our legacy layout implementation.

### Line layout API analysis document

Owner(s):dgrogan, leviw, pilgrim
Score:

Have an analysis document detailing the current organic API and a plan for
transitioning towards the API we'd like. Make decision, based on document,
whether the exercise was useful and if so if we should continue with the rest of
the layout API.

### Stretch: Ruby on top of custom layout as experimental web platform feature on trunk.

Owner(s):kojii
Score:

### Ship emoji & punctuation fallback overhaul on all platforms.

Owner(s):drott
Score:

### Feasibility study for replacing flipped blocks writing mode implementation.

Owner(s):wkorman
Score:

### Ship improved web typography to beta.

Owner(s):drott
Score:

- OpenType Enabling - smcp, - Enable all font-feature-settings and
font-variant-\* CSS - CSS font-synthesis attribute (faux bold & italics control)

### Triage all incoming bug reports within one week and eliminate backlog.

Owner(s):eae
Score:

### Fix all new P1 bug reports within one release.

Owner(s):eae
Score:

This is a bit o a stretch given that we don't yet have a team-wide definition of
a P1 bug but nevertheless it is something we really should try to do!

### Start to make vertical rhythm on the web easier.

Owner(s):kojii
Score:

https://github.com/kojiishi/snap-height

### Prototype scroll anchoring on layout.

Owner(s):skobes
Score:

crbug.com/558575 - user facing feature and a prerequisite for many potential
user agent interventions.
